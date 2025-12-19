/*
Flight Entertainment System Simulation
----------------------------------------------------------
This code sets up a network simulation using ns-3 to model a flight entertainment system.
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <vector>
#include <iostream>

NS_LOG_COMPONENT_DEFINE("FlightEntertainmentSystem");

// counters for traced packets
static uint64_t g_tx_count = 0;
static uint64_t g_rx_count = 0;

// trace callbacks
void TxTracer(ns3::Ptr<const ns3::Packet> p)
{
    ++g_tx_count;
}
void RxTracer(ns3::Ptr<const ns3::Packet> p, const ns3::Address &from)
{
    ++g_rx_count;
}

void debugNodes(ns3::NodeContainer nodes)
{
    for (uint32_t n = 0; n < nodes.GetN(); ++n)
    {
        ns3::Ptr<ns3::Node> node = nodes.Get(n);
        std::cout << "Node " << node->GetId() << " has IP addresses: ";
        ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
        for (uint32_t i = 0; i < ipv4->GetNInterfaces(); ++i)
        {
            for (uint32_t j = 0; j < ipv4->GetNAddresses(i); ++j)
            {
                ns3::Ipv4Address addr = ipv4->GetAddress(i, j).GetLocal();
                std::cout << addr << " ";
            }
        }
        std::cout << std::endl;
    }
}

int main(int argc, char *argv[])
{
    uint32_t nClients = 50; // default
    uint32_t runNum = 1;    // default RNG run number

    ns3::CommandLine cmd(__FILE__);
    cmd.AddValue("nClients", "Number of client nodes (passengers)", nClients);
    cmd.Parse(argc, argv);

    double simulation_time = 2400.0; // seconds
    double onTime_min = 1.0;
    double onTime_max = 3.0;
    double offTime_min = 1.0;
    double offTime_max = 40.0;
    std::string data_rate = "10Mbps";
    std::string delay = "3ms";
    std::string server_data_rate = "1Mbps"; // server to bottleneck link data rate
    std::string packet_size = "512";        // bytes

    // set different RNG run number for each simulation run to get different traffic patterns
    ns3::RngSeedManager::SetSeed(1);
    ns3::RngSeedManager::SetRun(runNum);

    ns3::Ptr<ns3::Node> server = ns3::CreateObject<ns3::Node>();
    ns3::Ptr<ns3::Node> bottleneck = ns3::CreateObject<ns3::Node>();
    ns3::NodeContainer clients;
    clients.Create(nClients);

    ns3::InternetStackHelper stack;
    stack.Install(server);
    stack.Install(bottleneck);
    stack.Install(clients);

    // Ip address setting
    ns3::Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    ns3::NetDeviceContainer all_devices;

    /*
    Setup point-to-point links between server and bottleneck, and bottleneck to each client
    */
    // Server to Bottleneck link
    ns3::PointToPointHelper p2p_server_bottleneck;
    p2p_server_bottleneck.SetDeviceAttribute("DataRate", ns3::StringValue(data_rate));
    p2p_server_bottleneck.SetChannelAttribute("Delay", ns3::StringValue(delay));
    ns3::NetDeviceContainer server_bottleneck = p2p_server_bottleneck.Install(server, bottleneck);
    all_devices.Add(server_bottleneck);
    address.Assign(server_bottleneck);
    address.NewNetwork();

    // Client to Bottleneck links
    ns3::PointToPointHelper p2p_client_bottleneck;
    p2p_client_bottleneck.SetDeviceAttribute("DataRate", ns3::StringValue(data_rate));
    p2p_client_bottleneck.SetChannelAttribute("Delay", ns3::StringValue(delay));

    std::vector<ns3::Ipv4Address> client_addresses;
    ns3::NetDeviceContainer client_device;
    for (uint32_t i = 0; i < nClients; ++i)
    {
        client_device = p2p_client_bottleneck.Install(bottleneck, clients.Get(i));
        all_devices.Add(client_device);
        ns3::Ipv4InterfaceContainer ifc = address.Assign(client_device);
        client_addresses.push_back(ifc.GetAddress(1));
        address.NewNetwork();
    }

    // Debug: print IP addresses of all nodes
    debugNodes(clients);
    debugNodes(ns3::NodeContainer(server, bottleneck));

    // Applications: PacketSink on each client. Install Applications on clients
    uint16_t port = 9000;
    ns3::PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port));
    ns3::ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < nClients; ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(clients.Get(i)));
    }
    sinkApps.Start(ns3::Seconds(0.0));
    sinkApps.Stop(ns3::Seconds(simulation_time));

    // OnOff application on server targeting each client
    ns3::OnOffHelper onoff("ns3::UdpSocketFactory", ns3::Address(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port)));
    onoff.SetAttribute("DataRate", ns3::StringValue(server_data_rate));
    onoff.SetAttribute("PacketSize", ns3::UintegerValue(std::stoul(packet_size)));

    ns3::Ptr<ns3::UniformRandomVariable> onTime = ns3::CreateObject<ns3::UniformRandomVariable>();
    onTime->SetAttribute("Min", ns3::DoubleValue(onTime_min));
    onTime->SetAttribute("Max", ns3::DoubleValue(onTime_max));
    onoff.SetAttribute("OnTime", ns3::PointerValue(onTime));

    ns3::Ptr<ns3::UniformRandomVariable> offTime = ns3::CreateObject<ns3::UniformRandomVariable>();
    offTime->SetAttribute("Min", ns3::DoubleValue(offTime_min));
    offTime->SetAttribute("Max", ns3::DoubleValue(offTime_max));
    onoff.SetAttribute("OffTime", ns3::PointerValue(offTime));

    ns3::ApplicationContainer onoffAppsPerClient = ns3::ApplicationContainer();
    for (uint32_t i = 0; i < nClients; ++i)
    {
        ns3::AddressValue remoteAddress(ns3::InetSocketAddress(client_addresses[i], port));
        onoff.SetAttribute("Remote", remoteAddress);
        onoffAppsPerClient.Add(onoff.Install(server));
    }
    onoffAppsPerClient.Start(ns3::Seconds(0.0));
    onoffAppsPerClient.Stop(ns3::Seconds(simulation_time));

    // connect Rx trace on each PacketSink to count received packets (Tracing of recieve packets on Client side)
    for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
    {
        sinkApps.Get(i)->TraceConnectWithoutContext("Rx", ns3::MakeCallback(&RxTracer));
    }

    // connect Tx trace on each OnOff to count sent packets (on Server side)
    for (uint32_t i = 0; i < onoffAppsPerClient.GetN(); ++i)
    {
        onoffAppsPerClient.Get(i)->TraceConnectWithoutContext("Tx", ns3::MakeCallback(&TxTracer));
    }

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    // std::cout << "Total packets sent by server: " << g_tx_count << std::endl;
    // std::cout << "Total packets received by clients: " << g_rx_count << std::endl;
    // std::cout << "Packet loss: " << (g_tx_count - g_rx_count) << " packets." << std::endl;
    // std::cout << "Packet loss rate: " << ((g_tx_count - g_rx_count) * 100.0 / g_tx_count) << " %" << std::endl;

    double lossRatio = 0.0;
    if (g_tx_count > 0)
    {
        lossRatio = ((double)(g_tx_count - g_rx_count) * 100.0 / g_tx_count);
    }

    std::cout << "CSV_RESULT," << nClients << "," << runNum << "," << lossRatio << std::endl;

    return 0;
}
