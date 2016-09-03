/*
	freemount/data_flow.hh
	----------------------
*/

#ifndef FREEMOUNT_DATAFLOW_HH
#define FREEMOUNT_DATAFLOW_HH


namespace freemount
{
	
	void set_congestion_window( long n_bytes );
	
	void data_transmitting( unsigned n_bytes );
	void data_acknowledged( unsigned n_bytes );
	
}

#endif
