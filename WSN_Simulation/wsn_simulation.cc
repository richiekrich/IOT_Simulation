#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/energy-module.h"
#include "ns3/applications-module.h"
#include <sstream>
#include <iomanip>

using namespace ns3;
// Use specific namespaces to avoid ambiguity
using namespace ns3::lrwpan;

NS_LOG_COMPONENT_DEFINE("WSNSimulation");

int main(int argc, char *argv[])
{
    // Command line arguments
    uint32_t numNodes = 10;        // Number of sensor nodes
    double simulationTime = 50.0;  // Simulation duration in seconds

    CommandLine cmd;
    cmd.AddValue("numNodes", "Number of WSN nodes", numNodes);
    cmd.AddValue("simulationTime", "Simulation duration in seconds", simulationTime);
    cmd.Parse(argc, argv);

    // Enable Logging
    LogComponentEnable("WSNSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    // Create WSN Nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(numNodes);

    // Create Coordinator Node
    NodeContainer coordinatorNode;
    coordinatorNode.Create(1);

    NS_LOG_INFO("Created " << numNodes << " sensor nodes and 1 coordinator node.");

    // Install Mobility Model
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < numNodes; ++i)
    {
        positionAlloc->Add(Vector(10 * i, 10, 0)); // Arrange nodes in a line
    }
    positionAlloc->Add(Vector(50, 50, 0)); // Coordinator position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);
    mobility.Install(coordinatorNode);

    // Install IEEE 802.15.4 (ZigBee) Protocol
    ns3::energy::LrWpanRadioEnergyModelHelper energyModelHelper;
    energyModelHelper.Set("TxCurrentA", ns3::DoubleValue(0.017));
    NetDeviceContainer lrWpanDevices = lrWpanHelper.Install(sensorNodes);
    NetDeviceContainer coordinatorDevice = lrWpanHelper.Install(coordinatorNode);

    // Associate Sensor Nodes with Coordinator
    for (uint32_t i = 0; i < lrWpanDevices.GetN(); i++) {
        Ptr<LrWpanNetDevice> device = DynamicCast<LrWpanNetDevice>(lrWpanDevices.Get(i));
        if (device != nullptr) {
            device->SetPanId(0x01); // Set PAN ID
            // Create a MAC address like "00:02", "00:03", etc.
            std::ostringstream oss;
            oss << "00:" << std::setw(2) << std::setfill('0') << (i + 2);
            Mac16Address macAddr;
            macAddr.SetFromString(oss.str().c_str());
            device->GetMac()->SetShortAddress(macAddr);
        }
    }

    // Associate Coordinator Device to PAN
    Ptr<LrWpanNetDevice> coordinatorDevicePtr = DynamicCast<LrWpanNetDevice>(coordinatorDevice.Get(0));
    if (coordinatorDevicePtr != nullptr) {
        coordinatorDevicePtr->SetPanId(0x01); // Set PAN ID
        Mac16Address coordinatorMac("00:01");
        coordinatorDevicePtr->GetMac()->SetShortAddress(coordinatorMac);
    }

    NS_LOG_INFO("Installed IEEE 802.15.4 devices and associated with PAN.");

    // Install Internet Stack
    InternetStackHelper internetStackHelper;
    internetStackHelper.Install(sensorNodes);
    internetStackHelper.Install(coordinatorNode);

    // Assign IP Addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(lrWpanDevices);
    ipv4.Assign(coordinatorDevice);

    // Install Energy Model
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(50.0));
    EnergySourceContainer energySources = energySourceHelper.Install(sensorNodes);

    LrWpanRadioEnergyModelHelper radioEnergyHelper;
    DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(lrWpanDevices, energySources);

    NS_LOG_INFO("Installed energy models on sensor nodes.");

    // Applications
    uint16_t port = 8080;
    Address sinkAddress = InetSocketAddress(interfaces.GetAddress(numNodes), port);
    OnOffHelper onOffHelper("ns3::UdpSocketFactory", sinkAddress);
    onOffHelper.SetConstantRate(DataRate("250kbps"));
    ApplicationContainer apps = onOffHelper.Install(sensorNodes);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(simulationTime));

    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = packetSinkHelper.Install(coordinatorNode);
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simulationTime));

    // Run Simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("WSN Simulation Finished.");

    return 0;
}
