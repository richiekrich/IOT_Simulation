#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"  // Include NetAnim for visualization

using namespace ns3;

int main(int argc, char *argv[])
{
    // Enable logging for various components
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_INFO);
    LogComponentEnable("YansWifiPhy", LOG_LEVEL_INFO);

    // Step 1: Create Node Containers for Sensor, Microcontroller, and Cloud
    NodeContainer p2pNodes; // Sensor and Microcontroller
    p2pNodes.Create(2);

    NodeContainer wifiNodes; // Microcontroller and Cloud Platform (STA node)
    wifiNodes.Add(p2pNodes.Get(1)); // Microcontroller Node as WiFi AP
    wifiNodes.Create(1);  // Cloud platform STA node

    // Step 2: Set up Point-to-Point Link between Sensor and Microcontroller
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    // Step 3: Set up WiFi between Microcontroller and Cloud Platform
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11);

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    // Setup AP Node (Microcontroller)
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, wifiNodes.Get(0)); // Microcontroller Node

    // Setup STA Node (Cloud Platform)
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevice = wifi.Install(phy, mac, wifiNodes.Get(1)); // Cloud Platform Node

    // Step 4.1: Set mobility for sensor node (Node 0 - part of point-to-point)
    Ptr<Node> sensorNode = p2pNodes.Get(0); // Sensor node
    Ptr<MobilityModel> mobility = CreateObject<ConstantPositionMobilityModel>();
    sensorNode->AggregateObject(mobility);

    // Step 4.2: Set mobility for microcontroller node (Node 1 - part of point-to-point and WiFi)
    Ptr<Node> microcontrollerNode = p2pNodes.Get(1); // Microcontroller node
    Ptr<MobilityModel> mobility2 = CreateObject<ConstantPositionMobilityModel>();
    microcontrollerNode->AggregateObject(mobility2);

    // Set mobility for WiFi station node (Cloud Platform - Node 2)
    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityHelper.Install(wifiNodes.Get(1)); // Cloud Platform (STA) is stationary

    // Step 5: Install Internet Stack and Assign IP Addresses
    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(0));  // Sensor Node
    stack.Install(wifiNodes);        // Microcontroller and Cloud Platform Nodes

    Ipv4AddressHelper address;
    
    // Point-to-point IP addresses
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

    // WiFi IP addresses
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiInterfaces = address.Assign(NetDeviceContainer(apDevice, staDevice));

    // Step 6: Set up Applications (UdpEchoServer on Sensor, UdpEchoClient on Cloud Platform)
    UdpEchoServerHelper echoServer(9);  // Port 9

    ApplicationContainer serverApps = echoServer.Install(p2pNodes.Get(0));  // Sensor Node
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    UdpEchoClientHelper echoClient(p2pInterfaces.GetAddress(0), 9); // Target Sensor Node
    echoClient.SetAttribute("MaxPackets", UintegerValue(1));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(wifiNodes.Get(1)); // Cloud Platform
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(10.0));

    // Enable Routing between Networks
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Enable animation for NetAnim visualization
    AnimationInterface anim ("iot-animation.xml");

    // Print starting message
    NS_LOG_UNCOND("Simulation starting...");

    // Step 7: Run the Simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    // Print finishing message
    NS_LOG_UNCOND("Simulation finished.");

    return 0;
}
