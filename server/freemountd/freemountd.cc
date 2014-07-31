/*
	freemountd.cc
	-------------
*/

// POSIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>

// Standard C
#include <string.h>

// vfs
#include "vfs/node.hh"
#include "vfs/node/types/posix.hh"

// freemount
#include "freemount/event_loop.hh"
#include "freemount/receiver.hh"

// freemountd
#include "freemount/server.hh"
#include "freemount/session.hh"


using namespace freemount;


static const char* the_native_root_directory = "/var/freemount";


static const vfs::node& root()
{
	static vfs::node_ptr root = vfs::new_posix_root( the_native_root_directory );
	
	return *root;
}


#define STR_LEN( s )  "" s, (sizeof s - 1)

#define ARRAYLEN( array )  (sizeof array / sizeof array[0])

static void bad_usage( const char* text, size_t text_size, const char* arg )
{
	const iovec iov[] =
	{
		{ (void*) text, text_size    },
		{ (void*) arg, strlen( arg ) },
		{ (void*) STR_LEN( "\n" )    },
	};
	
	writev( STDERR_FILENO, iov, ARRAYLEN( iov ) );
}

#define BAD_USAGE( text, arg )  (bad_usage( STR_LEN( text ": " ), arg ), (char**) NULL)

static const char* find_char( const char* begin, char c )
{
	while ( *begin != '\0'  &&  *begin != c )
	{
		++begin;
	}
	
	return begin;
}

static bool option_matches( const char*  option,
                            size_t       option_size,
                            const char*  name,
                            size_t       name_size )
{
	return option_size == name_size  &&  memcmp( option, name, name_size ) == 0;
}

#define OPTION_MATCHES( option, size, name )  option_matches( option, size, STR_LEN( name ) )

static char** get_options( char** argv )
{
	if ( *argv == NULL )
	{
		// POSIX says we have to check for this
		return argv;
	}
	
	while ( const char* arg = *++argv )
	{
		if ( arg[0] == '-' )
		{
			if ( arg[1] == '\0' )
			{
				// An "-" argument is not an option and means /dev/fd/0
				break;
			}
			
			if ( arg[1] == '-' )
			{
				// long option or "--"
				
				const char* option = arg + 2;
				
				if ( *option == '\0' )
				{
					++argv;
					break;
				}
				
				const char* equals = find_char( option, '=' );
				
				const size_t size = equals - option;
				
				if ( OPTION_MATCHES( option, size, "root" ) )
				{
					if ( *equals == '\0' )
					{
						++argv;
						
						if ( *argv == NULL )
						{
							return BAD_USAGE( "Argument required", arg );
						}
						
						the_native_root_directory = *argv;
					}
					else
					{
						const char* param = equals + 1;
						
						if ( param[0] == '\0' )
						{
							return BAD_USAGE( "Invalid option", arg );
						}
						
						the_native_root_directory = param;
					}
					
					continue;
				}
				
				return BAD_USAGE( "Unknown option", arg );
			}
			
			// short option
			
			const char* opt = arg + 1;
			
			if ( opt[0] == 'q' )
			{
				int dev_null = open( "/dev/null", O_WRONLY );
				
				dup2( dev_null, STDERR_FILENO );
				
				close( dev_null );
				
				continue;
			}
			
			return BAD_USAGE( "Unknown option", arg );
		}
		
		// not an option
		break;
	}
	
	return argv;
}

int main( int argc, char** argv )
{
	char** params = get_options( argv );
	
	if ( params == NULL )
	{
		return 2;
	}
	
	session s( STDOUT_FILENO, root(), root() );
	
	data_receiver r( &frame_handler, &s );
	
	int looped = run_event_loop( r, STDIN_FILENO );
	
	return looped != 0;
}
