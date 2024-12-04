#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3::energy;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("IoTNetworkSimulation");

int main(int argc, char *argv[])
{
    // Enable logging
    LogComponentEnable("IoTNetworkSimulation", LOG_LEVEL_INFO);

    // Simulation parameters
    double simulationTime = 20.0; // seconds
    std::string phyMode = "HtMcs7";
    std::string dataRate = "100Mbps";
    std::string delay = "1ms";

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue("simulationTime", "Duration of the simulation in seconds", simulationTime);
    cmd.Parse(argc, argv);

    // Step 1: Create Nodes
    NodeContainer sensorNode;
    sensorNode.Create(1);

    NodeContainer microcontrollerNode;
    microcontrollerNode.Create(1);

    NodeContainer cloudNode;
    cloudNode.Create(1);

    // Step 2: Set up Point-to-Point Link between Sensor and Microcontroller
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
    pointToPoint.SetChannelAttribute("Delay", StringValue(delay));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(sensorNode.Get(0), microcontrollerNode.Get(0));

    // Step 3: Set up Wi-Fi Network between Microcontroller and Cloud Platform
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    // Comment out SetStandard if causing issues
    // wifi.SetStandard(WIFI_PHY_STANDARD_80211n_2_4GHZ);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode));

    WifiMacHelper wifiMac;

    // Configure AP (Microcontroller Node)
    Ssid ssid = Ssid("IoT-WiFi");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(wifiPhy, wifiMac, microcontrollerNode.Get(0));

    // Configure STA (Cloud Node)
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(wifiPhy, wifiMac, cloudNode.Get(0));

    // Step 4: Mobility Models
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(sensorNode);
    mobility.Install(microcontrollerNode);
    mobility.Install(cloudNode);

    // Step 5: Install Internet Stack and Assign IP Addresses
    InternetStackHelper stack;
    stack.InstallAll();

    Ipv4AddressHelper address;

    // Assign IP addresses to point-to-point devices
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer sensorMicrocontrollerInterfaces;
    sensorMicrocontrollerInterfaces = address.Assign(p2pDevices);

    // Assign IP addresses to Wi-Fi devices
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer microcontrollerCloudInterfaces;
    microcontrollerCloudInterfaces.Add(address.Assign(apDevice));
    microcontrollerCloudInterfaces.Add(address.Assign(staDevices));

    // Step 6: Configure Applications

    // Sensor Node sends data to the Cloud Node
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(microcontrollerCloudInterfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(cloudNode.Get(0));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simulationTime));

    // Create a TCP connection from Sensor Node to Cloud Node through Microcontroller
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute("DataRate", StringValue("50Mbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer sourceApps = onOffHelper.Install(sensorNode.Get(0));
    sourceApps.Start(Seconds(2.0));
    sourceApps.Stop(Seconds(simulationTime));

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Step 7: Install Energy Model on Wi-Fi Devices

    // Install Energy Source on Microcontroller Node
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer energySources = energySourceHelper.Install(microcontrollerNode.Get(0));

    // Install Energy Model on AP Device
    WifiRadioEnergyModelHelper wifiEnergyModelHelper;
    wifiEnergyModelHelper.Set("TxCurrentA", DoubleValue(0.0174)); // Example values
    wifiEnergyModelHelper.Set("RxCurrentA", DoubleValue(0.0197));
    wifiEnergyModelHelper.Set("IdleCurrentA", DoubleValue(0.273));
    wifiEnergyModelHelper.Set("SleepCurrentA", DoubleValue(0.033));
    wifiEnergyModelHelper.Install(apDevice.Get(0), energySources.Get(0));

    // Repeat for Cloud Node if desired
    // Install Energy Source on Cloud Node
    // EnergySourceContainer cloudEnergySources = energySourceHelper.Install(cloudNode.Get(0));

    // Install Energy Model on STA Device
    // wifiEnergyModelHelper.Install(staDevices.Get(0), cloudEnergySources.Get(0));

    // Step 8: Install Flow Monitor to gather statistics
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Step 9: Enable NetAnim visualization
    AnimationInterface anim("iot-network-animation.xml");

    // Set node descriptions for NetAnim
    anim.UpdateNodeDescription(sensorNode.Get(0), "Sensor Node");
    anim.UpdateNodeDescription(microcontrollerNode.Get(0), "Microcontroller Node");
    anim.UpdateNodeDescription(cloudNode.Get(0), "Cloud Node");

    // Set node positions for NetAnim
    anim.SetConstantPosition(sensorNode.Get(0), 10.0, 20.0);
    anim.SetConstantPosition(microcontrollerNode.Get(0), 20.0, 20.0);
    anim.SetConstantPosition(cloudNode.Get(0), 30.0, 20.0);

    // Step 10: Run Simulation
    NS_LOG_INFO("Starting simulation...");
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // Flow monitor statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (const auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        NS_LOG_INFO("Flow ID: " << flow.first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
        NS_LOG_INFO("Tx Packets = " << flow.second.txPackets);
        NS_LOG_INFO("Rx Packets = " << flow.second.rxPackets);
        NS_LOG_INFO("Throughput: "
                    << flow.second.rxBytes * 8.0 /
                           (flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds()) /
                           1024 / 1024
                    << " Mbps");
    }

     // Energy consumption
    double remainingEnergy = energySources.Get(0)->GetRemainingEnergy();
    NS_LOG_INFO("Remaining energy in the microcontroller node: " << remainingEnergy << " Joules");

    Simulator::Destroy();
    NS_LOG_INFO("Simulation finished.");

    return 0;
}
