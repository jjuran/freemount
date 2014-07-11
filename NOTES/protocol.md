Freemount Protocol
==================

Introduction
------------

Freemount is a client-server network filesystem protocol.  It's defined in terms of a reliable transport medium.  Initial development assumes a stream-oriented connection, though in theory the protocol is adaptable to a datagram service (provided that it's reliable -- either SCTP or a UDP wrapper might be suitable here).  In practice, Freemount will be used atop a secure layer such as SSL/TLS or an SSH tunnel (or in special cases, over a local-domain socket).

Theory of Operation
-------------------

The Freemount protocol is designed as a bidirectional conversation between two nodes.  These will commonly take the form of a client connecting and sending requests to a server which handles and responds to them, although this is by no means required:  The roles could easily be reversed (i.e. one node listens for connections from other nodes to which it then sends requests), and it's possible for each of two connected nodes to both send requests to the other across the same protocol stream.

A conversation using the Freemount protocol consists of a bidirectional stream of frames.  Each frame begins with an 8-byte header (which includes a one-byte type field and a one-byte request ID field), followed by a payload whose (possibly zero) length is stored in the header, followed by enough padding (zero to three bytes) to align to a four-byte boundary.  A frame with a zero-length payload is an empty frame.  Several frame types (ping and pong) indicate control frames, which are independent of any request and have no semantic significance.  The others are message frames.

A message is either a request sent by a client, or a response sent by the server answering a client request.  (If the server needs to alert the client to asynchronous events, it can either do so in response to an outstanding request, or initiate a request of its own as a client, since Freemount is bidirectional.)

A message is divided into a sequence of frames, or message fragments.  Each frame in a message (request or response) is marked with a request ID which is unique to the request.  (Response frames use the same request ID as the request they're answering.)  A message is terminated by either an end-of-message fragment or an error fragment.

Multiple requests may be outstanding, and they may be answered in any order.  A single message frame must be sent unbroken by other data, but otherwise frames may be interleaved, so multiple requests and responses can be transmitted simultaneously.  (For example, transmission of a large file will be broken into multiple data fragments so it doesn't block other requests.)

A request message begins with a Request fragment, which specifies the request ID that the sender is using to identify the request, and the request type (e.g. file-read).  Additional fragments (with the same request ID) may be used to send parameters (e.g. path or file descriptor).  Finally, the request message is terminated by an end-of-message fragment (again, with the same request ID).

A response is a message sent back to a request's sender.  Unlike a request message, it doesn't begin with an initial fragment indicating that it's a response.  The response consists of zero or more result fragments followed by an end-of-message fragment, all with the request ID of the request to which they're a response.  Alternately, the response may be terminated by an error fragment, indicating that the operation failed.  Either way, an end-of-message or error fragment indicates that processing of the request is complete and the sender is free to reuse the ID for another request.

