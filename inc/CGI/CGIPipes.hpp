#pragma once

#include <cstddef>
# include <vector>
# include <functional>
# include <cstring>

# include <sys/types.h>
# include <fcntl.h>

# include "../Webserv/Epoll.hpp"


enum ePipe {
	FROM_CGI_READ,
	FROM_CGI_WRITE,
	TO_CGI_READ,
	TO_CGI_WRITE
};

class CGIPipes {
	private:
		std::vector<SharedFd>										pipes_;
		std::function<void(struct epoll_event, const SharedFd&)>	pipe_add_cb_;
		std::function<void(const SharedFd&)>						pipe_remove_cb_;
		SharedFd													client_fd_;

	public:
		CGIPipes( void );
		~CGIPipes( void );

		SharedFd& operator[](size_t i);

		void	setCallbackFunctions( const SharedFd& client_fd, \
								std::function<void(struct epoll_event, const SharedFd&)> pipe_add_cb, \
								std::function<void(const SharedFd&)> pipe_remove_cb );
		
		void	addNewPipes( void );
		void	deletePipesFromEpoll( SharedFd &fd );
		inline std::vector<SharedFd>	getPipes( void ) { return (pipes_); };
};
