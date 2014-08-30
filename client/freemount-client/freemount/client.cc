/*
	freemount/client.cc
	-------------------
*/

#include "freemount/client.hh"

// Standard C
#include <string.h>


#define STR_LEN( s )  "" s, (sizeof s - 1)


namespace freemount
{
	
	static const char* uloop_argv[] = { NULL, "uloop", NULL };
	
	static const char* ulocal_argv[] = { NULL, "ulocal", NULL, NULL };
	
	static const char* utcp_argv[] = { NULL, "utcp", NULL, "4564", NULL };
	
	static const char* ussh_argv[] = { NULL, "ussh", "--", NULL, "bin/freemountd", "-q", "--root", ".", NULL };
	
	
	static const char** parse_mnt_hostpath( char* hostpath )
	{
		char* slash = strchr( hostpath, '/' );
		
		if ( slash != NULL )
		{
			char* p = slash;
			
			*p++ = '\0';
			
			if ( *p != '\0' )
			{
				utcp_argv[ 0 ] = p;
			}
		}
		
		utcp_argv[ 2 ] = hostpath;  // "host" or "host:port"
		
		if ( strchr( hostpath, ':' ) )
		{
			utcp_argv[ 3 ] = NULL;
		}
		
		return utcp_argv + 1;
	}
	
	static const char** parse_unix_path( const char* path )
	{
		ulocal_argv[ 2 ] = path;
		
		return ulocal_argv + 1;
	}
	
	static const char** parse_ssh_path( char* path )
	{
		if ( path[ 0 ] != '\0' )
		{
			ussh_argv[ 7 ] = path;
			
			char* slashes = strstr( path, "//" );
			
			if ( slashes != NULL )
			{
				*slashes++ = '\0';
				
				ussh_argv[ 0 ] = slashes;
			}
		}
		
		return ussh_argv + 1;
	}
	
	const char** parse_address( char* address )
	{
		if ( address == NULL )
		{
			return uloop_argv + 1;
		}
		
		char* p = address;
		
		while ( *p != ':' )
		{
			if ( *p == '\0'  ||  *p == '/' )
			{
				ulocal_argv[ 2 ] = address;  // pathname
				
				return ulocal_argv + 1;
			}
			
			++p;
		}
		
		char* colon = p++;
		
		// ":" -> uloop
		
		if ( colon == address )
		{
			if ( *p == '\0' )
			{
				return uloop_argv + 1;
			}
			
			return NULL;
		}
		
		// "scheme://..."
		
		if ( p[ 0 ] == '/'  &&  p[ 1 ] == '/' )
		{
			p += 2;
			
			// "mnt://host..." -> utcp
			
			if ( memcmp( address, STR_LEN( "mnt:" ) ) == 0 )
			{
				return parse_mnt_hostpath( p );
			}
			
			// "unix://path..." -> ulocal
			
			if ( memcmp( address, STR_LEN( "unix:" ) ) == 0 )
			{
				return parse_unix_path( p );
			}
			
			// unrecognized URI scheme
			
			return NULL;
		}
		
		// "sshhost:path" -> ussh
		
		*colon = '\0';
		
		ussh_argv[ 3 ] = address;  // "sshhost"
		
		return parse_ssh_path( p );
	}
	
}
