/**
 * File              : drrPacketFilter.h
 * Author            : Kaushik S Kalmady, Vilas M Bhat, Akhil Udathu
 * Date              : 07.11.2017
 * Last Modified Date: 07.11.2017
 * Last Modified By  : Kaushik S Kalmady
 */


// This is to be appended to ipv4-packet-filter.h

/**
 * \ingroup internet
 *
 * DRRIpv4PacketFilter is the filter to be added to the DRRQueueDisc
 * to simulate the behavior of the DRR Linux queue disc.
 *       */
class DRRIpv4PacketFilter : public Ipv4PacketFilter {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DRRIpv4PacketFilter ();
  virtual ~DRRIpv4PacketFilter ();

private:
  virtual int32_t DoClassify (Ptr<QueueDiscItem> item) const;

  uint32_t m_mask;  //!< if not set hashes on just the node address otherwise on masked node address
};
