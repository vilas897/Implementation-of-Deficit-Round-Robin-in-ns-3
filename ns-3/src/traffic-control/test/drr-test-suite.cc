/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Authors: Akhil Udathu <akhilu077@gmail.com>
 *          Kaushik S Kalmady <kaushikskalmady@gmail.com>
 *          Vilas M <vilasnitk19@gmail.com>
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/drr-queue-disc.h"
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
class DRRQueueDiscNoSuitableFilter : public TestCase
{
public:
  DRRQueueDiscNoSuitableFilter ();
  virtual ~DRRQueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

DRRQueueDiscNoSuitableFilter::DRRQueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter")
{
}

DRRQueueDiscNoSuitableFilter::~DRRQueueDiscNoSuitableFilter ()
{
}

void
DRRQueueDiscNoSuitableFilter::DoRun (void)
{
  // Packets that cannot be classified by the available filters should be dropped
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ("ByteLimit", UintegerValue (1000));
  Ptr<DRRIpv4PacketFilter> filter = CreateObject<DRRIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (filter);

  // test 1: simple enqueue/dequeue with defaults, no drops
  //NS_TEST_EXPECT_MSG_EQ (queue->SetAttributeFailSafe ("Mode", mode), true,
  //                  "Verify that we can actually set the attribute Mode");
  queueDisc->SetQuantum (500);
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
 * This class tests the IP flows separation and the byte limit
 */


class DRRQueueDiscIPFlowsSeparationAndByteLimit : public TestCase
{
public:
  DRRQueueDiscIPFlowsSeparationAndByteLimit ();
  virtual ~DRRQueueDiscIPFlowsSeparationAndByteLimit ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr);
};

DRRQueueDiscIPFlowsSeparationAndByteLimit::DRRQueueDiscIPFlowsSeparationAndByteLimit ()
  : TestCase ("Test IP flows separation and bytelimit")
{
}

DRRQueueDiscIPFlowsSeparationAndByteLimit::~DRRQueueDiscIPFlowsSeparationAndByteLimit ()
{
}

void
DRRQueueDiscIPFlowsSeparationAndByteLimit::AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (500);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
DRRQueueDiscIPFlowsSeparationAndByteLimit::DoRun (void)
{
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ("ByteLimit", UintegerValue (2500));
  //Ptr<DRRIpv6PacketFilter> ipv6Filter = CreateObject<DRRIpv6PacketFilter> ();
  Ptr<DRRIpv4PacketFilter> ipv4Filter = CreateObject<DRRIpv4PacketFilter> ();
  //queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (600);
  queueDisc->Initialize ();

  // Dequeue from empty QueueDisc
  Ptr<QueueDiscItem> item;
  item = queueDisc->Dequeue ();
  NS_TEST_EXPECT_MSG_EQ ((item == 0), true, "Veryfying Dequeue on empty queue returns 0");

  Ipv4Header hdr;
  hdr.SetPayloadSize (500);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add three packets from the first flow

  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);

  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");

  // Add two packets from the second flow
  hdr.SetDestination (Ipv4Address ("10.10.1.7"));
  // Add the first packet
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the flow queue");
  // Add the second packet that causes one packet to be dropped from the fat flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}


/**
 * This class tests the TCP flows separation
 */

class DRRQueueDiscTCPFlowsSeparation : public TestCase
{
public:
  DRRQueueDiscTCPFlowsSeparation ();
  virtual ~DRRQueueDiscTCPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr);
};

DRRQueueDiscTCPFlowsSeparation::DRRQueueDiscTCPFlowsSeparation ()
  : TestCase ("Test TCP flows separation")
{
}

DRRQueueDiscTCPFlowsSeparation::~DRRQueueDiscTCPFlowsSeparation ()
{
}

void
DRRQueueDiscTCPFlowsSeparation::AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header ipHdr, TcpHeader tcpHdr)
{
  Ptr<Packet> p = Create<Packet> (500);
  p->AddHeader (tcpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
DRRQueueDiscTCPFlowsSeparation::DoRun (void)
{
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ("ByteLimit", UintegerValue (4000));
  //Ptr<DRRIpv6PacketFilter> ipv6Filter = CreateObject<DRRIpv6PacketFilter> ();
  Ptr<DRRIpv4PacketFilter> ipv4Filter = CreateObject<DRRIpv4PacketFilter> ();
  //queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (600);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (500);
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
  tcpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  tcpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, tcpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  tcpHdr.SetSourcePort (7);
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

class DRRQueueDiscUDPFlowsSeparation : public TestCase
{
public:
  DRRQueueDiscUDPFlowsSeparation ();
  virtual ~DRRQueueDiscUDPFlowsSeparation ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr);
};

DRRQueueDiscUDPFlowsSeparation::DRRQueueDiscUDPFlowsSeparation ()
  : TestCase ("Test UDP flows separation")
{
}

DRRQueueDiscUDPFlowsSeparation::~DRRQueueDiscUDPFlowsSeparation ()
{
}

void
DRRQueueDiscUDPFlowsSeparation::AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header ipHdr, UdpHeader udpHdr)
{
  Ptr<Packet> p = Create<Packet> (500);
  p->AddHeader (udpHdr);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, ipHdr);
  queue->Enqueue (item);
}

void
DRRQueueDiscUDPFlowsSeparation::DoRun (void)
{
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ("ByteLimit", UintegerValue (4000));
//  Ptr<DRRIpv6PacketFilter> ipv6Filter = CreateObject<DRRIpv6PacketFilter> ();
  Ptr<DRRIpv4PacketFilter> ipv4Filter = CreateObject<DRRIpv4PacketFilter> ();
  // queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (600);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (500);
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
  udpHdr.SetSourcePort (8);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 4, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");

  // Add a packet from the third flow
  udpHdr.SetDestinationPort (28);
  AddPacket (queueDisc, hdr, udpHdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 5, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (2)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the third flow queue");

  // Add two packets from the fourth flow
  udpHdr.SetSourcePort (7);
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
 * This class tests the deficit per flow for packets of variable size when enqueued to the same flow
 */

class DRRQueueDiscDeficitVariableSizeSameFlow : public TestCase
{
public:
  DRRQueueDiscDeficitVariableSizeSameFlow ();
  virtual ~DRRQueueDiscDeficitVariableSizeSameFlow ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr, uint32_t size);
};

DRRQueueDiscDeficitVariableSizeSameFlow::DRRQueueDiscDeficitVariableSizeSameFlow ()
  : TestCase ("Test credits and flows status")
{
}

DRRQueueDiscDeficitVariableSizeSameFlow::~DRRQueueDiscDeficitVariableSizeSameFlow ()
{
}

void
DRRQueueDiscDeficitVariableSizeSameFlow::AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr, uint32_t size)
{
  Ptr<Packet> p = Create<Packet> (size);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
DRRQueueDiscDeficitVariableSizeSameFlow::DoRun (void)
{
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ();
  //Ptr<DRRIpv6PacketFilter> ipv6Filter = CreateObject<DRRIpv6PacketFilter> ();
  Ptr<DRRIpv4PacketFilter> ipv4Filter = CreateObject<DRRIpv4PacketFilter> ();
  // queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (600);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (500);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr, 500);
  // Size of this is 500 + 20 bytes
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 520, "unexpected number of bytes in the first flow queue");

  Ptr<DRRFlow> flow1 = StaticCast<DRRFlow> (queueDisc->GetQueueDiscClass (0));
  //NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::ACTIVE, "the first flow must be in the list of active queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 0, "unexpected number of bytes in the first flow queue");
  // the deficit for the first flow becomes 600 - (500+20) = 80
  // But since there are no packets left, it is set back to 0
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 0, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::INACTIVE, "the first flow must be in the list of inactive queues");

  // Add 2 packets from the first flow
  AddPacket (queueDisc, hdr, 500);
  hdr.SetPayloadSize (400);
  AddPacket (queueDisc, hdr, 400);
  hdr.SetPayloadSize (600);
  AddPacket (queueDisc, hdr, 600);
  // Size of this is 500 + 20 bytes
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 3, "unexpected number of packets in the first flow queue");
  // Total bytes = 500 + 20 + 400 + 20 (header is 20 bytes long)
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 1560, "unexpected number of bytes in the first flow queue");

  flow1 = StaticCast<DRRFlow> (queueDisc->GetQueueDiscClass (0));
  //NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::ACTIVE, "the first flow must be in the list of active queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 1040, "unexpected number of bytes in the first flow queue");
  // the deficit for the first flow becomes 600 - (500+20) = 80
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 80, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::ACTIVE, "the first flow must be in the list of active queues since there is one packet left");

  //Dequeue second packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 620, "unexpected number of bytes in the first flow queue");
  // the deficit for the first flow becomes 80 + 600 - (400+20) = 260
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 260, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::ACTIVE, "the first flow must be in the list of active queues since there is one packet left");

  // dequeue last packet
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 0, "unexpected number of bytes in the first flow queue");
  // the deficit for the first flow becomes 260 + 600 - (600+20) = 280
  // But since there are no packets left, it is set back to 0
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 0, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::INACTIVE, "the first flow must be in the list of inactive queues");
  // Add two packets from the first flow

  Simulator::Destroy ();
}

class DRRQueueDiscDeficitVariableSizeDifferentFlow : public TestCase
{
public:
  DRRQueueDiscDeficitVariableSizeDifferentFlow ();
  virtual ~DRRQueueDiscDeficitVariableSizeDifferentFlow ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr, uint32_t size);
};

DRRQueueDiscDeficitVariableSizeDifferentFlow::DRRQueueDiscDeficitVariableSizeDifferentFlow ()
  : TestCase ("Test credits and flows status")
{
}

DRRQueueDiscDeficitVariableSizeDifferentFlow::~DRRQueueDiscDeficitVariableSizeDifferentFlow ()
{
}

void
DRRQueueDiscDeficitVariableSizeDifferentFlow::AddPacket (Ptr<DRRQueueDisc> queue, Ipv4Header hdr, uint32_t size)
{
  Ptr<Packet> p = Create<Packet> (size);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
DRRQueueDiscDeficitVariableSizeDifferentFlow::DoRun (void)
{
  Ptr<DRRQueueDisc> queueDisc = CreateObjectWithAttributes<DRRQueueDisc> ();
  //Ptr<DRRIpv6PacketFilter> ipv6Filter = CreateObject<DRRIpv6PacketFilter> ();
  Ptr<DRRIpv4PacketFilter> ipv4Filter = CreateObject<DRRIpv4PacketFilter> ();
  // queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->SetQuantum (600);
  queueDisc->Initialize ();

  Ipv4Header hdr;
  hdr.SetPayloadSize (500);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add 2 packet from the first flow
  AddPacket (queueDisc, hdr, 500);
  hdr.SetPayloadSize (600);
  AddPacket (queueDisc, hdr, 600);

  hdr.SetDestination (Ipv4Address ("10.10.1.3"));
  hdr.SetPayloadSize (800);
  AddPacket (queueDisc, hdr, 800);
  // Size of this is 500 + 20 bytes
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 2, "unexpected number of packets in the first flow queue");
  // Total bytes = 500 + 20 + 400 + 20 (header is 20 bytes long)
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 1140, "unexpected number of bytes in the first flow queue");

  Ptr<DRRFlow> flow1 = StaticCast<DRRFlow> (queueDisc->GetQueueDiscClass (0));
  Ptr<DRRFlow> flow2 = StaticCast<DRRFlow> (queueDisc->GetQueueDiscClass (1));
  //NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), static_cast<int32_t> (queueDisc->GetQuantum ()), "the deficit of the first flow must equal the quantum");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetStatus (), DRRFlow::ACTIVE, "the first flow must be in the list of active queues");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), DRRFlow::ACTIVE, "the second flow must be in the list of active queues");
  // Dequeue a packet
  queueDisc->Dequeue ();
  //First flow should now have 1 packet 620 bytes and deficit of 600 - 520 = 80
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 2, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNBytes (), 620, "unexpected number of bytes in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (flow1->GetDeficit (), 80, "unexpected deficit for the first flow");

  ///Dequeue second packet
  queueDisc->Dequeue ();
  //the deficit for the second flow becomes 600 but packet is not dequeued since size is more than deficit
  //As a result now packet is dequeud again from the first flow since deficit there is greater than packet size
  // deficit increases to 80 + 600 = 680>600
  NS_TEST_ASSERT_MSG_EQ (flow2->GetDeficit (), 600, "unexpected deficit for the first flow");
  NS_TEST_ASSERT_MSG_EQ (flow2->GetStatus (), DRRFlow::ACTIVE, "the second flow must be in the list of active queues since there is one packet left");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 1, "unexpected number of packets in the second flow queue");


  //Dequeue third packet
  queueDisc->Dequeue ();
  //Now deficit = 1200 > 800, so packet is dequeued
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (0)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the first flow queue");
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetQueueDiscClass (1)->GetQueueDisc ()->GetNPackets (), 0, "unexpected number of packets in the second flow queue");

  Simulator::Destroy ();
}

class DRRQueueDiscTestSuite : public TestSuite
{
public:
  DRRQueueDiscTestSuite ();
};

DRRQueueDiscTestSuite::DRRQueueDiscTestSuite ()
  : TestSuite ("drr-queue-disc", UNIT)
{
  AddTestCase (new DRRQueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new DRRQueueDiscIPFlowsSeparationAndByteLimit, TestCase::QUICK);
  AddTestCase (new DRRQueueDiscTCPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new DRRQueueDiscUDPFlowsSeparation, TestCase::QUICK);
  AddTestCase (new DRRQueueDiscDeficitVariableSizeSameFlow, TestCase::QUICK);
  AddTestCase (new DRRQueueDiscDeficitVariableSizeDifferentFlow, TestCase::QUICK);


}

static DRRQueueDiscTestSuite DRRQueueDiscTestSuite;
