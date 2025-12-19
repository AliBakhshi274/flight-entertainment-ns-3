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
void TxTracer(ns3::Ptr<const ns3::Packet> p) {
    ++g_tx_count;
}
void RxTracer(ns3::Ptr<const ns3::Packet> p, const ns3::Address &from) {
    ++g_rx_count;
}

void debugNodes(ns3::NodeContainer nodes) {
    for (uint32_t n = 0; n < nodes.GetN(); ++n) {
        ns3::Ptr<ns3::Node> node = nodes.Get(n);
        std::cout << "Node " << node->GetId() << " has IP addresses: ";
        ns3::Ptr<ns3::Ipv4> ipv4 = node->GetObject<ns3::Ipv4>();
        for (uint32_t i = 0; i < ipv4->GetNInterfaces(); ++i) {
            for (uint32_t j = 0; j < ipv4->GetNAddresses(i); ++j) {
                ns3::Ipv4Address addr = ipv4->GetAddress(i, j).GetLocal();
                std::cout << addr << " ";
            }
        }
        std::cout << std::endl;
    }
}


int main(int argc, char* argv[]) {
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

    // create the experiment scenario following the parameters above
    return 0;
}
