/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Aditya Katapadi Kamath <akamath1997@gmail.com>
 */

/** Network topology
*    800kb/s, 2ms
* n0--------------|
*                 |
*    800kb/s, 2ms |                     
* n1--------------|                    
*                 |   56kbps/s, 20ms  
*                 n4------------------n5
*    800kb/s, 5s  |                      
* n2--------------|                    
*                 |
*    800kb/s, 5s  |
* n3--------------|
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DRRExample");

uint32_t checkTimes;
double avgQueueDiscSize;

// The times
double global_start_time;
double global_stop_time;
double sink_start_time;
double sink_stop_time;
double client_start_time;
double client_stop_time;

NodeContainer n0n4;
NodeContainer n1n4;
NodeContainer n2n4;
NodeContainer n3n4;
NodeContainer n4n5;

Ipv4InterfaceContainer i0i4;
Ipv4InterfaceContainer i1i4;
Ipv4InterfaceContainer i2i4;
Ipv4InterfaceContainer i3i4;
Ipv4InterfaceContainer i4i5;

std::stringstream filePlotQueueDisc;
std::stringstream filePlotQueueDiscAvg;


void
CheckQueueDiscSize (Ptr<QueueDisc> queue)
{
  uint32_t qSize = StaticCast<DRRQueueDisc> (queue)->GetNBytes ();

  avgQueueDiscSize += qSize;
  checkTimes++;

  // check queue disc size every 1/100 of a second
  Simulator::Schedule (Seconds (0.01), &CheckQueueDiscSize, queue);

  std::ofstream fPlotQueueDisc (filePlotQueueDisc.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDisc << Simulator::Now ().GetSeconds () << " " << qSize << std::endl;
  fPlotQueueDisc.close ();

  std::ofstream fPlotQueueDiscAvg (filePlotQueueDiscAvg.str ().c_str (), std::ios::out | std::ios::app);
  fPlotQueueDiscAvg << Simulator::Now ().GetSeconds () << " " << avgQueueDiscSize / checkTimes << std::endl;
  fPlotQueueDiscAvg.close ();
}

void
BuildAppsTest ()
{
  // SINK is in the right side
  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (n4n5.Get (1));
  sinkApp.Start (Seconds (sink_start_time));
  sinkApp.Stop (Seconds (sink_stop_time));

  // Connection one
  // Clients are in left side
  /*
   * Create the OnOff applications to send TCP to the server
   * onoffhelper is a client that send data to TCP destination
  */
  // Two connections with 1000 byte packets
  OnOffHelper clientHelper1 ("ns3::TcpSocketFactory", Address ());
  clientHelper1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper1.SetAttribute ("PacketSize", UintegerValue (1000));
  clientHelper1.SetAttribute ("DataRate", DataRateValue (DataRate ("800kb/s")));

  // Connection two
  OnOffHelper clientHelper2 ("ns3::TcpSocketFactory", Address ());
  clientHelper2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper2.SetAttribute ("PacketSize", UintegerValue (1000));
  clientHelper2.SetAttribute ("DataRate", DataRateValue (DataRate ("800kb/s")));

  // Two connections with 40 byte packets
  // Connection three
  OnOffHelper clientHelper3 ("ns3::TcpSocketFactory", Address ());
  clientHelper3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper3.SetAttribute ("PacketSize", UintegerValue (40));
  clientHelper3.SetAttribute ("DataRate", DataRateValue (DataRate ("800kb/s")));

  // Connection four
  OnOffHelper clientHelper4 ("ns3::TcpSocketFactory", Address ());
  clientHelper4.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  clientHelper4.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  clientHelper4.SetAttribute ("PacketSize", UintegerValue (40));
  clientHelper4.SetAttribute ("DataRate", DataRateValue (DataRate ("800kb/s")));

  ApplicationContainer clientApps1;
  AddressValue remoteAddress (InetSocketAddress (i4i5.GetAddress (1), port));
  clientHelper1.SetAttribute ("Remote", remoteAddress);
  clientApps1.Add (clientHelper1.Install (n0n4.Get (0)));
  clientApps1.Start (Seconds (client_start_time));
  clientApps1.Stop (Seconds (client_stop_time));

  ApplicationContainer clientApps2;
  clientHelper2.SetAttribute ("Remote", remoteAddress);
  clientApps2.Add (clientHelper2.Install (n1n4.Get (0)));
  clientApps2.Start (Seconds (client_start_time));
  clientApps2.Stop (Seconds (client_stop_time));
  
  ApplicationContainer clientApps3;
  clientHelper3.SetAttribute ("Remote", remoteAddress);
  clientApps3.Add (clientHelper3.Install (n2n4.Get (0)));
  clientApps3.Start (Seconds (client_start_time));
  clientApps3.Stop (Seconds (client_stop_time));

  ApplicationContainer clientApps4;
  clientHelper4.SetAttribute ("Remote", remoteAddress);
  clientApps4.Add (clientHelper4.Install (n3n4.Get (0)));
  clientApps4.Start (Seconds (client_start_time));
  clientApps4.Stop (Seconds (client_stop_time));
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("DRRQueueDisc", LOG_LEVEL_INFO);

  std::string DRRLinkDataRate = "56kbps";
  std::string DRRLinkDelay = "20ms";

  std::string pathOut;
  bool writeForPlot = false;
  bool writePcap = false;
  bool flowMonitor = false;

  bool printDRRStats = true;

  global_start_time = 0.0;
  sink_start_time = global_start_time;
  client_start_time = global_start_time + 1;
  global_stop_time = 2002;
  sink_stop_time = global_stop_time + 1.0;
  client_stop_time = global_stop_time - 1.0;

  // Configuration and command line parameter parsing
  // Will only save in the directory if enable opts below
  pathOut = "."; // Current directory
  CommandLine cmd;
  cmd.AddValue ("pathOut", "Path to save results from --writeForPlot/--writePcap/--writeFlowMonitor", pathOut);
  cmd.AddValue ("writeForPlot", "<0/1> to write results for plot (gnuplot)", writeForPlot);
  cmd.AddValue ("writePcap", "<0/1> to write results in pcapfile", writePcap);
  cmd.AddValue ("writeFlowMonitor", "<0/1> to enable Flow Monitor and write their results", flowMonitor);

  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes");
  NodeContainer c;
  c.Create (6);
  Names::Add ( "N0", c.Get (0));
  Names::Add ( "N1", c.Get (1));
  Names::Add ( "N2", c.Get (2));
  Names::Add ( "N3", c.Get (3));
  Names::Add ( "N4", c.Get (4));
  Names::Add ( "N5", c.Get (5));
  n0n4 = NodeContainer (c.Get (0), c.Get (4));
  n1n4 = NodeContainer (c.Get (1), c.Get (4));
  n2n4 = NodeContainer (c.Get (2), c.Get (4));
  n3n4 = NodeContainer (c.Get (3), c.Get (4));
  n4n5 = NodeContainer (c.Get (4), c.Get (5));

  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
  // 42 = headers size
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000 - 42));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (1));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (false));

  // DRR params
  NS_LOG_INFO ("Set DRR params");
  Config::SetDefault ("ns3::DRRQueueDisc::Flows", UintegerValue (10));
  Config::SetDefault ("ns3::DRRQueueDisc::MaxSize", StringValue ("40p"));
  Config::SetDefault ("ns3::DRRQueueDisc::MeanPktSize", UintegerValue (1000 - 42));

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  internet.Install (c);

  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));

  TrafficControlHelper tchDRR;
  tchDRR.SetRootQueueDisc ("ns3::DRRQueueDisc");

  tchDRR.AddPacketFilter (handle,"ns3::DRRIpv4PacketFilter");
  tchDRR.AddPacketFilter (handle,"ns3::DRRIpv6PacketFilter");

  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;

  NetDeviceContainer devn0n4;
  NetDeviceContainer devn1n4;
  NetDeviceContainer devn2n4;
  NetDeviceContainer devn3n4;
  NetDeviceContainer devn4n5;

  QueueDiscContainer queueDiscs;

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("800kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  devn0n4 = p2p.Install (n0n4);
  tchPfifo.Install (devn0n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("800kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  devn1n4 = p2p.Install (n1n4);
  tchPfifo.Install (devn1n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("800kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5s"));
  devn2n4 = p2p.Install (n2n4);
  tchPfifo.Install (devn2n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("800kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("5s"));
  devn3n4 = p2p.Install (n3n4);
  tchPfifo.Install (devn3n4);

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue (DRRLinkDataRate));
  p2p.SetChannelAttribute ("Delay", StringValue (DRRLinkDelay));
  devn4n5 = p2p.Install (n4n5);
  // only backbone link has DRR queue disc
  queueDiscs = tchDRR.Install (devn4n5);

  NS_LOG_INFO ("Assign IP Addresses");
  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  i0i4 = ipv4.Assign (devn0n4);

  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  i1i4 = ipv4.Assign (devn1n4);

  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  i2i4 = ipv4.Assign (devn2n4);

  ipv4.SetBase ("10.1.4.0", "255.255.255.0");
  i3i4 = ipv4.Assign (devn3n4);

  ipv4.SetBase ("10.1.5.0", "255.255.255.0");
  i4i5 = ipv4.Assign (devn4n5);

  // Set up the routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BuildAppsTest ();

  if (writePcap)
    {
      PointToPointHelper ptp;
      std::stringstream stmp;
      stmp << pathOut << "/DRR";
      ptp.EnablePcapAll (stmp.str ().c_str ());
    }

  Ptr<FlowMonitor> flowmon;
  if (flowMonitor)
    {
      FlowMonitorHelper flowmonHelper;
      flowmon = flowmonHelper.InstallAll ();
    }

  if (writeForPlot)
    {
      filePlotQueueDisc << pathOut << "/" << "DRR-queue-disc.plotme";
      filePlotQueueDiscAvg << pathOut << "/" << "DRR-queue-disc_avg.plotme";

      remove (filePlotQueueDisc.str ().c_str ());
      remove (filePlotQueueDiscAvg.str ().c_str ());
      Ptr<QueueDisc> queue = queueDiscs.Get (0);
      Simulator::ScheduleNow (&CheckQueueDiscSize, queue);
    }

  Simulator::Stop (Seconds (sink_stop_time));
  Simulator::Run ();

  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();

  if (flowMonitor)
    {
      std::stringstream stmp;
      stmp << pathOut << "/DRR.flowmon";

      flowmon->SerializeToXmlFile (stmp.str ().c_str (), false, false);
    }

  if (printDRRStats)
    {
      std::cout << "*** DRR stats from Node 4 queue ***" << std::endl;
      //std::cout << "\t " << st.GetNDroppedPackets (DRRQueueDisc::UNFORCED_DROP)
      //        << " drops due to prob mark" << std::endl;
      std::cout << "\t " << st.GetNDroppedPackets (DRRQueueDisc::OVERLIMIT_DROP)
                << " drops due to queue limits" << std::endl;
    }

  Simulator::Destroy ();

  return 0;
}
