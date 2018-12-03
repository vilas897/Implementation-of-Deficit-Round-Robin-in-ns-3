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
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ipv4-queue-disc-item.h"
#include "ipv4-packet-filter.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ipv4PacketFilter");

NS_OBJECT_ENSURE_REGISTERED (Ipv4PacketFilter);

TypeId 
Ipv4PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ipv4PacketFilter")
    .SetParent<PacketFilter> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

Ipv4PacketFilter::Ipv4PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

Ipv4PacketFilter::~Ipv4PacketFilter()
{
  NS_LOG_FUNCTION (this);
}

bool
Ipv4PacketFilter::CheckProtocol (Ptr<QueueDiscItem> item) const
{
  NS_LOG_FUNCTION (this << item);
  return (DynamicCast<Ipv4QueueDiscItem> (item) != 0);
}

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (DRRIpv4PacketFilter);

TypeId
DRRIpv4PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DRRIpv4PacketFilter")
    .SetParent<Ipv4PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<DRRIpv4PacketFilter> ()
    ;
  return tid;
}

DRRIpv4PacketFilter::DRRIpv4PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

DRRIpv4PacketFilter::~DRRIpv4PacketFilter ()
{
  NS_LOG_FUNCTION (this);
}

int32_t
DRRIpv4PacketFilter::DoClassify (Ptr<QueueDiscItem> item) const
{
  NS_LOG_FUNCTION (this << item);
  Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);

    if (!ipv4Item)
      {
        NS_LOG_DEBUG ("No match");
        return PacketFilter::PF_NO_MATCH;
      }

  Ipv4Header hdr = ipv4Item->GetHeader ();
  Ipv4Address src = hdr.GetSource ();
  Ipv4Address dest = hdr.GetDestination ();
  uint8_t prot = hdr.GetProtocol ();
  uint16_t fragOffset = hdr.GetFragmentOffset ();

  TcpHeader tcpHdr;
  UdpHeader udpHdr;
  uint16_t srcPort = 0;
  uint16_t destPort = 0;

  Ptr<Packet> pkt = ipv4Item->GetPacket ();


  if (prot == 6 && fragOffset == 0) // TCP
    {
        pkt->PeekHeader (tcpHdr);
        srcPort = tcpHdr.GetSourcePort ();
        destPort = tcpHdr.GetDestinationPort ();
    }
  else if (prot == 17 && fragOffset == 0) // UDP
    {
        pkt->PeekHeader (udpHdr);
        srcPort = udpHdr.GetSourcePort ();
        destPort = udpHdr.GetDestinationPort ();
    }

  //---------------------------------------------------------------------------//
  /* This is as per the ns2 code. Only uses the source address. Or if specified,
   * uses the mask as well */

  /*
  if(m_mask)
    src.CombineMask(Ipv4Mask(m_mask)); // As per ns-2 code, this is a bitwise & with the network mask 
  */

  /* TODO: Can the mask be obtained from the packet itself? It feel cumbersome */
  /* for the user to pass the network mask while initialising the filter */
  /*
  uint32_t source = src.Get(); // Get the host-order 32-bit IP address.
  uint32_t hash = ((source + (source >> 8) + ~(source>>4)) % ((2<<23)-1))+1; // modulo a large prime
  */


  //---------------------------------------------------------------------------//
  
  /* This is a modified version of the hash function in the fq-codel ns3 code. Probably a more robust hash function
   * calculated using a variety of parameters */

    uint8_t buf[13];
    src.Serialize (buf);
    dest.Serialize (buf + 4);
    buf[8] = prot;
    buf[9] = (srcPort >> 8) & 0xff;
    buf[10] = srcPort & 0xff;
    buf[11] = (destPort >> 8) & 0xff;
    buf[12] = destPort & 0xff;
    
    /* murmur3 used in fq-codel */
    uint32_t hash = Hash32 ((char*) buf, 13);

  //******************************************************************//
  
  NS_LOG_DEBUG ("Found Ipv4 packet; hash value " << hash);

  return hash;
}



} // namespace ns3
