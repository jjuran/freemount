Freemount Overview
==================

Introduction
------------

Freemount is a client-server network filesystem protocol.  It's defined in terms of a reliable transport medium.  Initial development assumes a stream-oriented connection, though in theory the protocol is adaptable to a datagram service (provided that it's reliable -- either SCTP or a UDP wrapper might be suitable here).  In practice, Freemount will be used atop a secure layer such as SSL/TLS or an SSH tunnel (or in special cases, over a local-domain socket).

Features
--------

While Freemount is intended to be fully usable as a file sharing system (e.g. as a replacement for AppleShare), its primary purpose is to be a communication protocol for FORGE (File-Oriented, Reflective Graphical Environment).  FORGE requires additional semantics that aren't available in typical network filesystems:

* non-local named streams (e.g. a FIFO written by the server and read by the client)
* select/poll operations (e.g. block until one of several files is readable)
* terminal control (e.g. server sends SIGHUP to client on close of controlling terminal)

Additionally, Freemount has a number of optimizations for UI performance over high latency links:

* overlapping asynchronous communication model (multiple outstanding requests)
* path-based read-all and write-all macro operations (single packet per direction for small data)
* chained operations (batched requests run in serial, any failure aborts)

Freemount will, to the extent possible, bridge impedance mismatches between client and server OSes or filesystems (for example, by translating names to UTF-8 but also offering the original in case the peer can use it).

Future plans
------------

One of Freemount's goals is to solve the "I want to send you a file" problem.  This problem has two major subsets: "over the Internet" and "while we're in the same room".  The latter in particular needs some attention -- it's unacceptable to require two devices which are feet or even inches apart from each other to send data to a distant third-party and back, when they have the means to connect directly.  The Freemount ecosystem will include user applications to facilitate communication between two devices on a network.

