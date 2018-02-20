#=================================================================
# This script simulates heavy TCP traffic for PI evaluation		     
# Authors: Shravya K.S, Smriti Murali and Mohit P. Tahiliani 
# Wireless Information Networking Group			     
# NITK Surathkal, Mangalore, India
# Tool used: NSG2.1			     
#=================================================================

#===================================
#     Simulation parameters setup
#===================================
set val(stop)   101.0                         ;# time of simulation end
set val(nn)	53	  		      ;# number of nodes in the simulation
set val(left)	50		  	      ;# number of nodes on the left hand side

#===================================
#        Initialization
#===================================
#Create a ns simulator
set ns [new Simulator]

#Open the NS trace file
set tracefile [open second-ftp.tr w]
$ns trace-all $tracefile

#Open the NAM trace file
set namfile [open second-ftp.nam w]
$ns namtrace-all $namfile

#===================================
#	DRR Parameter Settings
#===================================
Queue/DRR set buckets_ 1024
Queue/DRR set blimit_ 102400
Queue/DRR set quantum_ 1500
Queue/DRR set mask_ 0
Agent/TCPSink set ts_echo_rfc1323_ true
Agent/TCP set ssthresh_ 0
Agent/TCP set windowInit_ 1 
Agent/TCP set tcpip_base_hdr_size_ 54
Agent/TCP set window_ 65535

#===================================
#        Nodes Definition
#===================================
#Create nodes
for {set i 0} {$i < $val(nn)} {incr i} {
	set node_($i) [$ns node]
}

#===================================
#        Links Definition
#===================================
#Create links between nodes
for {set j 0} {$j < $val(left)} {incr j} {
	$ns duplex-link $node_($j) $node_(50) 10.0Mb 5ms DropTail
	$ns queue-limit $node_($j) $node_(50) 50
}

$ns duplex-link $node_(51) $node_(52) 10.0Mb 5ms DropTail
$ns queue-limit $node_(51) $node_(52) 50
$ns duplex-link $node_(50) $node_(51) 10.0Mb 50ms DRR
$ns queue-limit $node_(50) $node_(51) 200

#===================================
#        Agents Definition
#===================================

Agent/TCP set packetSize_ 1000

#Setup TCP/Newreno connections
for {set k 0} {$k < $val(left)} {incr k} {
	set tcp_($k) [new Agent/TCP/Newreno]
	$ns attach-agent $node_($k) $tcp_($k)
	set sink_($k) [new Agent/TCPSink]
	$ns attach-agent $node_(52) $sink_($k)
	$ns connect $tcp_($k) $sink_($k)
}

#===================================
#        Applications Definition
#===================================
#Setup a CBR Application over TCP/Newreno connections
for {set m 0} {$m < $val(left)} {incr m} {
	set ftp_($m) [new Application/FTP]
	$ftp_($m) attach-agent $tcp_($m)
	$ns at 0.0 "$ftp_($m) start"
	$ns at 100.0 "$ftp_($m) stop"
}

set trace_file_main tmp-main-second-ftp.tr
set trace_file_drr tmp-drr-second-ftp.tr

$ns trace-queue $node_(50) $node_(51) [open $trace_file_main w]
#set drr [[$ns link $node_(50) $node_(51)] queue]
#$drr trace curq_
#$drr trace prob_
#$drr trace dropcount_
#$drr attach [open $trace_file_drr w]

#===================================
#        Termination
#===================================
#Define a 'finish' procedure
proc finish {} {
    global ns tracefile namfile
    $ns flush-trace
    close $tracefile
    close $namfile
    exec ./nam second-ftp.nam &
    exit 0
}
$ns at $val(stop) "$ns nam-end-wireless $val(stop)"
$ns at $val(stop) "finish"
$ns at $val(stop) "puts \"done\" ; $ns halt"
$ns run
