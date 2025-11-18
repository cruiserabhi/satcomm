.. _cellular-connection-report-listener:

Receive cellular security reports 
==========================================================================

Steps to register listener and receive cellular connection security reports are:

1. Define listener that will receive report.
2. Get ConnectionSecurityFactory instance.
3. Get ICellularSecurityManager instance from ConnectionSecurityFactory.
4. Register listener to receive security reports.
5. Receive reports in the registered listener.
6. Optional, implement onServiceStatusChange() if SSR updates are required.
7. When the use-case is complete, deregister the listener.

.. code-block::

  #include <iostream>
  #include <chrono>
  #include <thread>
  
  #include <telux/sec/ConnectionSecurityFactory.hpp>
  
  class CellSecurityReportListener : public telux::sec::ICellularScanReportListener {
  
      /* Step - 5 */
      void onScanReportAvailable(telux::sec::CellularSecurityReport report,
              telux::sec::EnvironmentInfo envInfo) {
  
            std::cout << "Threat score: " << static_cast<uint32_t>(report.threatScore) << std::endl;
            std::cout << "Cell ID     : " << static_cast<uint32_t>(report.cellId) << std::endl;
            std::cout << "PID         : " << static_cast<uint32_t>(report.pid) << std::endl;
            std::cout << "MCC         : " << report.mcc << std::endl;
            std::cout << "MNC         : " << report.mnc << std::endl;
            std::cout << "Action type : " << static_cast<uint32_t>(report.actionType) << std::endl;
            std::cout << "RAT         : " << static_cast<uint32_t>(report.rat) << std::endl;

          for (size_t x = 0; x < report.threatTypesDetected.size(); x++) {
              std::cout << "Threat type : " <<
                  static_cast<uint32_t>(report.threatTypesDetected[x]) << std::endl;
          }
      }

      /* Step - 6 */
      void onServiceStatusChange(telux::common::ServiceStatus newStatus) {

          std::cout << "New status: " << static_cast<uint32_t>(newStatus) << std::endl;
      }
  };
  
  int main(int argc, char **argv) {
  
      telux::common::ErrorCode ec;
      std::shared_ptr<CellSecurityReportListener> reportListener;
      std::shared_ptr<telux::sec::ICellularSecurityManager> cellConSecMgr;
  
      /* Step - 1 */
      try {
          reportListener = std::make_shared<CellSecurityReportListener>();
      } catch (const std::exception& e) {
          std::cout << "can't allocate CellSecurityReportListener" << std::endl;
          return -ENOMEM;
      }
  
      /* Step - 2 */
      auto &cellConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();
  
      /* Step - 3 */
      cellConSecMgr = cellConSecFact.getCellularSecurityManager(ec);
      if (!cellConSecMgr) {
          std::cout <<
             "can't get ICellularSecurityManager, err " << static_cast<int>(ec) << std::endl;
          return -ENOMEM;
      }
  
      /* Step - 4 */
      ec = cellConSecMgr->registerListener(reportListener);
      if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
          return -EIO;
    }
  
      /* Add application specific business logic here */
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  
      /* Step - 7 */
      ec = cellConSecMgr->deRegisterListener(reportListener);
      if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "can't deregister listener, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
      }

        return 0;
  }
