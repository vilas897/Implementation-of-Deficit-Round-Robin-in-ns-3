.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

DRRDRR queue disc
------------------

This chapter describes the DRR ([SHR16]_) queue disc implementation in |ns3|.

The Deficit Round Robin (DRR) algorithm is a packet scheduling algorithm
for the network scheduler that is based on fair queueing and round robin
servicing of packet queues.

DRR classifies incoming packets into different queues using stochastic fair
queuing (by default, 1024 queues are created), which are served according to
a modified Round Robin (DRR) scheduler i.e, by assigning a quantum of service to
each queue per cycle. The difference here being, the remainder of the quantum in
the previous round gets carried on to the next round.

Model Description
*****************

The source code for the DRR queue disc is located in the directory
``src/traffic-control/model`` and consists of 2 files `drr-queue-disc.h`
and `drr-queue-disc.cc` defining a DRRQueueDisc class and a helper
DRRFlow class.

* class :cpp:class:`DRRQueueDisc`: This class implements the main DRR algorithm:

  * ``DRRQueueDisc::DoEnqueue ()``: This routine uses the configured packet filters to classify the given packet into an appropriate queue. And, if the queue is not currently active, it is added to the end of the list of active queues, and its deficit is initiated to the configured quantum. Otherwise, the queue is left in its current queue list. If the filters are unable to classify the packet, the packet is assigned to a separate queue. Finally, the total number of enqueued packets is compared with the configured limit, and if it is above this value (which can happen since a packet was just enqueued), packets are dropped from the head of the queue with the largest current byte count until the total byte size remains larger than the configured limit value. Note that this in most cases means that the packet that was just enqueued is not among the packets that get dropped, which may even be from a different queue.

  * ``DRRQueueDisc::DoDequeue ()``: This routine first identifies the next queue from which a packet is to be dequeued. This selection is done based on the Round Robin scheme. The Quantum value is added to the deficit counter corresponding to that particular queue. If the size of the deficit counter is now greater than the first packet in the queue, the packet is dequeued. If the queue has no more packets, it is marked inactive and removed from the list of active queues, else the queue is popped and moved to the end of the list of queues. If the deficit counter is however, smaller than the size of the first packet in that queue, the queue is moved to the end of the active list of queues and the scheduler moves to the next queue.

  * ``DRRQueueDisc::DRRDrop ()``: This routine is invoked by ``DRRQueueDisc::DoEnqueue()`` to drop packets from the head of the queue with the largest current byte count. This routine keeps dropping packets until the byte count becomes lesser than the configured value.

* class :cpp:class:`DRRFlow`: This class implements a flow queue, by keeping its current status (ACTIVE or INACTIVE) and its current deficit.

In Linux, by default, packet classification is done by hashing (using a Jenkins
hash function) on the 5-tuple of IP protocol, and source and destination IP
addresses and port numbers (if they exist), and taking the hash value modulo
the number of queues. The hash is salted by modulo addition of a random value
selected at initialisation time, to prevent possible DoS attacks if the hash
is predictable ahead of time. Alternatively, any other packet filter can be
configured.
In |ns3|, at least one packet filter must be added to a DRR queue disc.
The Linux default classifier is provided via the DRRIpv{4,6}PacketFilter classes.
Finally, neither internal queues nor classes can be configured for an DRR
queue disc.


References
==========

.. [SHR16] M. Shreedhar and George Varghese, Efficient Fair Queueing using Deficit Round-Robin. Available online at `<https://web.stanford.edu/class/ee384x/EE384X//papers/DRR.pdf>`_
.. [LNX16] Linux kernel implementation of DRR. Available at `<https://github.com/torvalds/linux/blob/master/net/sched/sch_drr.c>`_
.. [NS16] Ns2 implementation of DRR. Available at '<https://github.com/paultsr/ns-allinone-2.35/blob/master/ns-2.35/queue/drr.cc>'_

Attributes
==========

The key attributes that the DRRQueue class holds include the following:

* ``Byte limit:`` The limit on the maximum number of bytes stored by DRR.
* ``Flows:`` The number of queues into which the incoming packets are classified.
* ``Packet Sum:`` The cumulative sum of packets across all flows.
* ``Quantum``: The quantum of service assigned to each queue in every round.

Note that the quantum, i.e., the number of bytes each queue gets to dequeue on
each round of the scheduling algorithm, is set by default to the MTU size of the
device (at initialisation time). The ``DRR::SetQuantum ()`` method
can be used (at any time) to configure a different value.

Examples
========

A typical usage pattern is to create a traffic control helper and to configure type
and attributes of queue disc and filters from the helper. For example, FqCodel
can be configured as follows:

.. sourcecode:: cpp

	TrafficControlHelper tchDRR;
  tchDRR.SetRootQueueDisc ("ns3::DRRQueueDisc");
  tchDRR.AddPacketFilter(handle, "ns3::DRRIpv4PacketFilter");
  tchDRR.AddPacketFilter(handle, "ns3::DRRIpv6PacketFilter");
  QueueDiscContainer qdiscs = tchDRR.Install (devices);

Validation
**********

The DRR model is tested using :cpp:class:`DRRQueueDiscTestSuite` class defined in `src/test/ns3tc/drr-test-suite.cc`.  The suite includes 5 test cases:

* Test 1: The first test checks that packets that cannot be classified by any available filter are added to a new queue.
* Test 2: The second test checks that IPv4 packets having distinct destination addresses are enqueued into different flow queues. Also, it checks that packets are dropped from the fat flow in case the queue disc capacity is exceeded.
* Test 3: The fourth test checks that TCP packets with distinct port numbers are enqueued into different flow queues.
* Test 4: The fifth test checks that UDP packets with distinct port numbers are enqueued into different flow queues.
* Test 5: The third test checks the dequeue operation, byte limit conditions and the deficit round robin-based scheduler for packets belonging to the same flow.
* Test 6: The third test checks the dequeue operation, byte limit conditions and the deficit round robin-based scheduler for packets belonging to differnet flows.

The test suite can be run using the following commands::

  $ ./waf configure --enable-examples --enable-tests
  $ ./waf build
  $ ./test.py -s drr-queue-disc

or::

  $ NS_LOG="DRRQueueDisc" ./waf --run "test-runner --suite=drr-queue-disc"
