/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <vector>
#include <iostream>

NS_LOG_COMPONENT_DEFINE("ScratchSimulator");

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
    NS_LOG_UNCOND("Hello World from ns-3 Scratch Simulator!");

    int num_clients = 5;
    std::string bottleneck_bw = "10Mbps";
    std::string bottleneck_delay = "10ms";

    std::string default_bw = "100Mbps";
    std::string default_delay = "1ms";

    double onTime_min = 1.0;
    double onTime_max = 3.0;
    double offTime_min = 1.0;
    double offTime_max = 40.0;
    std::string data_rate = "1Mbps";
    double simulation_time = 60.0;

    ns3::CommandLine cmd(__FILE__);
    cmd.AddValue("clients", "Number of client nodes", num_clients);
    cmd.Parse(argc, argv);

    ns3::Ptr<ns3::Node> server = ns3::CreateObject<ns3::Node>();
    ns3::Ptr<ns3::Node> bottleneck = ns3::CreateObject<ns3::Node>();
    ns3::NodeContainer nodes;
    nodes.Create(num_clients);

    ns3::InternetStackHelper stack;
    stack.Install(nodes);
    stack.Install(server);
    stack.Install(bottleneck);

    ns3::PointToPointHelper pointToPoint;

    ns3::NetDeviceContainer all_devices;
    ns3::Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue(bottleneck_bw));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue(bottleneck_delay));

    // Bottleneck to Server
    ns3::NetDeviceContainer devices;
    devices = pointToPoint.Install(server, bottleneck);
    all_devices.Add(devices);
    address.Assign(devices);
    address.NewNetwork();

    // Clients to Bottleneck
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue(default_bw));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue(default_delay));
    std::vector<ns3::Ipv4Address> client_addresses;
    for (int i = 0; i < num_clients; i++)
    {
        devices = pointToPoint.Install(nodes.Get(i), bottleneck);
        all_devices.Add(devices);
        ns3::Ipv4InterfaceContainer ifc = address.Assign(devices);
        client_addresses.push_back(ifc.GetAddress(0));
        address.NewNetwork();
    }

    debugNodes(nodes);
    debugNodes(ns3::NodeContainer(server, bottleneck));

    // Applications: PacketSink on each client, OnOff on server targeting each client
    uint16_t port = 9000;
    /*
    create a PacketSink application on each client node to receive packets
    */
    // PacketSink helper (UDP) listening on port on all client nodes
    ns3::PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port));
    ns3::ApplicationContainer sinkApps;
    for (int i = 0; i < num_clients; ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(nodes.Get(i)));
    }
    sinkApps.Start(ns3::Seconds(0.0));
    sinkApps.Stop(ns3::Seconds(simulation_time));

    /*
         create an OnOff application on the server node for each client;
         the work of this application is to send data to the corresponding client
     */
    // Init OnOff application helper with a dummy remote address, will be set per client later
    ns3::OnOffHelper onoff("ns3::UdpSocketFactory", ns3::Address(ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port)));
    onoff.SetAttribute("DataRate", ns3::StringValue("1Mbps"));

    ns3::Ptr<ns3::UniformRandomVariable> onTime = ns3::CreateObject<ns3::UniformRandomVariable>();
    onTime->SetAttribute("Min", ns3::DoubleValue(onTime_min));
    onTime->SetAttribute("Max", ns3::DoubleValue(onTime_max));
    onoff.SetAttribute("OnTime", ns3::PointerValue(onTime));

    ns3::Ptr<ns3::UniformRandomVariable> offTime = ns3::CreateObject<ns3::UniformRandomVariable>();
    offTime->SetAttribute("Min", ns3::DoubleValue(offTime_min));
    offTime->SetAttribute("Max", ns3::DoubleValue(offTime_max));
    onoff.SetAttribute("OffTime", ns3::PointerValue(offTime));

    ns3::ApplicationContainer onoffAppsPerClient = ns3::ApplicationContainer();
    for (int i = 0; i < num_clients; ++i)
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

    // build routes so server can reach client subnets
    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    // print traced counters after simulation
    std::cout << "Total sent (Tx) packets: " << g_tx_count << std::endl;
    std::cout << "Total received (Rx) packets: " << g_rx_count << std::endl;
    std::cout << "Total lost packets: " << (g_tx_count - g_rx_count) << std::endl;
    std::cout << "Packet delivery ratio: "
              << (static_cast<double>(g_rx_count) / static_cast<double>(g_tx_count)) * 100.0
              << " %" << std::endl;

    return 0;
}
