/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-apps-module.h"
#include <iostream>

#include "debug.h"
NS_LOG_COMPONENT_DEFINE("PingExample");


int main(int argc, char* argv[]) {
    NS_LOG_UNCOND("Hello World from ns-3 Scratch Simulator!");

    ns3::CommandLine cmd(__FILE__);    
    cmd.Parse(argc, argv);
    ns3::Time interPacketInterval = ns3::Seconds(1);
    uint32_t size = 128;
    uint32_t count = 4;
    ns3::NodeContainer nodes;
    nodes.Create(3);

    ns3::PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", ns3::StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", ns3::StringValue("2ms"));
    ns3::NetDeviceContainer linkAB = pointToPoint.Install(nodes.Get(0), nodes.Get(1));
    ns3::NetDeviceContainer linkBC = pointToPoint.Install(nodes.Get(1), nodes.Get(2));

    ns3::InternetStackHelper stack;
    stack.Install(nodes);
    ns3::Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.252");

    ns3::Ipv4InterfaceContainer ifAB = address.Assign(linkAB);
    address.NewNetwork();
    ns3::Ipv4InterfaceContainer ifBC = address.Assign(linkBC);

    
    debugNodes(nodes);
    
    // setup a ping application on node0 to ping node2

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    return 0;
}
