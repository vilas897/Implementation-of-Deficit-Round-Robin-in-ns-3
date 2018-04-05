/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
 *               2016 University of Washington
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 *           Tom Henderson <tomhend@u.washington.edu>
 *           Pasquale Imputato <p.imputato@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/timer.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ipv6-queue-disc-item.h"
#include "ipv6-packet-filter.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv6PacketFilter");

NS_OBJECT_ENSURE_REGISTERED (Ipv6PacketFilter);

TypeId
Ipv6PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv6PacketFilter")
    .SetParent<PacketFilter> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

Ipv6PacketFilter::Ipv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

Ipv6PacketFilter::~Ipv6PacketFilter()
{
  NS_LOG_FUNCTION (this);
}

bool
Ipv6PacketFilter::CheckProtocol (Ptr<QueueDiscItem> item) const
{
  NS_LOG_FUNCTION (this << item);
  return (DynamicCast<Ipv6QueueDiscItem> (item) != 0);
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (FqCoDelIpv6PacketFilter);

TypeId
FqCoDelIpv6PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FqCoDelIpv6PacketFilter")
    .SetParent<Ipv6PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<FqCoDelIpv6PacketFilter> ()
    .AddAttribute ("Perturbation",
                   "The salt used as an additional input to the hash function of this filter",
                   UintegerValue (0),
                   MakeUintegerAccessor (&FqCoDelIpv6PacketFilter::m_perturbation),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

FqCoDelIpv6PacketFilter::FqCoDelIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

FqCoDelIpv6PacketFilter::~FqCoDelIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

int32_t
FqCoDelIpv6PacketFilter::DoClassify (Ptr< QueueDiscItem > item) const
{
  NS_LOG_FUNCTION (this << item);
  Ptr<Ipv6QueueDiscItem> ipv6Item = DynamicCast<Ipv6QueueDiscItem> (item);

  if(!ipv6Item)
  {
    NS_LOG_DEBUG("No match");
    return PacketFilter::PF_NO_MATCH;
  }

  Ipv6Header hdr = ipv6Item->GetHeader ();
  Ipv6Address src = hdr.GetSourceAddress ();
  Ipv6Address dest = hdr.GetDestinationAddress ();
  uint8_t prot = hdr.GetNextHeader ();

  TcpHeader tcpHdr;
  UdpHeader udpHdr;
  uint16_t srcPort = 0;
  uint16_t destPort = 0;

  Ptr<Packet> pkt = ipv6Item->GetPacket ();

  if (prot == 6) // TCP
    {
      pkt->PeekHeader (tcpHdr);
      srcPort = tcpHdr.GetSourcePort ();
      destPort = tcpHdr.GetDestinationPort ();
    }
  else if (prot == 17) // UDP
    {
      pkt->PeekHeader (udpHdr);
      srcPort = udpHdr.GetSourcePort ();
      destPort = udpHdr.GetDestinationPort ();
    }

  /* serialize the 5-tuple and the perturbation in buf */
  uint8_t buf[41];
  src.Serialize (buf);
  dest.Serialize (buf + 16);
  buf[32] = prot;
  buf[33] = (srcPort >> 8) & 0xff;
  buf[34] = srcPort & 0xff;
  buf[35] = (destPort >> 8) & 0xff;
  buf[36] = destPort & 0xff;
  buf[37] = (m_perturbation >> 24) & 0xff;
  buf[38] = (m_perturbation >> 16) & 0xff;
  buf[39] = (m_perturbation >> 8) & 0xff;
  buf[40] = m_perturbation & 0xff;

  /* Linux calculates the jhash2 (jenkins hash), we calculate the murmur3 */
  uint32_t hash = Hash32 ((char*) buf, 41);

  NS_LOG_DEBUG ("Found Ipv6 packet; hash of the five tuple " << hash);

  return hash;
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (DRRIpv6PacketFilter);

TypeId
DRRIpv6PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DRRIpv6PacketFilter")
    .SetParent<Ipv6PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<DRRIpv6PacketFilter> ()
  ;
  return tid;
}

DRRIpv6PacketFilter::DRRIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

DRRIpv6PacketFilter::~DRRIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

int32_t
DRRIpv6PacketFilter::DoClassify (Ptr< QueueDiscItem > item) const
{
  NS_LOG_FUNCTION (this << item);
  Ptr<Ipv6QueueDiscItem> ipv6Item = DynamicCast<Ipv6QueueDiscItem> (item);

  if (!ipv6Item)
    {
      NS_LOG_DEBUG ("No match");
      return PacketFilter::PF_NO_MATCH;
    }

  Ipv6Header hdr = ipv6Item->GetHeader ();
  Ipv6Address src = hdr.GetSourceAddress ();
  Ipv6Address dest = hdr.GetDestinationAddress ();
  uint8_t prot = hdr.GetNextHeader ();

  TcpHeader tcpHdr;
  UdpHeader udpHdr;
  uint16_t srcPort = 0;
  uint16_t destPort = 0;

  Ptr<Packet> pkt = ipv6Item->GetPacket ();

  if (prot == 6) // TCP
    {
      pkt->PeekHeader (tcpHdr);
      srcPort = tcpHdr.GetSourcePort ();
      destPort = tcpHdr.GetDestinationPort ();
    }
  else if (prot == 17) // UDP
    {
      pkt->PeekHeader (udpHdr);
      srcPort = udpHdr.GetSourcePort ();
      destPort = udpHdr.GetDestinationPort ();
    }

  /* serialize the 5-tuple and the perturbation in buf */
  uint8_t buf[37];
  src.Serialize (buf);
  dest.Serialize (buf + 16);
  buf[32] = prot;
  buf[33] = (srcPort >> 8) & 0xff;
  buf[34] = srcPort & 0xff;
  buf[35] = (destPort >> 8) & 0xff;
  buf[36] = destPort & 0xff;


  /* Linux calculates the jhash2 (jenkins hash), we calculate the murmur3 */
  uint32_t hash = Hash32 ((char*) buf, 37);

  NS_LOG_DEBUG ("Found Ipv6 packet; hash " << hash);

  return hash;
}

// ----------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (SfqIpv6PacketFilter);

TypeId
SfqIpv6PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqIpv6PacketFilter")
    .SetParent<Ipv6PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<SfqIpv6PacketFilter> ()
    .AddAttribute ("PerturbationTime",
                   "The time duration after which salt used as an additional input to the hash function is changed",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&SfqIpv6PacketFilter::m_perturbTime),
                   MakeTimeChecker ())
  ;
  return tid;
}

SfqIpv6PacketFilter::SfqIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
  rand = CreateObject<UniformRandomVariable> ();
  rand->SetAttribute ("Min", DoubleValue (0));
  rand->SetAttribute ("Max", DoubleValue (UINT32_MAX));
  if (m_perturbTime != 0)
    {
      Simulator::Schedule (m_perturbTime, &SfqIpv6PacketFilter::PerturbHash, this);
    }
}

SfqIpv6PacketFilter::~SfqIpv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

void
SfqIpv6PacketFilter::PerturbHash ()
{
  if (m_perturbTime == 0)
    {
      return;
    }
  m_perturbation = rand->GetInteger ();
  NS_LOG_DEBUG ("Set new perturbation value to " << m_perturbation);
  Simulator::Schedule (m_perturbTime, &SfqIpv6PacketFilter::PerturbHash, this);
}

int32_t
SfqIpv6PacketFilter::DoClassify (Ptr< QueueDiscItem > item) const
{
  NS_LOG_FUNCTION (this << item);
  Ptr<Ipv6QueueDiscItem> ipv6Item = DynamicCast<Ipv6QueueDiscItem> (item);

  if (!ipv6Item)
    {
      NS_LOG_DEBUG ("No match");
      return PacketFilter::PF_NO_MATCH;
    }

  Ipv6Header hdr = ipv6Item->GetHeader ();
  Ipv6Address src = hdr.GetSourceAddress ();
  Ipv6Address dest = hdr.GetDestinationAddress ();

  Ptr<Packet> pkt = ipv6Item->GetPacket ();

  /* serialize the 5-tuple and the perturbation in buf */
  uint8_t buf[36];
  src.Serialize (buf);
  dest.Serialize (buf + 16);
  buf[32] = (m_perturbation >> 24) & 0xff;
  buf[33] = (m_perturbation >> 16) & 0xff;
  buf[34] = (m_perturbation >> 8) & 0xff;
  buf[35] = m_perturbation & 0xff;

  /* Linux calculates the jhash2 (jenkins hash), we calculate the murmur3 */
  uint32_t hash = Hash32 ((char*) buf, 36);

  NS_LOG_DEBUG ("Found Ipv6 packet; hash of the five tuple " << hash);

  return hash;
}

// ----------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (SfqNs2Ipv6PacketFilter);

TypeId
SfqNs2Ipv6PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqNs2Ipv6PacketFilter")
    .SetParent<Ipv6PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<SfqNs2Ipv6PacketFilter> ()
  ;
  return tid;
}

SfqNs2Ipv6PacketFilter::SfqNs2Ipv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

SfqNs2Ipv6PacketFilter::~SfqNs2Ipv6PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

int32_t
SfqNs2Ipv6PacketFilter::DoClassify (Ptr< QueueDiscItem > item) const
{
  NS_LOG_FUNCTION (this << item);
  Ptr<Ipv6QueueDiscItem> ipv6Item = DynamicCast<Ipv6QueueDiscItem> (item);

  if (!ipv6Item)
    {
      NS_LOG_DEBUG ("No match");
      return PacketFilter::PF_NO_MATCH;
    }

  Ipv6Header hdr = ipv6Item->GetHeader ();
  Ipv6Address src = hdr.GetSourceAddress ();
  Ipv6Address dest = hdr.GetDestinationAddress ();
  uint8_t srcBytes[16];
  uint8_t destBytes[16];
  src.GetBytes (srcBytes);
  dest.GetBytes (destBytes);

  int32_t i = 0;
  int32_t j = 0;

  const int32_t PRIME = ((2 << 19) - 1);

  for (uint32_t it = 15; it >= 0; --it)
    {
      i = (i * 10 + srcBytes[it]) % PRIME;
      j = (j * 10 + destBytes[it]) % PRIME;
    }

  int32_t k = i + j;

  int32_t hash = (k + (k >> 8) + ~(k >> 4)) % PRIME; // modulo a large prime
  NS_LOG_DEBUG ("Found Ipv6 packet; hash value " << hash);

  return hash;
}

} // namespace ns3
