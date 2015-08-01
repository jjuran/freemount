/*
	freemount/send_lock.hh
	----------------------
*/

#ifndef FREEMOUNT_SENDLOCK_HH
#define FREEMOUNT_SENDLOCK_HH


namespace freemount
{
	
	class send_lock
	{
		private:
			// non-copyable
			send_lock           ( const send_lock& );
			send_lock& operator=( const send_lock& );
		
		public:
			send_lock();
			~send_lock();
	};
	
#ifdef __RELIX__
	
	inline send_lock::send_lock()
	{
	}
	
	inline send_lock::~send_lock()
	{
	}
	
#endif
	
}

#endif
