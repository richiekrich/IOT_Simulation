/* SPDX-License-Identifier: GPL-2.0-or-later */

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

using namespace ns3;
using namespace ns3::energy;

// Define the logging component
NS_LOG_COMPONENT_DEFINE("EnhancedIoTNetworkSimulation");

int main(int argc, char *argv[])
{
    // ------------------------------
    // 1. Setup and Configuration
    // ------------------------------

    // Enable logging for specific components to aid debugging
    LogComponentEnable("EnhancedIoTNetworkSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO); // Enables TCP retransmission trace
    LogComponentEnable("WifiRadioEnergyModel", LOG_LEVEL_INFO);
    LogComponentEnable("BasicEnergySource", LOG_LEVEL_INFO);
    LogComponentEnable("FlowMonitor", LOG_LEVEL_INFO);

    // Simulation parameters with default values
    double simulationTime = 30.0; // Duration of simulation in seconds
    uint32_t numSensors = 5;      // Number of sensor nodes
    double failureTime = 15.0;    // Time to simulate node failure (Optional)
    std::string phyMode = "HtMcs7"; // Physical layer mode
    std::string dataRate = "100Mbps"; // Data rate for Point-to-Point links
    std::string delay = "1ms";        // Delay for Point-to-Point links

    // Parse command-line arguments to allow customization
    CommandLine cmd;
    cmd.AddValue("simulationTime", "Duration of the simulation in seconds", simulationTime);
    cmd.AddValue("numSensors", "Number of sensor nodes", numSensors);
    cmd.AddValue("failureTime", "Time to simulate node failure", failureTime);
    cmd.Parse(argc, argv);

    // ------------------------------
    // 2. Node Creation
    // ------------------------------

    // Create sensor nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(numSensors);
    NS_LOG_INFO("Created " << numSensors << " sensor nodes.");

    // Create microcontroller node
    NodeContainer microcontrollerNode;
    microcontrollerNode.Create(1);
    NS_LOG_INFO("Created Microcontroller Node.");

    // Create cloud node
    NodeContainer cloudNode;
    cloudNode.Create(1);
    NS_LOG_INFO("Created Cloud Node.");

    // ------------------------------
    // 3. Device and Channel Setup
    // ------------------------------

    // Set up Point-to-Point Links between Sensor Nodes and Microcontroller
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
    pointToPoint.SetChannelAttribute("Delay", StringValue(delay));

    NetDeviceContainer p2pDevices;
    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        NetDeviceContainer link = pointToPoint.Install(sensorNodes.Get(i), microcontrollerNode.Get(0));
        p2pDevices.Add(link);
        NS_LOG_INFO("Installed Point-to-Point link between Sensor Node " << i + 1 << " and Microcontroller Node.");
    }

    // Set up Wi-Fi Network between Microcontroller and Cloud Platform
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(phyMode));

    WifiMacHelper wifiMac;

    // Configure AP (Microcontroller Node)
    Ssid ssid = Ssid("IoT-WiFi");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, microcontrollerNode.Get(0));
    NS_LOG_INFO("Installed Wi-Fi AP on Microcontroller Node.");

    // Configure STA (Cloud Node)
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, cloudNode.Get(0));
    NS_LOG_INFO("Installed Wi-Fi STA on Cloud Node.");

    // ------------------------------
    // 4. Mobility Setup
    // ------------------------------

    // Install Constant Position Mobility Model on all nodes
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);
    mobility.Install(microcontrollerNode);
    mobility.Install(cloudNode);
    NS_LOG_INFO("Installed Constant Position Mobility Model on all nodes.");

    // ------------------------------
    // 5. Internet Stack and IP Assignment
    // ------------------------------

    // Install Internet Stack on all nodes
    InternetStackHelper stack;
    stack.InstallAll();
    NS_LOG_INFO("Installed Internet Stack on all nodes.");

    Ipv4AddressHelper address;

    // Assign IP addresses to Point-to-Point devices
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer sensorMicrocontrollerInterfaces = address.Assign(p2pDevices);
    NS_LOG_INFO("Assigned IP addresses to Point-to-Point links.");

    // Assign IP addresses to Wi-Fi devices
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer microcontrollerCloudInterfaces;
    microcontrollerCloudInterfaces.Add(address.Assign(apDevice));
    microcontrollerCloudInterfaces.Add(address.Assign(staDevices));
    NS_LOG_INFO("Assigned IP addresses to Wi-Fi devices.");

    // ------------------------------
    // 6. Application Setup
    // ------------------------------

    // Configure PacketSink on Cloud Node to receive data
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(microcontrollerCloudInterfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(cloudNode.Get(0));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simulationTime));
    NS_LOG_INFO("Installed PacketSink on Cloud Node.");

    // Configure OnOff Applications on Sensor Nodes to send data to Cloud Node
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute("DataRate", StringValue("50Mbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(1024));
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer sourceApps;
    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        ApplicationContainer app = onOffHelper.Install(sensorNodes.Get(i));
        app.Start(Seconds(2.0 + i)); // Stagger start times for better flow distribution
        app.Stop(Seconds(simulationTime));
        sourceApps.Add(app);
        NS_LOG_INFO("Installed OnOffApplication on Sensor Node " << i + 1 << ".");
    }

    // Enable routing in the network
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    NS_LOG_INFO("Populated Routing Tables.");

    // ------------------------------
    // 7. Energy Model Setup
    // ------------------------------

    // Initialize BasicEnergySourceHelper for Microcontroller Node
    BasicEnergySourceHelper microSourceHelper;
    microSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0)); // Set initial energy

    // Install BasicEnergySource on Microcontroller Node
    EnergySourceContainer microEnergySources = microSourceHelper.Install(microcontrollerNode.Get(0));
    Ptr<BasicEnergySource> microEnergySource = DynamicCast<BasicEnergySource>(microEnergySources.Get(0));

    // Verify successful installation of BasicEnergySource
    if (microEnergySource == nullptr)
    {
        NS_LOG_ERROR("Failed to install BasicEnergySource on Microcontroller Node.");
    }
    else
    {
        NS_LOG_INFO("BasicEnergySource successfully installed on Microcontroller Node.");
    }

    // Verify that AP device is a WifiNetDevice before attaching the energy model
    Ptr<NetDevice> apDevicePtr = apDevice.Get(0);
    Ptr<WifiNetDevice> wifiNetDevice = DynamicCast<WifiNetDevice>(apDevicePtr);
    if (wifiNetDevice == nullptr)
    {
        NS_LOG_ERROR("AP device is not a WifiNetDevice!");
    }
    else
    {
        NS_LOG_INFO("AP device verified as WifiNetDevice.");
    }

    // Initialize WifiRadioEnergyModelHelper with appropriate current values
    WifiRadioEnergyModelHelper wifiEnergyHelper;
    wifiEnergyHelper.Set("TxCurrentA", DoubleValue(0.0174));
    wifiEnergyHelper.Set("RxCurrentA", DoubleValue(0.0197));
    wifiEnergyHelper.Set("IdleCurrentA", DoubleValue(0.273));
    wifiEnergyHelper.Set("SleepCurrentA", DoubleValue(0.033));

    // Install WifiRadioEnergyModel on AP device only if both device and energy source are valid
    if (wifiNetDevice != nullptr && microEnergySource != nullptr)
    {
        wifiEnergyHelper.Install(apDevice.Get(0), microEnergySource);
        NS_LOG_INFO("WifiRadioEnergyModel installed on AP device.");
    }
    else
    {
        NS_LOG_ERROR("Cannot install WifiRadioEnergyModel due to missing device or energy source.");
    }

    // ------------------------------
    // 8. Flow Monitor Setup
    // ------------------------------

    // Install FlowMonitor on all nodes to gather network statistics
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    NS_LOG_INFO("FlowMonitor installed on all nodes.");

    // ------------------------------
    // 9. NetAnim Setup
    // ------------------------------

    // Configure NetAnim animation output
    AnimationInterface anim("enhanced-iot-network-animation.xml");
    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        anim.UpdateNodeDescription(sensorNodes.Get(i), "Sensor Node " + std::to_string(i + 1));
        anim.SetConstantPosition(sensorNodes.Get(i), 10 + i * 10, 10); // Arrange sensor nodes linearly
    }
    anim.UpdateNodeDescription(microcontrollerNode.Get(0), "Microcontroller Node");
    anim.UpdateNodeDescription(cloudNode.Get(0), "Cloud Node");

    anim.SetConstantPosition(microcontrollerNode.Get(0), 20, 20);
    anim.SetConstantPosition(cloudNode.Get(0), 30, 20);
    NS_LOG_INFO("NetAnim configuration completed.");

    // ------------------------------
    // 10. Simulation Execution
    // ------------------------------

    NS_LOG_INFO("Starting enhanced IoT network simulation...");
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // ------------------------------
    // 11. Flow Monitor Statistics
    // ------------------------------

    // Retrieve and display flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (const auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        NS_LOG_INFO("Flow ID: " << flow.first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
        NS_LOG_INFO("Tx Packets = " << flow.second.txPackets);
        NS_LOG_INFO("Rx Packets = " << flow.second.rxPackets);

        // Calculate throughput only if the duration is positive to avoid negative values
        double duration = flow.second.timeLastRxPacket.GetSeconds() - flow.second.timeFirstTxPacket.GetSeconds();
        if (duration > 0)
        {
            double throughput = (flow.second.rxBytes * 8.0) / duration / 1024 / 1024; // Mbps
            NS_LOG_INFO("Throughput: " << throughput << " Mbps");
        }
        else
        {
            NS_LOG_INFO("Throughput: 0 Mbps");
        }
    }

    // ------------------------------
    // 12. Energy Consumption Logging
    // ------------------------------

    // Log remaining energy of the Microcontroller Node
    if (microEnergySource == nullptr)
    {
        NS_LOG_ERROR("Microcontroller Node does not have a BasicEnergySource.");
    }
    else
    {
        double remainingMicroEnergy = microEnergySource->GetRemainingEnergy();
        NS_LOG_INFO("Remaining energy in the Microcontroller Node: " << remainingMicroEnergy << " Joules");
    }

    // ------------------------------
    // 13. Cleanup and Exit
    // ------------------------------

    Simulator::Destroy();
    NS_LOG_INFO("Enhanced IoT network simulation finished.");

    return 0;
}

