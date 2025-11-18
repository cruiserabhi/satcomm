/**
 * @file AudioTest.cpp
 * @brief Simple NB-NTN audio push/receive sample using C_SatCommService.
 *
 * Build-time assumptions:
 *  - Qualcomm TelSDK headers+lib available (telux headers in /usr/include/telux; link telux_satcom)
 *  - Your cJSON-based NtnConfigLoader.hpp is present (header-only) and cJSON sources are compiled in.
 *  - C++11 only.
 */

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

#include "HarmanSatCommService/C_SatCommService.hpp"

// ---------- Hardcoded test paths/macros ----------
#define AUDIO_INPUT_FILE  "/tmp/ntn_audio_input.raw"   // 16-bit PCM or any byte stream
#define AUDIO_OUTPUT_FILE "/tmp/ntn_audio_downlink.raw"
#define CONFIG_JSON_PATH  "/etc/ntn/ntn_prod_like_ipv4.json" // any of your prepared profiles
// ICCID must be provisioned; you can also set environment NTN_ICCID instead.
#define DEFAULT_ICCID     "REPLACE_WITH_VALID_ICCID"

// Utility: load entire file to vector
static bool load_file(const char *path, std::vector<uint8_t> &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    std::streampos sz = f.tellg();
    if (sz <= 0) return false;
    out.resize((size_t)sz);
    f.seekg(0, std::ios::beg);
    f.read((char*)out.data(), sz);
    return f.good();
}

int main() {
    // 1) Load config (header-only cJSON loader already validated elsewhere)
    ntncfg::Loader loader;
    loader.load(CONFIG_JSON_PATH);
    const ntncfg::Config &cfg = loader.config();

    // 2) Construct service and wire callbacks
    std::shared_ptr<harman::C_SatCommService> svc =
        std::make_shared<harman::C_SatCommService>(cfg);

    // Open output file (append)
    std::ofstream downlink(AUDIO_OUTPUT_FILE, std::ios::binary | std::ios::app);

    harman::C_SatCommCallbacks cb;
    cb.onIncomingData = [&](const uint8_t *data, uint32_t sz) {
        std::cout << "[RX] " << sz << " bytes\n";
        if (downlink) {
            downlink.write((const char*)data, sz);
            downlink.flush();
        }
    };
    cb.onNtnState = [&](telux::satcom::NtnState st) {
        std::cout << "[STATE] " << (int)st << std::endl;
    };
    cb.onCapabilities = [&](const telux::satcom::NtnCapabilities &cap) {
        std::cout << "[CAP] maxDataSize=" << cap.maxDataSize << std::endl;
    };
    cb.onSignalStrength = [&](telux::satcom::SignalStrength s) {
        std::cout << "[RSSI] bucket=" << (int)s << std::endl;
    };
    cb.onServiceStatus = [&](telux::common::ServiceStatus ss) {
        std::cout << "[SRV] " << (int)ss << std::endl;
    };
    cb.onDataAck = [&](telux::common::ErrorCode ec, telux::satcom::TransactionId tid) {
        std::cout << "[ACK] ec=" << (int)ec << " tid=" << tid << std::endl;
    };
    cb.onCellularCoverageAvailable = [&](bool avail) {
        std::cout << "[CELL] coverage=" << (int)avail << std::endl;
    };
    svc->setCallbacks(cb);

    // 3) Init TelSDK + NtnManager
    if (!svc->init()) {
        std::cerr << "Init failed\n";
        return 2;
    }

    // 4) Support check (from TelSDK)
    bool supported = false;
    svc->isNtnSupported(supported);
    if (!supported) {
        std::cerr << "NB-NTN not supported on this UE/SIM\n";
        return 3;
    }

    // 5) Enable NB-NTN (non-emergency). ICCID must be valid (ENV or macro).
    const char *envIccid = ::getenv("NTN_ICCID");
    std::string iccid = envIccid ? envIccid : DEFAULT_ICCID;
    telux::common::ErrorCode ec = svc->enableNtn(true, false, iccid);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "enableNtn failed, ec=" << (int)ec << "\n";
        return 4;
    }

    // Optional: switch scan behavior while in NB-NTN
    svc->enableCellularScan(false); // prefer satellite continuity

    // 6) Load audio file and push asynchronously (non-blocking enqueue)
    std::vector<uint8_t> pcm;
    if (!load_file(AUDIO_INPUT_FILE, pcm)) {
        std::cerr << "Failed to load " << AUDIO_INPUT_FILE << "\n";
        return 5;
    }
    std::cout << "Loaded " << pcm.size() << " bytes; sending...\n";

    // One big frame; the library will segment to MTU and pace by ACK
    ec = svc->sendAudioFrame(pcm.data(), pcm.size(), false);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "sendAudioFrame enqueue failed, ec=" << (int)ec << "\n";
    }

    // Give the worker some time to run; a real app would have an event loop.
    // (No busy-wait; just sleep here for demo simplicity.)
    ::sleep(10);

    // 7) Clean shutdown
    svc->shutdown();
    std::cout << "Done.\n";
    return 0;
}
