/*
	freemount/frame_size.hh
	-----------------------
*/

#ifndef FREEMOUNT_FRAMESIZE_HH
#define FREEMOUNT_FRAMESIZE_HH

// Standard C
#include <stdint.h>

// iota
#include "iota/endian.hh"

// freemount
#include "freemount/fragment.hh"


namespace freemount
{
	
	inline uint16_t get_size( const fragment_header& frame )
	{
		return iota::u16_from_big( frame.big_size );
	}
	
}


#endif

