/*
 * This script tests the classify of packet filters
 * Author: Aditya Katapadi Kamath
 * NITK Surathkal, Mangalore, India
*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include <string>
#include <vector>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SFQTests");

std::vector<uint32_t> received (20, 0);

void
AddPacket (Ptr<SfqQueueDisc> queue)
{
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  rand->SetAttribute ("Min", DoubleValue (0));
  rand->SetAttribute ("Max", DoubleValue (39));
  
  // Insert 4 packets
  for(uint32_t i = 0; i < 4; ++i)
    {
      uint32_t node = rand->GetInteger ();
      if(node >= 19)
        node = 19;
      Ptr<Packet> p = Create<Packet> (100);
      Ipv4Header hdr;
      hdr.SetPayloadSize (100);
      hdr.SetProtocol (7);
      Address dest;
      // For simplicity, just set the addresses to that of the chosen value
      hdr.SetSource (Ipv4Address (node));
      hdr.SetDestination (Ipv4Address (node));
      Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
      queue->Enqueue (item);
    }

  // Remove 1 packet
  Ptr<Ipv4QueueDiscItem> item = DynamicCast<Ipv4QueueDiscItem> (queue->Dequeue ());
  Ipv4Header hdr = item->GetHeader ();
  ++received[hdr.GetSource ().Get ()];
}


int main (int argc, char *argv[])
{
  LogComponentEnable ("SFQTests", LOG_LEVEL_INFO);
  uint32_t noOfPackets = 10000;
  bool outputToFile = true;
  std::string fileName = "scenario-two.txt";
  std::ofstream file (fileName.c_str ());
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("160p")), "Flows", UintegerValue (160), "FlowLimit", UintegerValue(5));
  Ptr<SfqIpv6PacketFilter> ipv6Filter = CreateObjectWithAttributes<SfqIpv6PacketFilter> ("PerturbationTime", TimeValue (MilliSeconds (1000)));
  Ptr<SfqIpv4PacketFilter> ipv4Filter = CreateObjectWithAttributes<SfqIpv4PacketFilter> ("PerturbationTime", TimeValue (MilliSeconds (1000)));
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);
  // Set quantum to size of a single packet
  queueDisc->SetQuantum (120);
  queueDisc->Initialize ();

  // Insert packets every millisecond
  for (uint32_t i = 0; i < noOfPackets; ++i)
    {
        Simulator::Schedule (MilliSeconds (i), &AddPacket, queueDisc);           
    }
  Simulator::Stop(MilliSeconds (noOfPackets));
  Simulator::Run();
  
  // Output results
  for (uint32_t i = 0; i < 20; ++i)
    {
      NS_LOG_DEBUG ("For queue " << i << " number of packets received is " << received[i]);
      if (outputToFile)
        {
          file << i << "\t" << received[i] << "\n";
        }
    }
  Simulator::Destroy();
  return 0;
}
