/* 

Aufgabe 9 - Transportprotokolle und Ihr Entwurf
name: Ali Bakhshi
Matrikelnummer: 3625262

*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include <string>
#include <iostream>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Aufgabe9_Sim");

static double g_queueSizeSum = 0;
static Time g_lastQueueChangeTime = Seconds(0);
static uint32_t g_currentQueueSize = 0;

// Tracer function to monitor queue size changes (for Task 3 to calculate average queue size)
void QueueTracer(uint32_t oldValue, uint32_t newValue)
{
    Time now = Simulator::Now();
    double duration = (now - g_lastQueueChangeTime).GetSeconds();
    g_queueSizeSum += g_currentQueueSize * duration; 
    g_lastQueueChangeTime = now;
    g_currentQueueSize = newValue;
}

int main(int argc, char *argv[])
{
    // Default command line parameters
    double meanIpd = 0.002;      // Mean Inter-Packet Delay in seconds (controls load)
    uint32_t queueMaxPackets = 50; // Max queue capacity for the bottleneck
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("meanIpd", "Mean Inter-Packet Delay (seconds)", meanIpd); // for Task 1c to control load
    cmd.AddValue("queueSize", "Max Packets in Bottleneck Queue", queueMaxPackets); // for Task 3 to set queue capacity
    cmd.AddValue("verbose", "Enable logging", verbose); // it must be set to true to see logs
    cmd.Parse(argc, argv); 

    if (verbose) {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // Create Nodes (Task 1b)
    NodeContainer nodes;
    nodes.Create(4); // Topology: n0  n1  n2  n3

    InternetStackHelper stack;
    stack.Install(nodes);

    // Create Point-to-Point Links (Task 1b):

    // Fast Link: n0 -> n1 (10Mbps, 5ms)
    PointToPointHelper p2pFast;
    p2pFast.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2pFast.SetChannelAttribute("Delay", StringValue("5ms"));

    // Bottleneck Link: n1 -> n2 (5Mbps, 5ms)
    PointToPointHelper p2pBottleneck;
    p2pBottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2pBottleneck.SetChannelAttribute("Delay", StringValue("5ms"));
    
    // Setup queue for the bottleneck (for Task 3)
    p2pBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(std::to_string(queueMaxPackets) + "p"));
    p2pBottleneck.DisableFlowControl(); // Important: Disable FC to allow actual packet drops

// Installation of network devices and IP addresses
    Ipv4AddressHelper address;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer interfaces;

    // n0-n1
    address.SetBase("10.1.1.0", "255.255.255.0");
    devices = p2pFast.Install(nodes.Get(0), nodes.Get(1));
    address.Assign(devices);

    // n1-n2 (The Bottleneck)
    address.SetBase("10.1.2.0", "255.255.255.0");
    NetDeviceContainer devBottleneck = p2pBottleneck.Install(nodes.Get(1), nodes.Get(2));
    address.Assign(devBottleneck);

    // n2-n3
    address.SetBase("10.1.3.0", "255.255.255.0");
    devices = p2pFast.Install(nodes.Get(2), nodes.Get(3));
    interfaces = address.Assign(devices); 

    Ipv4Address dstAddress = interfaces.GetAddress(1); // Address of node n3

    // Setup global routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // --- Traffic Generation (Task 1c) ---
    uint32_t packetSize = 1000; // Payload size in bytes
    uint16_t port = 9000;

    // Packet Sink on n3 ('Receiver')
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(3));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(30.0));

    // OnOff Application on n0 ('Sender')
    OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(dstAddress, port));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
    
    // Set App DataRate very high to minimize serialization delay at the app layer
    StringValue appDataRate = StringValue("100Mbps"); 
    onOffHelper.SetAttribute("DataRate", appDataRate);

    // Logic to send exactly ONE packet per On-phase (Task 1c)
    // Calculate txTime for one packet at 100Mbps
    double txTime = (packetSize * 8.0) / 100000000.0; 
    
    // OnTime
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(txTime) + "]"));

    // OffTime
    double meanOffTime = meanIpd - txTime;
    if (meanOffTime < 0) meanOffTime = 0;

    // Define OffTime attribute string
    std::string offTimeAttr = "ns3::ExponentialRandomVariable[Mean=" + 
                                std::to_string(meanOffTime) + 
                                "|Bound=" +  
                                std::to_string(meanOffTime * 20) + 
                                "]";


    onOffHelper.SetAttribute("OffTime", StringValue(offTimeAttr));

    ApplicationContainer sourceApp = onOffHelper.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(30.0));

    // Connect Tracer to Bottleneck Queue (n1 -> n2)
    // Getting the TxQueue of the first device in devBottleneck (it belongs to n1)
    PointerValue ptr;
    devBottleneck.Get(0)->GetAttribute("TxQueue", ptr);
    Ptr<Queue<Packet>> queue = ptr.Get<Queue<Packet>>();
    queue->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(&QueueTracer));

    // ''' ------------------------------------ Flow Monitor (Task 2) --------------------------------------------------------'''
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    // Run Sim
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();

    // ''' ------------------------------------ Calculate Average Queue Size (Task 3) --------------------------------------------------------'''
    // for the last interval until simulation end (this is needed to close the integral and Account for the final queue size)
    double simDuration = (Simulator::Now() - g_lastQueueChangeTime).GetSeconds();
    g_queueSizeSum += g_currentQueueSize * simDuration;
    double avgQueueSize = g_queueSizeSum / Simulator::Now().GetSeconds();

    // ''' ----------------------------------------------- Monitor and Print Results (Task 2 & 3) --------------------------------------------------------'''
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    // Calc theoretical Offered Load
    double offeredLoadMbps = (packetSize * 8.0) / (meanIpd * 1e6);

    // Iterate flows and print CSV format
    // TITLES for styling CSV structure: MeanIPD, OffredLoad, QueueCap, AvgQueueSize, PacketLossRate, AvgDelay, Throughput
    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        
        // Filter: only look at the main flow coming from (10.1.1.1)
        if (t.sourceAddress == "10.1.1.1") 
        {
            double lossRate = (double)flow.second.lostPackets / flow.second.txPackets * 100.0;
            double avgDelayMs = (flow.second.rxPackets > 0) ? 
                                (flow.second.delaySum.GetSeconds() / flow.second.rxPackets) * 1000 : 0;
            double throughputMbps = (flow.second.rxBytes * 8.0) / (30.0 * 1e6); // based on total sim time

            std::cout << meanIpd << "," 
                      << offeredLoadMbps << ", "
                      << queueMaxPackets << ", "
                      << avgQueueSize << ", "
                      << lossRate << ", " 
                      << avgDelayMs << ", "
                      << throughputMbps
                      << std::endl;
        }
    }

    Simulator::Destroy();
    return 0;
}