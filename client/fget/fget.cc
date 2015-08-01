/*
	fget.cc
	-------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>

// Standard C
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// more-posix
#include "more/perror.hh"

// command
#include "command/get_option.hh"

// unet-connect
#include "unet/connect.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/frame_size.hh"
#include "freemount/queue_utils.hh"
#include "freemount/receiver.hh"
#include "freemount/send_ack.hh"
#include "freemount/send_queue.hh"
#include "freemount/write_in_full.hh"

// freemount-client
#include "freemount/address.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)


using namespace freemount;


enum
{
	Option_clobber  = 'C',
	Option_continue = 'c',  // resume downloads
	Option_quiet    = 'q',
};

static command::option options[] =
{
	{ "clobber",  Option_clobber  },
	{ "continue", Option_continue },
	{ "quiet",    Option_quiet    },
	{ NULL }
};


static unet::connection_box the_connection;

static int protocol_in  = -1;
static int protocol_out = -1;

static int output_fd = -1;


static const char* the_path;

static uint64_t the_expected_size;
static uint64_t n_written;

static bool clobbering;
static bool resume_downloads;
static bool show_progress;
static float the_divisor;

static int the_result;


static void update_progress()
{
	if ( show_progress  &&  the_expected_size != 0 )
	{
		int percent = int( n_written / the_divisor );
		
		printf( "\r%d%% (%llu / %llu)", percent, n_written, the_expected_size );
		
		fflush( stdout );
	}
}

static void end_progress()
{
	if ( show_progress  &&  the_expected_size != 0 )
	{
		printf( "\n" );
	}
}

static int frame_handler( void* that, const frame_header& frame )
{
	switch ( frame.type )
	{
		case Frame_fatal:
			write( STDERR_FILENO, STR_LEN( "[FATAL]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_error:
			write( STDERR_FILENO, STR_LEN( "[ERROR]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_debug:
			write( STDERR_FILENO, STR_LEN( "[DEBUG]: " ) );
			write( STDERR_FILENO, (const char*) get_data( frame ), get_size( frame ) );
			write( STDERR_FILENO, STR_LEN( "\n" ) );
			return 0;
		
		case Frame_stat_size:
			the_expected_size = get_u64( frame );
			the_divisor       = the_expected_size / 100.0;
			break;
		
		case Frame_recv_data:
			uint16_t size;
			size = get_size( frame );
			
			n_written += size;
			
			send_read_ack( protocol_out, size );
			
			update_progress();
			
			write_in_full( output_fd, get_data( frame ), size );
			
			break;
		
		case Frame_result:
			the_result = get_u32( frame );
			
			shutdown( protocol_out, SHUT_WR );
			
			end_progress();
			break;
		
		default:
			write( STDERR_FILENO, STR_LEN( "Unfrag\n" ) );
			
			abort();
	}
	
	return 0;
}

static const char* name_from_path( const char* path )
{
	if ( const char* slash = strrchr( path, '/' ) )
	{
		path = slash + 1;
	}
	
	if ( *path == '\0' )
	{
		return NULL;
	}
	
	return path;
}

static void open_file( const char* name )
{
	const bool reopening = clobbering || resume_downloads;
	
	const int flags = reopening ? O_WRONLY|O_CREAT
	                            : O_WRONLY|O_CREAT|O_EXCL;
	
	output_fd = open( name, flags, 0666 );
	
	if ( output_fd < 0 )
	{
		more::perror( "fget", name, errno );
		
		exit( 1 );
	}
	
	if ( clobbering )
	{
		const int nok = ftruncate( output_fd, 0 );
		
		if ( nok )
		{
			more::perror( "fget", name, errno );
		
			exit( 1 );
		}
	}
	else if ( resume_downloads )
	{
		struct stat st;
		
		const int nok = fstat( output_fd, &st );
		
		if ( nok )
		{
			more::perror( "fget", name, errno );
		
			exit( 1 );
		}
		
		const off_t mark = lseek( output_fd, st.st_size, SEEK_SET );
		
		if ( mark < 0 )
		{
			more::perror( "fget", name, errno );
		
			exit( 1 );
		}
		
		n_written = mark;
	}
}

static inline void queue_request( send_queue& queue, uint8_t request_type )
{
	queue_int( queue, Frame_request, request_type );
}

static inline void queue_submit( send_queue& queue )
{
	queue_empty( queue, Frame_submit );
}

static void send_read_request( int fd, const char* path, uint32_t size, off_t offset )
{
	send_queue queue( fd );
	
	queue_request( queue, req_read );
	
	queue_string( queue, Frame_arg_path, path, size );
	
	if ( offset != 0 )
	{
		queue_int( queue, Frame_seek_offset, offset );
	}
	
	queue_submit( queue );
	
	queue.flush();
}

static char* const* get_options( char* const* argv )
{
	++argv;  // skip arg 0
	
	short opt;
	
	while ( (opt = command::get_option( &argv, options )) )
	{
		switch ( opt )
		{
			case Option_clobber:
				clobbering = true;
				break;
			
			case Option_continue:
				resume_downloads = true;
				break;
			
			case Option_quiet:
				show_progress = false;
				break;
			
			default:
				abort();
		}
	}
	
	return argv;
}


int main( int argc, char** argv )
{
	if ( argc <= 1  ||  argv[1][0] == '\0' )
	{
		return 0;
	}
	
	show_progress = isatty( 1 );
	
	char *const *args = get_options( argv );
	
	char* address = args[ 0 ];
	
	const char** connector_argv = parse_address( address );
	
	if ( connector_argv == NULL )
	{
		return 2;
	}
	
	the_connection = unet::connect( connector_argv );
	
	protocol_in  = the_connection.get_input ();
	protocol_out = the_connection.get_output();
	
	the_path = connector_argv[ -1 ];
	
	if ( the_path == NULL )
	{
		the_path = "/";
	}
	
	const char* name = name_from_path( the_path );
	
	if ( name == NULL )
	{
		return 2;  // path doesn't end with a non-slash, ergo not a file
	}
	
	open_file( name );
	
	send_read_request( protocol_out, the_path, strlen( the_path ), n_written );
	
	data_receiver r( &frame_handler, NULL );
	
	int looped = run_event_loop( r, protocol_in );
	
	int nok = close( output_fd );
	
	if ( nok )
	{
		more::perror( "fget", name, errno );
		
		return 1;
	}
	
	if ( looped < 0 )
	{
		more::perror( "fget", -looped );
		
		return 1;
	}
	
	if ( the_result != 0 )
	{
		more::perror( "fget", name, the_result );
		
		return 1;
	}
	
	if ( n_written != the_expected_size )
	{
		
		fprintf( stderr,
		         "fget: %s: Transferred size of %llu doesn't match expected size of %llu\n",
		         name,
		         n_written,
		         the_expected_size );
		
		return 1;
	}
	
	return 0;
}
