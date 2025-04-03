#pragma once

# include <vector>
# include <functional>
# include <cstring>

# include <sys/types.h>
# include <fcntl.h>

# include "../Webserv/Epoll.hpp"

#ifndef WRITE
# define WRITE 0
#endif

#ifndef READ
# define READ 1
#endif

#ifndef FROM_CHILD_READ
# define FROM_CHILD_READ 0
#endif

#ifndef TO_CHILD_WRITE
# define TO_CHILD_WRITE 3
#endif

typedef std::function<void(struct epoll_event, const SharedFd&)> ADD_EPOLL_CBFUNCTION;

class CGIPipes {
	private:
		std::vector<int>		pipes_;
		ADD_EPOLL_CBFUNCTION	addToEpoll_cb_;
		SharedFd				client_sock_;

	public:
		CGIPipes( void );
		~CGIPipes( void );

		void	setCallbackFunction( ADD_EPOLL_CBFUNCTION callback, const SharedFd& client_sock );
		void	addNewPipes( void );
		void	addPipesToEpoll( void );
		void	closeAllPipes( void );
		
		std::vector<int>	getPipes( void );
};
