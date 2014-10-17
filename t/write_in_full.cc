/*
	write_in_full.cc
	----------------
*/

// POSIX
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

// freemount
#include "freemount/write_in_full.hh"

// tap-out
#include "tap/check.hh"
#include "tap/test.hh"


static const unsigned n_tests = 3 + 3 * 8;


using freemount::write_in_full;
using freemount::failed_write;


static sig_atomic_t caught_sigalrm;
static sig_atomic_t caught_sigpipe;

static int fd_to_read  = -1;
static int fd_to_close = -1;

const size_t n_to_read = 65536;

const size_t read_buffer_size = sizeof (unsigned int) * n_to_read;

static unsigned int* read_buffer;

static unsigned int last_i = 0;

static bool data_read_matches_written = true;

static void check_read( int fd )
{
	ssize_t n_read = CHECK( read( fd, read_buffer + last_i, read_buffer_size ) );
	
	unsigned int i = last_i;
	
	last_i = n_read / sizeof (unsigned int);
	
	for ( ;  i < last_i;  ++i )
	{
		if ( read_buffer[ i ] != i )
		{
			data_read_matches_written = false;
			
			break;
		}
	}
}

static void sigalrm_read_handler( int )
{
	if ( fd_to_read >= 0 )
	{
		check_read( fd_to_read );
		
		alarm( 1 );
	}
}

static void multi_write()
{
	read_buffer = (unsigned int*) ::operator new( read_buffer_size );
	
	for ( unsigned int i = 0;  i < n_to_read;  ++i )
	{
		read_buffer[ i ] = i;
	}
	
	int fds[ 2 ];
	
	CHECK( pipe( fds ) );
	
	fd_to_read = fds[0];
	
	CHECK( (long) signal( SIGALRM, &sigalrm_read_handler ) );
	
	CHECK( alarm( 1 ) );
	
	CHECK( write( fds[1], read_buffer, sizeof (unsigned int) ) );
	
	bool exception_caught = false;
	
	try
	{
		write_in_full( fds[1], read_buffer + 1, read_buffer_size - sizeof (unsigned int) );
	}
	catch ( const failed_write& error )
	{
		exception_caught = true;
	}
	
	alarm( 0 );
	
	close( fds[1] );
	
	fd_to_read = -1;
	
	check_read( fds[0] );
	
	EXPECT( last_i > 0 );
	
	EXPECT( data_read_matches_written );
	
	EXPECT( !exception_caught );
	
	::operator delete( read_buffer );
	
	read_buffer = NULL;
}

static void sigalrm_handler( int )
{
	if ( caught_sigalrm++ )
	{
		CHECK( close( fd_to_close ) );
		
		fd_to_close = -1;
	}
	
	CHECK( alarm( 1 ) );
}

static void sigpipe_handler( int )
{
	++caught_sigpipe;
}

static void set_fd_nonblocking( int fd )
{
	int flags = CHECK( fcntl( fd, F_GETFL, 0 ) );
	
	CHECK( fcntl( fd, F_SETFL, flags | O_NONBLOCK ) );
}

static void pipe_buffer_overrun( size_t buffer_size, bool nonblocking, bool closing_writer )
{
	char* local_buffer = (char*) ::operator new( buffer_size );
	
	int fds[ 2 ];
	
	CHECK( pipe( fds ) );
	
	fd_to_close = fds[ closing_writer ];
	
	if ( nonblocking )
	{
		set_fd_nonblocking( fds[1] );
	}
	
	int errnum = 0;
	
	caught_sigalrm = 0;
	caught_sigpipe = 0;
	
	CHECK( (long) signal( SIGALRM, &sigalrm_handler ) );
	CHECK( (long) signal( SIGPIPE, &sigpipe_handler ) );
	
	CHECK( alarm( 1 ) );
	
	try
	{
		for ( ;; )
		{
			write_in_full( fds[1], local_buffer, buffer_size );
		}
	}
	catch ( const failed_write& error )
	{
		errnum = error.errnum;
	}
	
	alarm( 0 );
	
	EXPECT( caught_sigalrm == 2 );
	
	EXPECT( caught_sigpipe == !closing_writer );
	
	EXPECT( errnum == (closing_writer ? EBADF : EPIPE) );
	
	close( fds[0] );
	close( fds[1] );
	
	::operator delete( local_buffer );
}

int main( int argc, char** argv )
{
	tap::start( "write_in_full", n_tests );
	
	multi_write();
	
	for ( int i = 0;  i < 8;  ++i )
	{
		const size_t buffer_size = i < 4 ? 512 : 512 * 1024;
		
		const bool nonblocking    = i & 2;
		const bool closing_writer = i & 1;
		
		pipe_buffer_overrun( buffer_size, nonblocking, closing_writer );
	}
	
	return 0;
}

