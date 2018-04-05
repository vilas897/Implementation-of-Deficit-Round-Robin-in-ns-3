# Implementation of Stochastic Fair Queueing in NS-3
## Course Code: CO300

### SFQ Queue Disc

The Stochastic Fair Queuing (SFQ) algorithm is a Queue Management algorithm.
SFQ is an attempt to distribute network usage fairly among different 
source-destination pairs of messages. For absolute fairness, a hash 
between a source-destination pair and a queue is created, and round-robin 
is employed among the queues. However, this consumes a lot of resources and 
is not practical. Stochastic Fair Queueing employs an approxiate hash, which 
is more viable. The hash function is perturbed periodically to maintain 
fairness. Each queue is served according to its current allotment, a positive 
allotment means that the queue may be served, if it is next in order, while a 
negative allotment means that the next queue in the list should be taken.

### Model Description

The source code for the Sfq queue disc is located in the directory
``src/traffic-control/model`` and consists of 2 files `sfq-queue-disc.h`
and `sfq-queue-disc.cc` defining a SfqQueueDisc class and a helper
SfqFlow class. The code was ported to |ns3| based on Linux kernel code
implemented by Alexey Kuznetsov as well as the ns-2 code for Sfq.

* class `SfqQueueDisc`: This class implements the main Sfq algorithm:

  * ``SfqQueueDisc::DoEnqueue ()``: This routine uses the configured packet 
filters to classify the given packet into an appropriate queue. If the filters 
are unable to classify the packet, the packet is added to a spare queue. If the
queue is an empty slot, it is added to the end of the list of queues, and its
allotment is initialized to the configured quantum. Otherwise, the queue is
 left in its current queue list. In ns-2 mode, a quantum is not allocated.
In case the queue is already full, or the number of remaining number of slots
is less than the number of queues, and the current queue has already exceeded
its allocated space (which is given by total limit / number of queues), then
the incoming packet is dropped.

  * ``SfqQueueDisc::DoDequeue ()``: In the case of ns-2 mode, the first packet 
in the queue at the front of the round robin is removed and returned. If the 
queue still has remaining packets, it is removed from the front of the round 
robin and placed at the end. In the case of default execution, the first task 
performed is selecting a queue from which to dequeue a packet. The scheduler 
looks at the front of the list, if the current queue has a negative allotment, 
its allotment is increased by quantum and sent to the back. Otherwise, that 
queue is selected for dequeue. After having selected a queue from which to 
dequeue a packet, the fifo algorithm is invoked on that queue. If the fifo 
algorithm does not return a packet, then the queue must be empty, and the 
scheduler removes it from the list, marking it as an unused slot to be added 
back (as a new queue) the next time a packet for that queue arrives. Then 
(since no packet was available for dequeue), the whole dequeue process is 
restarted from the beginning. If, instead, the scheduler did get a packet back
from the fifo algorithm, it subtracts the size of the packet from the byte 
allotment for the selected queue and returns the packet as the result of the 
dequeue operation.

* class `SfqFlow`: This class implements a flow queue, by keeping 
its current status (an empty slot, or in use) and its current allotment.

In Linux, by default, packet classification is done by hashing (using a 
Jenkins hash function) and taking the hash value modulo the number of queues.
The hash is salted by modulo addition of a random value selected at regular 
intervals, in order to perturb the hash function, reducing the chances of 
the same collision consistently occuring. Alternatively, any other packet 
filter can be configured. In the ns-2 version, the hashing is done manually 
without perturb using the formula (k + (k >> 8) + ~(k >> 4)) % ((2 << 19) - 1)
where k is the sum of addresses of source and destination in integer format.
In |ns3|, at least one packet filter must be added to an Sfq queue disc.

### Attributes

The key attributes that the SfqQueueDisc class holds include the following:

* ``MaxSize:`` The limit on the maximum number of packets stored by Sfq.
* ``Flows:`` The number of flow queues managed by Sfq.
* ``Ns2Style:`` Whether to use the ns-2 version of implementation.

Note that the quantum, i.e., the number of bytes each queue gets to dequeue on
each round of the scheduling algorithm, is set by default to the MTU size of 
the device (at initialisation time). The ``SfqQueueDisc::SetQuantum ()`` method
can be used (at any time) to configure a different value. Quantum is unused if 
ns-2 implementation is followed.

### Examples

A typical usage pattern is to create a traffic control helper and to 
configure type and attributes of queue disc and filters from the helper. 
For example, Sfq can be configured as follows:

```cpp

  TrafficControlHelper tch;
  uint16_t handle = tch.SetRootQueueDisc ("ns3::SfqQueueDisc");
  tch.AddPacketFilter (handle, "ns3::SfqIpv4PacketFilter", "PerturbationTime", UintegerValue (100));
  tch.AddPacketFilter (handle, "ns3::SfqIpv6PacketFilter", "PerturbationTime", UintegerValue (100));
  QueueDiscContainer qdiscs = tch.Install (devices);
```

An example programme for SFQ is provided in  
``src/traffic-control/examples/sfq-example.cc``  
and should be executed as  
``./waf --run "sfq-example"``   


### References

[1] Kuznetsov, Alexey. “Stochastic Fairness Queueing.” Github, github.com/torvalds/linux/blob/master/net/sched/sch_sfq.c

[2] Paul E. McKenney "Stochastic Fairness Queuing",	"Interworking: Research and Experience", v.2, 1991, p.113-131.

