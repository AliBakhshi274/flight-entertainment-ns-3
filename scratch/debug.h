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
