/*
 * This script simulates light TCP traffic for DRR evaluation
 * Authors: Viyom Mittal and Mohit P. Tahiliani
 * Wireless Information Networking Group
 * NITK Surathkal, Mangalore, India
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <fstream>
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include  <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DRRTests");

std::stringstream filePlotQueue;
std::stringstream filePlotQueueDisc;
std::stringstream filePlotQueueDiscAvg;

uint32_t checkTimes;
double avgQueueDiscSize;

void
CheckQueueSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = StaticCast<DRRQueueDisc> (queue)->GetNBytes ();

  avgQueueDiscSize += qSize;
  checkTimes++;

  // check queue size every 1/100 of a second
  Simulator::Schedule (Seconds (0.1), &CheckQueueSize, queue);

  std::ofstream fPlotQueueDisc (filePlotQueueDisc.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDisc << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueueDisc.close ();

  std::ofstream fPlotQueueDiscAvg (filePlotQueueDiscAvg.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDiscAvg << Simulator::Now ().GetSeconds () << " " << avgQueueDiscSize / checkTimes << std::endl;
  fPlotQueueDiscAvg.close ();
}

int main (int argc, char *argv[])
{
  bool printDRRStats = true;
  bool  isPcapEnabled = true;
  float startTime = 0.0;
  float simDuration = 101;      // in seconds
  std::string  pathOut = ".";
  bool writeForPlot = true;
  std::string pcapFileName = "second-bulksend.pcap";

  float stopTime = startTime + simDuration;

  CommandLine cmd;
  cmd.Parse (argc,argv);

//  LogComponentEnable ("DRRQueueDisc", LOG_LEVEL_INFO);

  std::string bottleneckBandwidth = "10Mbps";
  std::string bottleneckDelay = "50ms";

  std::string accessBandwidth = "10Mbps";
  std::string accessDelay = "5ms";

  NodeContainer source;
  source.Create (50);

  NodeContainer gateway;
  gateway.Create (2);

  NodeContainer sink;
  sink.Create (1);

  //Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue (13));
 // Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (50));

  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue(Seconds (0)));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocketBase::LimitedTransmit", BooleanValue (false));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));
  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (true));

  NS_LOG_INFO ("Set DRR params");
  Config::SetDefault ("ns3::DRRQueueDisc::ByteLimit", UintegerValue (100 * 1024));
  Config::SetDefault ("ns3::DRRQueueDisc::Flows", UintegerValue (1024));

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.InstallAll ();

  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));

  TrafficControlHelper tchDRR;
  tchDRR.SetRootQueueDisc ("ns3::DRRQueueDisc");
  tchDRR.AddPacketFilter (handle, "ns3::DRRIpv4PacketFilter");

// Create and configure access link and bottleneck link
  PointToPointHelper accessLink;
  accessLink.SetQueue ("ns3::DropTailQueue");
  accessLink.SetDeviceAttribute ("DataRate", StringValue (accessBandwidth));
  accessLink.SetChannelAttribute ("Delay", StringValue (accessDelay));

  NetDeviceContainer devices[50];
  for (int i = 0; i < 50; i++)
    {
      devices[i] = accessLink.Install (source.Get (i), gateway.Get (0));
      tchPfifo.Install (devices[i]);
    }

  NetDeviceContainer devices_sink;
  devices_sink = accessLink.Install (gateway.Get (1), sink.Get (0));
  tchPfifo.Install (devices_sink);

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bottleneckBandwidth));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  bottleneckLink.SetQueue ("ns3::DropTailQueue");

  NetDeviceContainer devices_gateway;
  devices_gateway = bottleneckLink.Install (gateway.Get (0), gateway.Get (1));
  // only backbone link has DRR queue disc
  QueueDiscContainer queueDiscs = tchDRR.Install (devices_gateway);

  NS_LOG_INFO ("Assign IP Addresses");

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");

  // Configure the source and sink net devices
  // and the channels between the source/sink and the gateway
  //Ipv4InterfaceContainer sink_Interfaces;
  Ipv4InterfaceContainer interfaces[50];
  Ipv4InterfaceContainer interfaces_sink;
  Ipv4InterfaceContainer interfaces_gateway;

  for (int i = 0; i < 50; i++)
    {
      address.NewNetwork ();
      interfaces[i] = address.Assign (devices[i]);
    }

  address.NewNetwork ();
  interfaces_gateway = address.Assign (devices_gateway);

  address.NewNetwork ();
  interfaces_sink = address.Assign (devices_sink);

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  // Configure application
  //AddressValue remoteAddress (InetSocketAddress (sink_Interfaces.GetAddress (0, 0), port));
  AddressValue remoteAddress (InetSocketAddress (interfaces_sink.GetAddress (1), port));

  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", remoteAddress);
  ftp.SetAttribute ("SendSize", UintegerValue (1000));

  ApplicationContainer sourceApp = ftp.Install (source);
  sourceApp.Start (Seconds (0));
  sourceApp.Stop (Seconds (stopTime - 1));

  sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ApplicationContainer sinkApp = sinkHelper.Install (sink);
  sinkApp.Start (Seconds (0));
  sinkApp.Stop (Seconds (stopTime));

  if (writeForPlot)
    {
      filePlotQueue << pathOut << "/" << "DRR-queue2.plotme";
      remove (filePlotQueue.str ().c_str ());
      Ptr<QueueDisc> queue = queueDiscs.Get (0);
      Simulator::ScheduleNow (&CheckQueueSize, queue);
    }

  if (isPcapEnabled)
    {
      bottleneckLink.EnablePcap (pcapFileName,gateway,false);
    }

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> allMon;
  allMon = flowmon.InstallAll ();
  flowmon.SerializeToXmlFile ("second-bulksend.xml", true, true);

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();

  if (printDRRStats)
    {
      DRRQueueDisc::Stats st = StaticCast<DRRQueueDisc> (queueDiscs.Get (0))->GetStats ();
      std::cout << "*** DRR stats from bottleneck queue ***" << std::endl;
      std::cout << "\t " << st.GetNDroppedPackets (DRRQueueDisc::UNCLASSIFIED_DROP)
                << " drops because packet could not be classified by any filter" << std::endl;
      std::cout << "\t " << st.GetNDroppedPackets (DRRQueueDisc::OVERLIMIT_DROP)
                << " drops because of byte limit overflow" << std::endl;
    }

  Simulator::Destroy ();
  return 0;
}
