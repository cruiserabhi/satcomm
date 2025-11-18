/**
 * @file C_SatCommService.cpp
 * @brief Implementation of C_SatCommService.
 */

#include "HarmanSatCommService/C_SatCommService.hpp"
#include <iostream>  // logging

using namespace telux::satcom;
using namespace telux::common;

namespace harman {

C_SatCommService::C_SatCommService(const ntncfg::Config &cfg)
    : cfg_(cfg)
    , mtu_(0)
    , running_(false)
    , inFlight_(false)
    , serviceOk_(false) {}

C_SatCommService::~C_SatCommService() {
    shutdown();
}

bool C_SatCommService::init() {
    if (ntn_) return true;

    // Gate on TelSDK readiness (same pattern as Qualcomm ntn_test_app)
    // getNtnManager accepts an init callback returning ServiceStatus. :contentReference[oaicite:2]{index=2}
    std::mutex initM;
    std::condition_variable initCv;
    ServiceStatus ready = ServiceStatus::SERVICE_UNAVAILABLE;

    SatcomFactory &fac = SatcomFactory::getInstance();
    {
        std::unique_lock<std::mutex> lk(initM);
        ntn_ = fac.getNtnManager(
            [&](ServiceStatus s) {
                std::lock_guard<std::mutex> g(initM);
                ready = s;
                initCv.notify_all();
            }
        );
        initCv.wait(lk); // wait for ServiceStatus
    }

    if (!ntn_) {
        std::cerr << "C_SatCommService: getNtnManager returned nullptr\n";
        return false;
    }
    if (ready != ServiceStatus::SERVICE_AVAILABLE) {
        std::cerr << "C_SatCommService: NTN subsystem is not ready\n";
        return false;
    }

    // Register listener (same as test app registerListener). :contentReference[oaicite:3]{index=3}
    Status st = ntn_->registerListener(shared_from_this());
    if (st != Status::SUCCESS) {
        std::cerr << "C_SatCommService: registerListener failed\n";
        return false;
    }

    // Prime capabilities (MTU)
    NtnCapabilities cap;
    if (getNtnCapabilities(cap) == ErrorCode::SUCCESS && cap.maxDataSize > 0) {
        mtu_.store(cap.maxDataSize);
    }

    // Observe state gate
    NtnState ns = ntn_->getNtnState();             // DISABLED/OUT_OF_SERVICE/IN_SERVICE
    ServiceStatus ss = ntn_->getServiceStatus();   // AVAILABLE/UNAVAILABLE/FAILED
    serviceOk_.store(ns == NtnState::IN_SERVICE && ss == ServiceStatus::SERVICE_AVAILABLE);

    running_.store(true);
    startWorker_();
    return true;
}

void C_SatCommService::shutdown() {
    stopWorker_();
    if (ntn_) {
        ntn_->deregisterListener(shared_from_this()); 
        ntn_.reset();
    }
}

void C_SatCommService::setCallbacks(const C_SatCommCallbacks &cb) {
    cb_ = cb;
}

ErrorCode C_SatCommService::isNtnSupported(bool &supported) const {
    if (!ntn_) return ErrorCode::NOT_READY;
    return ntn_->isNtnSupported(supported); 
}

ErrorCode C_SatCommService::enableNtn(bool enable, bool emergency, const std::string &iccid) {
    if (!ntn_) return ErrorCode::NOT_READY;

    return ntn_->enableNtn(enable ? 1 : 0, emergency ? 1 : 0, iccid);
}

NtnState C_SatCommService::getNtnState() const {
    if (!ntn_) return NtnState::DISABLED;
    return ntn_->getNtnState();
}

ServiceStatus C_SatCommService::getServiceStatus() const {
    if (!ntn_) return ServiceStatus::SERVICE_UNAVAILABLE;
    return ntn_->getServiceStatus(); 
}

ErrorCode C_SatCommService::getNtnCapabilities(NtnCapabilities &cap) const {
    if (!ntn_) return ErrorCode::NOT_READY;
    return ntn_->getNtnCapabilities(cap);
}

ErrorCode C_SatCommService::enableCellularScan(bool enable) {
    if (!ntn_) return ErrorCode::NOT_READY;
    return ntn_->enableCellularScan(enable ? 1 : 0); 
}

ErrorCode C_SatCommService::updateSystemSelectionSpecifiers(
    const std::vector<SystemSelectionSpecifier> &sfl) {
    if (!ntn_) return ErrorCode::NOT_READY;
    return ntn_->updateSystemSelectionSpecifiers(sfl); 
}

ErrorCode C_SatCommService::sendAudioFrame(const uint8_t *data, size_t size, bool emergency) {
    if (!ntn_) return ErrorCode::NOT_READY;
    if (!data || size == 0) return ErrorCode::INVALID_ARGUMENT;

    // Enqueue copy (non-blocking)
    Pdu p; p.bytes.assign(data, data + size); p.emergency = emergency;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        q_.push(std::move(p));
    }
    cv_.notify_one();
    return ErrorCode::SUCCESS;
}

ErrorCode C_SatCommService::abortUplink() {
    if (!ntn_) return ErrorCode::NOT_READY;
    return ntn_->abortData(); 
}

/* ---------- INtnListener ---------- */

void C_SatCommService::onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
    if (cb_.onIncomingData && data && size) cb_.onIncomingData(data.get(), size);
}

void C_SatCommService::onNtnStateChange(NtnState state) {
    serviceOk_.store(state == NtnState::IN_SERVICE &&
                     getServiceStatus() == ServiceStatus::SERVICE_AVAILABLE);
    if (cb_.onNtnState) cb_.onNtnState(state);
}

void C_SatCommService::onCapabilitiesChange(NtnCapabilities capabilities) {
    if (capabilities.maxDataSize > 0) mtu_.store(capabilities.maxDataSize);
    if (cb_.onCapabilities) cb_.onCapabilities(capabilities);
}

void C_SatCommService::onSignalStrengthChange(SignalStrength newStrength) {
    if (cb_.onSignalStrength) cb_.onSignalStrength(newStrength);
}

void C_SatCommService::onServiceStatusChange(ServiceStatus status) {
    serviceOk_.store(getNtnState() == NtnState::IN_SERVICE &&
                     status == ServiceStatus::SERVICE_AVAILABLE);
    if (cb_.onServiceStatus) cb_.onServiceStatus(status);
}

void C_SatCommService::onDataAck(ErrorCode err, TransactionId id) {
    // Release the next PDU upon ACK (or error, to avoid deadlock).
    inFlight_.store(false);
    cv_.notify_one();
    if (cb_.onDataAck) cb_.onDataAck(err, id);
}

void C_SatCommService::onCellularCoverageAvailable(bool available) {
    if (cb_.onCellularCoverageAvailable) cb_.onCellularCoverageAvailable(available);
}

/* ---------- internals ---------- */

void C_SatCommService::startWorker_() {
    if (worker_.joinable()) return;
    worker_ = std::thread(&C_SatCommService::workerLoop_, this);
}

void C_SatCommService::stopWorker_() {
    running_.store(false);
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    // drain
    std::queue<Pdu> empty;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        std::swap(q_, empty);
    }
}

ErrorCode C_SatCommService::sendChunk_(
    const uint8_t *data, size_t size, bool emergency, TransactionId &tid) {
    return ntn_->sendData(data, (uint32_t)size, emergency, tid);
}

void C_SatCommService::workerLoop_() {
    running_.store(true);
    for (;;) {
        // Wait until: running && serviceOk && !inFlight && queue-not-empty
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this]() {
            return !running_.load() || (!q_.empty() && serviceOk_.load() && !inFlight_.load());
        });
        if (!running_.load()) break;
        if (q_.empty() || !serviceOk_.load() || inFlight_.load()) continue;

        Pdu p = std::move(q_.front());
        q_.pop();
        lk.unlock();

        // Compute MTU (fallback to capability probe if unknown).
        uint32_t m = mtu_.load();
        if (m == 0) {
            NtnCapabilities cap;
            if (getNtnCapabilities(cap) == ErrorCode::SUCCESS) m = cap.maxDataSize;
            if (m == 0) m = 256; // very conservative fallback if modem didn't report yet
            mtu_.store(m);
        }

        // Chunk & send sequentially; wait for ACK between PDUs (inFlight gate).
        size_t offset = 0;
        while (offset < p.bytes.size()) {
            size_t remain = p.bytes.size() - offset;
            size_t sendSz = remain < m ? remain : m;

            TransactionId tid = 0;
            inFlight_.store(true);
            ErrorCode ec = sendChunk_(&p.bytes[offset], sendSz, p.emergency, tid);
            if (ec != ErrorCode::SUCCESS) {
                inFlight_.store(false);
                // Push back remainder to preserve delivery order.
                if (offset + sendSz < p.bytes.size()) {
                    Pdu retry;
                    retry.emergency = p.emergency;
                    retry.bytes.assign(p.bytes.begin() + offset + sendSz, p.bytes.end());
                    std::lock_guard<std::mutex> lk2(mtx_);
                    q_.emplace(std::move(retry));
                }
                if (cb_.onDataAck) cb_.onDataAck(ec, tid);
                break;
            }

            // Await ACK (released in onDataAck)
            std::unique_lock<std::mutex> lk2(mtx_);
            cv_.wait(lk2, [this]() {
                return !running_.load() || !inFlight_.load();
            });
            if (!running_.load()) return;

            offset += sendSz;
        }
    }
}

} // namespace harman
