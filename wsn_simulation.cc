#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/lr-wpan-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WSNSimulation");

int main(int argc, char *argv[])
{
    // ------------------------------
    // 1. Setup and Configuration
    // ------------------------------
    uint32_t numSensors = 10;  // Number of sensor nodes
    double simulationTime = 60.0; // Simulation duration in seconds
    CommandLine cmd;
    cmd.AddValue("numSensors", "Number of sensor nodes", numSensors);
    cmd.AddValue("simulationTime", "Simulation duration in seconds", simulationTime);
    cmd.Parse(argc, argv);

    NS_LOG_INFO("Starting WSN Simulation...");

    // ------------------------------
    // 2. Node Creation
    // ------------------------------
    NodeContainer sensorNodes;
    NodeContainer sinkNode;
    sensorNodes.Create(numSensors);
    sinkNode.Create(1);

    NS_LOG_INFO("Created " << numSensors << " sensor nodes and a sink node.");

    // ------------------------------
    // 3. Energy Model Setup
    // ------------------------------
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer energySources = energySourceHelper.Install(sensorNodes);

    WifiRadioEnergyModelHelper energyModelHelper;
    energyModelHelper.Set("TxCurrentA", DoubleValue(0.017));
    energyModelHelper.Set("RxCurrentA", DoubleValue(0.019));
    energyModelHelper.Set("IdleCurrentA", DoubleValue(0.273));
    energyModelHelper.Set("SleepCurrentA", DoubleValue(0.033));
    energyModelHelper.Install(sensorNodes, energySources);

    NS_LOG_INFO("Installed energy models on sensor nodes.");

    // ------------------------------
    // 4. Communication Protocol Setup
    // ------------------------------
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer devices = lrWpanHelper.Install(sensorNodes);
    lrWpanHelper.AssociateToPan(devices, 0);

    InternetStackHelper internet;
    internet.Install(sensorNodes);
    internet.Install(sinkNode);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    NS_LOG_INFO("Configured communication protocols using IEEE 802.15.4.");

    // ------------------------------
    // 5. Mobility Model Setup
    // ------------------------------
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::RandomRectanglePositionAllocator",
                              "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"),
                              "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=100.0]"));
    mobility.Install(sensorNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sinkNode);

    NS_LOG_INFO("Installed mobility models.");

    // ------------------------------
    // 6. Application Layer
    // ------------------------------
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces.GetAddress(numSensors), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = packetSinkHelper.Install(sinkNode.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    OnOffHelper onOffHelper("ns3::UdpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute("DataRate", StringValue("50kbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(64));
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer apps;
    for (uint32_t i = 0; i < numSensors; ++i)
    {
        apps.Add(onOffHelper.Install(sensorNodes.Get(i)));
    }
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(simulationTime));

    NS_LOG_INFO("Installed application layer.");

    // ------------------------------
    // 7. Flow Monitor Setup
    // ------------------------------
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_INFO("Installed FlowMonitor.");

    // ------------------------------
    // 8. NetAnim Visualization
    // ------------------------------
    AnimationInterface anim("wsn-animation.xml");
    for (uint32_t i = 0; i < numSensors; ++i)
    {
        anim.UpdateNodeDescription(sensorNodes.Get(i), "Sensor Node " + std::to_string(i + 1));
    }
    anim.UpdateNodeDescription(sinkNode.Get(0), "Sink Node");
    NS_LOG_INFO("Configured NetAnim visualization.");

    // ------------------------------
    // 9. Simulation Execution
    // ------------------------------
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // ------------------------------
    // 10. Flow Monitor Statistics
    // ------------------------------
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        NS_LOG_INFO("Flow " << flow.first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")");
        NS_LOG_INFO("  Tx Packets: " << flow.second.txPackets);
        NS_LOG_INFO("  Rx Packets: " << flow.second.rxPackets);
        NS_LOG_INFO("  Throughput: " << (flow.second.rxBytes * 8.0 / simulationTime / 1024 / 1024) << " Mbps");
    }

    Simulator::Destroy();
    NS_LOG_INFO("Simulation complete.");
    return 0;
}
