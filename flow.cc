#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Lab1");
int main (int argc, char *argv[])
{
double lat = 2.0;
uint64_t rate = 5000000; // Data rate in bps
double interval = 0.05;
CommandLine cmd;
cmd.AddValue ("latency", "P2P link Latency in miliseconds", lat);
cmd.AddValue ("rate", "P2P data rate in bps", rate);
cmd.AddValue ("interval", "UDP client packet interval", interval);
cmd.Parse (argc, argv);
// Explicitly create the nodes required by the topology (shown above).
NS_LOG_INFO ("Create nodes.");
NodeContainer n;
n.Create (2);
NS_LOG_INFO ("Create channels.");
// Explicitly create the channels required by the topology (shown above).
PointToPointHelper p2p;
p2p.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (lat)));
p2p.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (rate)));
p2p.SetDeviceAttribute ("Mtu", UintegerValue (1400));
NetDeviceContainer dev = p2p.Install (n.Get(0), n.Get(1));
//NetDeviceContainer dev2 = p2p.Install (n.Get(1), n.Get(2));
// We've got the "hardware" in place. Now we need to add IP addresses.
// Install Internet Stack
InternetStackHelper internet;
internet.Install (n);
Ipv4AddressHelper ipv4;
NS_LOG_INFO ("Assign IP Addresses.");
ipv4.SetBase ("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer i = ipv4.Assign (dev);
/*ipv4.SetBase ("10.1.2.0", "255.255.255.0");
Ipv4InterfaceContainer i2 = ipv4.Assign (dev2);*/
Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
NS_LOG_INFO ("Create Applications.");
// Create one udpServer application on node one.
uint16_t port1 = 8000; // Need different port numbers to ensure there is no conflict
// uint16_t port2 = 8001;
UdpServerHelper server1 (port1);
//UdpServerHelper server2 (port2);
ApplicationContainer apps;
apps = server1.Install (n.Get (1));
//apps = server2.Install (n.Get (2));
apps.Start (Seconds (1.0));
apps.Stop (Seconds (10.0));
// Create one UdpClient application to send UDP datagrams from node zero to
// node one.
uint32_t MaxPacketSize = 1024;
Time interPacketInterval = Seconds (interval);
uint32_t maxPacketCount = 320;
UdpClientHelper client1 (i.GetAddress (1), port1);
//UdpClientHelper client2 (i2.GetAddress (1), port2);
client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
client1.SetAttribute ("Interval", TimeValue (interPacketInterval));
client1.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
/*client2.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
client2.SetAttribute ("Interval", TimeValue (interPacketInterval));
client2.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));*/
apps = client1.Install (n.Get (0));
//apps = client2.Install (n.Get (0));
apps.Start (Seconds (2.0));
apps.Stop (Seconds (10.0));
p2p.EnablePcap("lab-1", dev, false);
// Calculate Throughput using Flowmonitor
FlowMonitorHelper flowmon;
Ptr<FlowMonitor> monitor = flowmon.InstallAll();
// Now, do the actual simulation.
NS_LOG_INFO ("Run Simulation.");
Simulator::Stop (Seconds(11.0));
Simulator::Run ();
monitor->CheckForLostPackets ();
Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier ( ));
std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end(); ++i)
{
Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
std::cout<<" t.sourceAddress="<<t.sourceAddress<<"t.destinationAddress="<<t.destinationAddress<<" Source port:"<<t.sourcePort<<" "<<"Destination port:"<<t.destinationPort<<"\n";
if ((t.sourceAddress=="10.1.1.1" && t.destinationAddress == "10.1.1.2"))
{
std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " <<t.destinationAddress << ")\n";
std::cout << " Tx Bytes: " << i->second.txBytes << "\n";
std::cout << " Rx Bytes: " << i->second.rxBytes << "\n";
std::cout << " Tx Packets: " << i->second.txPackets << "\n";
std::cout << " Rx Packets: " << i->second.rxPackets << "\n";
std::cout << " Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000 << "Mbps\n";
}
}
monitor->SerializeToXmlFile("lab-1.flowmon", true, true);
Simulator::Destroy ();
NS_LOG_INFO ("Done.");
}
