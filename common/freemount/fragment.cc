/*
	freemount/fragment.cc
	---------------------
*/

#include "freemount/fragment.hh"

// iota
#include "iota/endian.hh"


namespace freemount
{
	
	class integer_overflow {};
	class bad_integer_size {};
	
	
	uint32_t get_u32( const fragment_header& frame )
	{
		uint64_t result = get_u64( frame );
		
		if ( result > 0xFFFFFFFFul )
		{
			throw integer_overflow();
		}
		
		return result;
	}
	
	uint64_t get_u64( const fragment_header& frame )
	{
		const int big_sizeof_32 = iota::big_u16( sizeof (uint32_t) );
		const int big_sizeof_64 = iota::big_u16( sizeof (uint64_t) );
		
		if ( frame.big_size == 0 )
		{
			return frame.data;
		}
		
		if ( frame.big_size == big_sizeof_32 )
		{
			return iota::u32_from_big( *(uint32_t*) get_data( frame ) );
		}
		
		if ( frame.big_size == big_sizeof_64 )
		{
			return iota::u64_from_big( *(uint64_t*) get_data( frame ) );
		}
		
		throw bad_integer_size();
	}
	
}
