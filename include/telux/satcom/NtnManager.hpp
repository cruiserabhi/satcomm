/*
 *
 *    Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *    SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file    NtnManager.hpp
 *
 *          Non Terrestrial Networks (NTN) allows the UE to connect to the satellite based networks.
 *
 *          NtnManager class provides following capabilities:
 *          - enable/disable NTN mode
 *          - send/receive non-IP data over NTN network
 *          - enable/disable cellular terrestrial network scan while NTN is active
 *          - configure system selection specifiers
 *          - monitor NTN state
 *          - monitor NTN service availability
 *          - monitor NTN network capabilities
 *          - monitor signal strength of the NTN network
 *
 *          @note
 *          Only once instance of this manager can be active throughout the system. Creating
 *          multiple instances of NtnManager within one or more processes is undefined behavior.
 *
 */

#ifndef TELUX_SATCOM_NTNMANAGER_HPP
#define TELUX_SATCOM_NTNMANAGER_HPP

#include <vector>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace satcom {

/** @addtogroup telematics_satcom
 * @{ */

// Forward declarations
class INtnListener;

/**
 * Defines the supported NTN states.
 */
enum class NtnState {
    DISABLED, /**< NTN is disabled */
    OUT_OF_SERVICE, /**< NTN is enabled but device is not registered with the NTN service provider*/
    IN_SERVICE, /**< Normal operation, device is registered with a NTN service provider and
                   is online */
};

/**
 * Every sent data packet has a unique Transaction identifier. @ref telux::satcom::sendData API
 * will return a transaction identifier which can be used to map to acknowledgement received in
 * @ref telux::satcom::onDataAck
 * .
 */
using TransactionId = uint64_t;

/**
 *
 * Capabilities of the underlying NTN network. Client can call
 * @ref telux::satcom::getNtnCapabilities to get the NTN network capabilities. The NTN network
 * capabilities might change over time and client shall implement the onCapabilitiesChange
 * listener in order to get the latest capabilities.
 *
 */
struct NtnCapabilities {
    int64_t maxDataSize; /**< Maximum size of the data that can be sent in bytes*/
};

/**
 * System selection information
 */
struct SystemSelectionSpecifier {
    std::string mcc; /**< Mobile country code */
    std::string mnc; /**< Mobile network code */
    std::vector<uint64_t> ntnBands; /**< List of RF bands */
    std::vector<uint64_t> ntnEarfcns; /**< List of E-UTRAN absolute radio frequency channels */
};

enum SignalStrength {
    NONE     = -1,
    POOR     = 1,
    MODERATE = 2,
    GOOD     = 3,
    GREAT    = 4,
};

/**
 *@brief    NtnManager is a primary interface for configuring NTN network and sending non-IP data.
 */
class INtnManager {
 public:
    /**
     * Checks the status of NtnManager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If NtnManager object is ready for service.
     *          SERVICE_UNAVAILABLE    If NtnManager object is temporarily unavailable due to a
     *                                 crash in an underlying service.
     *          SERVICE_FAILED       - If NtnManager object encountered an irrecoverable failure.
     *
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Checks if NTN mode is supported on this device.
     *
     * @param [out] isSupported True, if NTN mode is supported, false in all other cases
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode isNtnSupported(bool &isSupported) = 0;

    /**
     * Enable or disable NTN mode. Enabling NTN will result into modem disabling the
     * TN (terrestrial network). Disabling NTN will result into modem enabling the TN.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_NTN_CONFIG
     * permission to invoke this API successfully.
     *
     * @note If isEmergency is set to true, the NTN network can be used for both emergency and
     * non-emergency purposes. If isEmergency is set to false, the NTN can only be used
     * for non-emergency purposes.
     * Whether a data packet being sent is emergency or non-emergency can be specified
     * while calling the @ref telux::satcom::sendData API.
     *
     * @param [in] enable       Enable/Disable NTN mode.
     * @param [in] isEmergency  True, if this NTN connection can be used for emergency purposes.
     * @param [in] iccid        Integrated Circuit Card Identification (ICCID) of the SIM to be used
     *                          for NTN. @ref telux::tel::ISubscription::getIccId can be used to
     *                          get the ICCID of the SIM to be used for NTN.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode enableNtn(
        bool enable, bool isEmergency, const std::string &iccid) = 0;

    /**
     * Send non-IP data over NTN network.
     * @note This API should only be called when the NTN state is IN_SERVICE. Refer to @ref
     * getNtnState() API to get the NTN state.
     *
     * @note The maximum size of the data packet must be less than maxDataSize returned by the
     * @ref telux::satcom::getNtnCapabilities or @ref telux::satcom::onCapabilitiesChange.
     *
     * @note This function does not guarantee the delivery of the packet. Refer to @ref
     * telux::satcom::onDataAck listener to get the delivery status (L2 ack/timeout) of the packet.
     * The transaction Id returned can be used to map messages to their respective acknowledgements.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_NTN_DATA
     * permission to invoke this API successfully.
     *
     * @param [in]  data Data to be sent over the NTN network.
     * @param [in]  size Number of bytes to be sent.
     * @param [in]  isEmergency indicate if this is emergency data. This parameter can be set to
     *              true only if @ref telux::satcom::enableNtn is called with isEmergency set to true.
     * @param [out] TransactionId The message ID of the data packet.
     *
     * @returns @ref telux::common::Status::SUCCESS if the modem accepts the data packet to send
     *               over the NTN network or appropriate status otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     *
     */
    virtual telux::common::Status sendData(
        uint8_t *data, uint32_t size, bool isEmergency, TransactionId &TransactionId) = 0;

    /**
     * Abort all the data packets waiting in the queue for transmission.
     * This API has no effect on already transmitted packets. All the aborted packets will have
     * corresponding @onDataAck called with appropriate error.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_NTN_DATA
     * permission to invoke this API successfully.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode abortData() = 0;

    /**
     * Get the capabilties of NTN network.
     * @note that the capabilities returned by this API might change over the period of time.
     * The client shall implement @ref telux::satcom::onCapabilitiesChange to receive the updated
     * capabilities.
     *
     * @param [out] capabilities Capabilities of the NTN network.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getNtnCapabilities(NtnCapabilities &capabilities) = 0;

    /**
     * Get the signal strength of the NTN network.
     *
     * @param [out] SignalStrength Signal strength of the NTN network.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getSignalStrength(SignalStrength &signalStrength) = 0;

    /**
     * Update the system selection specifiers (SFL list) that modem uses to scan for NTN network.
     * Modem will prioritize the SFL list provided by this API to expedite acquisition of service
     * If the modem fails to acquire service using SFL, modem will perform band scan.
     * This API shall be called only before calling @ref enableNtn, otherwise it will not have any
     * effect.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_NTN_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in]  params SFL list.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode updateSystemSelectionSpecifiers(
        std::vector<SystemSelectionSpecifier> &params)
        = 0;

    /**
     * Returns current NTN state. For further details on NTN states,
     * refer to @NtnState
     *
     * @returns Current NTN state.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual NtnState getNtnState() = 0;

    /**
     * Enable/disable background cellular scanning. If the background cellular scanning
     * is enabled, the modem will scan for the availability of TN networks while in the NTN mode.
     * The modem will run this scan periodically and the result will be communicated by
     * @ref telux::satcom::INtnManager::onCellularCoverageAvailable. The modem will not perform the
     * NTN to TN switch on its own. It is up to the client to decide whether to switch to TN mode
     * or not based on the result of the scan.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_NTN_CONFIG
     * permission to invoke this API successfully.
     *
     * @param[in] enable True, to enable cellular scan.
     *
     * @returns Error code which indicates whether operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode enableCellularScan(bool enable) = 0;

    /**
     * Register with NtnManager as listener for receiving service status, NTN state changes
     * and data availability notifications.
     *
     * @param[in] listener Receives the notifications
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     * an appropriate status otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<INtnListener> listener) = 0;

    /**
     * Deregisters a listener registered previously
     * with @ref telux::satcom::INtnManager::registerListener().
     *
     * @param [in] listener listener to be deregistered
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     * an appropriate status otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<INtnListener> listener) = 0;

    /**
     * Destructor for INtnManager
     */
    virtual ~INtnManager(){};
};

/**
 * Interface for NTN listener that allows client to be notified of asynchronous events.
 * Receives a notification whenever service status, NTN state, NTN capabilities or signal strength
 * is changed or data is received from NTN network.
 *
 * It is recommended that the client should not perform any blocking operation from
 * within the methods in this class. The implementation of the methods should be thread safe.
 *
 */
class INtnListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when NTN state changes.
     *
     * @param [in] state State of the NTN network
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onNtnStateChange(NtnState newState) {
    }

    /**
     * This function is called when the capabilities of the NTN network change.
     *
     * @param [in] capabilities The updated capabilities of the NTN network.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onCapabilitiesChange(NtnCapabilities capabilities) {
    }

    /**
     * This function is called when signal strength of the NTN network changes.
     *
     * @param [in] signalStrength The signal strength of the NTN network.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onSignalStrengthChange(SignalStrength signalStrength) {
    }

    /**
     * This function is called when service status changes. Service status will change when
     * the modem services are not available for any operations.
     *
     * @param [in] status - @ref telux::satcom::ServiceStatus
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {
    }

    /**
     * This function is called when the modem receives the acknowledgement for a sent data packet.
     * @note that the acknowledgement refers to L2 ack from the vendor eNodeB (eNB) in the NTN
     * network and not an end-to-end ack.
     *
     * @param [in] err telux::common::ErrorCode::SUCCESS if acknowledgement was received or
     *                 error otherwise.
     * @param [in] id Transaction id of the sent data packet
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onDataAck(telux::common::ErrorCode err, TransactionId id) {
    }

    /**
     * This function is called when the modem receives a data packet over NTN network.
     *
     * @param [in] data Data packet received over NTN network. The implementation of this
     *                  function is responsible to manage the lifetime of the buffer.
     * @param [in] size Size of the buffer.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onIncomingData(std::unique_ptr<uint8_t[]> data, uint32_t size) {
    }

    /**
     * This function is called when the modem scans for the cellular coverage and has a result
     * available. This API only indicates if ANY cellular coverage is available. It does not specify
     * whether this cell would provide full service vs limited service only.
     *
     * Cellular coverage is enabled by calling @ref telux::satcom::INtnManager::enableCellularScan.
     *
     * @param [in] isCellularCoverageAvailable Flag indicating availability of cellular coverage.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onCellularCoverageAvailable(bool isCellularCoverageAvailable) {
    }

    /**
     * Destructor for INtnListener
     */
    virtual ~INtnListener(){};
};

/** @} */ /* end_addtogroup telematics_satcom */
}  // namespace satcom
}  // namespace telux

#endif  // TELUX_SATCOM_NTNMANAGER_HPP
