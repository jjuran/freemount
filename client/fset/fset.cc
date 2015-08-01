/*
	fset.cc
	-------
*/

// POSIX
#include <unistd.h>

// Standard C
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/queue_utils.hh"
#include "freemount/receiver.hh"
#include "freemount/send_queue.hh"

// freemount-client
#include "freemount/client.hh"


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;


struct request_status
{
	ssize_t result;
};

static void report_error( const char* path, uint32_t err )
{
	more::perror( "fset", path, err );
}

static inline void queue_request( send_queue& queue, uint8_t request_type )
{
	queue_int( queue, Frame_request, request_type );
}

static inline void queue_submit( send_queue& queue )
{
	queue_empty( queue, Frame_submit );
}

static void send_put_request( int fd, const char* path, const char* data, size_t size )
{
	send_queue queue( fd );
	
	queue_request( queue, req_write );
	
	queue_string( queue, Frame_arg_path,  path, strlen( path ) );
	queue_string( queue, Frame_send_data, data, size           );
	
	queue_submit( queue );
	
	queue.flush();
}

static int wait_for_result( int fd, frame_handler_function handler, const char* path )
{
	request_status req;
	
	data_receiver r( handler, &req );
	
	int looped = run_event_loop( r, fd );
	
	if ( looped < 0  ||  (looped == 0  &&  (looped = -ECONNRESET)) )
	{
		more::perror( "fset", path, -looped );
		
		exit( 1 );
	}
	
	if ( looped > 2 )
	{
		abort();
	}
	
	int result = req.result;
	
	if ( looped == 2 )
	{
		result = -result;  // errno
	}
	
	return result;
}

static int frame_handler( void* that, const frame_header& frame )
{
	if ( frame.type == Frame_ack_write )
	{
		return 0;
	}
	
	int result = ((request_status*) that)->result = get_u32( frame );
	
	if ( frame.type == Frame_result )
	{
		return result == 0 ? 1 : 2;
	}
	
	return 3;
}

static ssize_t request_put( const char* path, const char* data, size_t length )
{
	send_put_request( protocol_out, path, data, length );
	
	ssize_t n = wait_for_result( protocol_in, &frame_handler, path );
	
	if ( n < 0 )
	{
		report_error( path, -n );
		
		exit( 1 );
	}
	
	return n;
}

int main( int argc, char** argv )
{
	if ( argc <= 2  ||  argv[1][0] == '\0' )
	{
		return 0;
	}
	
	char* address = argv[ 1 ];
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	const char* path = connector_argv[ -1 ];
	
	if ( path == NULL )
	{
		path = "/";
	}
	
	char* data = argv[ 2 ];
	
	size_t len = strlen( data );
	
	data[ len++ ] = '\n';
	
	request_put( path, data, len );
	
	return 0;
}
