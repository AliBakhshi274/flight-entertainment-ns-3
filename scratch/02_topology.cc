/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include "debug.h"
NS_LOG_COMPONENT_DEFINE("TopologyExample");



int main(int argc, char* argv[]) {
    NS_LOG_UNCOND("Hello World from ns-3 Scratch Simulator!");

    ns3::CommandLine cmd(__FILE__);    
    cmd.Parse(argc, argv);

    ns3::NodeContainer nodes;
    nodes.Create(3);

    // Create point-to-point links between node0-node1 and node1-node2 and set ip addresses


    debugNodes(nodes);

    ns3::Simulator::Run();
    ns3::Simulator::Destroy();

    return 0;
}
