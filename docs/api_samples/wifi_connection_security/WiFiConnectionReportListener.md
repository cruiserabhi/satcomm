Receive wifi security reports {#wifi_connection_report_listener}
==========================================================================

Steps to register listener and receive wifi connection security reports are:

1. Define listener that will receive report and receive invocation for consent to trusting an AP.
2. Get ConnectionSecurityFactory instance.
3. Get IWiFiSecurityManager instance from ConnectionSecurityFactory.
4. Register listener to receive security reports.
5. Receive reports in the registered listener.
6. When the use-case is complete, deregister the listener.

   ~~~~~~{.cpp}
#include <iostream>
#include <chrono>
#include <thread>

#include <telux/sec/ConnectionSecurityFactory.hpp>

class WiFiSecurityReportListener : public telux::sec::IWiFiReportListener {

    /* Step - 5 */
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

    void isTrustedAP(std::string ssid, bool& isTrusted) {
        /* In this example we always trust the AP */
        isTrusted = true;
    }
};

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    std::shared_ptr<WiFiSecurityReportListener> reportListener;
    std::shared_ptr<telux::sec::IWiFiSecurityManager> wifiConSecMgr;

    /* Step - 1 */
    try {
        reportListener = std::make_shared<WiFiSecurityReportListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate WiFiSecurityReportListener" << std::endl;
        return -ENOMEM;
    }

    /* Step - 2 */
    auto &wifiConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();

    /* Step - 3 */
    wifiConSecMgr = wifiConSecFact.getWiFiSecurityManager(ec);
    if (!wifiConSecMgr) {
        std::cout <<
         "can't get IWiFiSecurityManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    /* Step - 4 */
    ec = wifiConSecMgr->registerListener(reportListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    /* Add application specific business logic here */
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    /* Step - 6 */
    ec = wifiConSecMgr->deRegisterListener(reportListener);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}
   ~~~~~~