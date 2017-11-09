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

class DRRFlow : public QueueDiscClass {
Public:

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
 int32_t m_deficit;	//!< the deficit for this flow
 FlowStatus m_status;  //!< the status of this flow
};


/**
* \ingroup traffic-control
*
* \brief A DRRpacket queue disc
*/

class DRRQueueDisc : public QueueDisc {
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

 virtual ~DRRDRRQueueDisc ();


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

private:
 virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
 virtual Ptr<QueueDiscItem> DoDequeue (void);
 virtual Ptr<const QueueDiscItem> DoPeek (void) const;
 virtual bool CheckConfig (void);
 virtual void InitializeParams (void);

 /**
  * \brief Drop a packet from the tail of the queue with the largest current byte count (Packet Stealing)
  * \return the index of the queue with the largest current byte count
  */
 uint32_t DRRDrop (void);

 uint32_tm_mask; 		/*if set hashes on just the node address otherwise on
       node+port address*/
 uint32_t m_bytes ; 		//cumulative sum of bytes across all flows
 uint32_t m_packets ; 	// cumulative sum of packets across all flows
 uint32_t m_limit;      		//!< Maximum number of bytes in the queue disc
 uint32_t m_quantum;    	//!< Deficit assigned to flows at each round
 uint32_t m_flows;      	//!< Number of flow queues
 uint32_t m_dropBatchSize;  //!< Max number of packets dropped from the fat flow


 std::list<Ptr<DRRFlow> > m_Flows;	//!< The list of flows

 std::map<uint32_t, uint32_t> m_flowsIndices;	//!< Map with the index of class for each flow

 ObjectFactory m_flowFactory;     	//!< Factory to create a new flow
 ObjectFactory m_queueDiscFactory;	//!< Factory to create a new queue
};

} // namespace ns3

#endif /* DRR_QUEUE_DISC */

