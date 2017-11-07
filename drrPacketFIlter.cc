/**
 * File              : drrPacketFIlter.cc
 * Author            : Kaushik S Kalmady, Vilas M Bhat, Akhil Udathu
 * Date              : 07.11.2017
 * Last Modified Date: 07.11.2017
 * Last Modified By  : Kaushik S Kalmady
 */

// This is to be added to the end of ipv4-packet-filter.cc
// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (DRRIpv4PacketFilter);

TypeId
DRRIpv4PacketFilter::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DRRIpv4PacketFilter")
    .SetParent<Ipv4PacketFilter> ()
    .SetGroupName ("Internet")
    .AddConstructor<DRRIpv4PacketFilter> ()
    .AddAttribute ("Mask",
                   "The network mask used as an additional input to the hash function of this filter",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DRRIpv4PacketFilter::m_mask),
                   MakeUintegerChecker<uint32_t> ())
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

  NS_ASSERT (ipv4Item != 0);

  Ipv4Header hdr = ipv4Item->GetHeader ();
  Ipv4Address src = hdr.GetSource ();
  Ipv4Address dest = hdr.GetDestination ();
  uint8_t prot = hdr.GetProtocol ();
  uint16_t fragOffset = hdr.GetFragmentOffset ();

  TcpHeader tcpHdr;
  UdpHeader udpHdr;
  uint16_t srcPort = 0;
  uint16_t destPort = 0;


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

  //-----------------------------------------------------------------------------//
  
  /* This is as per the ns2 code. Only uses the source address. Or if specified,
   * uses the mask as well */
  if(m_mask)
  	src.CombineMask(Ipv4Mask(m_mask)); // As per ns-2 code, this is a bitwise & with the network mask 
  /* TODO: Can the mask be obtained from the packet itself? It feel cumbersome
   * for the user to pass the network mask while initialising the filter */
  
  uint32_t source = src.Get(); // Get the host-order 32-bit IP address.
  uint32_t hash = ((source + (source >> 8) + ~(source>>4)) % ((2<<23)-1))+1; // modulo a large prime
  
  //----------------------------------------------------------------------------//
  

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
