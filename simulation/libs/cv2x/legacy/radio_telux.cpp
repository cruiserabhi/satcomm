/*
 *  Copyright (c) 2024-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdint.h>
#include <stdlib.h>

#include "telux/cv2x/legacy/v2x_radio_api.h"
#include "v2x_log.h"

#include <net/if.h>
#include <sys/time.h>

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cstring>
#include <future>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xRxMetaDataHelper.hpp>

using namespace std;

using std::atomic;
using std::begin;
using std::bitset;
using std::condition_variable;
using std::dynamic_pointer_cast;
using std::end;
using std::lock_guard;
using std::make_shared;
using std::map;
using std::mutex;
using std::promise;
using std::shared_ptr;
using std::string;
using std::stringstream;
using std::thread;
using std::vector;
using telux::common::ErrorCode;
using telux::common::ServiceStatus;
using telux::common::Status;
using telux::cv2x::Cv2xCauseType;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xRadioCapabilities;
using telux::cv2x::Cv2xRxMetaDataHelper;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::EventFlowInfo;
using telux::cv2x::GlobalIPUnicastRoutingInfo;
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xRadioListener;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::ICv2xTxFlow;
using telux::cv2x::ICv2xTxRxSocket;
using telux::cv2x::ICv2xTxStatusReportListener;
using telux::cv2x::IPv6AddrType;
using telux::cv2x::L2FilterInfo;
using telux::cv2x::MacDetails;
using telux::cv2x::Periodicity;
using telux::cv2x::Priority;
using telux::cv2x::RadioConcurrencyMode;
using telux::cv2x::SlssRxInfo;
using telux::cv2x::SocketInfo;
using telux::cv2x::SpsFlowInfo;
using telux::cv2x::SpsSchedulingInfo;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::TrustedUEInfo;
using telux::cv2x::TrustedUEInfoList;
using telux::cv2x::TxStatusReport;

class SlssRxListener;

#define API_VERSION_NUMBER (1)

/* FIXME: get these numbers from some .H file in cv2x_tools.h  or QMI
 * wireless_data_service_*.h TQ presently only supports 2.  Ideally this limit
 * would be returned via QMI */
#define MAX_SUPPORTED_SPS_FLOWS (2)
#define MAX_TQ_PRECONFIGURED_SERVICE_ID (4)

/* Should be less than or equal to WDS_V2X_NON_SPS_MAX_FLOWS_V01 which comes
 * from QMI header services/wireless_data_service_v01.h:#define
 * WDS_V2X_NON_SPS_MAX_FLOWS_V01 255 */
#define MAX_SUPPORTED_EVENT_FLOWS (254)

/* Handle of CV2X IP interface */
#define V2X_RADIO_IP_HANDLE (1)

/* Handle of CV2X non-IP interface */
#define V2X_RADIO_NON_IP_HANDLE (2)

#define V2X_IFACE_NUM (2)

#ifndef BUILD_STRING
#define BUILD_STRING "unknown build info"
#endif

/* Deprecated: These hard coded interface names should no longer be used
 * OVERRIDE_* interface names for the two (ip/non_ip) linux
 * interface names.
 */
static const string OVERRIDE_IP_IFACE("rmnet_data0");
static const string OVERRIDE_NON_IP_IFACE("rmnet_data1");

/* Default Radio type. Currently unused by the radio.
 */
static const TrafficCategory DEFAULT_TRAFFIC_CATEGORY = TrafficCategory::SAFETY_TYPE;

/** @brief CONVERSION_OFFSET_QMI_TO_V2X_PRIORITY is presently 1,
 *  because the v2x_radio_api enum starts at 0, and the QMI priorities use 1 as
 * the highest priority.
 */
static constexpr uint16_t CONVERSION_OFFSET_QMI_TO_V2X_PRIORITY = 1u;

/** @brief The default multicast broadcast address which is used for V2X TX,
 *  packets, but you must be bound to the particular rmnet_dataX interface in
 * order for the broadcast to be the intended IP or non-IP packet over the air.
 */
static const string DEFAULT_DESTINATION_ADDR("ff02::1");

typedef struct {
    int sps_id;  // ID returned during registration -- needed for reservation
                 // modification and dereg
#if 0
    int used; // flag indicating if the array element for sps flow is active/used, 0=unused/ or previously used/dereg
    int fd;
    sps_flow_t flow; // contains  v2x_id as "service_id" , port# and sps params
    v2x_per_sps_reservation_calls_t calls;
#endif
} per_sps_instance_t;

typedef struct {
    int used;  // flag indicating if the array element for sps flow is active/used,
               // 0=unused/ or previously used/dereg
#if 0
    int fd;
    non_sps_flow_t flow; // Contains v2x_id=service_id & port
#endif
} per_non_sps_instance_t;

// The session concept included in this implementation is not needed/used and
// can be eliminated We most likely only want to support one session per
// linked/instance
typedef struct {
    int active;  // boolean flag
#if 0
    data_call_type_t data_call_type ;  // Is non-IP or IP
    char iface_name[IFNAMSIZ]; // interface name for IP or NON_IP traffic
    v2x_radio_macphy_params_t macphy_p;
#endif
} instance_t;

typedef struct {
    instance_t ses[V2X_MAX_RADIO_SESSIONS];
} sessions_t;

typedef struct {
    v2x_radio_handle_t id;
    TrafficIpType ipType;
    string ifName;
} iface_handle_t;

typedef struct {
    map<int, shared_ptr<ICv2xRxSubscription>> sock_to_rx_map;
    map<int, shared_ptr<ICv2xTxFlow>> sock_to_tx_map;
    map<int, shared_ptr<ICv2xTxRxSocket>> fd_to_tcp_sock_map;
    mutex container_mutex;
    Cv2xStatusEx cv2x_status;
    ServiceStatus service_status = ServiceStatus::SERVICE_UNAVAILABLE;
    mutex service_mutex;
    mutex capability_mutex;
    Cv2xRadioCapabilities capabilities;
    v2x_radio_calls_t *callbacks = NULL;
    map<uint32_t, v2x_per_sps_reservation_calls_t *> sps_callback_map;
    v2x_event_t event          = V2X_INACTIVE;
    void *context              = NULL;  // optional client context for callbacks
    v2x_concurrency_sel_t mode = V2X_WWAN_NONCONCURRENT;
    atomic<bool> doing_periodic_measures{true};
    string dest_ip_addr        = DEFAULT_DESTINATION_ADDR;
    bool need_initial_callback = true;  // flag to make sure apps always get at
                                        // least one status callback.
    uint64_t last_status_timestamp_usec = 0ull;
    uint16_t dest_portnum_override      = 0u;  // Really only used for sim platform.
                                          // uses Source port, unless nonzero
    uint16_t rx_portnum = V2X_RX_WILDCARD_PORTNUM;  // Really only used for sim platform.
                                                    // Real modem does not care
    v2x_radio_macphy_params_t macphy_p = {0};

    std::array<iface_handle_t, V2X_IFACE_NUM> ifHandles
        = {{{V2X_RADIO_IP_HANDLE, TrafficIpType::TRAFFIC_IP, OVERRIDE_IP_IFACE},
            {V2X_RADIO_NON_IP_HANDLE, TrafficIpType::TRAFFIC_NON_IP, OVERRIDE_NON_IP_IFACE}}};
    mutex cv2x_mgr_init_mutex;
    condition_variable cv2x_mgr_init_cv;
    mutex ext_radio_status_mtx;
    v2x_ext_radio_status_listener ext_radio_status_listener = nullptr;
    // flag to make sure the ext radio status listener gets the initial cv2x
    // status
    std::atomic<bool> need_initial_ext_callback{false};
    // Keep following at the end to make sure they will be destructed
    // before the others.
    shared_ptr<ICv2xRadioManager> radio_mgr       = nullptr;
    shared_ptr<ICv2xRadio> radio                  = nullptr;
    shared_ptr<ICv2xRadioListener> radio_listener = nullptr;
    shared_ptr<ICv2xListener> cv2x_listener       = nullptr;
    mutex slss_listener_mtx;
    vector<shared_ptr<SlssRxListener>> slss_listeners;
} cv2x_config_state_t;

static cv2x_config_state_t state_g;

/* SVM Added for Congestion testing, override second Event flow as another SPS
 * just in case ITS stack is not doing it.
 */
int config_override_event_as_sps    = 0;  // KABOB: need to load this from a config file
int config_override_event_flow_prio = 0;  // Highest priority

/* Decide whether to connect the transmit sockets or not */
static int socket_connect_enabled = 1;

/* Thread used to block on init and invoke user-supplied init callback */
static thread init_thread;

/* Maps Cv2x status to string */
static map<Cv2xStatusType, string> Cv2xStatusType2String
    = {{Cv2xStatusType::INACTIVE, "INACTIVE"}, {Cv2xStatusType::ACTIVE, "ACTIVE"},
        {Cv2xStatusType::SUSPENDED, "SUSPENDED"}, {Cv2xStatusType::UNKNOWN, "UNKNOWN"}};

/* Maps v2x_event_t to string */
static map<v2x_event_t, string> V2xEventType2String
    = {{V2X_INACTIVE, "INACTIVE"}, {V2X_ACTIVE, "ACTIVE"}, {V2X_TX_SUSPENDED, "TX_SUSPENDED"},
        {V2X_RX_SUSPENDED, "RX_SUSPENDED"}, {V2X_TXRX_SUSPENDED, "TXRX_SUSPENDED"}};

//*****************************************************************************
// Forward declarations
//*****************************************************************************
static void cv2x_status_listener(const Cv2xStatusEx &status);
static void cv2x_l2addr_change_listener(uint32_t new_l2_address);
static int close_rx_sub(shared_ptr<ICv2xRxSubscription> &rx_sub);
static int close_tx_flow(shared_ptr<ICv2xTxFlow> &tx_flow);
static void cv2x_capability_listener(
    const Cv2xRadioCapabilities &capabilities, telux::common::ErrorCode error);
static void cv2x_service_status_listener(const ServiceStatus &serviceStatus);
static void cv2x_sps_scheduling_changed_listener(const SpsSchedulingInfo &schedulingInfo);
static shared_ptr<ICv2xTxRxSocket> find_tcp_socket(int fd);
static void add_tcp_socket(int fd, const shared_ptr<ICv2xTxRxSocket> &sock);
static void erase_tcp_socket(int fd);
static int close_tcp_socket(shared_ptr<ICv2xTxRxSocket> &sock);
static void convert_tx_status_report(const TxStatusReport &in, v2x_tx_status_report_t &out);
static void convert_v2x_ext_radio_status(const Cv2xStatusEx &in, v2x_radio_status_ex_t *out);
static void convert_v2x_slss_rx_info(const SlssRxInfo &in, v2x_slss_rx_info_t *out);

//*****************************************************************************
// This class implements the onSlssRxInfoChanged interface
//*****************************************************************************
class SlssRxListener : public ICv2xListener {
 public:
    SlssRxListener(v2x_slss_rx_listener cb) {
        cb_ = cb;
    }

    virtual void onSlssRxInfoChanged(const SlssRxInfo &slssInfo) {
        v2x_slss_rx_info_t info;
        convert_v2x_slss_rx_info(slssInfo, &info);

        if (cb_) {
            cb_(&info);
        }
    }

    v2x_slss_rx_listener getCallback() {
        return cb_;
    }

 private:
    v2x_slss_rx_listener cb_ = nullptr;  // user-supplied C callback
};

//*****************************************************************************
// This class implements the ICv2xTxStatusReportListener interface and involves
// user-supplied C-style callbak.
//*****************************************************************************
class TxStatusReportListener : public ICv2xTxStatusReportListener {
 public:
    TxStatusReportListener(v2x_tx_status_report_listener cb) {
        cb_ = cb;
    }

    virtual void onTxStatusReport(const TxStatusReport &info) {
        v2x_tx_status_report_t rpt;
        convert_tx_status_report(info, rpt);

        if (cb_) {
            cb_(rpt);
        }
    }

 private:
    v2x_tx_status_report_listener cb_;  // user-supplied C callback
};

//*****************************************************************************
// This class implements the ICv2xRadioListener interface
//*****************************************************************************
class RadioListener : public ICv2xRadioListener {
    virtual void onL2AddrChanged(uint32_t newL2Address) {
        cv2x_l2addr_change_listener(newL2Address);
    }

    virtual void onSpsSchedulingChanged(const SpsSchedulingInfo &schedulingInfo) {
        cv2x_sps_scheduling_changed_listener(schedulingInfo);
    }

    virtual void onCapabilitiesChanged(const Cv2xRadioCapabilities &capabilities) {
        cv2x_capability_listener(capabilities, telux::common::ErrorCode::SUCCESS);
    }
};

//*****************************************************************************
// This class implements the ICv2xListener interface
//*****************************************************************************
class Cv2xListener : public ICv2xListener {
    void onServiceStatusChange(ServiceStatus status) override {
        cv2x_service_status_listener(status);
    }

    void onStatusChanged(Cv2xStatusEx status) override {
        cv2x_status_listener(status);
    }
};

//*****************************************************************************
// Template function to convert from one enum to another. This assumes that
// the two enum classes match in terms of possible enum values and their
// associated ordinals. It also assumes that the enum types can be safely
// converted into a signed integer.
//*****************************************************************************
template <typename A, typename B>
static void convert_enum(A src, B &dst) {
    int tmp = static_cast<int>(src);
    dst     = static_cast<B>(tmp);
}

//*****************************************************************************
// Conversion function for Tx status report parameters
//*****************************************************************************
static void convert_tx_status_report(const TxStatusReport &in, v2x_tx_status_report_t &out) {
    for (uint32_t i = 0; i < V2X_MAX_ANTENNAS_SUPPORTED; ++i) {
        convert_enum(in.rfInfo[i].status, out.rf_info[i].status);
        convert_enum(in.rfInfo[i].power, out.rf_info[i].power);
    }
    out.num_rb   = in.numRb;
    out.start_rb = in.startRb;
    out.mcs      = in.mcs;
    out.seg_num  = in.segNum;
    convert_enum(in.segType, out.seg_type);
    convert_enum(in.txType, out.tx_type);
    out.ota_timing = in.otaTiming;
    out.port       = in.port;
}

//*****************************************************************************
// Retrieve interface handle using handle ID or old interface name
//*****************************************************************************
static int getIfHandle(
    const v2x_radio_handle_t id, const char *interface, iface_handle_t &ifHandle) {
    for (auto handle : state_g.ifHandles) {
        if ((V2X_RADIO_HANDLE_BAD != id and id == handle.id)
            or (interface and 0 == handle.ifName.compare(interface))) {
            ifHandle = handle;
            return 0;
        }
    }

    LOGE("Failed to find handle ID:%d.\n", id);
    return -EPERM;
}

//*****************************************************************************
// Conversion functions for SPS reservation parameters
//*****************************************************************************
static string supportedPeriodicityToString(const Cv2xRadioCapabilities &capabilities) {
    stringstream ss;

    ss << "V2X Supported Periodicity:"
       << "\n";

    for (auto i = 0u; i < capabilities.periodicities.size(); ++i) {
        ss << static_cast<uint64_t>(capabilities.periodicities[i]) << "ms ";
    }

    ss << " Only\n";

    return ss.str();
}

static Periodicity convert_interval_to_Periodicity(int period_interval_ms) {
    /**< Bandwidth reserved, periodicity interval in milliseconds. However,
     * for backwards compatibility, with earlier releases of this API --
     * FIXME eventually -- if called with number below 10 or below, assume that is
     * a Hz figure and not an interval
     */
    if (period_interval_ms <= 10) {
        return Periodicity::PERIODICITY_10MS;
    } else if (period_interval_ms <= 20) {
        return Periodicity::PERIODICITY_20MS;
    } else if (period_interval_ms <= 50) {
        return Periodicity::PERIODICITY_50MS;
    } else if (period_interval_ms <= 100) {
        return Periodicity::PERIODICITY_100MS;
    }
    return Periodicity::PERIODICITY_100MS;
}

static int convert_interval_to_PeriodicityMs(int period_interval_ms) {

    for (auto i = 0u; i < state_g.capabilities.periodicities.size(); ++i) {
        if (state_g.capabilities.periodicities[i] == static_cast<uint64_t>(period_interval_ms)) {
            return period_interval_ms;
        }
    }

    return -EINVAL;
}

static int convert_reservation(const v2x_tx_bandwidth_reservation_t &res, SpsFlowInfo &spsInfo) {
    convert_enum(res.priority, spsInfo.priority);
    spsInfo.periodicity = convert_interval_to_Periodicity(res.period_interval_ms);
    int ret             = convert_interval_to_PeriodicityMs(res.period_interval_ms);
    if (ret < 0) {
        LOGE("Requested periodicity not supported\n");
        return -EINVAL;
    }
    spsInfo.periodicityMs           = static_cast<uint64_t>(ret);
    spsInfo.nbytesReserved          = res.tx_reservation_size_bytes;
    spsInfo.autoRetransEnabledValid = false;
    spsInfo.peakTxPowerValid        = false;
    spsInfo.mcsIndexValid           = false;
    return 0;
}

static int convert_sps_flow_info(
    const v2x_tx_sps_flow_info_t &sps_flow_info, SpsFlowInfo &spsInfo) {
    auto &flow_info = sps_flow_info.flow_info;
    if (convert_reservation(sps_flow_info.reservation, spsInfo) < 0) {
        LOGE("%s\n", supportedPeriodicityToString(state_g.capabilities).c_str());
        return -EINVAL;
    }

    if (flow_info.retransmit_policy != V2X_AUTO_RETRANSMIT_DONT_CARE) {
        spsInfo.autoRetransEnabledValid = true;
        spsInfo.autoRetransEnabled      = flow_info.retransmit_policy;
    }
    if (flow_info.default_tx_power_valid) {
        spsInfo.peakTxPowerValid = true;
        spsInfo.peakTxPower      = flow_info.default_tx_power;
    }
    if (flow_info.mcs_index_valid) {
        spsInfo.mcsIndexValid = true;
        spsInfo.mcsIndex      = flow_info.mcs_index;
    }
    if (flow_info.tx_pool_id_valid) {
        spsInfo.txPoolIdValid = true;
        spsInfo.txPoolId      = flow_info.tx_pool_id;
    }
    return 0;
}

static void convert_event_flow_info(const v2x_tx_flow_info_t &flow_info, EventFlowInfo &eventInfo) {
    if (flow_info.retransmit_policy != V2X_AUTO_RETRANSMIT_DONT_CARE) {
        eventInfo.autoRetransEnabledValid = true;
        eventInfo.autoRetransEnabled      = flow_info.retransmit_policy;
    }
    if (flow_info.default_tx_power_valid) {
        eventInfo.peakTxPowerValid = true;
        eventInfo.peakTxPower      = flow_info.default_tx_power;
    }
    if (flow_info.mcs_index_valid) {
        eventInfo.mcsIndexValid = true;
        eventInfo.mcsIndex      = flow_info.mcs_index;
    }
    if (flow_info.tx_pool_id_valid) {
        eventInfo.txPoolIdValid = true;
        eventInfo.txPoolId      = flow_info.tx_pool_id;
    }
    if (flow_info.is_unicast_valid) {
        eventInfo.isUnicast = flow_info.is_unicast;
    }
}

/**
 * Convert telsdk Cv2xStatus to v2x_event_t
 */
static v2x_event_t convert_status_to_event(Cv2xStatus status) {
    v2x_event_t event;
    if (status.rxStatus == Cv2xStatusType::ACTIVE and status.txStatus == Cv2xStatusType::ACTIVE) {
        event = V2X_ACTIVE;
    } else if (status.rxStatus == Cv2xStatusType::ACTIVE
               and status.txStatus == Cv2xStatusType::SUSPENDED) {
        /**< Radio event indication on small loss of timing precision, and TX no
         * longer supported */
        event = V2X_TX_SUSPENDED;
    } else if (status.rxStatus == Cv2xStatusType::SUSPENDED
               and status.txStatus == Cv2xStatusType::ACTIVE) {
        /**< Radio event indication on small loss of timing precision, and RX no
         * longer supported */
        event = V2X_RX_SUSPENDED;
    } else if (status.rxStatus == Cv2xStatusType::SUSPENDED
               and status.txStatus == Cv2xStatusType::SUSPENDED) {
        event = V2X_TXRX_SUSPENDED;
    } else if (status.rxStatus == Cv2xStatusType::INACTIVE
               or status.txStatus == Cv2xStatusType::INACTIVE) {
        event = V2X_INACTIVE;
    } else {
        // Cv2xStatus type can be UNKNOWN.  If it is UNKNOWN,
        // we have to assume its inactive for purposes of
        // converting to v2x_event_t
        event = V2X_INACTIVE;
    }
    return event;
}

/**
 * Convert telsdk SpsSchedulingInfo struct to v2x_sps_mac_details_t
 */
static v2x_sps_mac_details_t convert_sps_scheduling_info(
    const SpsSchedulingInfo &spsSchedulingInfo) {
    v2x_sps_mac_details_t mac_details = {0};
    mac_details.periodicity_in_use_ns = spsSchedulingInfo.periodicity * 1000000u;
    mac_details.utc_time_ns           = spsSchedulingInfo.utcTime;
    return mac_details;
}

static string capabilitiesToString(const Cv2xRadioCapabilities &capabilities) {
    stringstream ss;

    ss << "V2X Capabilities:"
       << "\n";
    ss << "\t"
       << "linkIpMtuBytes: " << static_cast<int>(capabilities.linkIpMtuBytes) << "\n";
    ss << "\t"
       << "linkNonMtuBytes: " << static_cast<int>(capabilities.linkNonIpMtuBytes) << "\n";
    ss << "\t"
       << "maxSupportedConcurrency: "
       << ((capabilities.maxSupportedConcurrency == RadioConcurrencyMode::WWAN_CONCURRENT)
                  ? "WWAN_CONCURRENT"
                  : "WWAN_NONCONCURRENT")
       << "\n";
    ss << "\t"
       << "nonIpTxPayloadOffsetBytes: " << static_cast<int>(capabilities.nonIpTxPayloadOffsetBytes)
       << "\n";
    ss << "\t"
       << "nonIpRxPayloadOffsetBytes: " << static_cast<int>(capabilities.nonIpRxPayloadOffsetBytes)
       << "\n";
    ss << "\t"
       << "periodicitiesSupported: " << capabilities.periodicitiesSupported << "\n";
    ss << "\t"
       << "maxNumAutoRetransmissions: " << static_cast<int>(capabilities.maxNumAutoRetransmissions)
       << "\n";
    ss << "\t"
       << "layer2MacAddressSize: " << static_cast<int>(capabilities.layer2MacAddressSize) << "\n";
    ss << "\t"
       << "prioritiesSupported: " << capabilities.prioritiesSupported << "\n";
    ss << "\t"
       << "maxNumSpsFlows: " << static_cast<int>(capabilities.maxNumSpsFlows) << "\n";
    ss << "\t"
       << "maxNumNonSpsFlows: " << static_cast<int>(capabilities.maxNumNonSpsFlows) << "\n";
    ss << "\t"
       << "maxTxPower: " << static_cast<int>(capabilities.maxTxPower) << "\n";
    ss << "\t"
       << "minTxPower: " << static_cast<int>(capabilities.minTxPower) << "\n";
    ss << "\t"
       << "TX pool ids supported - size: " << capabilities.txPoolIdsSupported.size() << " values: ";
    for (auto i = 0u; i < capabilities.txPoolIdsSupported.size(); ++i) {
        ss << "Pool ID: " << static_cast<int>(capabilities.txPoolIdsSupported[i].poolId)
           << " minFreq: " << static_cast<int>(capabilities.txPoolIdsSupported[i].minFreq)
           << " maxFreq: " << static_cast<int>(capabilities.txPoolIdsSupported[i].maxFreq) << ", ";
    }
    ss << "\n";

    return ss.str();
}

static std::shared_ptr<ICv2xRadioManager> get_and_init_radio_mgr() {
    auto &factory                = Cv2xFactory::getInstance();
    bool cv2x_mgr_status_updated = false;

    auto cb = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(state_g.cv2x_mgr_init_mutex);
        cv2x_mgr_status_updated = true;
        state_g.service_status  = status;

        state_g.cv2x_mgr_init_cv.notify_all();
    };
    auto radio_mgr = factory.getCv2xRadioManager(cb);
    if (not radio_mgr) {
        LOGE("Failed to acquire Cv2xRadioManager\n");
        return nullptr;
    }

    if (state_g.cv2x_listener == nullptr) {
        state_g.cv2x_listener = make_shared<Cv2xListener>();
        // consider cv2x manager init fail
        if (not state_g.cv2x_listener) {
            state_g.service_status = ServiceStatus::SERVICE_FAILED;
            LOGE("%s: Error instantiating cv2x listener\n", __FUNCTION__);
            return nullptr;
        }
        radio_mgr->registerListener(state_g.cv2x_listener);
    }

    std::unique_lock<std::mutex> lc(state_g.cv2x_mgr_init_mutex);
    state_g.cv2x_mgr_init_cv.wait(lc, [&]() { return cv2x_mgr_status_updated; });

    if (state_g.service_status != ServiceStatus::SERVICE_AVAILABLE) {
        LOGE("Cv2xRadioManager fail to initialize\n");
        radio_mgr = nullptr;
    }

    return radio_mgr;
}

//*****************************************************************************
// Template functions to get the highest and lowest set bits in a bitset
//*****************************************************************************
template <int n>
static uint16_t highestBit(const bitset<n> &bs) {
    for (int i = n - 1; i >= 0; --i) {
        if (bs.test(i)) {
            return i;
        }
    }
    return 0u;
}

template <int n>
static uint16_t lowestBit(const bitset<n> &bs) {
    for (int i = 0; i < n; ++i) {
        if (bs.test(i)) {
            return i;
        }
    }
    return 0u;
}

static void convert_capabilities(v2x_iface_capabilities_t *caps, Cv2xRadioCapabilities tel_caps) {
    caps->link_ip_MTU_bytes     = tel_caps.linkIpMtuBytes;
    caps->link_non_ip_MTU_bytes = tel_caps.linkNonIpMtuBytes;

    convert_enum(tel_caps.maxSupportedConcurrency, caps->max_supported_concurrency);
    caps->non_ip_tx_payload_offset_bytes = tel_caps.nonIpTxPayloadOffsetBytes;
    caps->non_ip_rx_payload_offset_bytes = tel_caps.nonIpRxPayloadOffsetBytes;

    // Default periodicity values
    caps->int_min_periodicity_multiplier_ms = 100u;
    caps->int_maximum_periodicity_ms        = 1000u;

    for (auto &p : tel_caps.periodicities) {
        if (p <= UINT16_MAX) {  // p is 64 bit but caps members are 16 bit
            if (p < caps->int_min_periodicity_multiplier_ms) {
                caps->int_min_periodicity_multiplier_ms = p;
            }
            if (p > caps->int_maximum_periodicity_ms) {
                caps->int_maximum_periodicity_ms = p;
            }
        }
    }

    // 10ms periodicity is not supported
    caps->supports_10ms_periodicity = 0;

    caps->supports_20ms_periodicity
        = tel_caps.periodicitiesSupported[static_cast<int>(Periodicity::PERIODICITY_20MS)];
    caps->supports_50ms_periodicity
        = tel_caps.periodicitiesSupported[static_cast<int>(Periodicity::PERIODICITY_50MS)];
    caps->supports_100ms_periodicity
        = tel_caps.periodicitiesSupported[static_cast<int>(Periodicity::PERIODICITY_100MS)];

    caps->max_quantity_of_auto_retrans = tel_caps.maxNumAutoRetransmissions;

    caps->size_of_layer2_mac_address = tel_caps.layer2MacAddressSize;

    caps->v2x_number_of_priority_levels = 8;
    caps->highest_priority_value        = highestBit<8>(tel_caps.prioritiesSupported);
    caps->lowest_priority_value         = lowestBit<8>(tel_caps.prioritiesSupported);

    caps->max_qty_SPS_flows     = tel_caps.maxNumSpsFlows;
    caps->max_qty_non_SPS_flows = tel_caps.maxNumNonSpsFlows;

    caps->max_tx_pwr = tel_caps.maxTxPower;
    caps->min_tx_pwr = tel_caps.minTxPower;

    auto count = 0u;
    for (auto &e : tel_caps.txPoolIdsSupported) {
        if (count < MAX_POOL_IDS_LIST_LEN) {
            caps->tx_pool_ids_supported[count].pool_id  = e.poolId;
            caps->tx_pool_ids_supported[count].min_freq = e.minFreq;
            caps->tx_pool_ids_supported[count].max_freq = e.maxFreq;
        } else {
            break;
        }
        ++count;
    }
    caps->tx_pool_ids_supported_len = static_cast<uint32_t>(count);
}

//*****************************************************************************
// Since some API functions require us to keep track of the SPS flow,
// event-driven port, or RX registration is associated with each socket, we
// need to have a set of containers and associated functions for searching
// for the RX/TX unit based on the socket fd.
//*****************************************************************************
static shared_ptr<ICv2xRxSubscription> find_rx_sub(int sock) {
    lock_guard<mutex> lock(state_g.container_mutex);
    shared_ptr<ICv2xRxSubscription> rx_sub = nullptr;
    if (1 == state_g.sock_to_rx_map.count(sock)) {
        rx_sub = state_g.sock_to_rx_map[sock];
    }
    return rx_sub;
}

static shared_ptr<ICv2xTxFlow> find_tx_flow(int sock) {
    lock_guard<mutex> lock(state_g.container_mutex);
    shared_ptr<ICv2xTxFlow> tx_flow = nullptr;
    if (1 == state_g.sock_to_tx_map.count(sock)) {
        tx_flow = state_g.sock_to_tx_map[sock];
    }
    return tx_flow;
}

static v2x_per_sps_reservation_calls_t *find_sps_cb(uint32_t sps_id) {
    lock_guard<mutex> lock(state_g.container_mutex);
    v2x_per_sps_reservation_calls_t *cb = NULL;
    if (1 == state_g.sps_callback_map.count(sps_id)) {
        cb = state_g.sps_callback_map[sps_id];
    }
    return cb;
}

static int erase_rx_sub(int sock) {
    lock_guard<mutex> lock(state_g.container_mutex);
    return state_g.sock_to_rx_map.erase(sock);
}

static int erase_tx_flow(int sock) {
    lock_guard<mutex> lock(state_g.container_mutex);
    return state_g.sock_to_tx_map.erase(sock);
}

static int erase_sps_cb(uint32_t sps_id) {
    lock_guard<mutex> lock(state_g.container_mutex);
    return state_g.sps_callback_map.erase(sps_id);
}

static void add_rx_sub(int sock, const shared_ptr<ICv2xRxSubscription> &rx_sub) {
    lock_guard<mutex> lock(state_g.container_mutex);
    state_g.sock_to_rx_map[sock] = rx_sub;
}

static void add_tx_flow(int sock, const shared_ptr<ICv2xTxFlow> &tx_flow) {
    lock_guard<mutex> lock(state_g.container_mutex);
    state_g.sock_to_tx_map[sock] = tx_flow;
}

static void add_sps_cb(uint32_t sps_id, v2x_per_sps_reservation_calls_t *cb) {
    lock_guard<mutex> lock(state_g.container_mutex);
    state_g.sps_callback_map[sps_id] = cb;
}

void v2x_show_all_sessions(FILE *fd) {
    LOGW("%s is currently unimplemented\n", __FUNCTION__);
}

/*******************************************************************************
 * return current time stamp in milliseconds
 * @return long long
 ******************************************************************************/
static __inline uint64_t timestamp_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000LL + (uint64_t)tv.tv_usec;
}

/** @brief v2x_radio_api_versoin() returns the build version number
 * */
v2x_api_ver_t v2x_radio_api_version(void) {
    v2x_api_ver_t version_info;

    version_info.version_num = API_VERSION_NUMBER;
    snprintf(version_info.build_date_str, sizeof(version_info.build_date_str), __DATE__);
    snprintf(version_info.build_time_str, sizeof(version_info.build_time_str), __TIME__);
    snprintf(version_info.build_details_str, sizeof(version_info.build_details_str), BUILD_STRING);

    return version_info;
}

/**
 * Given a traffic priority, that is presumably one of the values between
 * min_priority_value and max_priority_value returned in the
 * v2x_iface_capabilities_t query, this function will convert that to one of the
 * 255 IPV6 traffic class bytes that is used in data-plane to indicate
 * per-packet priority, on non-SPS (event driven) data ports. The
 * v2x_convert_traffic_class_to_priority() is symmetric and reveres operation
 *
 * @param[in] the packet priority that is to be converted to a IPV6 traffic
 * class
 *
 * @return the IPV6 traffic class to be used to achieve the calling input
 * parameter priority level
 */
uint16_t v2x_convert_priority_to_traffic_class(v2x_priority_et priority) {
    return static_cast<uint16_t>(priority) + CONVERSION_OFFSET_QMI_TO_V2X_PRIORITY;
}

/**
 * Given a traffic priority, that is presumably one of the values between
 * min_priority_value and max_priority_value returned in the
 * v2x_iface_capabilities_t query,  this function will convert that to one of
 * the 255 IPV6 traffic class bytes that is used in data-plane to indicate
 * per-packet priority, on non-SPS (event driven) data ports. The
 * v2x_convert_traffic_class_to_priority() is the symmetric reverse operation
 *
 * @param[in] the IPV6 traffic classification, that presumably just came in on a
 * packet from the radio,
 *
 * @return priority level (between highest and lowest_priority_value )
 * equivalent to input IPV6 traffic class param
 */
v2x_priority_et v2x_convert_traffic_class_to_priority(uint16_t traffic_class) {
    auto val = traffic_class - CONVERSION_OFFSET_QMI_TO_V2X_PRIORITY;
    if (val > V2X_PRIO_BACKGROUND) {
        LOGW("Invalid traffic_class (%d) encountered\n", traffic_class);
        // TODO: figure out what to return in this case
        return V2X_PRIO_2;
    }

    return static_cast<v2x_priority_et>(val);
}

/**
 * Convert given C++ API Status response to C API v2x_status_enum_type
 *
 * @param[in] status
 *
 * @return v2x_status_enum_type
 */
v2x_status_enum_type convert_status_to_v2x_status(Status status) {
    v2x_status_enum_type result = V2X_STATUS_FAIL;

    switch (status) {
        case Status::SUCCESS:
            result = V2X_STATUS_SUCCESS;
            break;
        case Status::INVALIDSTATE:
            result = V2X_STATUS_RADIO_NOT_READY;
            break;
        default:
            result = V2X_STATUS_FAIL;
            break;
    }

    return result;
}

// Blocking call that sets Global radio object and validates that it was
// inited successfully
v2x_status_enum_type set_and_init_radio_sync(TrafficCategory trafficCategory) {
    if (not state_g.radio_mgr) {
        return V2X_STATUS_FAIL;
    }

    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus
        = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;

    auto cb = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus           = status;
        cv.notify_all();
    };

    state_g.radio = state_g.radio_mgr->getCv2xRadio(trafficCategory, cb);
    if (not state_g.radio) {
        LOGE("%s: Failed to acquire Cv2xRadio\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }
    LOGI("%s: Waiting Cv2x Radio initialization result\n", __FUNCTION__);

    std::unique_lock<std::mutex> lc(mtx);
    cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

    if (cv2xRadioStatus != ServiceStatus::SERVICE_AVAILABLE) {
        LOGE("Cv2xRadio fail to initialize\n");
        // need to set radio to null to release the object
        state_g.radio = nullptr;
        return V2X_STATUS_FAIL;
    }

    return V2X_STATUS_SUCCESS;
}

std::future<v2x_status_enum_type> set_and_init_radio(TrafficCategory trafficCategory) {
    auto f = std::async(std::launch::async,
        [&trafficCategory]() { return set_and_init_radio_sync(trafficCategory); });

    return f;
}

/**
 *  Retrieve the v2x_iface_capabilities_t structure with the capabilities of a
 * particular radio interface attached to the system  Presently, in the
 * sim-platform these are hard-coded,but in actual Beep-Beep, this should come
 * up from the Modem, since it could change with sim-card and geo-fence
 * parameters.
 *
 *  @param[in] iface_name - char * to the interface name we are interested in
 * the capabilities of. interface name to use.  This will be a RMNET interface
 * (HLOS) Ir could be either the interface supplied for IP communication or the
 * one for non-IP (WSMP/Geonetworking etc)
 *
 *  @param[out] capabilities* to the v2x_iface_capabilities_t  with the
 * capabilities of this particular interface
 *
 *  @return
 *      V2X_STATUS_SUCCESS if all went well, and radio is now ready for
 * data-plane sockets to be created and bound V2X_STATUS_FAIL if radio
 * unavailable or other issues V2X_STATUS_NO_MEMORY if resource issue
 */
v2x_status_enum_type v2x_radio_query_parameters(
    const char *iface_name, v2x_iface_capabilities_t *caps) {
    return v2x_radio_query_capabilities(caps);
}

/**
 *  Retrieve the v2x_iface_capabilities_t structure with the capabilities of
 * CV2X radio.
 *
 *  @param[out] capabilities* to the v2x_iface_capabilities_t
 *
 *  @return
 *      V2X_STATUS_SUCCESS if all went well, and radio is now ready for
 * data-plane sockets to be created and bound V2X_STATUS_FAIL if radio
 * unavailable or other issues
 */
v2x_status_enum_type v2x_radio_query_capabilities(v2x_iface_capabilities_t *caps) {
    if (not state_g.radio_mgr || not state_g.radio) {
        return V2X_STATUS_FAIL;
    }

    convert_capabilities(caps, state_g.capabilities);
    return V2X_STATUS_SUCCESS;
}

/* This function should be run everytime a new radio has been started to keep
   global variables updated to their latest values
   */
v2x_status_enum_type set_radio_info(v2x_concurrency_sel_t mode) {
    LOGI("%s: (lte_concurrancy_sel=%d)\n", __FUNCTION__, mode);

    // Request the initial status of the radio and synchronize result callback
    promise<ErrorCode> p_cap;
    auto status = state_g.radio->requestCapabilities(
        [&p_cap](Cv2xRadioCapabilities capabilities, ErrorCode error) {
            state_g.capabilities = capabilities;
            p_cap.set_value(error);
        });

    if (Status::SUCCESS != status || ErrorCode::SUCCESS != p_cap.get_future().get()) {
        LOGE("%s: Failed to obtain initial Cv2xRadioCapabilities\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    v2x_concurrency_sel_t capMode;
    convert_enum(state_g.capabilities.maxSupportedConcurrency, capMode);
    // Warn if requested mode is greater than supported concurrency
    if (mode > capMode) {
        LOGW("init() requested unsupported WWAN/C-V2X concurrency, switching to "
             "supported"
             " mode (%d)\n",
            capMode);
        state_g.mode = capMode;
    } else {
        state_g.mode = mode;
    }

    return V2X_STATUS_SUCCESS;
}

static void cv2x_status_listener(const Cv2xStatusEx &status) {
    v2x_chan_measurements_t measures;

    // ignore unknown status, we should never report unknown status to user
    if (status.status.rxStatus == Cv2xStatusType::UNKNOWN
        or status.status.txStatus == Cv2xStatusType::UNKNOWN) {
        LOGD("Ignore V2X status unknown\n");
        return;
    }

    state_g.last_status_timestamp_usec = timestamp_now();  // keep track of time of
                                                           // the last callback

    if (status.status.cbrValueValid) {
        measures.channel_busy_percentage = status.status.cbrValue * 1.0f;
    } else {
        measures.channel_busy_percentage = -1.0f;
    }

    if (status.timeUncertaintyValid) {
        measures.time_uncertainty = status.timeUncertainty;
    } else {
        measures.time_uncertainty = -1.0f;
    }

    LOGD("Radio listener callback: CBP=%.1f, tx_status=%s, rx_status=%s, "
         "time_uncertainty=%f**\n",
        measures.channel_busy_percentage, Cv2xStatusType2String[status.status.txStatus].c_str(),
        Cv2xStatusType2String[status.status.rxStatus].c_str(), measures.time_uncertainty);

    // Check if state transitioned to Inactive
    if (((state_g.cv2x_status.status.rxStatus == Cv2xStatusType::ACTIVE
             or state_g.cv2x_status.status.rxStatus == Cv2xStatusType::SUSPENDED)
            and status.status.rxStatus == Cv2xStatusType::INACTIVE)
        or ((state_g.cv2x_status.status.txStatus == Cv2xStatusType::ACTIVE
                or state_g.cv2x_status.status.txStatus == Cv2xStatusType::SUSPENDED)
            and status.status.txStatus == Cv2xStatusType::INACTIVE)) {
        LOGD("V2X status transitioned to inactive\n");
        if (state_g.radio) {
            state_g.radio->deregisterListener(state_g.radio_listener);
        }
    }

    state_g.cv2x_status = status;

    /* If status has changed, then further investigation necessary... */
    bool overall_status_changed = false;
    auto event                  = convert_status_to_event(status.status);
    if (state_g.event != event) {
        overall_status_changed = true;
        // Update event based on the latest status state
        state_g.event = event;
        LOGI("Status changed to %s\n", V2xEventType2String[state_g.event].c_str());
    }

    // involve legacy callback if overall status has changed or initial
    // notification is needed
    if (overall_status_changed or state_g.need_initial_callback) {
        if (state_g.callbacks and state_g.callbacks->v2x_radio_status_listener) {
            // Client is getting the initial status of the radio
            state_g.need_initial_callback = false;
            /**< CB made whenever status changes */
            state_g.callbacks->v2x_radio_status_listener(state_g.event, state_g.context);
        }
    }

    // involve new callback if overall status has changed or initial notification
    // is needed
    if (overall_status_changed or state_g.need_initial_ext_callback) {
        v2x_ext_radio_status_listener cb = nullptr;
        {
            lock_guard<mutex> lock(state_g.ext_radio_status_mtx);
            cb = state_g.ext_radio_status_listener;
        }
        if (cb) {
            // Client is getting the initial status of the radio
            state_g.need_initial_ext_callback = false;
            v2x_radio_status_ex_t ext_status;
            convert_v2x_ext_radio_status(status, &ext_status);
            cb(&ext_status);
        }
    }

    if (state_g.callbacks and state_g.callbacks->v2x_radio_chan_meas_listener
        and state_g.doing_periodic_measures) {
        state_g.callbacks->v2x_radio_chan_meas_listener(&measures, state_g.context);
    }
}

static void cv2x_capability_listener(
    const Cv2xRadioCapabilities &capabilities, telux::common::ErrorCode error) {

    LOGD("Capability listener called with error code:%s (%u)\n",
        (error == ErrorCode::SUCCESS) ? "SUCCESS" : "FAILURE", static_cast<int>(error));

    if (error == ErrorCode::SUCCESS) {

        lock_guard<mutex> lock(state_g.capability_mutex);

        state_g.capabilities = capabilities;

        LOGD("%s\n", capabilitiesToString(state_g.capabilities).c_str());

        v2x_iface_capabilities_t caps;

        if (state_g.callbacks and state_g.callbacks->v2x_radio_capabilities_listener) {
            convert_capabilities(&caps, state_g.capabilities);
            state_g.callbacks->v2x_radio_capabilities_listener(&caps, state_g.context);
        }
    }
}

static void cv2x_sps_scheduling_changed_listener(const SpsSchedulingInfo &schedulingInfo) {
    auto cb = find_sps_cb(schedulingInfo.spsId);

    if (cb && cb->v2x_radio_sps_offset_changed) {
        auto details = convert_sps_scheduling_info(schedulingInfo);
        cb->v2x_radio_sps_offset_changed(state_g.context, &details);
    }
}

static void cv2x_service_status_listener(const ServiceStatus &serviceStatus) {

    std::unique_lock<std::mutex> lock(state_g.service_mutex);
    if (state_g.service_status != serviceStatus) {
        if (serviceStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
            LOGI("Service has gone down\n");
        } else {
            LOGI("Service has come back up\n");
        }
        state_g.service_status = serviceStatus;
    }
    lock.unlock();

    if (state_g.callbacks->v2x_service_status_listener) {
        v2x_service_status_t c_status = (serviceStatus == ServiceStatus::SERVICE_AVAILABLE)
                                            ? SERVICE_AVAILABLE
                                            : SERVICE_UNAVAILABLE;
        state_g.callbacks->v2x_service_status_listener(c_status, state_g.context);
    }
}

v2x_event_t cv2x_status_poll(uint64_t *status_age_useconds) {
    uint64_t now         = timestamp_now();
    uint64_t elapsed     = (uint64_t)(now - state_g.last_status_timestamp_usec);
    *status_age_useconds = elapsed;
    return (state_g.event);
}

static void cv2x_l2addr_change_listener(uint32_t new_l2_address) {
    LOGD("L2 address changed to %x\n", new_l2_address);
    if (state_g.callbacks && state_g.callbacks->v2x_radio_l2_addr_changed_listener) {
        state_g.callbacks->v2x_radio_l2_addr_changed_listener(new_l2_address, state_g.context);
    }
}

void v2x_radio_set_log_level(int new_level, int use_syslog) {
    /* Set logging options in this v2x lib */
    v2x_log_level_set(new_level);
    v2x_log_to_syslog(use_syslog);
}

/**
 * Set the destination default IPV6 address that will be used on the socket
 * connect for SPS flows and the Event sockets
 *
 *  @param[in] string pointer to the new address in ascii human readable format
 */
void v2x_set_dest_ipv6_addr(char *new_addr) {
    if (!new_addr) {
        LOGE("%s: argument supplied is NULL\n", __FUNCTION__);
    } else {
        string newAddr(new_addr);
        LOGE("%s is not supported\n", __FUNCTION__);
    }
}

void v2x_disable_socket_connect() {
    socket_connect_enabled = 0;
}

void v2x_set_dest_port(uint16_t portnum) {
    state_g.dest_portnum_override = portnum;
    LOGD("destination portnum addr changed to %d\n", state_g.dest_portnum_override);
}

void v2x_set_rx_port(uint16_t portnum) {
    state_g.rx_portnum = portnum;
    LOGD("RX listen portnum to %d\n", state_g.rx_portnum);
}

v2x_event_t v2x_radio_get_status(void) {
    v2x_event_t event = V2X_INACTIVE;
    Cv2xStatusEx exStatus;
    promise<ErrorCode> p;

    auto radio_mgr = get_and_init_radio_mgr();
    if (not radio_mgr) {
        LOGE("Failed to initialize Cv2xRadioManager\n");
        return event;
    }

    auto ret = radio_mgr->requestCv2xStatus([&p, &exStatus](Cv2xStatusEx status, ErrorCode error) {
        exStatus = status;
        p.set_value(error);
    });

    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Failed to obtain Cv2xStatus\n", __FUNCTION__);
    } else {
        event = convert_status_to_event(exStatus.status);
        LOGI("V2X Status %s\n", V2xEventType2String[event].c_str());
    }

    return event;
}

// This is a blocking call that returns once the radio has been initialized and
// the cv2x data calls has been started
v2x_radio_handle_t v2x_radio_init(
    char *interface_name, v2x_concurrency_sel_t mode, v2x_radio_calls_t *callbacks_p, void *ctx_p) {
    if (!interface_name) {
        LOGE("%s: Interface is NULL\n", __FUNCTION__);
        return V2X_RADIO_HANDLE_BAD;
    }

    // Use old interface names to determine TrafficIpType
    iface_handle_t ifHandle;
    if (getIfHandle(V2X_RADIO_HANDLE_BAD, interface_name, ifHandle)) {
        return V2X_RADIO_HANDLE_BAD;
    }

    traffic_ip_type_t ip_type;
    convert_enum(ifHandle.ipType, ip_type);

    return v2x_radio_init_v2(ip_type, mode, callbacks_p, ctx_p);
}

v2x_radio_handle_t v2x_radio_init_v2(traffic_ip_type_t ip_type, v2x_concurrency_sel_t mode,
    v2x_radio_calls_t *callbacks_p, void *ctx_p) {
    v2x_radio_handle_t handle = V2X_RADIO_HANDLE_BAD;

    LOGD("%s: traffic type:%d\n", __FUNCTION__, ip_type);

    if (ip_type == TRAFFIC_IP) {
        if (v2x_radio_init_v3(mode, callbacks_p, ctx_p, &handle, nullptr)) {
            return V2X_RADIO_HANDLE_BAD;
        }
    } else {
        if (v2x_radio_init_v3(mode, callbacks_p, ctx_p, nullptr, &handle)) {
            return V2X_RADIO_HANDLE_BAD;
        }
    }

    return handle;
}

int v2x_radio_init_v3(v2x_concurrency_sel_t mode, v2x_radio_calls_t *callbacks_p, void *ctx_p,
    v2x_radio_handle_t *ip_handle_p, v2x_radio_handle_t *non_ip_handle_p) {
    if (!ip_handle_p and !non_ip_handle_p) {
        LOGE("%s: Invalid iface handle pointer\n", __FUNCTION__);
        return -EINVAL;
    }

    state_g.callbacks = callbacks_p;
    state_g.mode      = mode;  // Accept requested mode until we can verify capabilities
    state_g.context   = ctx_p;
    state_g.dest_portnum_override
        = 0;  // Really only used for sim platform. Real modem does not care
    state_g.rx_portnum     = V2X_RX_WILDCARD_PORTNUM;
    state_g.service_status = ServiceStatus::SERVICE_AVAILABLE;

    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return -EPERM;
    }

    auto radio_status = set_and_init_radio(DEFAULT_TRAFFIC_CATEGORY);

    // Request the initial status of v2x mode and synchronize result callback
    promise<ErrorCode> p;
    auto status = state_g.radio_mgr->requestCv2xStatus([&p](Cv2xStatusEx status, ErrorCode error) {
        state_g.cv2x_status = status;
        p.set_value(error);
    });

    if (Status::SUCCESS != status) {
        LOGE("%s: Failed to request for Cv2x status\n", __FUNCTION__);
        return -EPERM;
    }

    auto error = p.get_future().get();
    if (ErrorCode::SUCCESS == error) {
        // Update event based on the latest status state
        cv2x_status_listener(state_g.cv2x_status);
    } else {
        LOGE("%s: Failed to obtain Cv2x status\n", __FUNCTION__);
        return -EPERM;
    }

    // Wait for radio initialization to finish
    auto radio_init_status = radio_status.get();
    if (callbacks_p && callbacks_p->v2x_radio_init_complete) {
        callbacks_p->v2x_radio_init_complete(radio_init_status, ctx_p);
    }
    if (V2X_STATUS_SUCCESS != radio_init_status) {
        LOGE("%s: Failed to initialize Cv2x radio\n", __FUNCTION__);
        return -EPERM;
    }

    state_g.radio_listener = make_shared<RadioListener>();
    if (not state_g.radio_listener) {
        LOGE("%s: Error instantiating radio listener\n", __FUNCTION__);
        return -EPERM;
    }
    state_g.radio->registerListener(state_g.radio_listener);

    if (V2X_STATUS_SUCCESS != set_radio_info(mode)) {
        LOGE("%s: Error setting radio info\n", __FUNCTION__);
        return -EPERM;
    }

    if (ip_handle_p) {
        *ip_handle_p = V2X_RADIO_IP_HANDLE;
    }

    if (non_ip_handle_p) {
        *non_ip_handle_p = V2X_RADIO_NON_IP_HANDLE;
    }

    return 0;
}

static void print_macphy_params(v2x_radio_macphy_params_t *macphy_p) {
    int i;
    char buf[120];
    char *bp = buf;
    if (!macphy_p) {
        LOGE("NULL ptr to print_macphy_params\n");
        return;
    }

    bp += snprintf(bp, buf + sizeof(buf) - bp,
        "Mac/phy params: Freq=%1.3f, bw=%d, tx_power=%2.1f dBm, retrans=%d ",
        (float)macphy_p->channel_center_khz / 1000.0, macphy_p->channel_bandwidth_mhz,
        (float)macphy_p->tx_power_limit_decidbm / 10.0, macphy_p->qty_auto_retrans);

    if ((macphy_p->l2_source_addr_length_bytes > 0) && macphy_p->l2_source_addr_p) {
        bp += snprintf(bp, buf + sizeof(buf) - bp, "L2-HWaddr=");

        for (i = 0; i < macphy_p->l2_source_addr_length_bytes; i++) {
            bp += snprintf(bp, buf + sizeof(buf) - bp, "%02x", macphy_p->l2_source_addr_p[i]);
            if ((i + 1) < macphy_p->l2_source_addr_length_bytes) {
                bp += snprintf(bp, buf + sizeof(buf) - bp, ":");
            }
        }
    }

    LOGI("%s\n", buf);
}

/**
 *  On an initialized radio handle to either a IP or non-IP radio, configure the
 * MAC & Phy parameters, such as Source L2 Address, channel, bandwidth and
 * transmit power. After the  radio has been configured/changed a callback to
 * *v2x_radio_radio_macphy_change_complete_cb is made with the supplied context
 *
 *  @param[in] handle - to an already, successfully initialized via
 * v2x_radio_init()
 *
 *  @param[in] macphy * - pointer to a v2x_radio_macphy_params_t structure with
 * the configuration to set.
 *
 *  @param[in] Context  * - voluntary pointer passed in that will be supplied as
 * first parameter on callbacks made.
 *
 *  @return
 *      V2X_STATUS_SUCCESS if all went well, and radio is now ready for
 * data-plane sockets to be open and bound V2X_STATUS_ERR if radio unavailable
 * or other issues V2X_STATUS_NO_MEMORY if resource issue
 */
v2x_status_enum_type v2x_radio_set_macphy(
    v2x_radio_handle_t handle, v2x_radio_macphy_params_t *macphy_p, void *context) {
    v2x_status_enum_type result = V2X_STATUS_FAIL;

    memcpy(&state_g.macphy_p, macphy_p, sizeof(v2x_radio_macphy_params_t));
    result = V2X_STATUS_SUCCESS;

    print_macphy_params(macphy_p);

    if (state_g.callbacks && state_g.callbacks->v2x_radio_macphy_change_complete_cb) {
        state_g.callbacks->v2x_radio_macphy_change_complete_cb(context);
    }

    return result;
}

/**
 * De-initialize a particular radio, identified by the handle* returned on an
 * earlier init() call
 *
 * @param[in] handle of which radio was initialized, via pointer
 */
v2x_status_enum_type v2x_radio_deinit(v2x_radio_handle_t handle) {
    v2x_status_enum_type result = V2X_STATUS_SUCCESS;
    LOGI("%s\n", __FUNCTION__);
    if (state_g.radio) {
        if (state_g.radio_listener) {
            state_g.radio->deregisterListener(state_g.radio_listener);
        }
        state_g.radio = nullptr;
    }
    if (state_g.radio_mgr) {
        if (state_g.cv2x_listener) {
            state_g.radio_mgr->deregisterListener(state_g.cv2x_listener);
        }
        state_g.radio_mgr = nullptr;
    }
    return result;
}

/**
 * Open a new V2X radio *RECEIVE* socket and initialize the given sockaddr
 * buffer. This also binds as an AF_INET6 UDP type socket. You can execute any
 * "sockopts" that are appropriate for this type of socket (AF_INET6). Note: the
 * port number for receive path is not exposed, but is in the sockaddr_in6
 * structure if caller is interested
 *
 * REVISIT -- should add a parameter to let caller specify port#
 *
 * @param[in] radio handle prepared, with an init() call previously
 *
 * @param[out] socket on success returns the socket descriptor. Caller must
 *        release this socket with close()
 *
 * @param[out] rx_sockaddr  IPV6 UDP socket; struct sockaddr_in6 buffer, will be
 * initialized for subsequent use  recvfrom() and sendto(), etc
 *
 * @return int returns 0 on success.
 *         -EPERM  socket() creation failed, check errno for further details.
 *         -EAFNOSUPPORT if failing to find the interface
 *         -EACCES if failing to get the mac address of device
 */
int v2x_radio_rx_sock_create_and_bind(
    v2x_radio_handle_t handle, int *sock, struct sockaddr_in6 *rx_sockaddr) {
    return v2x_radio_rx_sock_create_and_bind_v2(handle, 0, nullptr, sock, rx_sockaddr);
}

/**
 * Open a new V2X radio *RECEIVE* socket with specific SIDs and initialize the
 * given sockaddr buffer. This also binds as an AF_INET6UDP type socket. You can
 * execute any "sockopts" that are appropriate for this type of socket
 * (AF_INET6). Note: For any specific SID subscription, the Rx port must be
 * pre-set with v2x_set_rx_port(), and a Tx flow must be pre-setup with the
 * requested service ID set to any service ID included in the parameter id_list
 * and the Tx source port number set to the same port number as Rx port.
 *
 * @param[in] radio handle prepared, with an init() call previously
 *
 * @param[in] length of service ID list
 *
 * @param[in] a list of service ID to subscribe, subscribe wildcard if input
 * nullptr
 *
 * @param[out] socket on success returns the socket descriptor. Caller must
 *        release this socket with close()
 *
 * @param[out] rx_sockaddr  IPV6 UDP socket; struct sockaddr_in6 buffer, will be
 * initialized for subsequent use  recvfrom() and sendto(), etc
 *
 * @return int returns 0 on success.
 *         -EPERM  socket() creation failed, check errno for further details.
 *         -EAFNOSUPPORT if failing to find the interface
 *         -EACCES if failing to get the mac address of device
 */
int v2x_radio_rx_sock_create_and_bind_v2(v2x_radio_handle_t handle, int id_list_len,
    uint32_t *id_list, int *sock, struct sockaddr_in6 *rx_sockaddr) {
    return v2x_radio_rx_sock_create_and_bind_v3(
        handle, state_g.rx_portnum, id_list_len, id_list, sock, rx_sockaddr);
}

/**
 * Open a new V2X radio *RECEIVE* socket with specific SIDs and specific Rx port
 * number, and initialize the given sockaddr buffer. This also binds as an
 * AF_INET6UDP type socket. You can execute any "sockopts" that are appropriate
 * for this type of socket (AF_INET6). Note: For any specific SID subscription,
 * a Tx flow must be pre-setup with the requested service ID set to any service
 * ID included in the parameter id_list and the Tx source port number set to the
 * same port number as Rx port.
 *
 * @param[in] radio handle prepared, with an init() call previously
 *
 * @param[in] the port number for the receive path
 *
 * @param[in] length of service ID list
 *
 * @param[in] a list of service ID to subscribe, subscribe wildcard if input
 * nullptr
 *
 * @param[out] socket on success returns the socket descriptor. Caller must
 *        release this socket with close()
 *
 * @param[out] rx_sockaddr  IPV6 UDP socket; struct sockaddr_in6 buffer, will be
 * initialized for subsequent use  recvfrom() and sendto(), etc
 *
 * @return int returns 0 on success.
 *         -EPERM  socket() creation failed, check errno for further details.
 *         -EAFNOSUPPORT if failing to find the interface
 *         -EACCES if failing to get the mac address of device
 */
int v2x_radio_rx_sock_create_and_bind_v3(v2x_radio_handle_t handle, uint16_t port_num,
    int id_list_len, uint32_t *id_list, int *sock, struct sockaddr_in6 *rx_sockaddr) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }
    TrafficIpType ipType = ifHandle.ipType;

    shared_ptr<ICv2xRxSubscription> rx_sub;
    promise<ErrorCode> p;
    vector<uint32_t> idVector;

    if ((0 >= id_list_len) || (nullptr == id_list)) {
        LOGD("subscribe wildcard with iptype %d port %d\n", ipType, port_num);
        state_g.radio->createRxSubscription(
            ipType, port_num, [&p, &rx_sub](shared_ptr<ICv2xRxSubscription> sub, ErrorCode error) {
                rx_sub = sub;
                p.set_value(error);
            });
    } else {
        LOGD("subscribe SIDs with iptype %d port %d\n", ipType, port_num);
        for (int i = 0; i < id_list_len && i < MAX_SUBSCRIBE_SIDS_LIST_LEN; ++i) {
            idVector.push_back(id_list[i]);
            LOGD("subscribe SID %d\n", id_list[i]);
        }

        auto idListPtr = make_shared<std::vector<uint32_t>>(idVector);

        state_g.radio->createRxSubscription(
            ipType, port_num,
            [&p, &rx_sub](shared_ptr<ICv2xRxSubscription> sub, ErrorCode error) {
                rx_sub = sub;
                p.set_value(error);
            },
            idListPtr);
    }

    auto error = p.get_future().get();
    if (ErrorCode::SUCCESS != error) {
        LOGE("%s: Failed to create RX Socket\n", __FUNCTION__);
        return -EPERM;
    }

    *sock        = rx_sub->getSock();
    *rx_sockaddr = rx_sub->getSockAddr();
    add_rx_sub(*sock, rx_sub);
    state_g.sock_to_rx_map[*sock] = rx_sub;
    return 0;
}
/*
 * Enable or disable the meta data for the received packets corresponding to the
 * Service IDs.
 */
int v2x_radio_enable_rx_meta_data(
    v2x_radio_handle_t handle, bool enable, int id_list_len, uint32_t *id_list) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (id_list_len <= 0 || nullptr == id_list) {
        LOGE("%s, invalid id list parameter provided\n", __FUNCTION__);
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    TrafficIpType ipType = ifHandle.ipType;

    promise<ErrorCode> p;
    vector<uint32_t> idVector;

    for (int i = 0; i < id_list_len && i < MAX_SUBSCRIBE_SIDS_LIST_LEN; ++i) {
        idVector.push_back(id_list[i]);
    }

    auto idListPtr = make_shared<std::vector<uint32_t>>(idVector);

    auto status = state_g.radio->enableRxMetaDataReport(
        ipType, enable, idListPtr, [&p](ErrorCode error) { p.set_value(error); });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Failed to enable RX meta data\n", __FUNCTION__);
        return -EPERM;
    }

    return 0;
}

/**
 * Create Tx or Rx socket with configure port number and initialize the given
 * sockaddr buffer. This also binds as an AF_INET6UDP type socket. You can
 * execute any "sockopts" that are appropriate for this type of socket
 * (AF_INET6). Note: A negative port number corresponds to no action on the Tx
 * or Rx.
 */
int v2x_radio_sock_create_and_bind(v2x_radio_handle_t handle, v2x_tx_sps_flow_info_t *tx_flow_info,
    v2x_per_sps_reservation_calls_t *calls, int tx_sps_portnum, int tx_event_portnum,
    int rx_portnum, v2x_sid_list_t *rx_id_list, v2x_sock_info_t *tx_sps_sock,
    v2x_sock_info_t *tx_event_sock, v2x_sock_info_t *rx_sock) {
    int ret = 0;

    LOGI("%s: tx sps port %d, tx event port %d, rx port %d\n", __FUNCTION__, tx_sps_portnum,
        tx_event_portnum, rx_portnum);

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    if (tx_flow_info && tx_sps_portnum > 0) {
        // create tx sps flow or tx sps and event flow
        ret = v2x_radio_tx_sps_sock_create_and_bind_v2(handle, tx_flow_info, calls, tx_sps_portnum,
            tx_event_portnum, &tx_sps_sock->sock, &tx_sps_sock->sockaddr, &tx_event_sock->sock,
            &tx_event_sock->sockaddr);
    } else if (tx_flow_info && tx_event_portnum > 0) {
        // create tx event flow
        ret = v2x_radio_tx_event_sock_create_and_bind_v2(ifHandle.ifName.c_str(),
            tx_flow_info->reservation.v2xid, tx_event_portnum, &tx_flow_info->flow_info,
            &tx_event_sock->sockaddr, &tx_event_sock->sock);
    }

    if (ret) {
        LOGE("%s: create Tx flow error\n", __FUNCTION__);
        return ret;
    }

    if (rx_portnum > 0) {
        int sid_list_len   = 0;
        uint32_t *sid_list = NULL;

        if (rx_id_list != NULL) {
            sid_list_len = rx_id_list->length;
            sid_list     = rx_id_list->sid;
        }

        ret = v2x_radio_rx_sock_create_and_bind_v3(
            handle, rx_portnum, sid_list_len, sid_list, &rx_sock->sock, &rx_sock->sockaddr);
        if (ret) {
            LOGE("%s: create Rx flow error, close Tx flow\n", __FUNCTION__);

            if (tx_sps_sock->sock >= 0) {
                v2x_radio_sock_close(&tx_sps_sock->sock);
            }

            if (tx_event_sock->sock >= 0) {
                v2x_radio_sock_close(&tx_event_sock->sock);
            }
        }
    }

    return ret;
}

void v2x_show_all_flows(cv2x_config_state_t *sp) {
    LOGE("%s is not supported\n", __FUNCTION__);
}

int v2x_radio_tx_sps_only_create(v2x_radio_handle_t handle, v2x_tx_bandwidth_reservation_t *res,
    v2x_per_sps_reservation_calls_t *calls, int sps_portnum, int *sps_sock,
    struct sockaddr_in6 *sps_sockaddr) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!res or !sps_sock or !sps_sockaddr) {
        LOGE("%s : Bad Params NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    LOGI("%s: (id=%d, sps_port=%d, \res={%d bytes, %d ms, pri = %d})\n", __FUNCTION__, res->v2xid,
        sps_portnum, res->tx_reservation_size_bytes, res->period_interval_ms, res->priority);

    LOGD("destination connect() addr: %s:%d\n", state_g.dest_ip_addr.c_str(),
        state_g.dest_portnum_override);

    SpsFlowInfo spsInfo;
    if (convert_reservation(*res, spsInfo) < 0) {
        LOGE("%s\n", supportedPeriodicityToString(state_g.capabilities).c_str());
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow> sps_flow;

    LOGI("Attempting SPS socket creation\n");

    auto status = state_g.radio->createTxSpsFlow(ifHandle.ipType, static_cast<uint32_t>(res->v2xid),
        spsInfo, sps_portnum, false, 0u,
        [&p, &sps_flow](shared_ptr<ICv2xTxFlow> tx_sps_flow, shared_ptr<ICv2xTxFlow> unused,
            ErrorCode sps_error, ErrorCode unused_error) {
            sps_flow = tx_sps_flow;
            p.set_value(sps_error);
        });

    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: creating sps flow failed\n", __FUNCTION__);
        return -EPERM;
    }

    *sps_sock     = sps_flow->getSock();
    *sps_sockaddr = sps_flow->getSockAddr();
    add_tx_flow(*sps_sock, sps_flow);
    add_sps_cb(sps_flow->getFlowId(), calls);

    return 0;
}  // Just SPS

/**
 * Create and bind a socket with a bandwidth-reserved (SPS) TX flow, with the
 *requested ID/Priority on the specified port number.   (Will be created as an
 *IPV6 UDP socket, and bound) Radio attempts to reserve the flow with the
 *specified size and rate passed in on the request parameters. Used only for Tx,
 *Sets up two UDP sockets, on the requested two HLOS port numbers ..
 *
 * *Note* The Priority parameter of the SPS reservation is only used for the
 *reserved tx bandwidth (SPS) flow, the non-SPS/event driven data to the
 *event_portnum is prioritized on the air, based on the packet's IPV67 Traffic
 *Class.
 *
 * @param[in] handle - parameter identifying  the initialized radio interface on
 *which this data connection will be connected to.
 *
 * @param[in] v2x_tx_bandwidth_reservation *  - pointer to parameter structure
 *(how often sent, how many bytes reserved)
 *
 * @param[in] calls * - ptr to a v2x_per_sps_reservation_calls_t structure
 *filled with callbacks/listeners, called as various underlying radio MAC
 *parameters change related to the "SPS" bandwidth contract. such as the
 *callback after a reservation change, or if the timing offset of the SPS
 *adjusts itself in response to traffic.  Pass a NULL, if no callbacks are
 *needed.
 *
 * @param[in]  sps_portnum - requested port number for the bandwidth reserved
 *"SPS" transmissions
 *
 * @param[in]  event_portnum  - requested port number for the bandwidth reserved
 *"SPS" transmissions
 *
 * @param[out]  sps_sock *  - socket bound to the requested port for TX with
 *reserved bandwidth
 * @param[in,out]  sps_sockaddr *  - INET6 address , that is supplied by calling
 *context and filled in for subsequent use with , recvfrom() and sendto()
 *
 * @param[out]  event_sock *  - socket  bound to the event driven transmission
 *port
 *
 * @param[in,out] event_sockaddr Given struct sockaddr_in6 buffer, will be
 *initialized for subsequent use with, recvfrom() and sendto()
 *
 * Caller is expected to identify two unused local port#'s to bind to, one for
 *the event driven and one for the SPS flow. This is a blocking call -- when it
 *returns, the sockets will be ready to used, if no error.
 *
 * @return int returns 0 on success.
 *         -EPERM  socket() creation failed, check errno for further details.
 *         -EAFNOSUPPORT if failing to find the interface
 *         -EACCES if failing to get the mac address of device
 **/
int v2x_radio_tx_sps_sock_create_and_bind(v2x_radio_handle_t handle,
    v2x_tx_bandwidth_reservation_t *res, v2x_per_sps_reservation_calls_t *calls, int sps_portnum,
    int event_portnum, int *sps_sock, struct sockaddr_in6 *sps_sockaddr, int *event_sock,
    struct sockaddr_in6 *event_sockaddr) {
    if (!res) {
        LOGE("%s : NULL reservation\n", __FUNCTION__);
        return -EINVAL;
    }

    v2x_tx_sps_flow_info_t sps_flow_info;
    memcpy(&(sps_flow_info.reservation), res, sizeof(v2x_tx_bandwidth_reservation_t));

    return v2x_radio_tx_sps_sock_create_and_bind_v2(handle, &sps_flow_info, calls, sps_portnum,
        event_portnum, sps_sock, sps_sockaddr, event_sock, event_sockaddr);
}

void tx_reservation_change_cb(shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
    auto flow_id = flow->getFlowId();
    auto cb      = find_sps_cb(flow_id);
    if (ErrorCode::SUCCESS == error) {
        LOGI("tx reservation change succeeded for flow %d\n", flow_id);
    } else {
        LOGE("tx reservation change failed for flow %d\n", flow_id);
    }
    /* Invoke v2x_radio_l2_reservation_change_complete_cb when handling this
     * WDS_V2X_SPS_FLOW_UPDATE_RESULT_IND; this indication NOT include new
     * MAC details so we can not pass sps_mac_details to registered callback,
     * instead, pass error code in 2nd parameter only;
     *
     * We shall NOT invoke v2x_radio_sps_offset_changed callback here as
     * V2X_SPS_FLOW_UPDATE_RESULT_IND NOT mean sps_offset_change complete;
     * updated mac details including next SPS grant UTC time with SPS
     * periodicity shall be included in v2x_radio_sps_offset_changed
     * when handling WDS_V2X_SPS_SCHEDULING_INFO_IND.
     */
    if (cb && cb->v2x_radio_l2_reservation_change_complete_cb) {
        cb->v2x_radio_l2_reservation_change_complete_cb(
            state_g.context, reinterpret_cast<v2x_sps_mac_details_t *>(&error));
    }
}

/**
 * v2x_radio_tx_reservation_change() is used to adjust the reservation for
 * transmit bandwidth, after an earlier v2x_radio_sps_sock_create_and_bind()
 * that had set-up the reservation.  This is used when the bandwidth need has
 * changed in either periodicity (say due to an application layer DCC algorithm
 * or the packet size is increasing, say due to a growing path history size in a
 * BSM When the reservation change is complete, a callback to the structure
 * passed at v2x_radio_init() call.
 *
 * @param[in] updated_reservation *  -  ptr to a new
 * v2x_tx_bandwidth_reservation_t with new reservation info.
 *
 * @return V2X_STATUS_SUCCESS or other v2x_status_enum_type enum types if
 * problems
 */
v2x_status_enum_type v2x_radio_tx_reservation_change(
    int *sps_sock, v2x_tx_bandwidth_reservation_t *res) {
    if (!state_g.radio or !state_g.radio->isReady()) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    auto flow = find_tx_flow(*sps_sock);
    if (not flow) {
        LOGE("Invalid socket %d\n", *sps_sock);
        return V2X_STATUS_EBADPARM;
    }

    // actually_a_deregister flags a Special case, treat a res of 0 bytes or 0 ms
    // period as a deregister, since modem can't actually support such things
    auto flow_id = flow->getFlowId();

    auto actually_a_deregister = (res->tx_reservation_size_bytes < 1);
    if (actually_a_deregister) {
        LOGI("Deregister flow ID #%d due to zeros in reservation update.\n", flow_id);
        close_tx_flow(flow);
        erase_tx_flow(*sps_sock);
        erase_sps_cb(flow->getFlowId());
        return V2X_STATUS_SUCCESS;
    }

    // This is actually an Event flow, so we cannot change the reservation
    auto cb = find_sps_cb(flow_id);
    if (not cb) {
        LOGW("Called %s on an EVENT flow ID #%d\n", flow_id);
        return V2X_STATUS_FAIL;
    }

    SpsFlowInfo spsInfo;
    if (convert_reservation(*res, spsInfo) < 0) {
        LOGE("%s\n", supportedPeriodicityToString(state_g.capabilities).c_str());
        return V2X_STATUS_FAIL;
    }

    if (Status::SUCCESS
        != state_g.radio->changeSpsFlowInfo(flow, spsInfo, tx_reservation_change_cb)) {
        LOGE("TX Reservation Change failed\n");
        return V2X_STATUS_FAIL;
    }

    return V2X_STATUS_SUCCESS;
}

/**
 *  Adjusts the reservation for transmit bandwidth.
 *
 *  @datatypes
 *  v2x_tx_sps_flow_info_t
 *
 *  @param[out] sps_sock             Pointer to the socket bound to the
 * requested port.
 *  @param[in]  updated_flow_info    Pointer to a parameter structure with
 *                                   new reservation information.
 *
 *  @detdesc
 *  This function is used as follows:
 *  - When the bandwidth requirement changes in periodicity (for example, due to
 *      an application layer DCC algorithm)
 *  - Because the packet size is increasing (for example, due to a growing path
 *      history size in a BSM).
 *  @par
 *  When the reservation change is complete, a callback to the structure is
 *  passed at in a v2x_radio_init() call.
 *
 *  @return
 *  #V2X_STATUS_SUCCESS -- On success.
 *  @par
 *  Error code -- If there is a problem (see #v2x_status_enum_type).
 *
 *  @dependencies
 *  An SPS flow must have been successfully initialized with the
 *  v2x_radio_tx_sps_sock_create_and_bind() or
 *  v2x_radio_tx_sps_sock_create_and_bind_v2() methods.
 */
v2x_status_enum_type v2x_radio_tx_reservation_change_v2(
    int *sps_sock, v2x_tx_sps_flow_info_t *updated_flow_info) {
    if (!state_g.radio or !state_g.radio->isReady()) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    auto flow = find_tx_flow(*sps_sock);
    if (not flow) {
        LOGE("Invalid socket %d\n", *sps_sock);
        return V2X_STATUS_EBADPARM;
    }

    // actually_a_deregister flags a Special case, treat a res of 0 bytes or 0 ms
    // period as a deregister, since modem can't actually support such things
    auto flow_id = flow->getFlowId();

    auto actually_a_deregister = (updated_flow_info->reservation.tx_reservation_size_bytes < 1);
    if (actually_a_deregister) {
        LOGI("Deregister flow ID #%d due to zeros in reservation update.\n", flow_id);
        close_tx_flow(flow);
        erase_tx_flow(*sps_sock);
        erase_sps_cb(flow->getFlowId());
        return V2X_STATUS_SUCCESS;
    }

    // This is actually an Event flow, so we cannot change the reservation
    auto cb = find_sps_cb(flow_id);
    if (not cb) {
        LOGW("Called %s on an EVENT flow ID #%d\n", flow_id);
        return V2X_STATUS_FAIL;
    }

    SpsFlowInfo spsInfo;
    if (convert_sps_flow_info(*updated_flow_info, spsInfo) < 0) {
        return V2X_STATUS_FAIL;
    }

    if (Status::SUCCESS
        != state_g.radio->changeSpsFlowInfo(flow, spsInfo, tx_reservation_change_cb)) {
        LOGE("TX Reservation Change failed\n");
        return V2X_STATUS_FAIL;
    }

    return V2X_STATUS_SUCCESS;
}

/**
 * Flush the radio transmitter queue, for all packets on the specified interface
 * that have not been sent yet. This is necessary when a radio MAC address
 * change is coordinated for anonymity.
 *
 * @param[in] interface name to use.  this is the radio interface operating
 * system name.
 */
void v2x_radio_tx_flush(char *interface) {
    // TODO: Implement when supported in the modem
    if (interface) {
        LOGD("TX flush called on interface %s\n", interface);
        LOGE("%s called, but not supported by lower level radio yet\n", __FUNCTION__);
    } else {
        LOGE("v2x_radio_tx_flush called with NULL interface\n");
    }
}

/**
 *  Open and bind to an event driven socket. ( one with no bandwidth
 * reservation. This is Used only for Tx when no periodicity is available for
 * the application type If your transmit data periodicity is known, then use the
 * v2x_radio_tx_sps_sock_create_and_bind() instead These event driven sockets
 * will pay attention to QoS params in the IP socket in order to set the
 * priority, you use the traffic class.
 *
 * @param[in]  interface string of OS name to use.  This will be a RMNET
 * interface (HLOS) Ir could be
 * @param[in]  v2x_id -- the V2X ID used for transmissions (which are ultimately
 * mapped to a L2 Dest ADDR
 * @param[in]  event_portnum to use as the port to which this particular TX will
 * go out, as event driven packets
 * @param[out] sockaddr*  iptor, loaded when successful given sockaddr buffer
 * will be initialized
 * @param[out] sock*  pointer file descriptor, loaded when successful given
 * sockaddr_in6 buffer will be initialized
 *
 * REVISIT: API would be more consistent if we just use the handle here perhaps,
 * instead of interface name
 *
 *
 * @return int returns 0 on success.
 *         -EPERM  socket() creation failed, check errno for further details.
 *         -EAFNOSUPPORT if failing to find the interface
 *         -EACCES if failing to get the mac address of device
 */
int v2x_radio_tx_event_sock_create_and_bind(const char *interface, int v2x_id, int event_portnum,
    struct sockaddr_in6 *event_sockaddr, int *sock) {
    if (!interface) {
        LOGE("%s: Interface is NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    LOGI("%s(if:%s, v2x_id:%d, port:%d dest_addr:%s)\n", __FUNCTION__, interface, v2x_id,
        event_portnum, state_g.dest_ip_addr.c_str());

    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!event_sockaddr) {
        LOGE("Event sockaddr is NULL\n");
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(V2X_RADIO_HANDLE_BAD, interface, ifHandle)) {
        return -EINVAL;
    }

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow> event_flow;

    LOGI("Attempting event socket creation\n");

    auto status = state_g.radio->createTxEventFlow(ifHandle.ipType, static_cast<uint32_t>(v2x_id),
        static_cast<uint16_t>(event_portnum),
        [&p, &event_flow](shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
            event_flow = flow;
            p.set_value(error);
        });

    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("Error in creating Tx Event sock\n");
        return -EPERM;
    }

    *sock           = event_flow->getSock();
    *event_sockaddr = event_flow->getSockAddr();
    add_tx_flow(*sock, event_flow);

    return 0;
}

int v2x_radio_tx_event_sock_create_and_bind_v2(const char *interface, int v2x_id, int event_portnum,
    v2x_tx_flow_info_t *event_flow_info, struct sockaddr_in6 *event_sockaddr, int *sock) {
    if (!interface) {
        LOGE("%s: Interface is NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(V2X_RADIO_HANDLE_BAD, interface, ifHandle)) {
        return -EINVAL;
    }

    traffic_ip_type_t ip_type;
    convert_enum(ifHandle.ipType, ip_type);

    return v2x_radio_tx_event_sock_create_and_bind_v3(
        ip_type, v2x_id, event_portnum, event_flow_info, event_sockaddr, sock);
}

int v2x_radio_tx_event_sock_create_and_bind_v3(traffic_ip_type_t ip_type, int v2x_id,
    int event_portnum, v2x_tx_flow_info_t *event_flow_info, struct sockaddr_in6 *event_sockaddr,
    int *sock) {
    if (!event_flow_info) {
        LOGE("%s: called with NULL flow_info\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!event_sockaddr) {
        LOGE("%s: Event sockaddr is NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    LOGI("%s(ip type:%d, v2x_id:%d, port:%d dest_addr:%s, "
         "flow_info={retransmit=%d, tx_power=%d, "
         "mcs_index=%d, tx_pool_id=%d, is_unicast=%d})\n",
        __FUNCTION__, ip_type, v2x_id, event_portnum, state_g.dest_ip_addr.c_str(),
        static_cast<int>(event_flow_info->retransmit_policy),
        (event_flow_info->default_tx_power_valid)
            ? static_cast<int>(event_flow_info->default_tx_power)
            : -1,
        (event_flow_info->mcs_index_valid) ? static_cast<int>(event_flow_info->mcs_index) : -1,
        event_flow_info->tx_pool_id_valid ? static_cast<int>(event_flow_info->tx_pool_id) : -1,
        event_flow_info->is_unicast_valid ? static_cast<int>(event_flow_info->is_unicast) : -1);

    TrafficIpType ipType;
    convert_enum(ip_type, ipType);

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow> event_flow;

    EventFlowInfo eventInfo;
    convert_event_flow_info(*event_flow_info, eventInfo);

    LOGI("Attempting event socket creation\n");

    auto status = state_g.radio->createTxEventFlow(ipType, static_cast<uint32_t>(v2x_id), eventInfo,
        static_cast<uint16_t>(event_portnum),
        [&p, &event_flow](shared_ptr<ICv2xTxFlow> flow, ErrorCode error) {
            event_flow = flow;
            p.set_value(error);
        });

    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("Error in creating Tx Event sock\n");
        return -EPERM;
    }

    *sock           = event_flow->getSock();
    *event_sockaddr = event_flow->getSockAddr();
    add_tx_flow(*sock, event_flow);

    return 0;
}

v2x_status_enum_type v2x_radio_tx_event_flow_info_change(
    int *sock, v2x_tx_flow_info_t *updated_flow_info) {
    if (!state_g.radio or !state_g.radio->isReady()) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    auto flow = find_tx_flow(*sock);
    if (not flow) {
        LOGE("Invalid socket %d\n", *sock);
        return V2X_STATUS_EBADPARM;
    }

    auto flow_id = flow->getFlowId();

    if (find_sps_cb(flow_id)) {
        LOGW("Called %s on an SPS flow ID #%d\n", flow_id);
        return V2X_STATUS_FAIL;
    }

    EventFlowInfo flowInfo;
    convert_event_flow_info(*updated_flow_info, flowInfo);

    promise<ErrorCode> p;
    auto status = state_g.radio->changeEventFlowInfo(flow, flowInfo,
        [&p](std::shared_ptr<ICv2xTxFlow> txFlow, ErrorCode error) { p.set_value(error); });

    if (Status::SUCCESS != status or p.get_future().get() != ErrorCode::SUCCESS) {
        LOGE("TX event flow info change failed for flow id: %d\n", flow_id);
        return V2X_STATUS_FAIL;
    }

    LOGI("TX event flow info change succeeded for flow id: %d\n", flow_id);
    return V2X_STATUS_SUCCESS;
}

/**
 * Request a channel utilization (CBP/CBR) measurement result on the
 * channel that the handle specified is presently tuned to.
 * will use the callbacks passed in at the init-time to deliver the measurements
 * measurement call-backs will continue until radio interface is closed
 *
 * @param[in] handle to which port to req
 * @param[in] measure_this_way struct set of parameters on how/what to measure,
 * and how often to send. Some higher level standards like J2945/1 and ETSI
 * TS102687 DCC have specific time windows and things to mesaure
 *
 * @return 0 if successful, otherwise error code, if say the interface is not
 * ready or does not support
 */
v2x_status_enum_type v2x_radio_start_measurements(
    v2x_radio_handle_t handle, v2x_chan_meas_params_t *measure_this_way) {
    if (V2X_RADIO_IP_HANDLE != handle and V2X_RADIO_NON_IP_HANDLE != handle) {
        LOGE("%s: called when C-V2X handle is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    if (not state_g.callbacks or not state_g.callbacks->v2x_radio_chan_meas_listener) {
        LOGE("%s: radio channel measurement listener is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    state_g.doing_periodic_measures = true;
    return V2X_STATUS_SUCCESS;
}

/**
 * Discontinue any periodic mac/phy channel measurements and the reporting of
 * them via listener calls
 *
 * @param[in] handle to which Radio measurements we are stopping
 *
 * @return 0 if successful, otherwise error code, if say the interface is not
 * ready or does not support
 */
v2x_status_enum_type v2x_radio_stop_measurements(v2x_radio_handle_t handle) {
    if (V2X_RADIO_IP_HANDLE != handle and V2X_RADIO_NON_IP_HANDLE != handle) {
        LOGE("%s: called when C-V2X handle is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    state_g.doing_periodic_measures = false;

    return V2X_STATUS_SUCCESS;
}

static int close_rx_sub(shared_ptr<ICv2xRxSubscription> &rx_sub) {
    if (not state_g.radio) {
        return -EPERM;
    }

    promise<int> p;
    auto status = state_g.radio->closeRxSubscription(
        rx_sub, [&p](const shared_ptr<ICv2xRxSubscription> unused, ErrorCode error) {
            p.set_value(static_cast<int>(error));
        });
    if (Status::SUCCESS != status) {
        return -EPERM;
    }
    return p.get_future().get();
}

static int close_tx_flow(shared_ptr<ICv2xTxFlow> &tx_flow) {
    if (not state_g.radio) {
        return -EPERM;
    }

    promise<int> p;
    auto status = state_g.radio->closeTxFlow(
        tx_flow, [&p](const shared_ptr<ICv2xTxFlow> unused, ErrorCode error) {
            p.set_value(static_cast<int>(error));
        });
    if (Status::SUCCESS != status) {
        return -EPERM;
    }
    return p.get_future().get();
}

int v2x_radio_sock_close(int *sock_fd) {
    int result = 0;

    // Validate sock_fd
    if (!sock_fd) {
        LOGE("NULL sockets\n");
        return -EINVAL;
    }

    if (*sock_fd < 0) {
        LOGE("Invalid socket\n");
        return -EINVAL;
    }

    // If the sock corresponds to a TCP socket, close it
    auto sock = find_tcp_socket(*sock_fd);
    if (sock) {
        if (static_cast<int>(ErrorCode::SUCCESS) != close_tcp_socket(sock)) {
            result = -EINVAL;
        }
        erase_tcp_socket(*sock_fd);
    }

    // If the sock corresponds to an rx port, close it
    auto rx_sub = find_rx_sub(*sock_fd);
    if (rx_sub) {
        if (static_cast<int>(ErrorCode::SUCCESS) != close_rx_sub(rx_sub)) {
            result = -EINVAL;
        }
        erase_rx_sub(*sock_fd);
    }

    // If the sock corresponds to an SPS flow or an event-driven
    // port, close it
    auto tx_flow = find_tx_flow(*sock_fd);
    if (tx_flow) {
        if (static_cast<int>(ErrorCode::SUCCESS) != close_tx_flow(tx_flow)) {
            result = -EINVAL;
        }
        erase_tx_flow(*sock_fd);
        erase_sps_cb(tx_flow->getFlowId());
    }

    if (0 == result) {
        // set to invalid socket fd
        *sock_fd = -1;
    } else {
        LOGE("Failed for socket %d\n", *sock_fd);
    }

    return result;
}

int v2x_radio_tx_sps_only_create_v2(v2x_radio_handle_t handle,
    v2x_tx_sps_flow_info_t *sps_flow_info, v2x_per_sps_reservation_calls_t *calls, int sps_portnum,
    int *sps_sock, struct sockaddr_in6 *sps_sockaddr) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (!sps_flow_info or !sps_sock or !sps_sockaddr) {
        LOGE("%s : Bad Params NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    uint32_t v2x_id = static_cast<uint32_t>(sps_flow_info->reservation.v2xid);

    LOGI("%s: (id=%u, sps_port=%d, flow_info={retransmit=%d, tx_power=%d, "
         "mcs_index=%d, tx_pool=%d, %d bytes, %d ms, pri = %d})\n",
        __FUNCTION__, v2x_id, sps_portnum,
        static_cast<int>(sps_flow_info->flow_info.retransmit_policy),
        (sps_flow_info->flow_info.default_tx_power_valid)
            ? static_cast<int>(sps_flow_info->flow_info.default_tx_power)
            : -1,
        (sps_flow_info->flow_info.mcs_index_valid)
            ? static_cast<int>(sps_flow_info->flow_info.mcs_index)
            : -1,
        (sps_flow_info->flow_info.tx_pool_id_valid)
            ? static_cast<int>(sps_flow_info->flow_info.tx_pool_id)
            : -1,
        sps_flow_info->reservation.tx_reservation_size_bytes,
        sps_flow_info->reservation.period_interval_ms, sps_flow_info->reservation.priority);

    LOGD("destination connect() addr: %s:%d\n", state_g.dest_ip_addr.c_str(),
        state_g.dest_portnum_override);

    SpsFlowInfo spsInfo;
    if (convert_sps_flow_info(*sps_flow_info, spsInfo) < 0) {
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    promise<ErrorCode> p;
    shared_ptr<ICv2xTxFlow> sps_flow;

    LOGI("Attempting SPS socket creation\n");

    auto status
        = state_g.radio->createTxSpsFlow(ifHandle.ipType, v2x_id, spsInfo, sps_portnum, false, 0u,
            [&p, &sps_flow](shared_ptr<ICv2xTxFlow> tx_sps_flow, shared_ptr<ICv2xTxFlow> unused,
                ErrorCode sps_error, ErrorCode unused_error) {
                sps_flow = tx_sps_flow;
                p.set_value(sps_error);
            });

    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: creating sps flow failed\n", __FUNCTION__);
        return -EPERM;
    }

    *sps_sock     = sps_flow->getSock();
    *sps_sockaddr = sps_flow->getSockAddr();
    add_tx_flow(*sps_sock, sps_flow);
    add_sps_cb(sps_flow->getFlowId(), calls);

    return 0;
}  // Just SPS

int v2x_radio_tx_sps_sock_create_and_bind_v2(v2x_radio_handle_t handle,
    v2x_tx_sps_flow_info_t *sps_flow_info, v2x_per_sps_reservation_calls_t *calls, int sps_portnum,
    int event_portnum, int *sps_sock, struct sockaddr_in6 *sps_sockaddr, int *event_sock,
    struct sockaddr_in6 *event_sockaddr) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    if (!sps_flow_info or !sps_sock or !sps_sockaddr
        or (event_portnum >= 0 and (!event_sock or !event_sockaddr))) {
        LOGE("%s: Bad Params NULL\n", __FUNCTION__);
        return -EINVAL;
    }

    uint32_t v2x_id = static_cast<uint32_t>(sps_flow_info->reservation.v2xid);

    LOGI("%s: (id=%u, sps_port=%d, event_port=%d, flow_info={retransmit=%d, "
         "tx_power=%d, "
         "mcs_index=%d, tx_pool=%d, %d bytes, %d ms, pri = %d})\n",
        __FUNCTION__, v2x_id, sps_portnum, event_portnum,
        static_cast<int>(sps_flow_info->flow_info.retransmit_policy),
        (sps_flow_info->flow_info.default_tx_power_valid)
            ? static_cast<int>(sps_flow_info->flow_info.default_tx_power)
            : -1,
        (sps_flow_info->flow_info.mcs_index_valid)
            ? static_cast<int>(sps_flow_info->flow_info.mcs_index)
            : -1,
        (sps_flow_info->flow_info.tx_pool_id_valid)
            ? static_cast<int>(sps_flow_info->flow_info.tx_pool_id)
            : -1,
        sps_flow_info->reservation.tx_reservation_size_bytes,
        sps_flow_info->reservation.period_interval_ms, sps_flow_info->reservation.priority);

    LOGD("destination connect() addr: %s:%d\n", state_g.dest_ip_addr.c_str(),
        state_g.dest_portnum_override);

    SpsFlowInfo spsInfo;
    if (convert_sps_flow_info(*sps_flow_info, spsInfo) < 0) {
        return -EINVAL;
    }

    promise<std::pair<ErrorCode, ErrorCode>> p;
    auto event_port_valid = event_portnum >= 0;
    shared_ptr<ICv2xTxFlow> sps_flow, event_flow;

    LOGI("Attempting SPS socket creation\n");

    auto status = state_g.radio->createTxSpsFlow(ifHandle.ipType, v2x_id, spsInfo, sps_portnum,
        event_port_valid, event_portnum,
        [&p, &sps_flow, &event_flow](shared_ptr<ICv2xTxFlow> tx_sps_flow,
            shared_ptr<ICv2xTxFlow> tx_event_flow, ErrorCode sps_error, ErrorCode event_error) {
            sps_flow   = tx_sps_flow;
            event_flow = tx_event_flow;
            p.set_value(std::pair<ErrorCode, ErrorCode>(sps_error, event_error));
        });

    auto error
        = std::pair<ErrorCode, ErrorCode>(ErrorCode::GENERIC_FAILURE, ErrorCode::GENERIC_FAILURE);
    if (Status::SUCCESS == status) {
        error = p.get_future().get();
    } else {
        LOGE("%s: Creating sps flow failed\n", __FUNCTION__);
        return -EPERM;
    }

    // both SPS flow and the optional event flow creation succeeded
    if (ErrorCode::SUCCESS == error.first
        and (not event_port_valid or ErrorCode::SUCCESS == error.second)) {
        // add SPS flow
        *sps_sock     = sps_flow->getSock();
        *sps_sockaddr = sps_flow->getSockAddr();
        add_tx_flow(*sps_sock, sps_flow);
        add_sps_cb(sps_flow->getFlowId(), calls);

        // add event flow
        if (event_port_valid) {
            *event_sock     = event_flow->getSock();
            *event_sockaddr = event_flow->getSockAddr();
            add_tx_flow(*event_sock, event_flow);
        }
        return 0;
    }

    // SPS flow count exceeds the maximum number, create two event flows in place
    if (ErrorCode::V2X_ERR_EXCEED_MAX == error.first) {
        LOGE("%s: SPS flow exceeds max, creating Event flow in its place\n", __FUNCTION__);
        int ret = v2x_radio_tx_event_sock_create_and_bind_v2(ifHandle.ifName.c_str(), v2x_id,
            sps_portnum, &sps_flow_info->flow_info, sps_sockaddr, sps_sock);
        if (not ret and event_port_valid) {
            // create event flow for SPS port succeeded, create another event flow for
            // event port
            ret = v2x_radio_tx_event_sock_create_and_bind_v2(ifHandle.ifName.c_str(), v2x_id,
                event_portnum, &sps_flow_info->flow_info, event_sockaddr, event_sock);
            if (ret) {
                // create event flow failed, close previous event flow created for sps
                // port
                v2x_radio_sock_close(sps_sock);
            }
        }
        return ret;
    }

    // for other failures, close created flows before return failure
    LOGE("%s: combined sps flow registration failed %d, %d\n", __FUNCTION__,
        static_cast<int>(error.first), static_cast<int>(error.second));
    if (ErrorCode::SUCCESS == error.first and sps_flow) {
        close_tx_flow(sps_flow);
    }
    if (ErrorCode::SUCCESS == error.second and event_flow) {
        close_tx_flow(event_flow);
    }
    return -EPERM;
}

int v2x_radio_trigger_l2_update(v2x_radio_handle_t handle) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    promise<ErrorCode> p;
    auto status = state_g.radio->updateSrcL2Info([&p](ErrorCode error) { p.set_value(error); });

    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("Error in updateSrcL2Info\n");
        return -EPERM;
    }

    return 0;
}

int v2x_radio_update_trusted_ue_list(unsigned int malicious_list_len,
    unsigned int malicious_list[MAX_MALICIOUS_IDS_LIST_LEN], unsigned int trusted_list_len,
    trusted_ue_info_t trusted_list[MAX_TRUSTED_IDS_LIST_LEN]) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return -EINVAL;
    }

    if (malicious_list_len > MAX_MALICIOUS_IDS_LIST_LEN) {
        LOGE("%s: malicious list length (%u) exceeds maximum allowed (%u).\n", __FUNCTION__,
            malicious_list_len, MAX_MALICIOUS_IDS_LIST_LEN);
        LOGE("    Ignoring malicious list elements with list index >= %u\n",
            MAX_MALICIOUS_IDS_LIST_LEN);
        malicious_list_len = MAX_MALICIOUS_IDS_LIST_LEN;
    }

    if (trusted_list_len > MAX_TRUSTED_IDS_LIST_LEN) {
        LOGE("%s: trusted list length (%u) exceeds maximum allowed (%u).\n", __FUNCTION__,
            trusted_list_len, MAX_TRUSTED_IDS_LIST_LEN);
        LOGE(
            "    Ignoring trusted list elements with list index >= %u\n", MAX_TRUSTED_IDS_LIST_LEN);
        trusted_list_len = MAX_TRUSTED_IDS_LIST_LEN;
    }

    TrustedUEInfoList info;
    if (malicious_list_len > 0) {
        info.maliciousIdsValid = true;
    }

    info.maliciousIds.reserve(malicious_list_len);
    for (auto i = 0u; i < malicious_list_len; ++i) {
        info.maliciousIds.push_back(malicious_list[i]);
    }

    if (trusted_list_len > 0) {
        info.trustedUEsValid = true;
    }

    info.trustedUEs.reserve(trusted_list_len);
    for (auto i = 0u; i < trusted_list_len; ++i) {
        TrustedUEInfo trusted{};
        trusted.sourceL2Id              = trusted_list[i].source_l2_id;
        trusted.timeUncertainty         = trusted_list[i].time_uncertainty;
        trusted.positionConfidenceLevel = trusted_list[i].position_confidence_level;
        trusted.propagationDelay        = trusted_list[i].propagation_delay;
        info.trustedUEs.push_back(trusted);
    }

    promise<ErrorCode> p;
    auto status
        = state_g.radio->updateTrustedUEList(info, [&p](ErrorCode error) { p.set_value(error); });

    ErrorCode error = p.get_future().get();

    if (Status::SUCCESS != status) {
        LOGE("sendTunnelModeInfo failed with Status code: %d\n", static_cast<int>(status));
        return -EPERM;
    }

    if (ErrorCode::SUCCESS != error) {
        LOGE("sendTunnelModeInfo failed with Error code: %d\n\n", static_cast<int>(error));
        return -EPERM;
    }

    return 0;
}

v2x_status_enum_type start_v2x_mode() {
    auto radio_mgr = get_and_init_radio_mgr();
    if (radio_mgr) {
        promise<ErrorCode> p;
        radio_mgr->startCv2x([&p](ErrorCode error) { p.set_value(error); });
        if (ErrorCode::SUCCESS == p.get_future().get()) {
            return V2X_STATUS_SUCCESS;
        }
    }
    LOGE("Failed to start v2x mode\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type stop_v2x_mode() {
    auto radio_mgr = get_and_init_radio_mgr();
    if (radio_mgr) {
        promise<ErrorCode> p;
        radio_mgr->stopCv2x([&p](ErrorCode error) { p.set_value(error); });
        if (ErrorCode::SUCCESS == p.get_future().get()) {
            return V2X_STATUS_SUCCESS;
        }
    }
    LOGE("Failed to stop v2x mode\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type get_iface_name(
    traffic_ip_type_t ip_type, char *iface_name, size_t buffer_size) {
    TrafficIpType ipType;
    convert_enum(ip_type, ipType);

    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio interface is invalid\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    // Verify buffer has been initialized and interface name won't be truncated
    if (!iface_name || IFNAMSIZ > buffer_size) {
        LOGE("%s: Bad Param, uninitialized buffer or buffer size\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }
    auto iface = state_g.radio->getIfaceNameFromIpType(ipType);
    auto size  = iface.size();
    if (size > 0 && size < IFNAMSIZ) {
        memcpy(iface_name, state_g.radio->getIfaceNameFromIpType(ipType).c_str(), size);
        iface_name[size] = 0;
    }

    return V2X_STATUS_SUCCESS;
}

static shared_ptr<ICv2xTxRxSocket> find_tcp_socket(int fd) {
    lock_guard<mutex> lock(state_g.container_mutex);
    shared_ptr<ICv2xTxRxSocket> sock = nullptr;
    if (1 == state_g.fd_to_tcp_sock_map.count(fd)) {
        sock = state_g.fd_to_tcp_sock_map[fd];
    }
    return sock;
}

static void add_tcp_socket(int fd, const shared_ptr<ICv2xTxRxSocket> &sock) {
    lock_guard<mutex> lock(state_g.container_mutex);
    state_g.fd_to_tcp_sock_map[fd] = sock;
}

static void erase_tcp_socket(int fd) {
    lock_guard<mutex> lock(state_g.container_mutex);
    state_g.fd_to_tcp_sock_map.erase(fd);
}

int v2x_radio_tcp_sock_create_and_bind(v2x_radio_handle_t handle,
    const v2x_tx_flow_info_t *event_info, const socket_info_t *sock_info, int *sock_fd,
    struct sockaddr_in6 *sockaddr) {
    iface_handle_t ifHandle;
    if (getIfHandle(handle, nullptr, ifHandle)) {
        return -EINVAL;
    }

    if (!state_g.radio || ifHandle.ipType != TrafficIpType::TRAFFIC_IP) {
        LOGE(
            "%s: error interface type %d or invlaid raido status\n", __FUNCTION__, ifHandle.ipType);
        return -EINVAL;
    }

    if (!event_info || !sock_info || !sock_fd || !sockaddr) {
        LOGE("%s: input parameter error", __FUNCTION__);
        return -EINVAL;
    }

    LOGI("%s: Atempting TCP socket creation, sid=%d, localPort=%d\n", __FUNCTION__,
        sock_info->service_id, sock_info->local_port);

    // convert event info
    EventFlowInfo eventInfo;
    convert_event_flow_info(*event_info, eventInfo);

    // convert TCP socket info
    SocketInfo sockInfo;
    sockInfo.serviceId = sock_info->service_id;
    sockInfo.localPort = sock_info->local_port;

    // create new sock and register corresponding Tx/Rx flow
    promise<ErrorCode> p;
    shared_ptr<ICv2xTxRxSocket> socket;
    auto status = state_g.radio->createCv2xTcpSocket(
        eventInfo, sockInfo, [&p, &socket](shared_ptr<ICv2xTxRxSocket> sock, ErrorCode error) {
            socket = sock;
            p.set_value(error);
        });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: creating Event TCP socket failed\n", __FUNCTION__);
        *sock_fd = -1;
        return -EPERM;
    }

    *sock_fd  = socket->getSocket();
    *sockaddr = socket->getSocketAddr();
    add_tcp_socket(*sock_fd, socket);

    LOGI("%s: Event TCP socket creation succeeded, fd=%d\n", __FUNCTION__, *sock_fd);
    return V2X_STATUS_SUCCESS;
}

static int close_tcp_socket(shared_ptr<ICv2xTxRxSocket> &sock) {
    if (!sock || !state_g.radio) {
        LOGE("%s: Invalid Input\n", __FUNCTION__);
        return -EPERM;
    }

    LOGI("%s: Closing TCP socket, fd=%d\n", __FUNCTION__, sock->getSocket());

    promise<int> p;
    auto status = state_g.radio->closeCv2xTcpSocket(
        sock, [&p](const shared_ptr<ICv2xTxRxSocket> unused, ErrorCode error) {
            p.set_value(static_cast<int>(error));
        });
    if (Status::SUCCESS != status) {
        return -EPERM;
    }
    return p.get_future().get();
}

v2x_status_enum_type v2x_set_peak_tx_power(int8_t txPower) {
    auto radio_mgr = get_and_init_radio_mgr();
    if (radio_mgr) {
        promise<ErrorCode> p;
        radio_mgr->setPeakTxPower(txPower, [&p](ErrorCode error) { p.set_value(error); });
        if (ErrorCode::SUCCESS == p.get_future().get()) {
            LOGD("success to set_peak_tx_power\n");
            return V2X_STATUS_SUCCESS;
        }
    }
    LOGE("Failed to set_peak_tx_power\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_set_l2_filters(uint32_t list_len, src_l2_filter_info *list_array) {
    std::vector<L2FilterInfo> filter_list;
    auto radio_mgr = get_and_init_radio_mgr();
    uint32_t len   = (list_len > MAX_FILTER_IDS_LIST_LEN) ? MAX_FILTER_IDS_LIST_LEN : list_len;
    if (radio_mgr && list_array != nullptr && len > 0) {
        promise<ErrorCode> p;
        for (uint32_t i = 0; i < len; i++) {
            L2FilterInfo filter_item;
            filter_item.srcL2Id    = list_array[i].src_l2_id;
            filter_item.durationMs = list_array[i].duration_ms;
            filter_item.pppp       = list_array[i].pppp;
            filter_list.emplace_back(filter_item);
        }
        radio_mgr->setL2Filters(filter_list, [&p](ErrorCode error) { p.set_value(error); });
        if (ErrorCode::SUCCESS == p.get_future().get()) {
            LOGD("success to v2x_set_filter_list\n");
            return V2X_STATUS_SUCCESS;
        }
    }
    LOGE("Failed to v2x_set_filter_list\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_remove_l2_filters(uint32_t list_len, uint32_t *l2_id_list) {
    std::vector<uint32_t> l2_ids;
    uint32_t len = (list_len > MAX_FILTER_IDS_LIST_LEN) ? MAX_FILTER_IDS_LIST_LEN : list_len;

    auto radio_mgr = get_and_init_radio_mgr();
    if (radio_mgr && l2_id_list != nullptr && len > 0) {
        promise<ErrorCode> p;
        for (uint32_t i = 0; i < len; i++) {
            l2_ids.emplace_back(l2_id_list[i]);
        }
        radio_mgr->removeL2Filters(l2_ids, [&p](ErrorCode error) { p.set_value(error); });
        if (ErrorCode::SUCCESS == p.get_future().get()) {
            LOGD("success for %s\n", __FUNCTION__);
            return V2X_STATUS_SUCCESS;
        }
    }
    LOGE("Failed for %s\n", __FUNCTION__);
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_register_tx_status_report_listener(
    uint16_t port, v2x_tx_status_report_listener callback) {

    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio is invalid\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }

    if (not callback) {
        LOGE("%s:Error callbak\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }

    auto listener = make_shared<TxStatusReportListener>(callback);
    if (not listener) {
        LOGE("%s: Error instantiating Tx status report listener\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    promise<ErrorCode> p;
    auto status = state_g.radio->registerTxStatusReportListener(
        port, listener, [&p](ErrorCode error) { p.set_value(error); });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s:register listener with port:%d failed\n", __FUNCTION__, port);
        return V2X_STATUS_FAIL;
    }

    LOGD("%s:register listener with port:%d succeeded\n", __FUNCTION__, port);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_deregister_tx_status_report_listener(uint16_t port) {

    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio is invalid\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }

    promise<ErrorCode> p;
    auto status = state_g.radio->deregisterTxStatusReportListener(
        port, [&p](ErrorCode error) { p.set_value(error); });
    if (Status::SUCCESS != status or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s:deregister listener with port:%d failed\n", __FUNCTION__, port);
        return V2X_STATUS_FAIL;
    }

    LOGD("%s:deregister listener with port:%d succeeded\n", __FUNCTION__, port);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_set_global_IPaddr(uint8_t prefix_len, uint8_t *ipv6_addr) {
    struct IPv6AddrType ipv6_info;

    if (nullptr == ipv6_addr) {
        LOGE("%s: Invalid parameters\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }
    if (!state_g.radio) {
        LOGE("%s: Invalid state\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }

    ipv6_info.prefixLen = prefix_len;
    memcpy(ipv6_info.ipv6Addr, ipv6_addr, CV2X_IPV6_ADDR_ARRAY_LEN);

    promise<ErrorCode> p;
    auto status
        = state_g.radio->setGlobalIPInfo(ipv6_info, [&p](ErrorCode error) { p.set_value(error); });
    if (Status::SUCCESS == status && ErrorCode::SUCCESS == p.get_future().get()) {
        LOGD("success for %s\n", __FUNCTION__);
        return V2X_STATUS_SUCCESS;
    };

    LOGE("Failed to v2x_set_global_IPaddr\n");
    return V2X_STATUS_FAIL;
}

v2x_status_enum_type v2x_set_ip_routing_info(uint8_t *dest_mac_addr) {
    struct GlobalIPUnicastRoutingInfo req;

    if (nullptr == dest_mac_addr) {
        LOGE("%s: Invalid parameters\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }
    if (!state_g.radio) {
        LOGE("%s: Invalid state\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }

    memcpy(req.destMacAddr, dest_mac_addr, CV2X_MAC_ADDR_LEN);
    promise<ErrorCode> p;
    auto status = state_g.radio->setGlobalIPUnicastRoutingInfo(
        req, [&p](ErrorCode error) { p.set_value(error); });
    if (Status::SUCCESS == status && ErrorCode::SUCCESS == p.get_future().get()) {
        LOGD("success for %s\n", __FUNCTION__);
        return V2X_STATUS_SUCCESS;
    };

    LOGE("Failed to v2x_set_ip_routing_info\n");
    return V2X_STATUS_FAIL;
}

static void convert_v2x_ext_radio_status(const Cv2xStatusEx &in, v2x_radio_status_ex_t *out) {
    if (nullptr == out) {
        LOGE("%s: invalid input\n", __FUNCTION__);
        return;
    }

    // convert overall Tx/Rx status and cause
    convert_enum(in.status.txStatus, out->status.tx_status.status);
    convert_enum(in.status.rxStatus, out->status.rx_status.status);
    convert_enum(in.status.txCause, out->status.tx_status.cause);
    convert_enum(in.status.rxCause, out->status.rx_status.cause);

    out->tx_pool_size = 0;
    out->rx_pool_size = 0;
    // Tx/Rx pool status unknown means Tx/Rx pool does not exist, not add pool in
    // this case
    for (auto i = 0u; i < in.poolStatus.size(); ++i) {
        if (out->tx_pool_size < V2X_MAX_TX_POOL_NUM
            and in.poolStatus[i].status.txStatus != Cv2xStatusType::UNKNOWN) {
            out->tx_pool_status[out->tx_pool_size].pool_id = in.poolStatus[i].poolId;
            convert_enum(in.poolStatus[i].status.txStatus,
                out->tx_pool_status[out->tx_pool_size].status.status);
            convert_enum(in.poolStatus[i].status.txCause,
                out->tx_pool_status[out->tx_pool_size].status.cause);
            out->tx_pool_size++;
        }

        if (out->rx_pool_size < V2X_MAX_RX_POOL_NUM
            and in.poolStatus[i].status.rxStatus != Cv2xStatusType::UNKNOWN) {
            out->rx_pool_status[out->rx_pool_size].pool_id = in.poolStatus[i].poolId;
            convert_enum(in.poolStatus[i].status.rxStatus,
                out->rx_pool_status[out->rx_pool_size].status.status);
            convert_enum(in.poolStatus[i].status.rxCause,
                out->rx_pool_status[out->rx_pool_size].status.cause);
            out->rx_pool_size++;
        }
    }
    LOGD("%s: Overall Tx status=%d cause=%d, Rx status=%d cause=%d, "
         "Tx pool size=%d, Rx pool size=%d\n",
        __FUNCTION__, out->status.tx_status.status, out->status.tx_status.cause,
        out->status.rx_status.status, out->status.rx_status.cause, out->tx_pool_size,
        out->rx_pool_size);
}

v2x_status_enum_type v2x_get_ext_radio_status(v2x_radio_status_ex_t *status) {
    if (nullptr == status) {
        LOGE("%s: Invalid parameters\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }

    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    Cv2xStatusEx exStatus;
    promise<ErrorCode> p;

    auto ret = state_g.radio_mgr->requestCv2xStatus([&](Cv2xStatusEx tmpStatus, ErrorCode error) {
        exStatus = tmpStatus;
        p.set_value(error);
    });

    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Get V2X Status failed\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    convert_v2x_ext_radio_status(exStatus, status);
    LOGI("%s: Get V2X Status Succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_register_ext_radio_status_listener(
    v2x_ext_radio_status_listener callback) {
    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    {
        lock_guard<mutex> lock(state_g.ext_radio_status_mtx);
        state_g.ext_radio_status_listener = callback;
    }

    if (callback) {
        // ensure new registered listener gets initial notification
        state_g.need_initial_ext_callback = true;
        LOGD("%s:register listener succeeded\n", __FUNCTION__);
        state_g.radio_mgr->registerListener(state_g.cv2x_listener);
    } else {
        // support deregistration of this listener
        state_g.need_initial_ext_callback = false;
        LOGD("%s:deregister listener succeeded\n", __FUNCTION__);
    }
    return V2X_STATUS_SUCCESS;
}

static void convert_v2x_slss_rx_info(const SlssRxInfo &in, v2x_slss_rx_info_t *out) {
    if (nullptr == out) {
        LOGE("%s: invalid input\n", __FUNCTION__);
        return;
    }

    out->num_ue = in.ueInfo.size();

    for (auto i = 0u; i < in.ueInfo.size() and i < V2X_MAX_SLSS_SYNC_REF_UE_NUM; ++i) {
        out->ue_info[i].slss_id     = in.ueInfo[i].slssId;
        out->ue_info[i].in_coverage = in.ueInfo[i].inCoverage;
        convert_enum(in.ueInfo[i].pattern, out->ue_info[i].pattern);
        out->ue_info[i].selected = in.ueInfo[i].selected;
        out->ue_info[i].rsrp     = in.ueInfo[i].rsrp;
    }
}

v2x_status_enum_type v2x_get_slss_rx_info(v2x_slss_rx_info_t *slss_info) {
    if (nullptr == slss_info) {
        LOGE("%s: Invalid parameters\n", __FUNCTION__);
        return V2X_STATUS_EBADPARM;
    }

    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    SlssRxInfo slssInfo;
    promise<ErrorCode> p;
    auto ret = state_g.radio_mgr->getSlssRxInfo([&](SlssRxInfo info, ErrorCode error) {
        slssInfo = info;
        p.set_value(error);
    });

    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Get SLSS Rx Info failed\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    convert_v2x_slss_rx_info(slssInfo, slss_info);
    LOGI("%s: Get SLSS Rx Info Succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_register_slss_rx_listener(v2x_slss_rx_listener callback) {
    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    shared_ptr<SlssRxListener> listener = nullptr;
    try {
        listener = make_shared<SlssRxListener>(callback);
    } catch (std::bad_alloc &e) {
        LOGE("%s: Error instantiating SLSS Rx info listener\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    {
        lock_guard<mutex> lock(state_g.slss_listener_mtx);
        if (Status::SUCCESS != state_g.radio_mgr->registerListener(listener)) {
            LOGE("%s:register listener failed\n", __FUNCTION__);
            return V2X_STATUS_FAIL;
        }
        state_g.slss_listeners.push_back(listener);
    }

    LOGD("%s:register listener succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_deregister_slss_rx_listener(v2x_slss_rx_listener callback) {
    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    vector<shared_ptr<SlssRxListener>>::iterator it;
    {
        lock_guard<mutex> lock(state_g.slss_listener_mtx);
        it = std::find_if(begin(state_g.slss_listeners), end(state_g.slss_listeners),
            [callback](
                shared_ptr<SlssRxListener> tmp) { return (tmp->getCallback() == callback); });
        if (it == end(state_g.slss_listeners)) {
            LOGE("%s: listener not exist\n", __FUNCTION__);
            return V2X_STATUS_FAIL;
        }

        if (Status::SUCCESS != state_g.radio_mgr->deregisterListener(*it)) {
            LOGE("%s:deregister listener failed\n", __FUNCTION__);
            return V2X_STATUS_FAIL;
        }
        state_g.slss_listeners.erase(it);
    }

    LOGD("%s:deregister listener succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_inject_coarse_utc_time(uint64_t utc) {
    state_g.radio_mgr = get_and_init_radio_mgr();
    if (not state_g.radio_mgr) {
        LOGE("%s: Failed to acquire Cv2xRadioManager\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }

    promise<ErrorCode> p;
    auto ret
        = state_g.radio_mgr->injectCoarseUtcTime(utc, [&p](ErrorCode err) { p.set_value(err); });
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Failed to set coarse utc\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }

    LOGD("%s:inject UTC succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}

v2x_status_enum_type v2x_inject_vehicle_speed(uint32_t speed) {
    if (!state_g.radio) {
        LOGE("%s: called when C-V2X radio is invalid\n", __FUNCTION__);
        return V2X_STATUS_RADIO_NOT_READY;
    }
    promise<ErrorCode> p;
    auto ret = state_g.radio->injectVehicleSpeed(speed, [&p](ErrorCode err) { p.set_value(err); });
    if (Status::SUCCESS != ret or ErrorCode::SUCCESS != p.get_future().get()) {
        LOGE("%s: Failed to inject speed\n", __FUNCTION__);
        return V2X_STATUS_FAIL;
    }
    LOGD("%s:inject speed succeeded\n", __FUNCTION__);
    return V2X_STATUS_SUCCESS;
}
