/*
	freemount/send_queue.hh
	-----------------------
*/

#ifndef FREEMOUNT_SENDQUEUE_HH
#define FREEMOUNT_SENDQUEUE_HH

// POSIX
#include <sys/types.h>


namespace freemount
{
	
	class buffer
	{
		private:
			static const size_t buffer_length = 1024;
			
			char its_buffer[ buffer_length ];
			
			size_t  its_mark;
		
		public:
			static size_t capacity() { return buffer_length; }
			
			buffer() : its_mark()
			{
			}
			
			const char* data() const  { return its_buffer; }
			size_t      size() const  { return its_mark;   }
			
			size_t freespace() const  { return capacity() - size(); }
			
			void append_UNCHECKED( const void* data, size_t n );
			
			void clear()  { its_mark = 0; }
	};
	
	class send_queue
	{
		private:
			buffer  its_buffer;
			int     its_fd;
		
		public:
			send_queue( int fd ) : its_fd( fd )
			{
			}
			
			void flush();
			
			void add( const void* data, size_t n );
	};
	
}

#endif
