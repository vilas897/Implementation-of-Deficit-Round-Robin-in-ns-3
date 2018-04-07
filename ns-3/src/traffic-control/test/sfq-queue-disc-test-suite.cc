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
 *          A Tarun Karthik <tarunkarthik999@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/sfq-queue-disc.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/ipv6-queue-disc-item.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

using namespace ns3;

/**
 * This class tests packets for which there is no suitable filter
 */
class SfqQueueDiscNoSuitableFilter : public TestCase
{
public:
  SfqQueueDiscNoSuitableFilter ();
  virtual ~SfqQueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

SfqQueueDiscNoSuitableFilter::SfqQueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter")
{
}

SfqQueueDiscNoSuitableFilter::~SfqQueueDiscNoSuitableFilter ()
{
}

void
SfqQueueDiscNoSuitableFilter::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be placed into a seperate flow queue
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("4p")), "Flows", UintegerValue (2));
  Ptr<SfqIpv4PacketFilter> filter = CreateObject<SfqIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<Ipv6QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 1, "One flow queue should have been created");

  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 1, "One flow queue should have been created");

  Simulator::Destroy ();
}

/**
 * This class tests the IP flows separation and the packet limit
 */
class SfqQueueDiscIPFlowsSeparationAndPacketLimit : public TestCase
{
public:
  SfqQueueDiscIPFlowsSeparationAndPacketLimit ();
  virtual ~SfqQueueDiscIPFlowsSeparationAndPacketLimit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr);
};

SfqQueueDiscIPFlowsSeparationAndPacketLimit::SfqQueueDiscIPFlowsSeparationAndPacketLimit ()
  : TestCase ("Test IP flows separation and packet limit")
{
}

SfqQueueDiscIPFlowsSeparationAndPacketLimit::~SfqQueueDiscIPFlowsSeparationAndPacketLimit ()
{
}

void
SfqQueueDiscIPFlowsSeparationAndPacketLimit::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SfqQueueDiscIPFlowsSeparationAndPacketLimit::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("8p")), "Flows", UintegerValue (4));
  Ptr<SfqIpv6PacketFilter> ipv6Filter = CreateObject<SfqIpv6PacketFilter> ();
  Ptr<SfqIpv4PacketFilter> ipv4Filter = CreateObject<SfqIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  // Try dequeuing from empty QueueDisc
  Ptr<QueueDiscItem> item;
  item = queueDisc->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "Verifying Dequeue of empty queue returns 0");

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add two packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");

  // Add three packets to the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.2"));
  hdr.SetDestination (Ipv4Address ("10.10.1.7"));
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  // Add a fourth packet to second flow, which gets dropped
  // This is dropped because the second flow is exceeding its
  // fair share of two packets (Fairshare = PacketLimit / Flows)
  // and, since the remaining number of packet slots is less
  // than the total number of possible flows, the second flow
  // is not allowed to have any more packets
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the TCP flows separation
 */
class SfqQueueDiscTCPFlowsSeparation : public TestCase
{
public:
  SfqQueueDiscTCPFlowsSeparation ();
  virtual ~SfqQueueDiscTCPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr);
};

SfqQueueDiscTCPFlowsSeparation::SfqQueueDiscTCPFlowsSeparation ()
  : TestCase ("Test TCP flows separation")
{
}

SfqQueueDiscTCPFlowsSeparation::~SfqQueueDiscTCPFlowsSeparation ()
{
}

void
SfqQueueDiscTCPFlowsSeparation::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (tcpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
SfqQueueDiscTCPFlowsSeparation::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObject<SfqQueueDisc> ();
  Ptr<SfqIpv6PacketFilter> ipv6Filter = CreateObject<SfqIpv6PacketFilter> ();
  Ptr<SfqIpv4PacketFilter> ipv4Filter = CreateObject<SfqIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (6);

  TcpHeader tcpHdr;
  tcpHdr.SetSourcePort (7);
  tcpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.3"));
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.4"));
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  hdr.SetSource (Ipv4Address ("10.10.1.5"));
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the UDP flows separation
 */
class SfqQueueDiscUDPFlowsSeparation : public TestCase
{
public:
  SfqQueueDiscUDPFlowsSeparation ();
  virtual ~SfqQueueDiscUDPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

SfqQueueDiscUDPFlowsSeparation::SfqQueueDiscUDPFlowsSeparation ()
  : TestCase ("Test UDP flows separation")
{
}

SfqQueueDiscUDPFlowsSeparation::~SfqQueueDiscUDPFlowsSeparation ()
{
}

void
SfqQueueDiscUDPFlowsSeparation::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (udpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
SfqQueueDiscUDPFlowsSeparation::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObject<SfqQueueDisc> ();
  Ptr<SfqIpv6PacketFilter> ipv6Filter = CreateObject<SfqIpv6PacketFilter> ();
  Ptr<SfqIpv4PacketFilter> ipv4Filter = CreateObject<SfqIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (17);

  UdpHeader udpHdr;
  udpHdr.SetSourcePort (7);
  udpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.3"));
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.4"));
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  hdr.SetSource (Ipv4Address ("10.10.1.5"));
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the deficit per flow
 */
class SfqQueueDiscDeficit : public TestCase
{
public:
  SfqQueueDiscDeficit ();
  virtual ~SfqQueueDiscDeficit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr);
};

SfqQueueDiscDeficit::SfqQueueDiscDeficit ()
  : TestCase ("Test credits and flows status")
{
}

SfqQueueDiscDeficit::~SfqQueueDiscDeficit ()
{
}

void
SfqQueueDiscDeficit::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SfqQueueDiscDeficit::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ();
  Ptr<SfqIpv6PacketFilter> ipv6Filter = CreateObject<SfqIpv6PacketFilter> ();
  Ptr<SfqIpv4PacketFilter> ipv4Filter = CreateObject<SfqIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (90);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  Ptr<SfqFlow> flow1 = StaticCast<SfqFlow> (queueDisc->GetQueueDiscClass (0));
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must be in the list of queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  // the deficit for the first flow becomes 90 - (100+20) = -30
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), -30, "unexpected deficit for the first flow");

  // Add two packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must still be in the list of queues");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.10"));
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the second flow queue");
  Ptr<SfqFlow> flow2 = StaticCast<SfqFlow> (queueDisc->GetQueueDiscClass (1));
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the second flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_IN_USE, "the second flow must be in the list of queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), 60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must be in the list of queues");
  // the second flow has a negative deficit (-30) and is still in the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), -30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_IN_USE, "the second flow must be in the list of queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (60-(100+20)= -60) and stays in the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), -60, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must be in the list of queues");
  // the second flow got a quantum of deficit (-30+90=60) and has been moved to the end of the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), 60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_IN_USE, "the second flow must be in the list of queues");

  // Dequeue a packet (from the second flow, as the first flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), 30, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must be in the list of queues");
  // the second flow has a negative deficit (60-(100+20)= -60)
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), -60, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_IN_USE, "the second flow must be in the list of queues");

  // Dequeue a packet (from the first flow, as the second flow has a negative deficit)
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");
  // the first flow has a negative deficit (30-(100+20)= -90)
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), -90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_IN_USE, "the first flow must be in the list of queues");
  // the second flow got a quantum of deficit (-60+90=30) and has been moved to the end of the list of queues
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_IN_USE, "the second flow must be in the list of queues");

  // Dequeue a packet
  queueDisc->Dequeue ();
  /*
   * The first flow is in the list of queues but has a negative deficit, thus it gets a quantun
   * of deficit (-90+90=0) and is moved to the end of the list of queues. Then, the second flow (which has a
   * positive deficit) is selected, but the second flow is empty and thus it is set to empty. The first flow is
   * reconsidered, but it has a null deficit, hence it gets another quantum of deficit (0+90=90). Then, the first
   * flow is reconsidered again, now it has a positive deficit and hence it is selected. But, it is empty and
   * therefore is set to empty, too.
   */
  NS_TEST_ASSERT_MSG_EQ (flow1->GetAllot (), 90, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), SfqFlow::SFQ_EMPTY_SLOT, "the first flow must be inactive");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetAllot (), 30, "unexpected deficit for the second flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), SfqFlow::SFQ_EMPTY_SLOT, "the second flow must be inactive");

  Simulator::Destroy ();
}

/**
 * This class tests whether packets get hashed into different flows after perturbation takes place
 */
class SfqQueueDiscPerturbationHashChange : public TestCase
{
public:
  SfqQueueDiscPerturbationHashChange ();
  virtual ~SfqQueueDiscPerturbationHashChange ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr);
};

SfqQueueDiscPerturbationHashChange::SfqQueueDiscPerturbationHashChange ()
  : TestCase ("Test hash perturbation changes")
{
}

SfqQueueDiscPerturbationHashChange::~SfqQueueDiscPerturbationHashChange ()
{
}

void
SfqQueueDiscPerturbationHashChange::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queue->GetNQueueDiscClasses (), 2, "Two flow queues should have been created");
}

void
SfqQueueDiscPerturbationHashChange::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be placed into a seperate flow queue
  Ptr<SfqQueueDisc> queueDisc = CreateObject<SfqQueueDisc> ();
  Ptr<SfqIpv4PacketFilter> filter = CreateObjectWithAttributes<SfqIpv4PacketFilter> ("PerturbationTime", TimeValue (MilliSeconds (100)));
  queueDisc->AddPacketFilter (filter);

  queueDisc->SetQuantum (1500);
  queueDisc->Initialize ();

  Ipv4Header ipv4Header;
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipv4Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 1, "One flow queue should have been created");

  Simulator::Schedule (MilliSeconds (150), &SfqQueueDiscPerturbationHashChange::AddPacket, this, queueDisc, ipv4Header);
  Simulator::Stop (MilliSeconds (155));
  Simulator::Run ();
}

/**
 * This class tests packets for which there is no suitable filter
 */
class SfqNs2QueueDiscNoSuitableFilter : public TestCase
{
public:
  SfqNs2QueueDiscNoSuitableFilter ();
  virtual ~SfqNs2QueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

SfqNs2QueueDiscNoSuitableFilter::SfqNs2QueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter for ns-2 implementation")
{
}

SfqNs2QueueDiscNoSuitableFilter::~SfqNs2QueueDiscNoSuitableFilter ()
{
}

void
SfqNs2QueueDiscNoSuitableFilter::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be placed into a seperate flow queue
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("4p")), "Flows", UintegerValue (2), "Ns2Style", BooleanValue (true));
  Ptr<SfqNs2Ipv4PacketFilter> filter = CreateObject<SfqNs2Ipv4PacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  queueDisc->Initialize ();

  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<Ipv6QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 1, "One flow queue should have been created");

  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 1, "One flow queue should have been created");

  Simulator::Destroy ();
}

/**
 * This class tests the IP flows separation and the packet limit
 */
class SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit : public TestCase
{
public:
  SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit ();
  virtual ~SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr);
};

SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit::SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit ()
  : TestCase ("Test IP flows separation and packet limit for ns-2")
{
}

SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit::~SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit ()
{
}

void
SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("MaxSize", QueueSizeValue (QueueSize ("12p")), "Flows", UintegerValue (4), "Ns2Style", BooleanValue (true));
  Ptr<SfqNs2Ipv6PacketFilter> ipv6Filter = CreateObject<SfqNs2Ipv6PacketFilter> ();
  Ptr<SfqNs2Ipv4PacketFilter> ipv4Filter = CreateObject<SfqNs2Ipv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->Initialize ();

  // Try dequeuing from empty QueueDisc
  Ptr<QueueDiscItem> item;
  item = queueDisc->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "Verifying Dequeue of empty queue returns 0");

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add two packets from the first flow
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");

  // Add three packets to the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.2"));
  hdr.SetDestination (Ipv4Address ("10.10.1.7"));
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  /*
   * Add a fourth packet to second flow, which gets dropped
   * This is dropped because the second flow is exceeding its
   * fair share of two packets (Fairshare = PacketLimit / Flows)
   * and, since the remaining number of packet slots is less
   * than the total number of possible flows, the second flow
   * is not allowed to have any more packets
   */
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the TCP flows separation
 */
class SfqNs2QueueDiscTCPFlowsSeparation : public TestCase
{
public:
  SfqNs2QueueDiscTCPFlowsSeparation ();
  virtual ~SfqNs2QueueDiscTCPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr);
};

SfqNs2QueueDiscTCPFlowsSeparation::SfqNs2QueueDiscTCPFlowsSeparation ()
  : TestCase ("Test TCP flows separation for ns-2 style")
{
}

SfqNs2QueueDiscTCPFlowsSeparation::~SfqNs2QueueDiscTCPFlowsSeparation ()
{
}

void
SfqNs2QueueDiscTCPFlowsSeparation::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (tcpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
SfqNs2QueueDiscTCPFlowsSeparation::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("Ns2Style", BooleanValue (true));
  Ptr<SfqNs2Ipv6PacketFilter> ipv6Filter = CreateObject<SfqNs2Ipv6PacketFilter> ();
  Ptr<SfqNs2Ipv4PacketFilter> ipv4Filter = CreateObject<SfqNs2Ipv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (6);

  TcpHeader tcpHdr;
  tcpHdr.SetSourcePort (7);
  tcpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.3"));
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.4"));
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  hdr.SetSource (Ipv4Address ("10.10.1.5"));
  AddPacket (queueDisc, hdr, tcpHdr);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

/**
 * This class tests the UDP flows separation
 */
class SfqNs2QueueDiscUDPFlowsSeparation : public TestCase
{
public:
  SfqNs2QueueDiscUDPFlowsSeparation ();
  virtual ~SfqNs2QueueDiscUDPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

SfqNs2QueueDiscUDPFlowsSeparation::SfqNs2QueueDiscUDPFlowsSeparation ()
  : TestCase ("Test UDP flows separation for ns-2 style")
{
}

SfqNs2QueueDiscUDPFlowsSeparation::~SfqNs2QueueDiscUDPFlowsSeparation ()
{
}

void
SfqNs2QueueDiscUDPFlowsSeparation::AddPacket (Ptr<SfqQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  p->AddHeader (udpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
SfqNs2QueueDiscUDPFlowsSeparation::DoRun (void)
{
  Ptr<SfqQueueDisc> queueDisc = CreateObjectWithAttributes<SfqQueueDisc> ("Ns2Style", BooleanValue (true));
  Ptr<SfqNs2Ipv6PacketFilter> ipv6Filter = CreateObject<SfqNs2Ipv6PacketFilter> ();
  Ptr<SfqNs2Ipv4PacketFilter> ipv4Filter = CreateObject<SfqNs2Ipv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (17);

  UdpHeader udpHdr;
  udpHdr.SetSourcePort (7);
  udpHdr.SetDestinationPort (27);

  // Add three packets from the first flow
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");

  // Add a packet from the second flow
  hdr.SetSource (Ipv4Address ("10.10.1.3"));
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  hdr.SetDestination (Ipv4Address ("10.10.1.4"));
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  hdr.SetSource (Ipv4Address ("10.10.1.5"));
  AddPacket (queueDisc, hdr, udpHdr);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (3)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the third flow queue");

  Simulator::Destroy ();
}

class SfqQueueDiscTestSuite : public TestSuite
{
public:
  SfqQueueDiscTestSuite ();
};

SfqQueueDiscTestSuite::SfqQueueDiscTestSuite ()
  : TestSuite ("sfq-queue-disc", UNIT)
{
  // Test cases for default implementation of SFQ
  AddTestCase (new SfqQueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new SfqQueueDiscIPFlowsSeparationAndPacketLimit, TestCase::QUICK);
  AddTestCase (new SfqQueueDiscTCPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new SfqQueueDiscUDPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new SfqQueueDiscDeficit, TestCase::QUICK);
  AddTestCase (new SfqQueueDiscPerturbationHashChange, TestCase::QUICK);
  // Test cases for ns-2 style implementation of SFQ
  AddTestCase (new SfqNs2QueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new SfqNs2QueueDiscIPFlowsSeparationAndPacketLimit, TestCase::QUICK);
  AddTestCase (new SfqNs2QueueDiscTCPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new SfqNs2QueueDiscUDPFlowsSeparation, TestCase::QUICK);
}

static SfqQueueDiscTestSuite SfqQueueDiscTestSuite;

