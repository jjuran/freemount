/*
	freemount/event_loop.hh
	-----------------------
*/

#ifndef FREEMOUNT_EVENTLOOP_HH
#define FREEMOUNT_EVENTLOOP_HH


namespace freemount
{
	
	class data_receiver;
	
	int run_event_loop( data_receiver& r, int fd );
	
}

#endif

