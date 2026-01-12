/* Aufgabe 9 */
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include <string>
#include <vector>
#include <iostream>

// For Flow Monitor for Aufgabe 2
#include "ns3/flow-monitor-module.h"

NS_LOG_COMPONENT_DEFINE("Aufgabe9-Topology");

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

// Transmission tracer flag for first packet to avoid double logging
static bool gen_first_packet = true;
// Last transmission time to calculate inter-arrival times
static ns3::Time gen_last_trans_time = ns3::Seconds(0.0);

void transmission_tracer(ns3::Ptr<const ns3::Packet> p)
{
    ns3::Time now = ns3::Simulator::Now();
    std::cout << "Packet of size " << p->GetSize() << " bytes transmitted at " << now.GetSeconds() << " seconds." << std::endl;
    if (!gen_first_packet)
    {
        ns3::Time inter_arrival = now - gen_last_trans_time;
        std::cout << "Inter-arrival time since last packet: " << inter_arrival.GetSeconds() << " seconds." << std::endl;
    }
    else
    {
        gen_first_packet = false;
    }
    gen_last_trans_time = now;
}

// for Aufgabe 3
uint32_t queueMaxPackets = 100;   // Queue size in packets
static double g_queueSizeSum = 0; // Sum of queue sizes for average calculation
static ns3::Time g_lastQueueChangeTime = ns3::Seconds(0);
static uint32_t g_currentQueueSize = 0;

void QueueTracer(uint32_t oldValue, uint32_t newValue)
{
    ns3::Time now = ns3::Simulator::Now();

    // Update the time-weighted sum of queue sizes
    double duration = (now - g_lastQueueChangeTime).GetSeconds();
    g_queueSizeSum += g_currentQueueSize * duration;

    g_lastQueueChangeTime = now;
    g_currentQueueSize = newValue;

    // std::cout << "Time: " << now.GetSeconds() << " QSize: " << newValue << std::endl;
}

int main(int argc, char *argv[])
{

    NS_LOG_UNCOND("Aufgabe 9: Custom Topology Simulation");

    double meanIpd = 0.01;
    ns3::CommandLine cmd(__FILE__);
    cmd.AddValue("meanIpd", "Mean interval between bursts", meanIpd);
    cmd.AddValue("queueSize", "Max Packets in Queue", queueMaxPackets);
    cmd.Parse(argc, argv);

    uint32_t num_clients = 4;

    ns3::NodeContainer nodes;
    nodes.Create(num_clients);

    ns3::InternetStackHelper stack;
    stack.Install(nodes);

    ns3::NetDeviceContainer all_devices;
    std::vector<ns3::Ipv4Address> client_addresses;
    ns3::Ipv4AddressHelper address;
    ns3::Ipv4InterfaceContainer ifc;
    address.SetBase("10.1.1.0", "255.255.255.0");

    // 0 <-> 1
    ns3::PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue("5ms"));
    ns3::NetDeviceContainer devices01 = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    all_devices.Add(devices01);
    ifc = address.Assign(devices01);
    client_addresses.push_back(ifc.GetAddress(0)); // Address of node 0
    address.NewNetwork();

    // 1 <-> 2
    // pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue("5Mbps"));
    // pointToPoint.SetChannelAttribute("Delay", ns3::StringValue("5ms"));
    // ns3::NetDeviceContainer devices12 = pointToPoint.Install(nodes.Get(1), nodes.Get(2));
    // all_devices.Add(devices12);
    // ifc = address.Assign(devices12);
    // client_addresses.push_back(ifc.GetAddress(0)); // Address of node 1
    // address.NewNetwork();

    // for Aufgabe 3: Create a bottleneck link with limited queue size
    ns3::PointToPointHelper p2pBottleneck;
    p2pBottleneck.SetDeviceAttribute("DataRate", ns3::StringValue("5Mbps"));
    p2pBottleneck.SetChannelAttribute("Delay", ns3::StringValue("5ms"));
    // Set DropTailQueue with specified max packets
    p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", ns3::StringValue(std::to_string(queueMaxPackets) + "p"));
    // Disable flow control to observe packet drops.
    p2pBottleneck.DisableFlowControl();

    // Install bottleneck link between node 1 and node 2
    NetDeviceContainer d12 = p2pBottleneck.Install(nodes.Get(1), nodes.Get(2));
    address.Assign(d12);
    address.NewNetwork();

    // 2 <-> 3
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue("5ms"));
    ns3::NetDeviceContainer devices23 = pointToPoint.Install(nodes.Get(2), nodes.Get(3));
    all_devices.Add(devices23);
    ifc = address.Assign(devices23);
    client_addresses.push_back(ifc.GetAddress(0)); // Address of node 2
    client_addresses.push_back(ifc.GetAddress(1)); // Address of node 3
    address.NewNetwork();

    ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    debugNodes(nodes);

    uint32_t packet_length = 1024;
    uint16_t port = 9000;

    // Applications: PacketSink on each client to receive packets
    ns3::PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(ns3::Ipv4Address::GetAny(), port));
    ns3::ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < num_clients; ++i)
    {
        sinkApps.Add(packetSinkHelper.Install(nodes.Get(i)));
    }
    sinkApps.Start(ns3::Seconds(0.0));
    sinkApps.Stop(ns3::Seconds(30.0));

    ns3::OnOffHelper onOffHelper("ns3::UdpSocketFactory", ns3::InetSocketAddress(ifc.GetAddress(1), port));
    onOffHelper.SetAttribute("PacketSize", ns3::UintegerValue(packet_length));
    onOffHelper.SetAttribute("DataRate", ns3::StringValue("100Mbps"));

    // Calculate transmission duration for 100 packets
    // rtt = packet_length * 8 / data_rate
    double tDuration = (packet_length * 8.0) / (100 * 1e6); // in seconds

    // Set OnTime to constant duration (only ONE packet must be sent)
    onOffHelper.SetAttribute("OnTime", ns3::StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(tDuration) + "]"));

    // Set OffTime to achieve mean interval between bursts of 10ms
    // meanIp => mean_interval_between_bursts
    double meanOffTime = meanIpd - tDuration;
    if (meanOffTime < 0)
    {
        meanOffTime = 0;
    }

    // Exponential distribution for OffTime
    std::string offTimeStr = "ns3::ExponentialRandomVariable[Mean=" + std::to_string(meanOffTime) + "|Bound=" + std::to_string(meanOffTime * 10) + "]"; // Bound to avoid extreme values, because Exponential is unbounded
    onOffHelper.SetAttribute("OffTime", ns3::StringValue(offTimeStr));

    // Install OnOff application on node 0
    ns3::ApplicationContainer app = onOffHelper.Install(nodes.Get(0));
    app.Start(ns3::Seconds(1.0));
    app.Stop(ns3::Seconds(30.0));

    // ns3::Simulator::Stop(ns3::Seconds(1.0));

    // Connect transmission tracer to the OnOff application
    app.Get(0)->TraceConnectWithoutContext("Tx", ns3::MakeCallback(&transmission_tracer));

    ns3::FlowMonitorHelper flowmon;
    ns3::Ptr<ns3::FlowMonitor> monitor = flowmon.InstallAll();

    // Aufgabe 3: Connect Queue Tracer to the bottleneck queue
    ns3::PointerValue ptr;
    d12.Get(0)->GetAttribute("TxQueue", ptr);
    ns3::Ptr<ns3::Queue<ns3::Packet>> queue = ptr.Get<ns3::Queue<ns3::Packet>>();
    queue->TraceConnectWithoutContext("PacketsInQueue", ns3::MakeCallback(&QueueTracer));

    ns3::Simulator::Run();

    // ---------------- [Start] Flow Monitor Statistics Output ---------------- Aufgabe 2

    monitor->CheckForLostPackets();

    ns3::Ptr<ns3::Ipv4FlowClassifier> classifier = ns3::DynamicCast<ns3::Ipv4FlowClassifier>(flowmon.GetClassifier());

    // Get all flow statistics in a std::map
    std::map<ns3::FlowId, ns3::FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    std::cout << "\n"
              << "---------------------- Flow Monitor Statistics: -----------------------" << std::endl;
    for (auto const &flow : stats)
    {
        ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        // Identify the flow by source and destination IP addresses
        if (t.sourceAddress == "10.1.1.1" && t.destinationAddress == "10.1.3.2")
        {
            // std::cout << "Flow: " << t.sourceAddress << " -> " << t.destinationAddress << "\n";
            // std::cout << "  Sent Packets:     " << flow.second.txPackets << "\n";
            // std::cout << "  Received Packets: " << flow.second.rxPackets << "\n";
            // std::cout << "  Lost Packets:     " << flow.second.lostPackets << "\n";

            double lossRate = 0.0;
            if (flow.second.txPackets > 0)
            {
                lossRate = (double)flow.second.lostPackets / flow.second.txPackets * 100.0;
            }

            std::cout << "  Packet Loss Rate: " << lossRate << " %\n";

            // Throughput calculation
            if (flow.second.rxPackets > 0)
            {
                double avgDelay = (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) * 1000;
                std::cout << "  Average Delay:    " << avgDelay << " ms\n";
            }
        }
    }

    // ---------------- [End] Flow Monitor Statistics Output ---------------- Aufgabe 2

    // Aufgabe 3: Calculate and output average queue size
    double duration = (ns3::Simulator::Now() - g_lastQueueChangeTime).GetSeconds();
    g_queueSizeSum += g_currentQueueSize * duration;

    double avgQueueSize = g_queueSizeSum / ns3::Simulator::Now().GetSeconds();
    std::cout << "  Average Queue Size: " << avgQueueSize << " packets" << std::endl;

    ns3::Simulator::Destroy();

    return 0;
}
