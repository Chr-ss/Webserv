#pragma once

# include "Server.hpp"
# include "Headers.hpp"

class Webserv {
	private:
		std::vector<Server>	m_servers;
		int					m_epoll_fd;
		epoll_event			m_events[MAX_EVENTS];

	public:
		Webserv();
		~Webserv();

		Server*	getServer( int fd );
		Client* getClient( int fd );
		void	addServer( Server& server );
		void	createEpoll( void );
		int		mainLoop(void);
};

// aways first create Epoll (already with Webserv())
// add Server while creating them