.. _wifi-connection-report-listener:

Receive wifi security reports
==========================================================================

Steps to register listener and receive wifi connection security reports are:

 1. Define listener that will receive report and receive invocation for consent to trusting an AP.
 2. Get ConnectionSecurityFactory instance.
 3. Get IWiFiSecurityManager instance from ConnectionSecurityFactory.
 4. Wait for the security service to become available.
 5. Register listener to receive security reports.
 6. Receive reports in the registered listener.
 7. When the use-case is complete, deregister the listener.

.. code-block::

#include <iostream>
#include <chrono>
#include <thread>
#include <future>

#include <telux/sec/ConnectionSecurityFactory.hpp>

class WiFiSecurityReportListener : public telux::sec::IWiFiReportListener,
                                   public std::enable_shared_from_this<WiFiSecurityReportListener> {
public:

    /* Step - 6 */
    void onReportAvailable(telux::sec::WiFiSecurityReport report) {
        std::cout << "ssid             : " << report.ssid << std::endl;
        std::cout << "bssid            : " << report.bssid << std::endl;
        std::cout << "is connected     : " << report.isConnectedToAP << std::endl;
        std::cout << "is open          : " << report.isOpenAP << std::endl;
        std::cout << "ml threat score  : " <<
            report.mlAlgorithmAnalysis.threatScore << std::endl;
        std::cout << "ml result        : " <<
            static_cast<int>(report.mlAlgorithmAnalysis.result) << std::endl;
        std::cout << "summoning result : " <<
            static_cast<int>(report.summoningAnalysis.result) << std::endl;
    }

    void onDeauthenticationAttack(telux::sec::DeauthenticationInfo deauthenticationInfo) {
        std::cout << "disconnect reason : " <<
            deauthenticationInfo.deauthenticationReason << std::endl;
        std::cout << "did AP initiated  : " <<
            deauthenticationInfo.didAPInitiateDisconnect << std::endl;
        std::cout << "threat score      : " <<
            deauthenticationInfo.threatScore << std::endl;
    }

    void isTrustedAP(telux::sec::ApInfo apInfo, bool& isTrusted) {
        std::cout << "ssid  : " << apInfo.ssid  << std::endl;
        std::cout << "bssid : " << apInfo.bssid << std::endl;

        /* In this example we always trust the AP */
        isTrusted = true;
    }

    /*
     * Initialize application and get a WiFiSecurityManager.
     */
    int init() {

        std::promise<telux::common::ServiceStatus> p{};
        telux::common::ServiceStatus serviceStatus;

        /* Step - 2 */
        auto &wifiConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();

        /* Step - 3 */
        wifiConSecMgr_ = wifiConSecFact.getWiFiSecurityManager([&](telux::common::ServiceStatus
        srvStatus) {
            p.set_value(srvStatus);
        });

        if (!wifiConSecMgr_) {
            std::cout << "failed to get IWiFiSecurityManager " << std::endl;
            return -ENOMEM;
        }

        /* Step - 4 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "security service unavailable" << std::endl;
            return -EIO;
        }

        std::cout << "initialization finished" << std::endl;
        return 0;
    }

    int registerSecurityReportListener() {
        telux::common::ErrorCode ec;
        /* Step - 5 */
        ec = wifiConSecMgr_->registerListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }

    int deregisterSecurityReportListener() {
        /* Step - 7 */
        telux::common::ErrorCode ec;
        ec = wifiConSecMgr_->deregisterListener(shared_from_this());
        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "can't deregister listener, err " << static_cast<int>(ec) << std::endl;
            return -EIO;
        }

        return 0;
    }
private:
    std::shared_ptr<telux::sec::IWiFiSecurityManager> wifiConSecMgr_;
};

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<WiFiSecurityReportListener> app;

    /* Step - 1 */
    try {
        app = std::make_shared<WiFiSecurityReportListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate WiFiSecurityReportListener" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->registerSecurityReportListener();
    if (ret < 0) {
        return ret;
    }

    /* Add application specific business logic here */
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    ret = app->deregisterSecurityReportListener();
    if (ret < 0) {
        return ret;
    }

    std::cout << "application exiting" << std::endl;
    return 0;
}
