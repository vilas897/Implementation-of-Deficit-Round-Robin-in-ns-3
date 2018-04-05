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
 *          Anuj Revankar <anujrevankar@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "sfq-queue-disc.h"
#include "ns3/queue.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/net-device-queue-interface.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SfqQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (SfqFlow);

TypeId SfqFlow::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqFlow")
    .SetParent<QueueDiscClass> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SfqFlow> ()
  ;
  return tid;
}

SfqFlow::SfqFlow ()
  : m_allot (0),
    m_status (SFQ_EMPTY_SLOT)
{
  NS_LOG_FUNCTION (this);
}

SfqFlow::~SfqFlow ()
{
  NS_LOG_FUNCTION (this);
}

void
SfqFlow::SetAllot (uint32_t allot)
{
  NS_LOG_FUNCTION (this << allot);
  m_allot = allot;
}

int32_t
SfqFlow::GetAllot (void) const
{
  NS_LOG_FUNCTION (this);
  return m_allot;
}

void
SfqFlow::IncreaseAllot (int32_t allot)
{
  NS_LOG_FUNCTION (this << allot);
  m_allot += allot;
}

void
SfqFlow::SetStatus (FlowStatus status)
{
  NS_LOG_FUNCTION (this);
  m_status = status;
}

SfqFlow::FlowStatus
SfqFlow::GetStatus (void) const
{
  NS_LOG_FUNCTION (this);
  return m_status;
}

NS_OBJECT_ENSURE_REGISTERED (SfqQueueDisc);

TypeId SfqQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SfqQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SfqQueueDisc> ()
    .AddAttribute ("PacketLimit",
                   "The hard limit on the real queue size, measured in packets",
                   UintegerValue (10 * 1024),
                   MakeUintegerAccessor (&SfqQueueDisc::SetLimit,
                                         &SfqQueueDisc::GetLimit),
                   MakeUintegerChecker<uint32_t> (),
                   TypeId::DEPRECATED,
                   "Use the MaxSize attribute instead")
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("102400p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("FlowLimit",
                   "The maximum number of packets each flow can have",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SfqQueueDisc::m_flowLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Flows",
                   "The number of queues into which the incoming packets are classified",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SfqQueueDisc::m_flows),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Ns2Style",
                   "Whether to use ns-2's implementation of SFQ",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SfqQueueDisc::m_useNs2Style),
                   MakeBooleanChecker ())
  ;
  return tid;
}

SfqQueueDisc::SfqQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::MULTIPLE_QUEUES, QueueSizeUnit::PACKETS),
    m_quantum (0)
{
  NS_LOG_FUNCTION (this);
}

SfqQueueDisc::~SfqQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
SfqQueueDisc::SetQuantum (uint32_t quantum)
{
  NS_LOG_FUNCTION (this << quantum);
  m_quantum = quantum;
}

uint32_t
SfqQueueDisc::GetQuantum (void) const
{
  return m_quantum;
}

bool
SfqQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  int32_t ret = Classify (item); //classify returns a hash function based on the packet filters
  uint32_t h;

  if (ret == PacketFilter::PF_NO_MATCH)
    {
      NS_LOG_ERROR ("No filter has been able to classify this packet, place it in seperate flow.");
      h = m_flows; // place all unfiltered packets into a seperate flow queue
    }
  else
    {
      h = ret % m_flows;
    }

  Ptr<SfqFlow> flow;
  if (m_flowsIndices.find (h) == m_flowsIndices.end ())
    {
      NS_LOG_DEBUG ("Creating a new flow queue with index " << h);
      flow = m_flowFactory.Create<SfqFlow> ();
      Ptr<QueueDisc> qd = m_queueDiscFactory.Create<QueueDisc> ();
      qd->Initialize ();
      flow->SetQueueDisc (qd);
      AddQueueDiscClass (flow);

      m_flowsIndices[h] = GetNQueueDiscClasses () - 1;
    }
  else
    {
      flow = StaticCast<SfqFlow> (GetQueueDiscClass (m_flowsIndices[h]));
    }

  if (m_useNs2Style)
    {
      uint32_t left = GetMaxSize ().GetValue () - GetNPackets ();
      // Drop packet if number of packets exceeds fairshare or limit is reached
      if ( (flow->GetQueueDisc ()->GetNPackets () >= (left >> 1))
           || (left < m_flows && flow->GetQueueDisc ()->GetNPackets () > m_fairshare)
           || (left <= 0))
        {
          DropBeforeEnqueue (item, OVERLIMIT_DROP);
          return false;
        }
      flow->GetQueueDisc ()->Enqueue (item);

      NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h]);
      if (flow->GetStatus () == SfqFlow::SFQ_EMPTY_SLOT)
        {
          flow->SetStatus (SfqFlow::SFQ_IN_USE);
          m_flowList.push_back (flow);
        }
      return true;
    }

  if (GetNPackets () >= GetMaxSize ().GetValue ()
      || (GetMaxSize ().GetValue () - GetNPackets () < m_flows && flow->GetQueueDisc ()->GetNPackets () > m_fairshare))
    {
      DropBeforeEnqueue (item, OVERLIMIT_DROP);
      return false;
    }

  if (flow->GetStatus () == SfqFlow::SFQ_EMPTY_SLOT)
    {
      flow->SetStatus (SfqFlow::SFQ_IN_USE);
      flow->SetAllot (m_quantum);
      m_flowList.push_back (flow);
    }

  flow->GetQueueDisc ()->Enqueue (item);

  NS_LOG_DEBUG ("Packet enqueued into flow " << h << "; flow index " << m_flowsIndices[h]);

  return true;
}

Ptr<QueueDiscItem>
SfqQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  if (m_useNs2Style)
    {
      Ptr<SfqFlow> flow;
      Ptr<QueueDiscItem> item;
      if (m_flowList.empty ())
        {
          NS_LOG_DEBUG ("No flow found to dequeue a packet");
          return 0;
        }
      flow = m_flowList.front ();
      item = flow->GetQueueDisc ()->Dequeue ();
      NS_LOG_DEBUG ("Dequeued packet " << item->GetPacket ());
      if (flow->GetQueueDisc ()->GetNPackets () == 0)
        {
          flow->SetStatus (SfqFlow::SFQ_EMPTY_SLOT);
          m_flowList.pop_front ();
        }
      else
        {
          m_flowList.push_back (flow);
          m_flowList.pop_front ();
        }
      return item;
    }

  Ptr<SfqFlow> flow;
  Ptr<QueueDiscItem> item;
  do
    {
      bool found = false;

      while (!found && !m_flowList.empty ())
        {
          flow = m_flowList.front ();

          if (flow->GetAllot () <= 0)
            {
              flow->IncreaseAllot (m_quantum);
              m_flowList.push_back (flow);
              m_flowList.pop_front ();
            }
          else
            {
              NS_LOG_DEBUG ("Found a new flow with positive value");
              found = true;
            }
        }
      if (!found)
        {
          NS_LOG_DEBUG ("No flow found to dequeue a packet");
          return 0;
        }

      item = flow->GetQueueDisc ()->Dequeue ();

      if (!item)
        {
          NS_LOG_DEBUG ("Could not get a packet from the selected flow queue");
          if (!m_flowList.empty ())
            {
              flow->SetStatus (SfqFlow::SFQ_EMPTY_SLOT);
              m_flowList.pop_front ();
            }
        }
      else
        {
          NS_LOG_DEBUG ("Dequeued packet " << item->GetPacket ());
        }
    }
  while (item == 0);

  flow->IncreaseAllot (item->GetSize () * -1);

  return item;
}

Ptr<const QueueDiscItem>
SfqQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  return PeekDequeued ();
}

void
SfqQueueDisc::SetLimit (uint32_t limit)
{
  NS_LOG_FUNCTION (this << limit);
  SetMaxSize (QueueSize (QueueSizeUnit::PACKETS, limit));
}

uint32_t
SfqQueueDisc::GetLimit (void) const
{
  NS_LOG_FUNCTION (this);
  return GetMaxSize ().GetValue ();
}

bool
SfqQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("SfqQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () == 0)
    {
      NS_LOG_ERROR ("SfqQueueDisc needs at least a packet filter");
      return false;
    }

  if (GetNInternalQueues () > 0)
    {
      NS_LOG_ERROR ("SfqQueueDisc cannot have internal queues");
      return false;
    }

  return true;
}

void
SfqQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);

  // we are at initialization time. If the user has not set a quantum value,
  // set the quantum to the MTU of the device
  if (!m_quantum && !m_useNs2Style)
    {
      Ptr<NetDevice> device = GetNetDevice ();
      NS_ASSERT_MSG (device, "Device not set for the queue disc");
      m_quantum = device->GetMtu ();
      NS_LOG_DEBUG ("Setting the quantum to the MTU of the device: " << m_quantum);
    }
  if (m_flows == 0 || GetMaxSize ().GetValue () == 0)
    {
      m_flows = 16;
      SetMaxSize (QueueSize ("40p"));
    }
  if (m_flowLimit == 0)
    m_flowLimit = GetMaxSize ().GetValue ();
  m_flowFactory.SetTypeId ("ns3::SfqFlow");
  m_queueDiscFactory.SetTypeId ("ns3::FifoQueueDisc");
  m_queueDiscFactory.Set ("MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::PACKETS, m_flowLimit)));
  m_fairshare = GetMaxSize ().GetValue () / m_flows;
}

} // namespace ns3
