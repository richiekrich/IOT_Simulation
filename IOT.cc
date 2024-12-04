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

NS_LOG_COMPONENT_DEFINE("IoTNetworkSimulation");

int main(int argc, char *argv[])
{
    // Enable logging for specific components
    LogComponentEnable("IoTNetworkSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Simulation parameters
    double simulationTime = 30.0; // seconds
    uint32_t numSensors = 5;      // Number of sensor nodes
    double failureTime = 15.0;    // Time to simulate node failure

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue("simulationTime", "Duration of the simulation in seconds", simulationTime);
    cmd.AddValue("numSensors", "Number of sensor nodes", numSensors);
    cmd.AddValue("failureTime", "Time to simulate node failure", failureTime);
    cmd.Parse(argc, argv);

    // Step 1: Create Nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(numSensors);

    NodeContainer microcontrollerNode;
    microcontrollerNode.Create(1);

    NodeContainer cloudNode;
    cloudNode.Create(1);

    // Step 2: Set up Point-to-Point Links between Sensor Nodes and Microcontroller
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        NetDeviceContainer link = pointToPoint.Install(sensorNodes.Get(i), microcontrollerNode.Get(0));
        p2pDevices.Add(link);
    }

    // Step 3: Set up Wi-Fi Network between Microcontroller and Cloud Platform
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n); // Compatible constant for 802.11n
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("HtMcs7"));

    WifiMacHelper wifiMac;

    // Configure AP (Microcontroller Node)
    Ssid ssid = Ssid("IoT-WiFi");
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, microcontrollerNode.Get(0));

    // Configure STA (Cloud Node)
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, cloudNode.Get(0));

    // Step 4: Mobility Models
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);
    mobility.Install(microcontrollerNode);
    mobility.Install(cloudNode);

    // Step 5: Install Internet Stack and Assign IP Addresses
    InternetStackHelper stack;
    stack.InstallAll();

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer sensorMicrocontrollerInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer microcontrollerCloudInterfaces;
    microcontrollerCloudInterfaces.Add(address.Assign(apDevice));
    microcontrollerCloudInterfaces.Add(address.Assign(staDevices));

    // Step 6: Install Energy Model
    ns3::energy::EnergySourceContainer energySources;

    for (uint32_t i = 0; i < sensorNodes.GetN(); ++i)
    {
        Ptr<ns3::energy::BasicEnergySource> energySource = CreateObject<ns3::energy::BasicEnergySource>();
        energySource->SetInitialEnergy(100.0); // Set initial energy for each sensor node
        sensorNodes.Get(i)->AggregateObject(energySource);
        energySources.Add(energySource);
    }

    // Step 7: NetAnim Visualization
    AnimationInterface anim("iot-animation.xml");
    anim.UpdateNodeDescription(sensorNodes.Get(0), "Sensor Node");
    anim.SetConstantPosition(sensorNodes.Get(0), 10, 10);

    // Step 8: Run Simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    Simulator::Destroy();
    NS_LOG_INFO("Simulation completed.");
    return 0;
}

