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

#ifndef DRR_QUEUE_DISC
#define DRR_QUEUE_DISC

#include "ns3/queue-disc.h"
#include "ns3/object-factory.h"
#include <list>
#include <map>

namespace ns3 {

/**
* \ingroup traffic-control
*
* \brief A flow queue used by the DRR queue disc
*/

class DRRFlow : public QueueDiscClass
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);


  /**
   * \brief DRRFlow constructor
   */
  DRRFlow ();

  virtual ~DRRFlow ();


  /**
   * \enum FlowStatus
   * \brief Used to determine the status of this flow queue
   */
  enum FlowStatus
  {
    INACTIVE,
    ACTIVE
  };


  /**
   * \brief Set the deficit for this flow
   * \param deficit the deficit for this flow
   */
  void SetDeficit (uint32_t deficit);


  /**
   * \brief Get the deficit for this flow
   * \return the deficit for this flow
   */
  int32_t GetDeficit (void) const;


  /**
   * \brief Increase the deficit for this flow
   * \param deficit the amount by which the deficit is to be increased
   */
  void IncreaseDeficit (int32_t deficit);


  /**
   * \brief Set the status for this flow
   * \param status the status for this flow
   */
  void SetStatus (FlowStatus status);

  /**
   * \brief Get the status of this flow
   * \return the status of this flow
   */
  FlowStatus GetStatus (void) const;


private:
  uint32_t m_deficit;   //!< the deficit for this flow
  FlowStatus m_status; //!< the status of this flow
};


/**
* \ingroup traffic-control
*
* \brief A DRRpacket queue disc
*/
class DRRQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief DRRQueueDisc constructor
   */
  DRRQueueDisc ();

  virtual ~DRRQueueDisc ();


  /**
 * \brief Set the quantum value.
 *
 * \param quantum The initial number of bytes each queue gets to dequeue on each round of the scheduling algorithm
 */
  void SetQuantum (uint32_t quantum);


  /**
 * \brief Get the quantum value.
 *
 * \returns The initial number of bytes each queue gets to dequeue on each round of the scheduling algorithm
 */
  uint32_t GetQuantum (void) const;

  // Reasons for dropping packets
  static constexpr const char* UNCLASSIFIED_DROP = "Unclassified drop";  //!< No packet filter able to classify packet
  static constexpr const char* OVERLIMIT_DROP = "Overlimit drop";        //!< Overlimit dropped packets


private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

  /**
   * \brief Drop a packet from the tail of the queue with the largest current byte count (Packet Stealing)
   * \return the index of the queue with the largest current byte count
   */
  uint32_t DRRDrop (void);

  uint32_t m_limit;          //!< Maximum number of bytes in the queue disc
  uint32_t m_quantum;        //!< total number of bytes that a flow can send at once
  uint32_t m_flows;          //!< Number of flows
  uint32_t m_flowLimit;      //!< Maximum number of packets allowed in each flow
  uint32_t m_meanPktSize;    //!< Mean packet size


  std::list<Ptr<DRRFlow> > m_flowList;    //!< The list of flows

  std::map<uint32_t, uint32_t> m_flowsIndices;    //!< Map with the index of class for each flow

  ObjectFactory m_flowFactory;         //!< Factory to create a new flow
  ObjectFactory m_queueDiscFactory; //!< Factory to create a new queue
};

} // namespace ns3

#endif /* DRR_QUEUE_DISC */
