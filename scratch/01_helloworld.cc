#include "ns3/core-module.h"
void PrintHello() {
    std::cout << "Hello from within the Simulation! It is now " << ns3::Simulator::Now().GetSeconds() << "s since simulation start." << std::endl;
}

int main(int argc, char* argv[]) {
    NS_LOG_UNCOND("Hello World from ns-3 Scratch Simulator!");
    float printTime = 1.5; // default value
    ns3::CommandLine cmd(__FILE__);
    cmd.AddValue("printTime", "Time delay before printing hello message", printTime);
    cmd.Parse(argc, argv);
    ns3::Simulator::Schedule(ns3::Seconds(printTime), &PrintHello);
    ns3::Simulator::Run();
    ns3::Simulator::Destroy();
    return 0;
}
