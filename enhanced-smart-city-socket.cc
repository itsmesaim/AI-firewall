#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EnhancedSmartCitySimulation");

std::string
GetDistrictFromIP(Ipv4Address ip)
{
    std::ostringstream oss;
    ip.Print(oss);
    std::string ipStr = oss.str();

    if (ipStr.find("192.168.50.") == 0)
        return "IoT";
    else if (ipStr.find("192.168.10.") == 0 || ipStr.find("192.168.11.") == 0)
        return "Hospital";
    else if (ipStr.find("192.168.20.") == 0 || ipStr.find("192.168.21.") == 0)
        return "PowerGrid";
    else if (ipStr.find("192.168.30.") == 0)
        return "Finance";
    else if (ipStr.find("192.168.1.") == 0)
        return "Home";
    else if (ipStr.find("192.168.2.") == 0)
        return "Office";
    else if (ipStr.find("192.168.3.") == 0)
        return "University";
    else
        return "Core";
}

bool
QueryMLFirewall(uint32_t flowId,
                Ipv4Address srcIP,
                Ipv4Address dstIP,
                uint16_t dstPort,
                uint32_t txPackets,
                uint32_t rxPackets,
                uint64_t txBytes,
                uint64_t rxBytes,
                double duration,
                double throughput,
                double packetLoss,
                double delay,
                double jitter,
                std::string district)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        close(sock);
        return false;
    }

    std::ostringstream json;
    json << "{\"flowId\":" << flowId << ",\"srcIP\":\"" << srcIP << "\"" << ",\"dstIP\":\"" << dstIP
         << "\"" << ",\"txPackets\":" << txPackets << ",\"rxPackets\":" << rxPackets
         << ",\"txBytes\":" << txBytes << ",\"rxBytes\":" << rxBytes << ",\"duration\":" << duration
         << ",\"throughput\":" << throughput << ",\"packetLoss\":" << packetLoss
         << ",\"delay\":" << delay << ",\"jitter\":" << jitter << ",\"dstPort\":" << dstPort
         << ",\"district\":\"" << district << "\"}";

    std::string jsonStr = json.str();
    send(sock, jsonStr.c_str(), jsonStr.length(), 0);

    char buffer[1024] = {0};
    recv(sock, buffer, sizeof(buffer), 0);
    close(sock);

    std::string response(buffer);
    return response.find("\"shouldBlock\":true") != std::string::npos;
}

int
main(int argc, char* argv[])
{
    // simulation parameters
    bool generateAttacks = false;
    std::string scenario = "normal";
    double simTime = 180.0;

    CommandLine cmd;
    cmd.AddValue("attacks", "Generate attack traffic patterns", generateAttacks);
    cmd.AddValue("scenario", "Traffic scenario type", scenario);
    cmd.AddValue("time", "Simulation duration in seconds", simTime);
    cmd.Parse(argc, argv);

    std::cout << "Enhanced Smart City Network Simulation" << std::endl;
    std::cout << "Scenario: " << scenario << std::endl;
    std::cout << "Attacks: " << (generateAttacks ? "enabled" : "disabled") << std::endl;
    std::cout << "Duration: " << simTime << " seconds" << std::endl;

    // NETWORK TOPOLOGY
    // Core infrastructure
    NodeContainer coreNodes, cdnNodes, dnsNodes;
    coreNodes.Create(3); // Primary, Secondary, Emergency core
    cdnNodes.Create(2);  // Content delivery network
    dnsNodes.Create(2);  // DNS servers

    // District Gateways (7 districts)
    NodeContainer homeGW, officeGW, universityGW, iotGW, hospitalGW, powerGW, financeGW;
    homeGW.Create(1);
    officeGW.Create(1);
    universityGW.Create(1);
    iotGW.Create(1);
    hospitalGW.Create(1);
    powerGW.Create(1);
    financeGW.Create(1);

    // HOME DISTRICT (8 devices)
    NodeContainer homeDevices;
    homeDevices.Create(8); // Mother, Father, Child1, Child2, SmartTV, Alexa, Security, Router

    // OFFICE DISTRICT ( 12 devices)
    NodeContainer officeDevices;
    officeDevices.Create(12); // Manager, Employees(6), Servers(3), Security(2)

    // UNIVERSITY DISTRICT (15 devices)
    NodeContainer uniDevices, researchCluster;
    uniDevices.Create(10);     // Students, Professors, Admin
    researchCluster.Create(5); // HPC cluster for research

    //  IOT DISTRICT ( 25 devices)
    NodeContainer iotDevices, trafficSys, smartVehicles, drones, sensors;
    trafficSys.Create(6);    // Traffic lights, cameras, sensors
    smartVehicles.Create(8); // Cars, buses, emergency vehicles
    drones.Create(4);        // Surveillance, delivery, emergency drones
    sensors.Create(7);       // Environmental, parking, noise, etc.

    // HOSPITAL DISTRICT (16 devices) ===
    NodeContainer hospitalDevices, medicalIoT, emergencyResponse;
    hospitalDevices.Create(8);   // Doctors, nurses, admin, AI systems
    medicalIoT.Create(6);        // Monitors, ventilators, imaging
    emergencyResponse.Create(2); // Emergency dispatch, ambulance coord

    // POWER GRID DISTRICT (12 devices)
    NodeContainer powerDevices, smartGrid, powerPlants;
    powerDevices.Create(4); // Control center, operators
    smartGrid.Create(6);    // Smart meters, transformers, substations
    powerPlants.Create(2);  // Power generation facilities

    // FINANCIAL DISTRICT (10 devices)
    NodeContainer financeDevices, bankingServers, atmNetwork;
    financeDevices.Create(4); // Bank operations, traders
    bankingServers.Create(4); // Core banking, transaction processing
    atmNetwork.Create(2);     // ATM network controllers

    // NETWORK LINKS
    // Ultra-high speed core backbone
    PointToPointHelper coreBackbone;
    coreBackbone.SetDeviceAttribute("DataRate", StringValue("200Gbps"));
    coreBackbone.SetChannelAttribute("Delay", StringValue("0.1ms"));

    // 6G Ultra connections (critical infrastructure)
    PointToPointHelper link6GUltra;
    link6GUltra.SetDeviceAttribute("DataRate", StringValue("100Gbps"));
    link6GUltra.SetChannelAttribute("Delay", StringValue("0.2ms"));

    // 6G High-speed connections
    PointToPointHelper link6G;
    link6G.SetDeviceAttribute("DataRate", StringValue("50Gbps"));
    link6G.SetChannelAttribute("Delay", StringValue("0.5ms"));

    // 5G connections
    PointToPointHelper link5G;
    link5G.SetDeviceAttribute("DataRate", StringValue("20Gbps"));
    link5G.SetChannelAttribute("Delay", StringValue("2ms"));

    // Fiber connections
    PointToPointHelper fiberLink;
    fiberLink.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
    fiberLink.SetChannelAttribute("Delay", StringValue("5ms"));

    // Home fiber
    PointToPointHelper homeFiber;
    homeFiber.SetDeviceAttribute("DataRate", StringValue("5Gbps"));
    homeFiber.SetChannelAttribute("Delay", StringValue("8ms"));

    // CORE NETWORK CONNECTIONS
    // Core mesh network
    NetDeviceContainer core01 = coreBackbone.Install(coreNodes.Get(0), coreNodes.Get(1));
    NetDeviceContainer core02 = coreBackbone.Install(coreNodes.Get(0), coreNodes.Get(2));
    NetDeviceContainer core12 = coreBackbone.Install(coreNodes.Get(1), coreNodes.Get(2));

    // CDN and DNS connections
    NetDeviceContainer cdn0 = fiberLink.Install(cdnNodes.Get(0), coreNodes.Get(0));
    NetDeviceContainer cdn1 = fiberLink.Install(cdnNodes.Get(1), coreNodes.Get(1));
    NetDeviceContainer dns0 = fiberLink.Install(dnsNodes.Get(0), coreNodes.Get(0));
    NetDeviceContainer dns1 = fiberLink.Install(dnsNodes.Get(1), coreNodes.Get(1));

    // DISTRICT CONNECTIONS TO CORE

    NetDeviceContainer homeToCore = homeFiber.Install(homeGW.Get(0), coreNodes.Get(0));
    NetDeviceContainer officeToCore = fiberLink.Install(officeGW.Get(0), coreNodes.Get(0));
    NetDeviceContainer uniToCore = link5G.Install(universityGW.Get(0), coreNodes.Get(1));
    NetDeviceContainer iotToCore = link6G.Install(iotGW.Get(0), coreNodes.Get(0));
    NetDeviceContainer hospitalToCore = link6GUltra.Install(hospitalGW.Get(0), coreNodes.Get(1));
    NetDeviceContainer powerToCore = link6GUltra.Install(powerGW.Get(0), coreNodes.Get(2));
    NetDeviceContainer financeToCore = link6GUltra.Install(financeGW.Get(0), coreNodes.Get(2));

    // LOCAL AREA NETWORKS
    CsmaHelper csmaLAN;
    csmaLAN.SetChannelAttribute("DataRate", StringValue("1Gbps"));
    csmaLAN.SetChannelAttribute("Delay", StringValue("2ms"));

    // High-speed LAN for critical infrastructure
    CsmaHelper csmaHighSpeed;
    csmaHighSpeed.SetChannelAttribute("DataRate", StringValue("10Gbps"));
    csmaHighSpeed.SetChannelAttribute("Delay", StringValue("0.5ms"));

    // Home district LAN
    NodeContainer homeLAN;
    homeLAN.Add(homeGW);
    homeLAN.Add(homeDevices);
    NetDeviceContainer homeLANDevices = csmaLAN.Install(homeLAN);

    // Office district LAN
    NodeContainer officeLAN;
    officeLAN.Add(officeGW);
    officeLAN.Add(officeDevices);
    NetDeviceContainer officeLANDevices = csmaLAN.Install(officeLAN);

    // University district LANs
    NodeContainer uniLAN;
    uniLAN.Add(universityGW);
    uniLAN.Add(uniDevices);
    NetDeviceContainer uniLANDevices = csmaLAN.Install(uniLAN);

    NodeContainer researchLAN;
    researchLAN.Add(universityGW);
    researchLAN.Add(researchCluster);
    NetDeviceContainer researchLANDevices = csmaHighSpeed.Install(researchLAN);

    // Hospital district LANs
    NodeContainer hospitalLAN;
    hospitalLAN.Add(hospitalGW);
    hospitalLAN.Add(hospitalDevices);
    hospitalLAN.Add(emergencyResponse);
    NetDeviceContainer hospitalLANDevices = csmaHighSpeed.Install(hospitalLAN);

    NodeContainer medicalIoTLAN;
    medicalIoTLAN.Add(hospitalGW);
    medicalIoTLAN.Add(medicalIoT);
    NetDeviceContainer medicalIoTDevices = csmaHighSpeed.Install(medicalIoTLAN);

    // Power grid LANs
    NodeContainer powerLAN;
    powerLAN.Add(powerGW);
    powerLAN.Add(powerDevices);
    powerLAN.Add(powerPlants);
    NetDeviceContainer powerLANDevices = csmaHighSpeed.Install(powerLAN);

    NodeContainer smartGridLAN;
    smartGridLAN.Add(powerGW);
    smartGridLAN.Add(smartGrid);
    NetDeviceContainer smartGridDevices = csmaLAN.Install(smartGridLAN);

    // Financial district LAN
    NodeContainer financeLAN;
    financeLAN.Add(financeGW);
    financeLAN.Add(financeDevices);
    financeLAN.Add(bankingServers);
    financeLAN.Add(atmNetwork);
    NetDeviceContainer financeLANDevices = csmaHighSpeed.Install(financeLAN);

    // IOT WIRELESS NETWORKS
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());
    wifiPhy.Set("TxPowerStart", DoubleValue(30.0));
    wifiPhy.Set("TxPowerEnd", DoubleValue(30.0));

    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("SmartCity6G");

    // IoT gateway as access point
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer iotAP = wifi.Install(wifiPhy, wifiMac, iotGW);

    // IoT devices as stations
    wifiMac.SetType("ns3::StaWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "ActiveProbing",
                    BooleanValue(false));
    NetDeviceContainer trafficDevices = wifi.Install(wifiPhy, wifiMac, trafficSys);
    NetDeviceContainer vehicleDevices = wifi.Install(wifiPhy, wifiMac, smartVehicles);
    NetDeviceContainer droneDevices = wifi.Install(wifiPhy, wifiMac, drones);
    NetDeviceContainer sensorDevices = wifi.Install(wifiPhy, wifiMac, sensors);

    // MOBILITY AND POSITIONING

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Core infrastructure - center triangle
    positionAlloc->Add(Vector(400.0, 400.0, 0)); // Primary core
    positionAlloc->Add(Vector(350.0, 350.0, 0)); // Secondary core
    positionAlloc->Add(Vector(450.0, 350.0, 0)); // Emergency core

    // CDN and DNS
    positionAlloc->Add(Vector(300.0, 450.0, 0)); // CDN 1
    positionAlloc->Add(Vector(500.0, 450.0, 0)); // CDN 2
    positionAlloc->Add(Vector(300.0, 350.0, 0)); // DNS 1
    positionAlloc->Add(Vector(500.0, 350.0, 0)); // DNS 2

    // District gateways - heptagon pattern
    positionAlloc->Add(Vector(150.0, 600.0, 0)); // Home gateway
    positionAlloc->Add(Vector(650.0, 600.0, 0)); // Office gateway
    positionAlloc->Add(Vector(750.0, 400.0, 0)); // University gateway (5G)
    positionAlloc->Add(Vector(650.0, 150.0, 0)); // IoT gateway (6G)
    positionAlloc->Add(Vector(400.0, 100.0, 0)); // Hospital gateway (6G Ultra)
    positionAlloc->Add(Vector(150.0, 150.0, 0)); // Power gateway (6G Ultra)
    positionAlloc->Add(Vector(50.0, 400.0, 0));  // Finance gateway (6G Ultra)

    // Home district devices
    for (int i = 0; i < 8; i++)
    {
        positionAlloc->Add(Vector(50.0 + i * 25.0, 650.0, 0));
    }

    // Office district devices
    for (int i = 0; i < 12; i++)
    {
        positionAlloc->Add(Vector(600.0 + (i % 4) * 25.0, 650.0 + (i / 4) * 25.0, 0));
    }

    // University district devices
    for (int i = 0; i < 10; i++)
    {
        positionAlloc->Add(Vector(700.0 + (i % 5) * 25.0, 450.0 + (i / 5) * 25.0, 0));
    }

    // Research cluster
    for (int i = 0; i < 5; i++)
    {
        positionAlloc->Add(Vector(700.0 + i * 25.0, 350.0, 0));
    }

    // Hospital devices
    for (int i = 0; i < 8; i++)
    {
        positionAlloc->Add(Vector(350.0 + (i % 4) * 25.0, 50.0 + (i / 4) * 25.0, 0));
    }

    // Medical IoT devices
    for (int i = 0; i < 6; i++)
    {
        positionAlloc->Add(Vector(450.0 + (i % 3) * 25.0, 50.0 + (i / 3) * 25.0, 0));
    }

    // Emergency response
    for (int i = 0; i < 2; i++)
    {
        positionAlloc->Add(Vector(350.0 + i * 25.0, 25.0, 0));
    }

    // Power grid devices
    for (int i = 0; i < 4; i++)
    {
        positionAlloc->Add(Vector(100.0 + i * 25.0, 200.0, 0));
    }

    // Smart grid devices
    for (int i = 0; i < 6; i++)
    {
        positionAlloc->Add(Vector(50.0 + (i % 3) * 25.0, 250.0 + (i / 3) * 25.0, 0));
    }

    // Power plants
    for (int i = 0; i < 2; i++)
    {
        positionAlloc->Add(Vector(75.0 + i * 50.0, 100.0, 0));
    }

    // Financial district devices
    for (int i = 0; i < 4; i++)
    {
        positionAlloc->Add(Vector(25.0, 350.0 + i * 25.0, 0));
    }

    // Banking servers
    for (int i = 0; i < 4; i++)
    {
        positionAlloc->Add(Vector(75.0, 350.0 + i * 25.0, 0));
    }

    // ATM network
    for (int i = 0; i < 2; i++)
    {
        positionAlloc->Add(Vector(50.0, 300.0 + i * 25.0, 0));
    }

    // IoT devices - distributed pattern
    for (int i = 0; i < 6; i++)
    {
        positionAlloc->Add(
            Vector(550.0 + (i % 3) * 50.0, 100.0 + (i / 3) * 50.0, 0)); // Traffic systems
    }

    for (int i = 0; i < 8; i++)
    {
        positionAlloc->Add(
            Vector(500.0 + (i % 4) * 25.0, 200.0 + (i / 4) * 25.0, 0)); // Smart vehicles
    }

    for (int i = 0; i < 4; i++)
    {
        positionAlloc->Add(Vector(600.0 + (i % 2) * 50.0, 50.0 + (i / 2) * 25.0, 0)); // Drones
    }

    for (int i = 0; i < 7; i++)
    {
        positionAlloc->Add(Vector(750.0 + (i % 3) * 25.0, 100.0 + (i / 3) * 25.0, 0)); // Sensors
    }

    mobility.SetPositionAllocator(positionAlloc);
    mobility.InstallAll();

    // INTERNET PROTOCOL STACK

    InternetStackHelper stack;
    stack.InstallAll();

    // IP ADDRESS ASSIGNMENT
    Ipv4AddressHelper address;

    // Core network
    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer coreInterfaces01 = address.Assign(core01);
    address.SetBase("10.0.1.0", "255.255.255.0");
    Ipv4InterfaceContainer coreInterfaces02 = address.Assign(core02);
    address.SetBase("10.0.2.0", "255.255.255.0");
    Ipv4InterfaceContainer coreInterfaces12 = address.Assign(core12);

    // CDN and DNS
    address.SetBase("10.1.0.0", "255.255.255.0");
    Ipv4InterfaceContainer cdnInterfaces0 = address.Assign(cdn0);
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer cdnInterfaces1 = address.Assign(cdn1);
    address.SetBase("10.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer dnsInterfaces0 = address.Assign(dns0);
    address.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer dnsInterfaces1 = address.Assign(dns1);

    // Home district
    address.SetBase("172.16.1.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer homeToCoreDev = address.Assign(homeToCore);
    address.SetBase("192.168.1.0", "255.255.255.0"); // Home LAN
    Ipv4InterfaceContainer homeLANInt = address.Assign(homeLANDevices);

    // Office district
    address.SetBase("172.16.2.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer officeToCoreDev = address.Assign(officeToCore);
    address.SetBase("192.168.2.0", "255.255.255.0"); // Office LAN
    Ipv4InterfaceContainer officeLANInt = address.Assign(officeLANDevices);

    // University district
    address.SetBase("172.16.3.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer uniToCoreDev = address.Assign(uniToCore);
    address.SetBase("192.168.3.0", "255.255.255.0"); // Uni LAN
    Ipv4InterfaceContainer uniLANInt = address.Assign(uniLANDevices);

    // Research cluster LAN
    address.SetBase("192.168.4.0", "255.255.255.0");
    Ipv4InterfaceContainer researchLANInt = address.Assign(researchLANDevices);

    // IoT district
    address.SetBase("172.16.5.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer iotToCoreDev = address.Assign(iotToCore);

    // ALL WiFi devices share ONE subnet
    address.SetBase("192.168.50.0", "255.255.255.0"); // Single subnet for all WiFi
    Ipv4InterfaceContainer iotAPInt = address.Assign(iotAP);
    Ipv4InterfaceContainer trafficInt = address.Assign(trafficDevices);
    Ipv4InterfaceContainer vehicleInt = address.Assign(vehicleDevices);
    Ipv4InterfaceContainer droneInt = address.Assign(droneDevices);
    Ipv4InterfaceContainer sensorInt = address.Assign(sensorDevices);

    // Hospital district
    address.SetBase("172.16.10.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer hospitalToCoreDev = address.Assign(hospitalToCore);
    address.SetBase("192.168.10.0", "255.255.255.0"); // Hospital LAN
    Ipv4InterfaceContainer hospitalLANInt = address.Assign(hospitalLANDevices);
    address.SetBase("192.168.11.0", "255.255.255.0"); // Medical IoT LAN
    Ipv4InterfaceContainer medicalIoTInt = address.Assign(medicalIoTDevices);

    // Power district
    address.SetBase("172.16.20.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer powerToCoreDev = address.Assign(powerToCore);
    address.SetBase("192.168.20.0", "255.255.255.0"); // Power LAN
    Ipv4InterfaceContainer powerLANInt = address.Assign(powerLANDevices);
    address.SetBase("192.168.21.0", "255.255.255.0"); // Smart Grid LAN
    Ipv4InterfaceContainer smartGridInt = address.Assign(smartGridDevices);

    // Finance district
    address.SetBase("172.16.30.0", "255.255.255.252"); // P2P link to core
    Ipv4InterfaceContainer financeToCoreDev = address.Assign(financeToCore);
    address.SetBase("192.168.30.0", "255.255.255.0"); // Finance LAN
    Ipv4InterfaceContainer financeLANInt = address.Assign(financeLANDevices);

    // Ipv4AddressHelper address;

    // // Core network
    // address.SetBase("10.0.0.0", "255.255.255.0");
    // Ipv4InterfaceContainer coreInterfaces01 = address.Assign(core01);
    // address.SetBase("10.0.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer coreInterfaces02 = address.Assign(core02);
    // address.SetBase("10.0.2.0", "255.255.255.0");
    // Ipv4InterfaceContainer coreInterfaces12 = address.Assign(core12);

    // // CDN and DNS
    // address.SetBase("10.1.0.0", "255.255.255.0");
    // Ipv4InterfaceContainer cdnInterfaces0 = address.Assign(cdn0);
    // address.SetBase("10.1.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer cdnInterfaces1 = address.Assign(cdn1);
    // address.SetBase("10.2.0.0", "255.255.255.0");
    // Ipv4InterfaceContainer dnsInterfaces0 = address.Assign(dns0);
    // address.SetBase("10.2.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer dnsInterfaces1 = address.Assign(dns1);

    // District networks
    // address.SetBase("192.168.1.0", "255.255.255.0");
    // Ipv4InterfaceContainer homeToCoreDev = address.Assign(homeToCore);
    // Ipv4InterfaceContainer homeLANInt = address.Assign(homeLANDevices);

    // address.SetBase("192.168.2.0", "255.255.255.0");
    // Ipv4InterfaceContainer officeToCoreDev = address.Assign(officeToCore);
    // Ipv4InterfaceContainer officeLANInt = address.Assign(officeLANDevices);

    // address.SetBase("192.168.3.0", "255.255.255.0");
    // Ipv4InterfaceContainer uniToCoreDev = address.Assign(uniToCore);
    // Ipv4InterfaceContainer uniLANInt = address.Assign(uniLANDevices);

    // address.SetBase("192.168.4.0", "255.255.255.0");
    // Ipv4InterfaceContainer researchLANInt = address.Assign(researchLANDevices);

    // address.SetBase("192.168.5.0", "255.255.255.0");
    // Ipv4InterfaceContainer iotToCoreDev = address.Assign(iotToCore);
    // Ipv4InterfaceContainer iotAPInt = address.Assign(iotAP);
    // Ipv4InterfaceContainer trafficInt = address.Assign(trafficDevices);

    // address.SetBase("192.168.6.0", "255.255.255.0");
    // Ipv4InterfaceContainer vehicleInt = address.Assign(vehicleDevices);

    // address.SetBase("192.168.7.0", "255.255.255.0");
    // Ipv4InterfaceContainer droneInt = address.Assign(droneDevices);

    // address.SetBase("192.168.8.0", "255.255.255.0");
    // Ipv4InterfaceContainer sensorInt = address.Assign(sensorDevices);

    // address.SetBase("192.168.10.0", "255.255.255.0");
    // Ipv4InterfaceContainer hospitalToCoreDev = address.Assign(hospitalToCore);
    // Ipv4InterfaceContainer hospitalLANInt = address.Assign(hospitalLANDevices);

    // address.SetBase("192.168.11.0", "255.255.255.0");
    // Ipv4InterfaceContainer medicalIoTInt = address.Assign(medicalIoTDevices);

    // address.SetBase("192.168.20.0", "255.255.255.0");
    // Ipv4InterfaceContainer powerToCoreDev = address.Assign(powerToCore);
    // Ipv4InterfaceContainer powerLANInt = address.Assign(powerLANDevices);

    // address.SetBase("192.168.21.0", "255.255.255.0");
    // Ipv4InterfaceContainer smartGridInt = address.Assign(smartGridDevices);

    // address.SetBase("192.168.30.0", "255.255.255.0");
    // Ipv4InterfaceContainer financeToCoreDev = address.Assign(financeToCore);
    // Ipv4InterfaceContainer financeLANInt = address.Assign(financeLANDevices);

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    //  TRAFFIC PATTERNS
    // 1. Multi-district emergency coordination
    UdpServerHelper emergencyServer(8100);
    ApplicationContainer emergencyServerApps = emergencyServer.Install(emergencyResponse.Get(0));
    emergencyServerApps.Start(Seconds(10.0));
    emergencyServerApps.Stop(Seconds(simTime));

    // Hospital emergency alert to traffic control and power grid
    UdpClientHelper emergencyAlert1(trafficInt.GetAddress(0), 8101);
    UdpClientHelper emergencyAlert2(powerLANInt.GetAddress(1), 8102);
    emergencyAlert1.SetAttribute("MaxPackets", UintegerValue(50));
    emergencyAlert1.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    emergencyAlert1.SetAttribute("PacketSize", UintegerValue(512));

    ApplicationContainer emergencyApps1 = emergencyAlert1.Install(emergencyResponse.Get(0));
    ApplicationContainer emergencyApps2 = emergencyAlert2.Install(emergencyResponse.Get(0));
    emergencyApps1.Start(Seconds(60.0));
    emergencyApps2.Start(Seconds(60.0));
    emergencyApps1.Stop(Seconds(90.0));
    emergencyApps2.Stop(Seconds(90.0));

    // 2. International medical consultation
    UdpServerHelper consultServer(8200);
    ApplicationContainer consultServerApps =
        consultServer.Install(cdnNodes.Get(0)); // Simulate international
    consultServerApps.Start(Seconds(20.0));
    consultServerApps.Stop(Seconds(simTime));

    UdpClientHelper consultation(cdnInterfaces0.GetAddress(0), 8200);
    consultation.SetAttribute("MaxPackets", UintegerValue(2000));
    consultation.SetAttribute("Interval", TimeValue(MilliSeconds(50)));
    consultation.SetAttribute("PacketSize", UintegerValue(1400)); // High-res medical data
    ApplicationContainer consultApps = consultation.Install(hospitalDevices.Get(0));
    consultApps.Start(Seconds(30.0));
    consultApps.Stop(Seconds(120.0));

    // 3. Smart grid real-time control
    for (uint32_t i = 0; i < 6; i++)
    {
        UdpServerHelper gridServer(8300 + i);
        ApplicationContainer gridServerApps = gridServer.Install(powerDevices.Get(0));
        gridServerApps.Start(Seconds(5.0));
        gridServerApps.Stop(Seconds(simTime));

        UdpClientHelper gridClient(powerLANInt.GetAddress(1), 8300 + i);
        gridClient.SetAttribute("MaxPackets", UintegerValue(1000));
        gridClient.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
        gridClient.SetAttribute("PacketSize", UintegerValue(200));
        ApplicationContainer gridApps = gridClient.Install(smartGrid.Get(i));
        gridApps.Start(Seconds(10.0 + i));
        gridApps.Stop(Seconds(simTime));
    }

    // 4. High-frequency trading
    BulkSendHelper tradingBulk("ns3::TcpSocketFactory",
                               InetSocketAddress(financeLANInt.GetAddress(2), 8400));
    tradingBulk.SetAttribute("MaxBytes", UintegerValue(50000000));

    PacketSinkHelper tradingSink("ns3::TcpSocketFactory",
                                 InetSocketAddress(Ipv4Address::GetAny(), 8400));
    ApplicationContainer tradingSinkApps = tradingSink.Install(bankingServers.Get(0));
    tradingSinkApps.Start(Seconds(1.0));
    tradingSinkApps.Stop(Seconds(simTime));

    ApplicationContainer tradingApps = tradingBulk.Install(financeDevices.Get(0));
    tradingApps.Start(Seconds(25.0));
    tradingApps.Stop(Seconds(simTime - 20.0));

    // 5. Multi-drone coordination
    // for (uint32_t i = 0; i < 4; i++)

    //     UdpServerHelper droneServer(8500 + i);
    // // ApplicationContainer droneServerApps = droneServer.Install(hospitalLANInt.GetAddress(1));
    // ApplicationContainer droneServerApps = droneServer.Install(hospitalDevices.Get(0));
    // droneServerApps.Start(Seconds(30.0));
    // droneServerApps.Stop(Seconds(simTime));

    // UdpClientHelper droneClient(hospitalLANInt.GetAddress(1), 8500 + i);
    // droneClient.SetAttribute("MaxPackets", UintegerValue(800));
    // droneClient.SetAttribute("Interval", TimeValue(MilliSeconds(125)));
    // droneClient.SetAttribute("PacketSize", UintegerValue(1200));
    // ApplicationContainer droneApps = droneClient.Install(drones.Get(i));
    // droneApps.Start(Seconds(40.0 + i * 5.0));
    // droneApps.Stop(Seconds(simTime - 10.0));

    for (uint32_t i = 0; i < drones.GetN(); i++)
    {
        uint16_t dronePort = 8500 + i;

        // Server side (at hospital or control center)
        UdpServerHelper droneServer(dronePort);
        ApplicationContainer serverApps = droneServer.Install(hospitalDevices.Get(0));
        serverApps.Start(Seconds(30.0));
        serverApps.Stop(Seconds(simTime));

        // Client side (on the drone)
        UdpClientHelper droneClient(hospitalLANInt.GetAddress(1), dronePort);
        droneClient.SetAttribute("MaxPackets", UintegerValue(800));
        droneClient.SetAttribute("Interval", TimeValue(MilliSeconds(125)));
        droneClient.SetAttribute("PacketSize", UintegerValue(1200));

        ApplicationContainer clientApps = droneClient.Install(drones.Get(i));
        clientApps.Start(Seconds(40.0 + i * 5.0));
        clientApps.Stop(Seconds(simTime - 10.0));
    }

    if (generateAttacks)
    {
        std::cout << "Generating attack scenarios for " << scenario << std::endl;

        // PORT SCAN ATTACK (Ports: 80, 443, 22, 21)
        if (scenario == "portscan" || scenario == "mixed")
        {
            std::cout << "Generating port scanning attack..." << std::endl;

            std::vector<Ipv4Address> scanTargets = {
                hospitalLANInt.GetAddress(1), // Hospital
                powerLANInt.GetAddress(1),    // Power grid
                financeLANInt.GetAddress(1)   // Financial
            };

            std::vector<uint16_t> scanPorts = {21, 22, 80, 443};

            for (uint32_t target = 0; target < scanTargets.size(); target++)
            {
                for (uint32_t port = 0; port < scanPorts.size(); port++)
                {
                    UdpClientHelper portScan(scanTargets[target], scanPorts[port]);
                    portScan.SetAttribute("MaxPackets", UintegerValue(3));
                    portScan.SetAttribute("Interval", TimeValue(MilliSeconds(200)));
                    portScan.SetAttribute("PacketSize", UintegerValue(64));

                    uint32_t scannerIndex = (target * 4 + port) % sensors.GetN();
                    ApplicationContainer portScanApps = portScan.Install(sensors.Get(scannerIndex));
                    portScanApps.Start(Seconds(50.0 + target * 5.0 + port * 0.5));
                    portScanApps.Stop(Seconds(52.0 + target * 5.0 + port * 0.5));
                }
            }
        }

        // DDOS ATTACK (Ports: 9200, 9201, 9202)
        if (scenario == "ddos" || scenario == "mixed")
        {
            std::cout << "Generating DDoS attack..." << std::endl;

            std::vector<std::pair<Ipv4Address, uint16_t>> ddosTargets = {
                {hospitalLANInt.GetAddress(1), 9200},
                {powerLANInt.GetAddress(1), 9201},
                {financeLANInt.GetAddress(1), 9202}};

            for (uint32_t target = 0; target < ddosTargets.size(); target++)
            {
                // Create servers for DDoS targets
                UdpServerHelper ddosServer(ddosTargets[target].second);
                ApplicationContainer ddosServerApps;
                if (target == 0)
                    ddosServerApps = ddosServer.Install(hospitalDevices.Get(0));
                else if (target == 1)
                    ddosServerApps = ddosServer.Install(powerDevices.Get(0));
                else
                    ddosServerApps = ddosServer.Install(financeDevices.Get(0));

                ddosServerApps.Start(Seconds(70.0));
                ddosServerApps.Stop(Seconds(simTime));

                // Multiple attackers per target
                for (uint32_t attacker = 0; attacker < 3; attacker++)
                {
                    UdpClientHelper ddosAttack(ddosTargets[target].first,
                                               ddosTargets[target].second);
                    ddosAttack.SetAttribute("MaxPackets", UintegerValue(500));
                    ddosAttack.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
                    ddosAttack.SetAttribute("PacketSize", UintegerValue(128));

                    ApplicationContainer ddosApps =
                        ddosAttack.Install(smartVehicles.Get(attacker + target));
                    ddosApps.Start(Seconds(80.0 + target * 5.0));
                    ddosApps.Stop(Seconds(100.0 + target * 5.0));
                }
            }
        }

        // APT ATTACK (Ports: 8700, 8701, 8702)
        if (scenario == "apt" || scenario == "mixed")
        {
            std::cout << "Generating APT attack..." << std::endl;

            // Stage 1: Initial compromise
            UdpServerHelper aptServer1(8700);
            ApplicationContainer aptServer1Apps = aptServer1.Install(coreNodes.Get(0));
            aptServer1Apps.Start(Seconds(60.0));
            aptServer1Apps.Stop(Seconds(simTime));

            UdpClientHelper aptStage1(coreInterfaces01.GetAddress(0), 8700);
            aptStage1.SetAttribute("MaxPackets", UintegerValue(50));
            aptStage1.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
            aptStage1.SetAttribute("PacketSize", UintegerValue(256));
            ApplicationContainer aptApps1 = aptStage1.Install(sensors.Get(0));
            aptApps1.Start(Seconds(70.0));
            aptApps1.Stop(Seconds(85.0));

            // Stage 2: Lateral movement
            UdpServerHelper aptServer2(8701);
            ApplicationContainer aptServer2Apps = aptServer2.Install(officeDevices.Get(5));
            aptServer2Apps.Start(Seconds(90.0));
            aptServer2Apps.Stop(Seconds(simTime));

            UdpClientHelper aptStage2(officeLANInt.GetAddress(6), 8701);
            aptStage2.SetAttribute("MaxPackets", UintegerValue(100));
            aptStage2.SetAttribute("Interval", TimeValue(MilliSeconds(50)));
            aptStage2.SetAttribute("PacketSize", UintegerValue(512));
            ApplicationContainer aptApps2 = aptStage2.Install(sensors.Get(1));
            aptApps2.Start(Seconds(95.0));
            aptApps2.Stop(Seconds(115.0));

            // Stage 3: Data exfiltration
            UdpServerHelper aptServer3(8702);
            ApplicationContainer aptServer3Apps = aptServer3.Install(coreNodes.Get(1));
            aptServer3Apps.Start(Seconds(120.0));
            aptServer3Apps.Stop(Seconds(simTime));

            UdpClientHelper aptStage3(coreInterfaces01.GetAddress(1), 8702);
            aptStage3.SetAttribute("MaxPackets", UintegerValue(200));
            aptStage3.SetAttribute("Interval", TimeValue(MilliSeconds(25)));
            aptStage3.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer aptApps3 = aptStage3.Install(officeDevices.Get(4));
            aptApps3.Start(Seconds(125.0));
            aptApps3.Stop(Seconds(150.0));
        }

        // RANSOMWARE ATTACK (Ports: 8800-8803)
        if (scenario == "ransomware" || scenario == "mixed")
        {
            std::cout << "Generating ransomware attack..." << std::endl;

            for (uint32_t i = 0; i < 4; i++)
            {
                uint16_t ransomPort = 8800 + i;

                UdpServerHelper ransomServer(ransomPort);
                ApplicationContainer ransomServerApps =
                    ransomServer.Install(bankingServers.Get(i % 4));
                ransomServerApps.Start(Seconds(100.0));
                ransomServerApps.Stop(Seconds(simTime));

                UdpClientHelper ransomAttack(financeLANInt.GetAddress(2 + i), ransomPort);
                ransomAttack.SetAttribute("MaxPackets", UintegerValue(300));
                ransomAttack.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
                ransomAttack.SetAttribute("PacketSize", UintegerValue(512));
                ApplicationContainer ransomApps = ransomAttack.Install(officeDevices.Get(i + 6));
                ransomApps.Start(Seconds(110.0 + i * 2.0));
                ransomApps.Stop(Seconds(130.0 + i * 2.0));
            }
        }

        // BOTNET ATTACK (Port: 8950)
        if (scenario == "botnet" || scenario == "mixed")
        {
            std::cout << "Generating botnet attack..." << std::endl;

            UdpServerHelper botnetServer(8950);
            ApplicationContainer botnetServerApps = botnetServer.Install(coreNodes.Get(0));
            botnetServerApps.Start(Seconds(60.0));
            botnetServerApps.Stop(Seconds(simTime));

            std::vector<NodeContainer*> botContainers = {&sensors, &smartVehicles, &trafficSys};

            for (uint32_t container = 0; container < botContainers.size(); container++)
            {
                for (uint32_t device = 0; device < std::min(3u, botContainers[container]->GetN());
                     device++)
                {
                    UdpClientHelper botClient(coreInterfaces01.GetAddress(0), 8950);
                    botClient.SetAttribute("MaxPackets", UintegerValue(100));
                    botClient.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
                    botClient.SetAttribute("PacketSize", UintegerValue(256));

                    ApplicationContainer botApps =
                        botClient.Install(botContainers[container]->Get(device));
                    botApps.Start(Seconds(70.0 + container * 10.0 + device * 2.0));
                    botApps.Stop(Seconds(90.0 + container * 10.0 + device * 2.0));
                }
            }
        }

        // MEDICAL DEVICE HIJACKING (Ports: 9000-9005)
        if (scenario == "medical" || scenario == "mixed")
        {
            std::cout << "Generating medical device hijacking..." << std::endl;

            for (uint32_t i = 0; i < 6; i++)
            {
                uint16_t medPort = 9000 + i;

                UdpServerHelper medServer(medPort);
                ApplicationContainer medServerApps = medServer.Install(hospitalDevices.Get(0));
                medServerApps.Start(Seconds(90.0));
                medServerApps.Stop(Seconds(simTime));

                UdpClientHelper medAttack(hospitalLANInt.GetAddress(1), medPort);
                medAttack.SetAttribute("MaxPackets", UintegerValue(200));
                medAttack.SetAttribute("Interval", TimeValue(MilliSeconds(50)));
                medAttack.SetAttribute("PacketSize", UintegerValue(512));
                ApplicationContainer medApps = medAttack.Install(medicalIoT.Get(i));
                medApps.Start(Seconds(100.0 + i * 3.0));
                medApps.Stop(Seconds(120.0 + i * 3.0));
            }
        }

        // GRID ATTACK (Ports: 8900-8905)
        if (scenario == "grid" || scenario == "mixed")
        {
            std::cout << "Generating power grid attack..." << std::endl;

            for (uint32_t i = 0; i < 6; i++)
            {
                uint16_t gridPort = 8900 + i;

                UdpServerHelper gridServer(gridPort);
                ApplicationContainer gridServerApps = gridServer.Install(powerDevices.Get(0));
                gridServerApps.Start(Seconds(80.0));
                gridServerApps.Stop(Seconds(simTime));

                UdpClientHelper gridAttack(smartGridInt.GetAddress(i + 1), gridPort);
                gridAttack.SetAttribute("MaxPackets", UintegerValue(400));
                gridAttack.SetAttribute("Interval", TimeValue(MilliSeconds(25)));
                gridAttack.SetAttribute("PacketSize", UintegerValue(256));
                ApplicationContainer gridApps = gridAttack.Install(smartVehicles.Get(i % 8));
                gridApps.Start(Seconds(90.0 + i * 2.0));
                gridApps.Stop(Seconds(110.0 + i * 2.0));
            }
        }

        // SUPPLY CHAIN ATTACK (Port: 8750)
        if (scenario == "supply" || scenario == "mixed")
        {
            std::cout << "Generating supply chain attack..." << std::endl;

            UdpServerHelper supplyServer(8750);
            ApplicationContainer supplyServerApps = supplyServer.Install(hospitalDevices.Get(2));
            supplyServerApps.Start(Seconds(70.0));
            supplyServerApps.Stop(Seconds(simTime));

            UdpClientHelper supplyAttack(hospitalLANInt.GetAddress(3), 8750);
            supplyAttack.SetAttribute("MaxPackets", UintegerValue(150));
            supplyAttack.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
            supplyAttack.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer supplyApps = supplyAttack.Install(researchCluster.Get(2));
            supplyApps.Start(Seconds(80.0));
            supplyApps.Stop(Seconds(110.0));
        }

        // FINANCIAL DATA EXFILTRATION (Port: 9100)
        if (scenario == "finance" || scenario == "mixed")
        {
            std::cout << "Generating financial data exfiltration..." << std::endl;

            UdpServerHelper financeServer(9100);
            ApplicationContainer financeServerApps = financeServer.Install(coreNodes.Get(0));
            financeServerApps.Start(Seconds(100.0));
            financeServerApps.Stop(Seconds(simTime));

            UdpClientHelper financeExfil(coreInterfaces01.GetAddress(0), 9100);
            financeExfil.SetAttribute("MaxPackets", UintegerValue(500));
            financeExfil.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
            financeExfil.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer financeApps = financeExfil.Install(bankingServers.Get(1));
            financeApps.Start(Seconds(120.0));
            financeApps.Stop(Seconds(160.0));
        }

        // RECONNAISSANCE ATTACK (Ports: 9500-9510)
        if (scenario == "recon" || scenario == "mixed")
        {
            std::cout << "Generating network reconnaissance..." << std::endl;

            for (uint32_t subnet = 1; subnet <= 30; subnet += 10)
            {
                for (uint32_t host = 1; host <= 5; host++)
                {
                    std::ostringstream targetIP;
                    targetIP << "192.168." << subnet << "." << host;

                    UdpClientHelper reconScan(Ipv4Address(targetIP.str().c_str()), 9500 + subnet);
                    reconScan.SetAttribute("MaxPackets", UintegerValue(2));
                    reconScan.SetAttribute("Interval", TimeValue(MilliSeconds(500)));
                    reconScan.SetAttribute("PacketSize", UintegerValue(32));

                    uint32_t reconIndex = ((subnet - 1) / 10 * 5 + host - 1) % smartVehicles.GetN();
                    ApplicationContainer reconApps =
                        reconScan.Install(smartVehicles.Get(reconIndex));
                    reconApps.Start(Seconds(45.0 + subnet + host * 0.2));
                    reconApps.Stop(Seconds(47.0 + subnet + host * 0.2));
                }
            }
        }

        // 6G MAN-IN-THE-MIDDLE ATTACK (Port: 9600)
        if (scenario == "mitm6g" || scenario == "mixed")
        {
            std::cout << "Generating 6G Man-in-the-Middle attack..." << std::endl;

            UdpServerHelper rogueBaseStation(9600);
            ApplicationContainer rogueApps = rogueBaseStation.Install(smartVehicles.Get(0));
            rogueApps.Start(Seconds(30.0));
            rogueApps.Stop(Seconds(simTime));

            for (uint32_t i = 0; i < 6; i++)
            {
                UdpClientHelper interceptor(vehicleInt.GetAddress(0), 9600);
                interceptor.SetAttribute("MaxPackets", UintegerValue(200));
                interceptor.SetAttribute("Interval", TimeValue(MilliSeconds(250)));
                interceptor.SetAttribute("PacketSize", UintegerValue(1024));

                ApplicationContainer interceptApps =
                    interceptor.Install(drones.Get(i % drones.GetN()));
                interceptApps.Start(Seconds(35.0 + i * 2.0));
                interceptApps.Stop(Seconds(80.0));
            }
        }

        // SIDE-CHANNEL ATTACK (Ports: 9700-9703)
        if (scenario == "sidechannel" || scenario == "mixed")
        {
            std::cout << "Generating 6G Ultra side-channel attack..." << std::endl;

            for (uint32_t i = 0; i < 4; i++)
            {
                uint16_t sidePort = 9700 + i;

                UdpServerHelper sideServer(sidePort);
                ApplicationContainer sideServerApps =
                    sideServer.Install(hospitalDevices.Get(i % hospitalDevices.GetN()));
                sideServerApps.Start(Seconds(70.0));
                sideServerApps.Stop(Seconds(simTime));

                UdpClientHelper sideChannel(hospitalLANInt.GetAddress(1 + i), sidePort);
                sideChannel.SetAttribute("MaxPackets", UintegerValue(1000));
                sideChannel.SetAttribute("Interval", TimeValue(MilliSeconds(5)));
                sideChannel.SetAttribute("PacketSize", UintegerValue(32));

                ApplicationContainer sideApps =
                    sideChannel.Install(sensors.Get(i % sensors.GetN()));
                sideApps.Start(Seconds(75.0));
                sideApps.Stop(Seconds(100.0));
            }
        }

        // NETWORK SLICING ATTACK (Ports: 9800-9802)
        if (scenario == "slicing" || scenario == "mixed")
        {
            std::cout << "Generating 6G network slicing attack..." << std::endl;

            std::vector<std::pair<Ipv4Address, uint16_t>> sliceTargets = {
                {hospitalLANInt.GetAddress(1), 9800}, // Medical slice
                {powerLANInt.GetAddress(1), 9801},    // Power slice
                {financeLANInt.GetAddress(1), 9802}   // Financial slice
            };

            for (uint32_t slice = 0; slice < sliceTargets.size(); slice++)
            {
                UdpServerHelper sliceServer(sliceTargets[slice].second);
                ApplicationContainer sliceServerApps;
                if (slice == 0)
                    sliceServerApps = sliceServer.Install(hospitalDevices.Get(0));
                else if (slice == 1)
                    sliceServerApps = sliceServer.Install(powerDevices.Get(0));
                else
                    sliceServerApps = sliceServer.Install(financeDevices.Get(0));
                sliceServerApps.Start(Seconds(85.0));
                sliceServerApps.Stop(Seconds(simTime));

                for (uint32_t attacker = 0; attacker < 3; attacker++)
                {
                    UdpClientHelper sliceAttack(sliceTargets[slice].first,
                                                sliceTargets[slice].second);
                    sliceAttack.SetAttribute("MaxPackets", UintegerValue(500));
                    sliceAttack.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
                    sliceAttack.SetAttribute("PacketSize", UintegerValue(256));

                    ApplicationContainer sliceApps = sliceAttack.Install(
                        trafficSys.Get((slice * 3 + attacker) % trafficSys.GetN()));
                    sliceApps.Start(Seconds(90.0 + slice * 10.0));
                    sliceApps.Stop(Seconds(120.0));
                }
            }
        }

        // ML MODEL POISONING ATTACK (Ports: 9900-9907)
        if (scenario == "mlpoison" || scenario == "mixed")
        {
            std::cout << "Generating AI/ML model poisoning attack..." << std::endl;

            for (uint32_t i = 0; i < 8; i++)
            {
                uint16_t mlPort = 9900 + i;

                UdpServerHelper mlServer(mlPort);
                ApplicationContainer mlServerApps =
                    mlServer.Install(hospitalDevices.Get(i % hospitalDevices.GetN()));
                mlServerApps.Start(Seconds(95.0));
                mlServerApps.Stop(Seconds(simTime));

                UdpClientHelper mlPoison(hospitalLANInt.GetAddress(2), mlPort);
                mlPoison.SetAttribute("MaxPackets", UintegerValue(300));
                mlPoison.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
                mlPoison.SetAttribute("PacketSize", UintegerValue(2048));

                ApplicationContainer poisonApps =
                    mlPoison.Install(researchCluster.Get(i % researchCluster.GetN()));
                poisonApps.Start(Seconds(100.0 + i * 2.0));
                poisonApps.Stop(Seconds(140.0));
            }
        }

        // HOME NETWORK ATTACKS (Ports: 6000-6010)
        if (scenario == "home" || scenario == "mixed")
        {
            std::cout << "Generating home network attack..." << std::endl;

            // Simple DDoS on home network
            UdpServerHelper homeServer(6000);
            ApplicationContainer homeServerApps = homeServer.Install(homeDevices.Get(0));
            homeServerApps.Start(Seconds(60.0));
            homeServerApps.Stop(Seconds(simTime));

            for (uint32_t attacker = 0; attacker < 3; attacker++)
            {
                UdpClientHelper homeAttack(homeLANInt.GetAddress(1), 6000);
                homeAttack.SetAttribute("MaxPackets", UintegerValue(500));
                homeAttack.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
                homeAttack.SetAttribute("PacketSize", UintegerValue(256));

                ApplicationContainer homeAttackApps =
                    homeAttack.Install(smartVehicles.Get(attacker));
                homeAttackApps.Start(Seconds(70.0 + attacker * 2.0));
                homeAttackApps.Stop(Seconds(100.0));
            }

            // Home data exfiltration
            UdpServerHelper homeExfilServer(6001);
            ApplicationContainer homeExfilServerApps = homeExfilServer.Install(coreNodes.Get(0));
            homeExfilServerApps.Start(Seconds(80.0));
            homeExfilServerApps.Stop(Seconds(simTime));

            UdpClientHelper homeExfil(coreInterfaces01.GetAddress(0), 6001);
            homeExfil.SetAttribute("MaxPackets", UintegerValue(300));
            homeExfil.SetAttribute("Interval", TimeValue(MilliSeconds(50)));
            homeExfil.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer homeExfilApps = homeExfil.Install(homeDevices.Get(2));
            homeExfilApps.Start(Seconds(90.0));
            homeExfilApps.Stop(Seconds(120.0));
        }

        // SIMPLE UNIVERSITY NETWORK ATTACK (Ports: 5000-5010)
        if (scenario == "university" || scenario == "mixed")
        {
            std::cout << "Generating university network attack..." << std::endl;

            // University server compromise
            UdpServerHelper uniServer(5000);
            ApplicationContainer uniServerApps = uniServer.Install(uniDevices.Get(0));
            uniServerApps.Start(Seconds(50.0));
            uniServerApps.Stop(Seconds(simTime));

            for (uint32_t attacker = 0; attacker < 2; attacker++)
            {
                UdpClientHelper uniAttack(uniLANInt.GetAddress(1), 5000);
                uniAttack.SetAttribute("MaxPackets", UintegerValue(400));
                uniAttack.SetAttribute("Interval", TimeValue(MilliSeconds(30)));
                uniAttack.SetAttribute("PacketSize", UintegerValue(512));

                ApplicationContainer uniAttackApps = uniAttack.Install(sensors.Get(attacker));
                uniAttackApps.Start(Seconds(60.0 + attacker * 5.0));
                uniAttackApps.Stop(Seconds(90.0));
            }

            // Research data theft
            UdpServerHelper researchServer(5001);
            ApplicationContainer researchServerApps = researchServer.Install(coreNodes.Get(1));
            researchServerApps.Start(Seconds(70.0));
            researchServerApps.Stop(Seconds(simTime));

            UdpClientHelper researchTheft(coreInterfaces01.GetAddress(1), 5001);
            researchTheft.SetAttribute("MaxPackets", UintegerValue(600));
            researchTheft.SetAttribute("Interval", TimeValue(MilliSeconds(25)));
            researchTheft.SetAttribute("PacketSize", UintegerValue(1024));
            ApplicationContainer researchApps = researchTheft.Install(researchCluster.Get(2));
            researchApps.Start(Seconds(80.0));
            researchApps.Stop(Seconds(110.0));
        }

        // EDGE COMPUTING COMPROMISE (Ports: 10000-10005)
        if (scenario == "edge" || scenario == "mixed")
        {
            std::cout << "Generating edge computing compromise attack..." << std::endl;

            for (uint32_t i = 0; i < trafficSys.GetN(); i++)
            {
                uint16_t edgePort = 10000 + i;

                UdpServerHelper edgeServer(edgePort);
                ApplicationContainer edgeServerApps = edgeServer.Install(trafficSys.Get(i));
                edgeServerApps.Start(Seconds(50.0));
                edgeServerApps.Stop(Seconds(simTime));

                UdpClientHelper edgeAttack(trafficInt.GetAddress(i), edgePort);
                edgeAttack.SetAttribute("MaxPackets", UintegerValue(400));
                edgeAttack.SetAttribute("Interval", TimeValue(MilliSeconds(50)));
                edgeAttack.SetAttribute("PacketSize", UintegerValue(512));

                ApplicationContainer edgeApps =
                    edgeAttack.Install(smartVehicles.Get(i % smartVehicles.GetN()));
                edgeApps.Start(Seconds(55.0 + i * 3.0));
                edgeApps.Stop(Seconds(90.0));
            }
        }

        // QUANTUM CRYPTOGRAPHY ATTACK (Port: 10100)
        if (scenario == "quantum" || scenario == "mixed")
        {
            std::cout << "Generating quantum cryptography attack simulation..." << std::endl;

            UdpServerHelper quantumServer(10100);
            ApplicationContainer quantumServerApps = quantumServer.Install(bankingServers.Get(0));
            quantumServerApps.Start(Seconds(110.0));
            quantumServerApps.Stop(Seconds(simTime));

            UdpClientHelper quantumAttack(financeLANInt.GetAddress(2), 10100);
            quantumAttack.SetAttribute("MaxPackets", UintegerValue(1000));
            quantumAttack.SetAttribute("Interval", TimeValue(MilliSeconds(20)));
            quantumAttack.SetAttribute("PacketSize", UintegerValue(1024));

            ApplicationContainer quantumApps = quantumAttack.Install(officeDevices.Get(8));
            quantumApps.Start(Seconds(120.0));
            quantumApps.Stop(Seconds(simTime - 10.0));
        }

        // GPS SPOOFING ATTACK (Ports: 10200-10207)
        if (scenario == "gpsspoof" || scenario == "mixed")
        {
            std::cout << "Generating GPS spoofing attack..." << std::endl;

            for (uint32_t i = 0; i < smartVehicles.GetN(); i++)
            {
                uint16_t gpsPort = 10200 + i;

                UdpServerHelper gpsServer(gpsPort);
                ApplicationContainer gpsServerApps = gpsServer.Install(smartVehicles.Get(i));
                gpsServerApps.Start(Seconds(40.0));
                gpsServerApps.Stop(Seconds(simTime));

                UdpClientHelper gpsSpoof(vehicleInt.GetAddress(i), gpsPort);
                gpsSpoof.SetAttribute("MaxPackets", UintegerValue(100));
                gpsSpoof.SetAttribute("Interval", TimeValue(Seconds(1.0)));
                gpsSpoof.SetAttribute("PacketSize", UintegerValue(128));

                ApplicationContainer spoofApps = gpsSpoof.Install(drones.Get(i % drones.GetN()));
                spoofApps.Start(Seconds(45.0));
                spoofApps.Stop(Seconds(80.0));
            }
        }

        // BLOCKCHAIN NETWORK ATTACK (Port: 10300)
        if (scenario == "blockchain" || scenario == "mixed")
        {
            std::cout << "Generating blockchain network attack..." << std::endl;

            UdpServerHelper blockchainServer(10300);
            ApplicationContainer blockchainServerApps =
                blockchainServer.Install(financeDevices.Get(3));
            blockchainServerApps.Start(Seconds(125.0));
            blockchainServerApps.Stop(Seconds(simTime));

            for (uint32_t i = 0; i < 6; i++)
            {
                UdpClientHelper blockchainAttack(financeLANInt.GetAddress(4), 10300);
                blockchainAttack.SetAttribute("MaxPackets", UintegerValue(2000));
                blockchainAttack.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
                blockchainAttack.SetAttribute("PacketSize", UintegerValue(256));

                ApplicationContainer chainApps = blockchainAttack.Install(officeDevices.Get(i + 6));
                chainApps.Start(Seconds(130.0));
                chainApps.Stop(Seconds(170.0));
            }
        }
    }

    // PACKET CAPTURE AND MONITORING
    std::string pcapPrefix = scenario + "-enhanced-smartcity";

    // Enable packet capture for all major links
    // homeFiber.EnablePcapAll(pcapPrefix + "-home");
    // fiberLink.EnablePcapAll(pcapPrefix + "-office");
    // link5G.EnablePcapAll(pcapPrefix + "-5g");
    // link6G.EnablePcapAll(pcapPrefix + "-6g");
    // link6GUltra.EnablePcapAll(pcapPrefix + "-6g-ultra");
    // coreBackbone.EnablePcapAll(pcapPrefix + "-core");
    // csmaLAN.EnablePcapAll(pcapPrefix + "-lan");
    // csmaHighSpeed.EnablePcapAll(pcapPrefix + "-highspeed");
    // wifiPhy.EnablePcap(pcapPrefix + "-wifi", iotAP);
    // wifiPhy.EnablePcap(pcapPrefix + "-wifi", trafficDevices);
    // wifiPhy.EnablePcap(pcapPrefix + "-wifi", vehicleDevices);
    // wifiPhy.EnablePcap(pcapPrefix + "-wifi", droneDevices);
    // wifiPhy.EnablePcap(pcapPrefix + "-wifi", sensorDevices);

    // FLOW MONITORING
    FlowMonitorHelper flowMonitor;
    Ptr<FlowMonitor> monitor = flowMonitor.InstallAll();

    // NETWORK ANIMATION

    AnimationInterface anim(scenario + "-enhanced-smartcity.xml");

    // Enhanced node descriptions
    anim.UpdateNodeDescription(coreNodes.Get(0), "PRIMARY-CORE");
    anim.UpdateNodeDescription(coreNodes.Get(1), "SECONDARY-CORE");
    anim.UpdateNodeDescription(coreNodes.Get(2), "EMERGENCY-CORE");
    anim.UpdateNodeDescription(cdnNodes.Get(0), "CDN-1");
    anim.UpdateNodeDescription(cdnNodes.Get(1), "CDN-2");
    anim.UpdateNodeDescription(dnsNodes.Get(0), "DNS-1");
    anim.UpdateNodeDescription(dnsNodes.Get(1), "DNS-2");

    // District gateways
    anim.UpdateNodeDescription(homeGW.Get(0), "HOME-GATEWAY");
    anim.UpdateNodeDescription(officeGW.Get(0), "OFFICE-GATEWAY");
    anim.UpdateNodeDescription(universityGW.Get(0), "UNIVERSITY-5G-GATEWAY");
    anim.UpdateNodeDescription(iotGW.Get(0), "IOT-6G-GATEWAY");
    anim.UpdateNodeDescription(hospitalGW.Get(0), "HOSPITAL-6G-ULTRA-GATEWAY");
    anim.UpdateNodeDescription(powerGW.Get(0), "POWER-GRID-6G-ULTRA-GATEWAY");
    anim.UpdateNodeDescription(financeGW.Get(0), "FINANCE-6G-ULTRA-GATEWAY");

    // Enhanced color coding
    // Core infrastructure - Red tones
    anim.UpdateNodeColor(coreNodes.Get(0), 255, 0, 0); // Primary core
    anim.UpdateNodeColor(coreNodes.Get(1), 200, 0, 0); // Secondary core
    anim.UpdateNodeColor(coreNodes.Get(2), 150, 0, 0); // Emergency core
    anim.UpdateNodeColor(cdnNodes.Get(0), 255, 100, 100);
    anim.UpdateNodeColor(cdnNodes.Get(1), 255, 100, 100);
    anim.UpdateNodeColor(dnsNodes.Get(0), 200, 50, 50);
    anim.UpdateNodeColor(dnsNodes.Get(1), 200, 50, 50);

    // Home district - Blue tones
    anim.UpdateNodeColor(homeGW.Get(0), 0, 0, 255);
    for (uint32_t i = 0; i < homeDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(homeDevices.Get(i), 100, 150, 255);
    }

    // Office district - Green tones
    anim.UpdateNodeColor(officeGW.Get(0), 0, 255, 0);
    for (uint32_t i = 0; i < officeDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(officeDevices.Get(i), 150, 255, 150);
    }

    // University district - Purple tones (5G)
    anim.UpdateNodeColor(universityGW.Get(0), 128, 0, 128);
    for (uint32_t i = 0; i < uniDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(uniDevices.Get(i), 200, 100, 255);
    }
    for (uint32_t i = 0; i < researchCluster.GetN(); i++)
    {
        anim.UpdateNodeColor(researchCluster.Get(i), 150, 50, 200);
    }

    // IoT district - Orange tones (6G)
    anim.UpdateNodeColor(iotGW.Get(0), 255, 140, 0);
    for (uint32_t i = 0; i < trafficSys.GetN(); i++)
    {
        anim.UpdateNodeColor(trafficSys.Get(i), 255, 200, 100);
    }
    for (uint32_t i = 0; i < smartVehicles.GetN(); i++)
    {
        anim.UpdateNodeColor(smartVehicles.Get(i), 255, 180, 80);
    }
    for (uint32_t i = 0; i < drones.GetN(); i++)
    {
        anim.UpdateNodeColor(drones.Get(i), 255, 160, 60);
    }
    for (uint32_t i = 0; i < sensors.GetN(); i++)
    {
        anim.UpdateNodeColor(sensors.Get(i), 255, 220, 120);
    }

    // Hospital district - Pink tones (6G Ultra)
    anim.UpdateNodeColor(hospitalGW.Get(0), 255, 20, 147);
    for (uint32_t i = 0; i < hospitalDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(hospitalDevices.Get(i), 255, 182, 193);
    }
    for (uint32_t i = 0; i < medicalIoT.GetN(); i++)
    {
        anim.UpdateNodeColor(medicalIoT.Get(i), 255, 105, 180);
    }
    for (uint32_t i = 0; i < emergencyResponse.GetN(); i++)
    {
        anim.UpdateNodeColor(emergencyResponse.Get(i), 255, 0, 100);
    }

    // Power grid - Yellow tones (6G Ultra)
    anim.UpdateNodeColor(powerGW.Get(0), 255, 255, 0);
    for (uint32_t i = 0; i < powerDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(powerDevices.Get(i), 255, 255, 150);
    }
    for (uint32_t i = 0; i < smartGrid.GetN(); i++)
    {
        anim.UpdateNodeColor(smartGrid.Get(i), 255, 255, 100);
    }
    for (uint32_t i = 0; i < powerPlants.GetN(); i++)
    {
        anim.UpdateNodeColor(powerPlants.Get(i), 200, 200, 0);
    }

    // Financial district - Cyan tones (6G Ultra)
    anim.UpdateNodeColor(financeGW.Get(0), 0, 255, 255);
    for (uint32_t i = 0; i < financeDevices.GetN(); i++)
    {
        anim.UpdateNodeColor(financeDevices.Get(i), 150, 255, 255);
    }
    for (uint32_t i = 0; i < bankingServers.GetN(); i++)
    {
        anim.UpdateNodeColor(bankingServers.Get(i), 100, 200, 200);
    }
    for (uint32_t i = 0; i < atmNetwork.GetN(); i++)
    {
        anim.UpdateNodeColor(atmNetwork.Get(i), 0, 200, 200);
    }

    // Enhanced node sizes
    anim.UpdateNodeSize(coreNodes.Get(0), 25.0, 25.0); // Primary core
    anim.UpdateNodeSize(coreNodes.Get(1), 20.0, 20.0); // Secondary core
    anim.UpdateNodeSize(coreNodes.Get(2), 15.0, 15.0); // Emergency core

    // Large gateways
    for (uint32_t i = 0; i < 7; i++)
    {
        if (i == 0)
            anim.UpdateNodeSize(homeGW.Get(0), 15.0, 15.0);
        else if (i == 1)
            anim.UpdateNodeSize(officeGW.Get(0), 15.0, 15.0);
        else if (i == 2)
            anim.UpdateNodeSize(universityGW.Get(0), 15.0, 15.0);
        else if (i == 3)
            anim.UpdateNodeSize(iotGW.Get(0), 15.0, 15.0);
        else if (i == 4)
            anim.UpdateNodeSize(hospitalGW.Get(0), 15.0, 15.0);
        else if (i == 5)
            anim.UpdateNodeSize(powerGW.Get(0), 15.0, 15.0);
        else if (i == 6)
            anim.UpdateNodeSize(financeGW.Get(0), 15.0, 15.0);
    }

    std::cout << "Starting enhanced simulation with "
              << (homeDevices.GetN() + officeDevices.GetN() + uniDevices.GetN() +
                  researchCluster.GetN() + hospitalDevices.GetN() + medicalIoT.GetN() +
                  emergencyResponse.GetN() + powerDevices.GetN() + smartGrid.GetN() +
                  powerPlants.GetN() + financeDevices.GetN() + bankingServers.GetN() +
                  atmNetwork.GetN() + trafficSys.GetN() + smartVehicles.GetN() + drones.GetN() +
                  sensors.GetN())
              << " end devices..." << std::endl;

    // RUN SIMULATION
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // // POST-SIMULATION ANALYSIS
    // monitor->CheckForLostPackets();
    // Ptr<Ipv4FlowClassifier> classifier =
    //     DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    // std::map<FlowId, FlowMonitor::FlowStats> flowStats = monitor->GetFlowStats();

    // POST-SIMULATION ANALYSIS WITH AI
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowMonitor.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> flowStats = monitor->GetFlowStats();

    std::cout << "\n=== AI FIREWALL ANALYSIS ===" << std::endl;

    uint32_t totalFlows = 0;
    uint32_t blockedFlows = 0;

    for (auto& flow : flowStats)
    {
        Ipv4FlowClassifier::FiveTuple flowTuple = classifier->FindFlow(flow.first);
        FlowMonitor::FlowStats stats = flow.second;

        double duration = (stats.timeLastRxPacket - stats.timeFirstTxPacket).GetSeconds();
        double throughput = duration > 0 ? (stats.rxBytes * 8.0) / duration : 0.0;
        double packetLoss = stats.txPackets > 0
                                ? (double)(stats.txPackets - stats.rxPackets) / stats.txPackets
                                : 0.0;
        double avgDelay =
            stats.rxPackets > 0 ? (stats.delaySum.GetSeconds() / stats.rxPackets) : 0.0;
        double jitter =
            stats.rxPackets > 1 ? (stats.jitterSum.GetSeconds() / (stats.rxPackets - 1)) : 0.0;

        std::string district = GetDistrictFromIP(flowTuple.sourceAddress);

        // Query ML firewall
        bool shouldBlock = QueryMLFirewall(flow.first,
                                           flowTuple.sourceAddress,
                                           flowTuple.destinationAddress,
                                           flowTuple.destinationPort,
                                           stats.txPackets,
                                           stats.rxPackets,
                                           stats.txBytes,
                                           stats.rxBytes,
                                           duration,
                                           throughput,
                                           packetLoss,
                                           avgDelay,
                                           jitter,
                                           district);

        totalFlows++;
        if (shouldBlock)
        {
            blockedFlows++;

            std::cout << "[THREAT BLOCKED] Flow " << flow.first << std::endl;
            std::cout << "  " << flowTuple.sourceAddress << " -> " << flowTuple.destinationAddress
                      << ":" << flowTuple.destinationPort << std::endl;
            std::cout << "  District: " << district << std::endl;
            std::cout << "  Duration: " << duration << "s | Loss: " << packetLoss * 100 << "%"
                      << std::endl;
        }
    }

    std::cout << "\nAI Firewall Summary:" << std::endl;
    std::cout << "Total flows: " << totalFlows << std::endl;
    std::cout << "Blocked threats: " << blockedFlows << std::endl;
    std::cout << "Protection rate: " << (double)blockedFlows / totalFlows * 100 << "%" << std::endl;

    // Enhanced flow data export for ML training
    std::string csvFilename = scenario + "-enhanced-flows.csv";
    std::ofstream csvFile(csvFilename);
    csvFile << "FlowId,SrcIP,DstIP,SrcPort,DstPort,Protocol,TxPackets,RxPackets,TxBytes,RxBytes,"
               "Duration,Throughput,PacketLoss,Delay,Jitter,District,TrafficType,Label\n";

    uint32_t normalFlows = 0, attackFlows = 0;

    for (auto& flow : flowStats)
    {
        Ipv4FlowClassifier::FiveTuple flowTuple = classifier->FindFlow(flow.first);
        FlowMonitor::FlowStats stats = flow.second;

        double duration = (stats.timeLastRxPacket - stats.timeFirstTxPacket).GetSeconds();
        double throughput = duration > 0 ? (stats.rxBytes * 8.0) / duration : 0.0;
        double packetLoss = stats.txPackets > 0
                                ? (double)(stats.txPackets - stats.rxPackets) / stats.txPackets
                                : 0.0;
        double avgDelay =
            stats.rxPackets > 0 ? (stats.delaySum.GetSeconds() / stats.rxPackets) : 0.0;
        double jitter =
            stats.rxPackets > 1 ? (stats.jitterSum.GetSeconds() / (stats.rxPackets - 1)) : 0.0;

        // Enhanced labeling system
        int label = 0; // Normal
        std::string district = "Unknown";
        std::string trafficType = "Regular";

        // Determine district based on IP
        // std::string srcIP = flowTuple.sourceAddress.Print();
        std::ostringstream oss;
        flowTuple.sourceAddress.Print(oss);
        std::string srcIP = oss.str();
        if (srcIP.find("192.168.1.") == 0)
            district = "Home";
        else if (srcIP.find("192.168.2.") == 0)
            district = "Office";
        else if (srcIP.find("192.168.3.") == 0)
            district = "University";
        else if (srcIP.find("192.168.4.") == 0)
            district = "University-Research";
        else if (srcIP.find("192.168.5.") == 0 || srcIP.find("192.168.6.") == 0 ||
                 srcIP.find("192.168.7.") == 0 || srcIP.find("192.168.8.") == 0)
            district = "IoT";
        else if (srcIP.find("192.168.10.") == 0 || srcIP.find("192.168.11.") == 0)
            district = "Hospital";
        else if (srcIP.find("192.168.20.") == 0 || srcIP.find("192.168.21.") == 0)
            district = "PowerGrid";
        else if (srcIP.find("192.168.30.") == 0)
            district = "Finance";
        else if (srcIP.find("10.") == 0)
            district = "Core";

        // Determine traffic type and label
        uint16_t dstPort = flowTuple.destinationPort;

        if (generateAttacks)
        {
            if ((dstPort >= 5000 && dstPort <= 5010) || // University
                (dstPort >= 6000 && dstPort <= 6010) || // Home
                (dstPort >= 8700 && dstPort <= 8702) || // APT
                (dstPort >= 8750 && dstPort <= 8799) || // Supply chain
                (dstPort >= 8800 && dstPort <= 8849) || // Ransomware
                (dstPort >= 8900 && dstPort <= 8949) || // Grid attack
                (dstPort >= 8950 && dstPort <= 8999) || // Botnet
                (dstPort >= 9000 && dstPort <= 9099) || // Medical hijacking
                (dstPort >= 9100 && dstPort <= 9199) || // Finance exfiltration
                (dstPort >= 9200 && dstPort <= 9299) || // DDoS
                (dstPort == 21 || dstPort == 22 || dstPort == 80 || dstPort == 443) || // Port scan
                (dstPort >= 9500 && dstPort <= 9510) ||   // Reconnaissance
                (dstPort == 9600) ||                      // MiTM 6G
                (dstPort >= 9700 && dstPort <= 9703) ||   // Side channel
                (dstPort >= 9800 && dstPort <= 9802) ||   // Network slicing
                (dstPort >= 9900 && dstPort <= 9907) ||   // ML poisoning
                (dstPort >= 10000 && dstPort <= 10005) || // Edge computing
                (dstPort == 10100) ||                     // Quantum attack
                (dstPort >= 10200 && dstPort <= 10207) || // GPS spoofing
                (dstPort == 10300))
            { // DDoS
                label = 1;
                attackFlows++;
                trafficType = "Attack";

                if (dstPort >= 5000 && dstPort <= 5010)
                    trafficType = "UniversityAttack";
                if (dstPort >= 6000 && dstPort <= 6010)
                    trafficType = "HomeAttack";
                else if (dstPort >= 8700 && dstPort <= 8702)
                    trafficType = "APT";
                else if (dstPort >= 8750 && dstPort <= 8799)
                    trafficType = "SupplyChain";
                else if (dstPort >= 8800 && dstPort <= 8849)
                    trafficType = "Ransomware";
                else if (dstPort >= 8900 && dstPort <= 8949)
                    trafficType = "GridAttack";
                else if (dstPort >= 8950 && dstPort <= 8999)
                    trafficType = "Botnet";
                else if (dstPort >= 9000 && dstPort <= 9099)
                    trafficType = "MedicalHijack";
                else if (dstPort >= 9100 && dstPort <= 9199)
                    trafficType = "DataExfiltration";
                else if (dstPort >= 9200 && dstPort <= 9299)
                    trafficType = "DDoS";
                else if (dstPort == 21 || dstPort == 22 || dstPort == 80 || dstPort == 443)
                    trafficType = "PortScan";
                else if (dstPort >= 9500 && dstPort <= 9510)
                    trafficType = "Reconnaissance";
                else if (dstPort == 9600)
                    trafficType = "MiTM6G";
                else if (dstPort >= 9700 && dstPort <= 9703)
                    trafficType = "SideChannel";
                else if (dstPort >= 9800 && dstPort <= 9802)
                    trafficType = "NetworkSlicing";
                else if (dstPort >= 9900 && dstPort <= 9907)
                    trafficType = "MLPoisoning";
                else if (dstPort >= 10000 && dstPort <= 10005)
                    trafficType = "EdgeCompromise";
                else if (dstPort == 10100)
                    trafficType = "QuantumAttack";
                else if (dstPort >= 10200 && dstPort <= 10207)
                    trafficType = "GPSSpoofing";
                else if (dstPort == 10300)
                    trafficType = "BlockchainAttack";
            }
            else
            {
                normalFlows++;
                // Classify normal traffic types
                if (dstPort >= 8100 && dstPort <= 8199)
                    trafficType = "Emergency";
                else if (dstPort >= 8200 && dstPort <= 8299)
                    trafficType = "Medical";
                else if (dstPort >= 8300 && dstPort <= 8399)
                    trafficType = "PowerGrid";
                else if (dstPort >= 8400 && dstPort <= 8499)
                    trafficType = "Financial";
                else if (dstPort >= 8500 && dstPort <= 8599)
                    trafficType = "Surveillance";
                else
                    trafficType = "Regular";
            }
        }
        else
        {
            normalFlows++;
            if (dstPort >= 8100 && dstPort <= 8199)
                trafficType = "Emergency";
            else if (dstPort >= 8200 && dstPort <= 8299)
                trafficType = "Medical";
            else if (dstPort >= 8300 && dstPort <= 8399)
                trafficType = "PowerGrid";
            else if (dstPort >= 8400 && dstPort <= 8499)
                trafficType = "Financial";
            else if (dstPort >= 8500 && dstPort <= 8599)
                trafficType = "Surveillance";
            else
                trafficType = "Regular";
        }

        csvFile << flow.first << "," << flowTuple.sourceAddress << ","
                << flowTuple.destinationAddress << "," << flowTuple.sourcePort << ","
                << flowTuple.destinationPort << "," << (int)flowTuple.protocol << ","
                << stats.txPackets << "," << stats.rxPackets << "," << stats.txBytes << ","
                << stats.rxBytes << "," << duration << "," << throughput << "," << packetLoss << ","
                << avgDelay << "," << jitter << "," << district << "," << trafficType << ","
                << label << "\n";
    }
    csvFile.close();

    // Export XML flow data
    monitor->SerializeToXmlFile(scenario + "-enhanced-flows.xml", true, true);

    // Enhanced summary
    std::cout << "\nEnhanced Smart City Simulation completed!" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Network Statistics:" << std::endl;
    std::cout << "  Districts: 7 (Home, Office, University, IoT, Hospital, Power, Finance)"
              << std::endl;
    std::cout << "  Total devices: "
              << (homeDevices.GetN() + officeDevices.GetN() + uniDevices.GetN() +
                  researchCluster.GetN() + hospitalDevices.GetN() + medicalIoT.GetN() +
                  emergencyResponse.GetN() + powerDevices.GetN() + smartGrid.GetN() +
                  powerPlants.GetN() + financeDevices.GetN() + bankingServers.GetN() +
                  atmNetwork.GetN() + trafficSys.GetN() + smartVehicles.GetN() + drones.GetN() +
                  sensors.GetN())
              << std::endl;
    std::cout << "  Core nodes: 3 + 4 (CDN/DNS)" << std::endl;
    std::cout << "  Gateways: 7" << std::endl;

    std::cout << "\nGenerated Files:" << std::endl;
    std::cout << "  PCAP files: " << pcapPrefix << "-*.pcap" << std::endl;
    std::cout << "  Flow CSV: " << csvFilename << std::endl;
    std::cout << "  Flow XML: " << scenario << "-enhanced-flows.xml" << std::endl;
    std::cout << "  NetAnim: " << scenario << "-enhanced-smartcity.xml" << std::endl;

    std::cout << "\nFlow Analysis:" << std::endl;
    std::cout << "  Total flows: " << flowStats.size() << std::endl;
    std::cout << "  Normal flows: " << normalFlows << std::endl;
    std::cout << "  Attack flows: " << attackFlows << std::endl;

    if (generateAttacks)
    {
        std::cout << "  Attack scenarios executed: " << scenario << std::endl;
    }

    Simulator::Destroy();
    return 0;
}
