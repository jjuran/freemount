/*
	freemount/client.hh
	-------------------
*/

#ifndef FREEMOUNT_CLIENT_HH
#define FREEMOUNT_CLIENT_HH


namespace freemount
{
	
	/*
		Input:  A URI for a server address, or ":" or NULL for loopback.
		        Non-loopback strings may be modified and referenced in the
		        result.
		
		Output:  A NULL-terminated argv array with a path as element -1.
		         Clients should exec the result as given and may use the path
		         in whatever way is appropriate.
		         The result structure may reference the input string, and is
		         therefore only valid as long as the input string is in scope
		         (which is the entire life of the process for argv data).
		         
	*/
	
	const char** parse_address( char* address );
	
}

#endif
