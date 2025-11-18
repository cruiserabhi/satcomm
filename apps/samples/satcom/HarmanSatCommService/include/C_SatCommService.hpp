#pragma once
/**
 * @file C_SatCommService.hpp
 * @brief NB-NTN wrapper over Qualcomm TelSDK NtnManager with MTU-aware, ACK-paced audio I/O.
 *
 * This class hides TelSDK wiring (init, listener registration, capability/state updates) and
 * offers a simple callback-based API for applications that want to push/receive audio frames
 * over NB-NTN with low jitter and minimal head-of-line blocking.
 *
 * It uses:
 *   - telux::satcom::SatcomFactory::getInstance().getNtnManager(initCb)
 *   - INtnManager APIs: isNtnSupported, enableNtn, getNtnCapabilities, sendData, abortData,
 *     updateSystemSelectionSpecifiers, enableCellularScan, getNtnState, getServiceStatus
 *   - INtnListener callbacks: onIncomingData, onNtnStateChange, onCapabilitiesChange,
 *     onSignalStrengthChange, onServiceStatusChange, onDataAck, onCellularCoverageAvailable
 *
 * See Qualcomm's ntn_test_app for the same API usage. (This source uses those exact calls.)
 */

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <telux/satcom/SatcomFactory.hpp>
#include <telux/satcom/NtnManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "ConfigLoader/NtnConfigLoader.hpp"  // your header-only cJSON loader

namespace harman {

/**
 * @brief Callbacks an application can register to receive NB-NTN events and data.
 *
 * All callbacks are invoked from internal threads; if you need single-thread affinity,
 * forward them onto your own executor.
 */
struct C_SatCommCallbacks {
    /** Incoming downlink data (already contiguous). */
    std::function<void(const uint8_t *data, uint32_t size)> onIncomingData;

    /** NTN service-state changes (DISABLED/OUT_OF_SERVICE/IN_SERVICE). */
    std::function<void(telux::satcom::NtnState state)> onNtnState;

    /** Network capability updates (e.g., MTU via maxDataSize). */
    std::function<void(const telux::satcom::NtnCapabilities &cap)> onCapabilities;

    /** RF signal-strength buckets (NONE/POOR/MODERATE/GOOD/GREAT). */
    std::function<void(telux::satcom::SignalStrength ss)> onSignalStrength;

    /** Service status (AVAILABLE/UNAVAILABLE/FAILED). */
    std::function<void(telux::common::ServiceStatus st)> onServiceStatus;

    /** L2 ACK: success/error per transaction id (used to pace uplink). */
    std::function<void(telux::common::ErrorCode ec, telux::satcom::TransactionId id)> onDataAck;

    /** Warning: terrestrial coverage toggle state while in NB-NTN. */
    std::function<void(bool available)> onCellularCoverageAvailable;
};

/**
 * @class C_SatCommService
 * @brief A thin, safe wrapper around TelSDK NtnManager for NB-NTN audio transport.
 *
 * ## Highlights
 * - **Init/Deinit**: Synchronous readiness gate (waits on factory init callback).
 * - **Capability tracking**: MTU captured from `getNtnCapabilities()` and updates.
 * - **ACK-paced uplink**: Only one in-flight PDU at a time; next chunk released on ACK.
 * - **MTU-aware chunking**: Frames automatically segmented to `maxDataSize` or smaller.
 * - **Non-blocking send**: `sendAudioFrame()` enqueues; worker thread handles actual TX.
 * - **Callbacks**: Full surface of NB-NTN events and downlink data to the user class.
 * - **Scan toggle**: `enableCellularScan()` wrapper when operating in NB-NTN.
 *
 * **Thread safety**: Public API is thread-safe. Callbacks are invoked from internal threads.
 */
class C_SatCommService
    : public telux::satcom::INtnListener
    , public std::enable_shared_from_this<C_SatCommService> {
public:
    /**
     * @brief Construct with a validated config (from NtnConfigLoader).
     * @param cfg A fully materialized configuration (defaults are fine).
     */
    explicit C_SatCommService(const ntncfg::Config &cfg);

    /** @brief Dtor performs clean shutdown if not already done. */
    ~C_SatCommService();

    /**
     * @brief Initialize TelSDK, acquire NtnManager, wait until service is ready.
     * @return true when ready (SERVICE_AVAILABLE) and manager is not null.
     */
    bool init();

    /** @brief Deregister listener, stop worker thread, release NtnManager. */
    void shutdown();

    /** @brief Register application callbacks. */
    void setCallbacks(const C_SatCommCallbacks &cb);

    /** @brief Check if NB-NTN is supported on this UE + SIM. */
    telux::common::ErrorCode isNtnSupported(bool &supported) const;

    /**
     * @brief Enable/disable NB-NTN (user-initiated). ICCID must be valid when enabling.
     * @param enable true to enable, false to disable.
     * @param emergency true to use emergency flow.
     * @param iccid eSIM/USIM ICCID string required by the modem for provisioning.
     */
    telux::common::ErrorCode enableNtn(bool enable, bool emergency, const std::string &iccid);

    /** @brief Get current NTN state (DISABLED/OUT_OF_SERVICE/IN_SERVICE). */
    telux::satcom::NtnState getNtnState() const;

    /** @brief Get service status (AVAILABLE/UNAVAILABLE/FAILED). */
    telux::common::ServiceStatus getServiceStatus() const;

    /** @brief Ask modem for capabilities (e.g., MTU maxDataSize). */
    telux::common::ErrorCode getNtnCapabilities(telux::satcom::NtnCapabilities &cap) const;

    /** @brief Turn terrestrial scan on/off while in NB-NTN mode. */
    telux::common::ErrorCode enableCellularScan(bool enable);

    /**
     * @brief Push system-selection specifiers (SFL) from config or user.
     * @details This maps to `updateSystemSelectionSpecifiers()`.
     */
    telux::common::ErrorCode updateSystemSelectionSpecifiers(
        const std::vector<telux::satcom::SystemSelectionSpecifier> &sfl);

    /**
     * @brief Enqueue an audio frame to uplink (non-blocking).
     * @details The frame will be segmented to <= MTU and sent chunk-by-chunk.
     *          Congestion is avoided by waiting for an ACK between PDUs.
     * @param data pointer to PCM/encoded bytes.
     * @param size number of bytes.
     * @param emergency emergency bearers if true.
     * @return telux::common::ErrorCode::SUCCESS means accepted into queue.
     */
    telux::common::ErrorCode sendAudioFrame(const uint8_t *data, size_t size, bool emergency);

    /** @brief Abort all outstanding data (maps to `abortData()`). */
    telux::common::ErrorCode abortUplink();

    // ---------------- INtnListener overrides (TelSDK) ----------------
    void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) override;
    void onNtnStateChange(telux::satcom::NtnState state) override;
    void onCapabilitiesChange(telux::satcom::NtnCapabilities capabilities) override;
    void onSignalStrengthChange(telux::satcom::SignalStrength newStrength) override;
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onDataAck(telux::common::ErrorCode err, telux::satcom::TransactionId id) override;
    void onCellularCoverageAvailable(bool isCellularCoverageAvailable) override;

private:
    // Config
    ntncfg::Config cfg_;

    // TelSDK
    std::shared_ptr<telux::satcom::INtnManager> ntn_;
    std::atomic<uint32_t> mtu_;  // maxDataSize; 0 means unknown

    // Callbacks
    C_SatCommCallbacks cb_;

    // Worker for ACK-paced send
    struct Pdu {
        std::vector<uint8_t> bytes;
        bool emergency;
    };
    std::thread worker_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<Pdu> q_;
    std::atomic<bool> running_;
    std::atomic<bool> inFlight_; // true = awaiting ACK
    std::atomic<bool> serviceOk_; // IN_SERVICE + AVAILABLE gate

    // internals
    void startWorker_();
    void stopWorker_();
    void workerLoop_();
    telux::common::ErrorCode sendChunk_(
        const uint8_t *data, size_t size, bool emergency, telux::satcom::TransactionId &tid);
};

} // namespace harman