Add data filter {#add_data_filter}
==================================

This sample application demonstrates how to add a data filter.

### 1. Get the DataFactory, DataConnectionManager and DataFilterManager instances

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr_ = dataFactory.getDataConnectionManager();
   auto dataFilterMgr_ = dataFactory.getDataFilterManager();
   ~~~~~~

### 2. Wait for the Data Connection Manager and Data Filter Manager sub system initialization

   ~~~~~~{.cpp}
   bool dataConnectionSubSystemStatus = dataConnMgr_->isSubsystemReady();
   if (!dataConnectionSubSystemStatus) {
       std::cout << "Data Connection Manager subsystem is not ready, Please wait" << std::endl;
       std::future<bool> f = dataConnMgr_->onSubsystemReady();
       // Wait unconditionally for data manager subsystem to be ready
       dataConnectionSubSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if (!dataConnectionSubSystemStatus) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }

   bool dataFilterSubSystemStatus = dataFilterMgr_->isReady();
   if (!dataFilterSubSystemStatus) {
       std::cout << "Data Filter Manager subsystem is not ready, Please wait" << std::endl;
       std::future<bool> f = dataFilterMgr_->onReady();
       // Wait unconditionally for data filter subsystem to be ready
       dataFilterSubSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if (!dataFilterSubSystemStatus) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }
   ~~~~~~

### 3. Set data filter mode to enable

   ~~~~~~{.cpp}
   std::promise<bool> p;
   int profileId = 2;
   telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;

   telux::data::DataRestrictMode enableMode;
   enableMode.filterAutoExit = telux::data::DataRestrictModeType::DISABLE;
   enableMode.filterMode = telux::data::DataRestrictModeType::ENABLE;

   auto status = dataFilterMgr_->setDataRestrictMode(enableMode,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set data filter mode" << std::endl;
                p.set_value(false);
            }
        }, profileId, ipFamilyType);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Set data filter mode Request sent" << std::endl;
   } else {
      std::cout << "Set data filter mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Set data filter mode succeeded." << std::endl;
   }
   ~~~~~~

### 4. Add data filter

   ~~~~~~{.cpp}
   std::promise<bool> p;
   std::string ipAddr = std::string("168.128.91.1");
   int port = 8888;
   telux::data::IPv4Info ipv4Info_ = {};
   ipv4Info_.srcAddr = ipAddr;

   telux::data::PortInfo srcPort;
   srcPort.port = port;
   srcPort.range = 0;
   telux::data::UdpInfo udpInfo_ = {};
   udpInfo_.src = srcPort;

   // IpProtocol for UDP
   int PROTO_UDP =  17;
   // create a filter of UDP type, and set source IP and port.
   std::shared_ptr<telux::data::IIpFilter> dataFilter =
   dataFactory.getNewIpFilter(PROTO_UDP);
   dataFilter->setIPv4Info(ipv4Info_);

   auto udpRestrictFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(dataFilter);
   udpRestrictFilter->setUdpInfo(udpInfo_);

   auto status = dataFilterMgr_->addDataRestrictFilter(dataFilter,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to add data filter" << std::endl;
                p.set_value(false);
            }
        }, profileId, ipFamilyType);
   if(status == telux::common::Status::SUCCESS) {
        std::cout << "Add data filter Request sent" << std::endl;
   } else {
        std::cout << "Add data filter Request failed" << std::endl;
   }

   if (p.get_future().get()) {
             std::cout << "Add data filter succeeded." << std::endl;
   }
   ~~~~~~
