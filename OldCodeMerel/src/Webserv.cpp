# include "Headers.hpp"
# include "Webserv.hpp"

extern std::atomic<bool> keep_alive(true);

Webserv::Webserv() {
	createEpoll();
}

Webserv::~Webserv() {}

void	Webserv::addServer(Server& server) {
	m_servers.push_back(server);
}

Client* Webserv::getClient( int fd ) {
	Client *client;

	for (auto it = m_servers.begin(); it != m_servers.end(); it++)
	{
		client = (*it).getCurrentClient(fd);
		if (client != nullptr)
			return (client);
	}
	return (client);
}

void	Webserv::createEpoll()
{
	m_epoll_fd = epoll_create(m_servers.size()); 
	if (m_epoll_fd == -1)
	{
		std::perror(RED "Epoll_create failed" RES);
		// close(m_listen_sock); in server 
		std::exit(1);
	}
	for (auto it = m_servers.begin(); it != m_servers.end(); it++)
	{
		epoll_event ev;

		std::memset(&ev, 0, sizeof(ev));
	    ev.events = EPOLLIN;
	    ev.data.fd = (*it).getFd();
	    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, (*it).getFd(), &ev) == -1)
		{
			{
				std::perror(RED "Epoll_ctl: listen_sock failed" RES);
				close(m_epoll_fd);
				std::exit(1);
			}
		}
	}
}

int	Webserv::mainLoop(void)
{
	
	struct epoll_event	events[MAX_EVENTS];
	struct epoll_event	ev;
	struct sockaddr_in	address;
	uint32_t			ip_addr;
	socklen_t			addreslen;
	int					timeout;
	int					nfds;
	int 				conn_sock;
	int 				flags;

	addreslen = sizeof(address);
	ev.events = EPOLLIN;
	timeout = 1000;
	while(keep_alive)
	{
		nfds = epoll_wait(m_epoll_fd, &ev, MAX_EVENTS, timeout);
		if (nfds == -1)
		{
			std::perror(RED "Epoll wait failed" RES);
			return (1);
		}
		if (nfds == 0)
			continue ;
		for (int n = 0; n < nfds; ++n)
		{
			Server* currentServer = getServer(n);
			if (currentServer != nullptr) // fd from server comes in, means that a new Client wants to join
			{
				std::cout << "Add client here" << std::endl;
				conn_sock = accept(n, (struct sockaddr *)&address, &addreslen);
				if (conn_sock == -1)
				{
					std::perror(RED "Accept failed" RES);
					continue ;
				}
				flags = fcntl(conn_sock, F_GETFL, 0);
				if (flags == -1) {
					std::perror(RED "Fcntl failed" RES);
					return(1);
				}

				fcntl(conn_sock, F_SETFL, O_NONBLOCK);
				ip_addr = address.sin_addr.s_addr;

				std::unique_ptr<Client> newClient = std::make_unique<Client>(n);
				newClient->setIp(ip_addr);
				
				// IN = data available and ET = Edge-Triggered, only when something changes EPOLLET
				events[n].events = EPOLLIN | EPOLLERR | EPOLLHUP;
				events[n].data.fd = conn_sock;
				newClient->setFd(conn_sock);
				if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, conn_sock, &events[n]) == -1)
				{
					std::perror(RED "Epoll_ctl: conn_sock failed");
					close(conn_sock);
					continue ;
				}
				if (fcntl(conn_sock, F_SETFL, O_NONBLOCK) < 0)
        			throw std::runtime_error("fcntl error");
				currentServer->addClient(newClient);
			}
			else
			{
				Client* currentClient = getClient(n);
				if (!currentClient)
					std::cout << "Not a server or client" << std::endl;
				else
					currentClient->parseData(n);
			}
		}
	}
	return (0);
}

Server*	Webserv::getServer(int fd) {
	for (auto it = m_servers.begin(); it != m_servers.end(); it++)
	{
		if ((*it).getFd() == fd)
			return &(*it);
	}
	return (nullptr);
}

