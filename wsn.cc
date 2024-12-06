/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/energy-module.h"
#include "ns3/netanim-module.h"

using namespace ns3;
using namespace ns3::lrwpan;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("WSNSimulation");

int main(int argc, char *argv[])
{
    uint32_t numSensors = 10;
    double simulationTime = 60.0;

    CommandLine cmd;
    cmd.AddValue("numSensors", "Number of sensor nodes", numSensors);
    cmd.AddValue("simulationTime", "Duration of the simulation in seconds", simulationTime);
    cmd.Parse(argc, argv);

    // Create nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(numSensors);

    NodeContainer coordinatorNode;
    coordinatorNode.Create(1);

    // Install mobility model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);
    mobility.Install(coordinatorNode);

    // Configure LR-WPAN
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer sensorDevices = lrWpanHelper.Install(sensorNodes);
    NetDeviceContainer coordinatorDevice = lrWpanHelper.Install(coordinatorNode);

    // Assign PAN IDs
    for (uint32_t i = 0; i < sensorDevices.GetN(); ++i)
    {
        Ptr<NetDevice> device = sensorDevices.Get(i);
        Ptr<LrWpanNetDevice> lrWpanDevice = DynamicCast<LrWpanNetDevice>(device);
        lrWpanDevice->GetMac()->SetPanId(0x01);
    }

    Ptr<NetDevice> device = coordinatorDevice.Get(0);
    Ptr<LrWpanNetDevice> lrWpanCoordinatorDevice = DynamicCast<LrWpanNetDevice>(device);
    lrWpanCoordinatorDevice->GetMac()->SetPanId(0x01);

    // Enable packet reception tracing for LR-WPAN
    lrWpanHelper.EnablePcap("wsn", sensorDevices);
    lrWpanHelper.EnablePcap("wsn", coordinatorDevice);

    // Install energy sources on sensor nodes
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0)); // Initial energy in Joules
    EnergySourceContainer energySources = energySourceHelper.Install(sensorNodes);

    // Manually create and configure DeviceEnergyModels for LR-WPAN
    for (uint32_t i = 0; i < sensorDevices.GetN(); ++i)
    {
        Ptr<NetDevice> netDevice = sensorDevices.Get(i);
        Ptr<Node> node = sensorNodes.Get(i);
        Ptr<BasicEnergySource> energySource = node->GetObject<BasicEnergySource>();

        // Create a simple device energy model
        Ptr<DeviceEnergyModel> energyModel = CreateObject<SimpleDeviceEnergyModel>();
        energyModel->SetEnergySource(energySource);
        energySource->AppendDeviceEnergyModel(energyModel);

        netDevice->AggregateObject(energyModel);
    }

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(sensorNodes);
    stack.Install(coordinatorNode);

    // Configure applications
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(coordinatorNode.Get(0));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simulationTime));

    OnOffHelper onOffHelper("ns3::UdpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute("DataRate", StringValue("10Kbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(64));

    ApplicationContainer sourceApps;
    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        ApplicationContainer app = onOffHelper.Install(sensorNodes.Get(i));
        app.Start(Seconds(2.0 + i * 0.5));
        app.Stop(Seconds(simulationTime - 5));
        sourceApps.Add(app);
    }

    // Enable NetAnim output
    AnimationInterface anim("wsn-animation.xml");

    // Run the simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
