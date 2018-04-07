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

NS_LOG_COMPONENT_DEFINE ("DRRTests");

int main (int argc, char *argv[])
{
  LogComponentEnable ("DRRTests", LOG_LEVEL_INFO);
  uint32_t minQueueSize = 2;
  uint32_t maxQueueSize = 512;
  uint32_t noOfPackets = 10000;
  bool outputToFile = true;
  std::string fileName = "scenario-one-DRR.txt";
  std::ofstream file (fileName.c_str ());
  Ptr<DRRIpv4PacketFilter> filter = CreateObject<DRRIpv4PacketFilter> ();
  
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  rand->SetAttribute ("Min", DoubleValue (0));
  rand->SetAttribute ("Max", DoubleValue (UINT32_MAX));
  
  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetProtocol (7);
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;

  // Test for all possible queue sizes
  for (uint32_t queueSize = minQueueSize; queueSize <= maxQueueSize; ++queueSize)
    {
      std::vector<uint32_t> queue (queueSize, 0);
      // Insert noOfPackets into queue
      for (uint32_t i = 0; i < noOfPackets; ++i)
        {
            // Set source and destination addresses to random values
            hdr.SetSource (Ipv4Address (rand->GetInteger ()));
            hdr.SetDestination (Ipv4Address (rand->GetInteger ()));
            Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
            // Add to corresponding queue
            ++queue[filter->Classify (item) % queueSize];            
        }
      double deviation = 0;
      for (uint32_t i = 0; i < queueSize; ++i)
        {
          deviation += abs((double)queue[i] - ((double)noOfPackets / (double)queueSize));
        }
      deviation /= sqrt((double)queueSize);
      NS_LOG_DEBUG ("For queue size " << queueSize << " the standard deviation is " << deviation);
      if (outputToFile)
        {
          file << queueSize << "\t" << deviation << "\n";
        }
    }

  return 0;
}
