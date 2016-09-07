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
#include "freemount/frame.hh"


namespace freemount
{
	
	inline
	uint16_t get_payload_size( const frame_header& frame )
	{
		return iota::u16_from_big( frame.big_size );
	}
	
	inline
	uint16_t get_size( const frame_header& frame )
	{
		return frame.big_size != 0 ? get_payload_size( frame )
		                           : frame.data != 0;
	}
	
}


#endif
