/*
	fmping.cc
	---------
*/

// POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

// Standard C
#include <stdlib.h>

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"
#include "freemount/send.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

const int interval = 1;

static int count = -1;

static int event_loop_result;

static int fragment_handler( void* that, const fragment_header& fragment )
{
	switch ( fragment.type )
	{
		case frag_ping:
			write( STDERR_FILENO, STR_LEN( "ping\n" ) );
			
			send_empty_fragment( protocol_out, frag_pong );
			break;
		
		case frag_pong:
			write( STDERR_FILENO, STR_LEN( "pong\n" ) );
			break;
		
		default:
			abort();
	}
	
	return 0;
}

static void* event_loop_start( void* arg )
{
	data_receiver r( &fragment_handler, NULL );
	
	event_loop_result = run_event_loop( r, protocol_in );
	
	return NULL;
}

static void* pinger_start( void* arg )
{
	for ( ;; )
	{
		send_empty_fragment( protocol_out, frag_ping );
		
		if ( count > 0  &&  --count == 0 )
		{
			break;
		}
		
		sleep( interval );
	}
	
	shutdown( protocol_out, SHUT_WR );
	
	return NULL;
}

static void get_options( int argc, char** argv )
{
	if ( *argv == NULL )
	{
		return;
	}
	
	bool saw_dash = false;
	
	while ( const char* arg = *++argv )
	{
		if ( arg[0] == '-' )
		{
			if ( arg[1] == '-' )
			{
				if ( arg[2] == '\0' )
				{
					saw_dash = true;
					
					continue;
				}
				
				exit( 2 );  // no long options
			}
			
			switch ( arg[1] )
			{
				case 'c':
					count = atoi( arg[2] ? arg + 2 : *++argv );
					break;
				
				case '\0':  // "-" argument not recognized
				default:
					exit( 2 );  // undefined option
			}
		}
		else
		{
			exit( 2 );  // non-option arguments not recognized
		}
	}
}

int main( int argc, char** argv )
{
	get_options( argc, argv );
	
	const char* connector = getenv( "FREEMOUNT_CONNECT" );
	
	if ( connector == NULL )
	{
		connector = getenv( "UNET_CONNECT" );
	}
	
	if ( connector == NULL )
	{
		connector = "uloop";
	}
	
	const char* connector_argv[] = { "/bin/sh", "-c", connector, NULL };
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	pthread_t event_loop;
	pthread_t pinger;
	
	pthread_create( &event_loop, NULL, &event_loop_start, NULL );
	pthread_create( &pinger,     NULL, &pinger_start,     NULL );
	
	pthread_join( event_loop, NULL );
	
	return event_loop_result != 0;
}

