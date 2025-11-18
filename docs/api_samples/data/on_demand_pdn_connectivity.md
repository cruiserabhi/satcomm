Establish on-demand PDN connectivity {#on_demand_pdn_connectivity}
==================================================================

For on-demand PDNs connectivity:
- The DNS servers are not updated in resolv.conf and routines like gethostbyname would fail
- The default routes are not setup and socket connection without binding to an interface would fail
The purpose of the section is to provide information about:
- DNS resolution using dig
- Binding to an interface to enable connectivity

### 1. Get Data Connection Manager and wait for service availability

Get the data factory, data connection manager and wait until the service is available.

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   {
        std::promise<telux::common::ServiceStatus> p;
        dataConnMgr = dataFactory.getDataConnectionManager(
            slotId, [&](telux::common::ServiceStatus status) { p.set_value(status); });
        if (dataConnMgr) {
            std::cout << "\n\nInitializing Data connection manager subsystem on slot " << slotId
                      << ", Please wait ..." << std::endl;
            subSystemStatus = p.get_future().get();
        }
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data sub-system is ready" << std::endl;
        } else {
            std::cerr << "Unable to initialize data subsystem. Exiting..." << std::endl;
            exit(1);
        }
   }
   ~~~~~~

### 2. Register listener

Register the listener with the data connection manager for listening to data call status.

   ~~~~~~{.cpp}
   dataConnMgr->registerListener(dataListener);
   ~~~~~~

### 3. Start data call on the mentioned slot Id and profile id and operation type

   ~~~~~~{.cpp}
    {
     std::promise<telux::common::ErrorCode> p;
     telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
     dataConnMgr->startDataCall(
            profileId, ipFamilyType,
            [&](const std::shared_ptr<telux::data::IDataCall> &dataCall,
                telux::common::ErrorCode errorCode) {
                std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode)
                          << std::endl;
                p.set_value(errorCode);
            },
            opType);

     telux::common::ErrorCode errorCode = p.get_future().get();
     if (errorCode != telux::common::ErrorCode::SUCCESS) {
         std::cerr << "Failed to start data call. Exiting..." << std::endl;
         exit(1);
     }
    }
   ~~~~~~

### 4. Wait for the data call to get connected

When the data call is connected, obtain a reference to the data call that was brought up. This
information would be sent to the listener.

   ~~~~~~{.cpp}
   // In the listener
   void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
        std::cout << "\n onDataCallInfoChanged";
        logDataCallDetails(dataCall);
        if (dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
            p_.set_value(dataCall);
        }
   }

   // In the main method
   std::shared_ptr<telux::data::IDataCall> dataCall = dataCallFuture.get();
   if (dataCall == nullptr) {
        std::cerr << "Could not get data call object. Exiting..." << std::endl;
        exit(1);
   }
   ~~~~~~

### 5. Resolve the remote host using the DNS address provided by the data call

Use the DNS address (Ex: primaryDnsAddress) information from the data call object to resolve the
domain using dig (Domain Information Groper). dig has multiple modes and accepts a variety of
parameters, however, for the purpose of name resolution, we use the "+short" mode where detailed
answers are not output by dig, but only the IP addresses are provided. We parse the IP addresses
provided by dig to see if it is a valid IP address.

   ~~~~~~{.cpp}
   // The resolve method
   std::string resolve(std::string domain, std::string dnsAddress) {
        std::cout << "Resolving " << domain << " using DNS server at " << dnsAddress << std::endl;
        FILE *cmd;
        std::string ipAddress;
        char cipAddress[SIZE_IP_ADDR_BUF] = {0};

        // Use the provided DNS address to request dig for name resolution
        std::string command = "/usr/bin/dig @" + dnsAddress + " " + domain + " +short";
        std::cout << "Command: " << command << std::endl;
        cmd = popen(command.c_str(), "r");
        if (cmd) {
            sockaddr_in address;
            // Get all the answers from the DNS server
            while (NULL != fgets(cipAddress, SIZE_IP_ADDR_BUF, cmd)) {
                cipAddress[strlen(cipAddress) - 1] = '\0';

                // If the received answer is verified as a valid IP address, return the address
                if (inet_pton(AF_INET, cipAddress, &address.sin_addr) == 1) {
                    ipAddress = cipAddress;
                    std::cout << ipAddress;
                    ipAddress.erase(
                        std::remove(ipAddress.begin(), ipAddress.end(), '\n'), ipAddress.end());
                    break;
                }
            }
        }
        std::cout << "\n\n";  // Declutters output from dig
        return ipAddress;
   }

   // In the main method
   std::string remoteIp = resolve(domain, dataCall->getIpv4Info().addr.primaryDnsAddress);
   if (remoteIp == "") {
       std::cerr << "Could not resolve " << domain << ". Exiting..." << std::endl;
       exit(1);
   }
   std::cout << "Resolved " << domain << " to " << remoteIp << std::endl;
   ~~~~~~

### 6. Connect to the remote host

In addition to the usual connection routine on the client side, we additionally would need to
bind to the interface that would allow us to reach the remote host. This is done here by using the
setsockopt method.

   ~~~~~~{.cpp}
   // The connect method
    int connect(std::string ipAddress, std::string outBoundIf, std::string portNumber) {
        std::cout << "Connecting to " << ipAddress << " on port " << portNumber << " via " << outBoundIf
                << std::endl;
        int sockfd = 0;
        sockaddr_in serverIpAddress;
        serverIpAddress.sin_family = AF_INET;
        serverIpAddress.sin_port = htons(stoi(portNumber));

        if (inet_pton(AF_INET, ipAddress.c_str(), &serverIpAddress.sin_addr) <= 0) {
            std::cerr << "Cannot parse IP address" << std::endl;
            return -1;
        }
        // Create the socket
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "Socket creation failed" << std::endl;
            return -1;
        }

        // Bind the socket to the interface
        ifreq ifr;
        g_strlcpy(ifr.ifr_name, outBoundIf.c_str(), outBoundIf.length());
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
            std::cerr << "Socket bind failed with " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }

        // Connect to the remote host
        if (connect(sockfd, (sockaddr *)&serverIpAddress, sizeof(serverIpAddress)) < 0) {
            std::cerr << "Connect failed: " << strerror(errno) << std::endl;
            close(sockfd);
            return -1;
        }
        return sockfd;
    }

    // In the main method
    int sockfd = connect(remoteIp, dataCall->getInterfaceName(), portNumber);
    if (sockfd < 0) {
        std::cerr << "Could not connect to " << domain << ". Exiting..." << std::endl;
        exit(1);
    }
    std::cout << "Connected to " << domain << std::endl;
   ~~~~~~

### 7. Clean-up

   ~~~~~~{.cpp}
    std::cout << "Cleaning up" << std::endl;
    close(sockfd);
    dataConnMgr->deregisterListener(dataListener);
    dataConnMgr = nullptr;
   ~~~~~~
