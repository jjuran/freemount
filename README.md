Freemount
=========

Freemount is a network filesystem protocol.  It runs over a reliable transport medium, such as local sockets, TCP, TLS, and SSH.

Freemount's key features include

* extended semantics, including operations like select()
* low latency
	- small requests can be batched together in a single packet
	- messages can be interleaved -- a large read or write needn't block other requests
		- unrelated requests can be parallelized
	- related requests can be merged into chains -- if one fails, the rest abort
	- related requests can be combined, e.g. open/write-fd/close -> write-path

Freemount is designed to be the optimal protocol for FORGE (the File-Oriented, Reflective Graphical Environment).

The provided reference implementation is copyright Joshua Juran, and released under the [GNU GPL version 3][GPL] (or later).

[GPL]:  <GPLv3.txt>

