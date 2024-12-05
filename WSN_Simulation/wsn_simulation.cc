#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/energy-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("WSNSimulation");

int main(int argc, char *argv[])
{
    // Command line arguments
    uint32_t numNodes = 10;  // Number of sensor nodes
    double simulationTime = 50.0; // Simulation duration

    CommandLine cmd;
    cmd.AddValue("numNodes", "Number of WSN nodes", numNodes);
    cmd.AddValue("simulationTime", "Simulation duration in seconds", simulationTime);
    cmd.Parse(argc, argv);

    // Logging
    LogComponentEnable("WSNSimulation", LOG_LEVEL_INFO);

    // Create WSN Nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(numNodes);

    // Create a coordinator node
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
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrWpanDevices = lrWpanHelper.Install(sensorNodes);
    NetDeviceContainer coordinatorDevice = lrWpanHelper.Install(coordinatorNode);
    // Associate each sensor device with the coordinator
    for (uint32_t i = 0; i < lrWpanDevices.GetN(); i++) {
        Ptr<LrWpanNetDevice> device = DynamicCast<LrWpanNetDevice>(lrWpanDevices.Get(i));
        device->SetPanId(0x01); // PAN ID for the network
    }

    lrWpanHelper.AssociateToPan(coordinatorDevice.Get(0), lrWpanDevices);

    NS_LOG_INFO("Installed IEEE 802.15.4 devices and associated with PAN.");

    // Install Energy Model
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(50.0));
    EnergySourceContainer energySources = energySourceHelper.Install(sensorNodes);

    LrWpanRadioEnergyModelHelper radioEnergyHelper;
    DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(lrWpanDevices, energySources);

    NS_LOG_INFO("Installed energy models on sensor nodes.");

    // Internet Stack (Optional)
    InternetStackHelper internetStackHelper;
    internetStackHelper.Install(coordinatorNode);

    // Applications
    uint16_t port = 8080;
    OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("255.255.255.255"), port)));
    onOffHelper.SetConstantRate(DataRate("250kbps"));
    ApplicationContainer apps = onOffHelper.Install(sensorNodes);
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(simulationTime));

    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = packetSinkHelper.Install(coordinatorNode);
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simulationTime));

    // Simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    NS_LOG_INFO("WSN Simulation Finished.");

    return 0;
}
