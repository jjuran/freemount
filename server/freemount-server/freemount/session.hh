/*
	freemount/session.hh
	--------------------
*/

#ifndef FREEMOUNT_SESSION_HH
#define FREEMOUNT_SESSION_HH

// vfs
#include "vfs/node.hh"
#include "vfs/node_ptr.hh"


namespace freemount
{
	
	struct request;
	
	class request_box
	{
		private:
			request* its_request;
			
			void destroy_without_clearing();
			
			// non-copyable
			request_box           ( const request_box& );
			request_box& operator=( const request_box& );
		
		public:
			request_box() : its_request()
			{
			}
			
			~request_box()
			{
				destroy_without_clearing();
			}
			
			void swap( request_box& that )
			{
				request* r = its_request;
				
				its_request = that.its_request;
				
				that.its_request = r;
			}
			
			request* get() const  { return its_request; }
			
			void acquire( request* r )
			{
				destroy_without_clearing();
				
				its_request = r;
			}
			
			void reset()
			{
				acquire( 0 );  // NULL
			}
	};
	
	inline void swap( request_box& a, request_box& b )
	{
		a.swap( b );
	}
	
	
	class session
	{
		private:
			static const int n_requests = 1 << 8;  // 256
			
			request_box its_requests[ n_requests ];
			
			vfs::node_ptr its_root;
			vfs::node_ptr its_cwd;
			
			// non-copyable
			session           ( const session& );
			session& operator=( const session& );
			
		public:
			const int send_fd;
			
		public:
			session( int send_fd, const vfs::node& root, const vfs::node& cwd )
			:
				its_root( &root ),
				its_cwd( &cwd ),
				send_fd( send_fd )
			{
			}
			
			~session();
			
			const vfs::node& root() const  { return *its_root; }
			const vfs::node& cwd () const  { return *its_cwd;  }
			
			request* get_request( int i ) const
			{
				if ( unsigned( i ) >= n_requests )
				{
					return 0;  // NULL
				}
				
				return its_requests[ i ].get();
			}
			
			void set_request( int i, request* r )
			{
				if ( unsigned( i ) >= n_requests )
				{
					return;
				}
				
				its_requests[ i ].acquire( r );
			}
	};
	
}

#endif
