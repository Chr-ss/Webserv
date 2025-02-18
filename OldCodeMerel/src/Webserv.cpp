# include "../Includes/Headers.hpp"
# include "../Includes/Webserv.hpp"

extern std::atomic<bool> keep_alive;

Webserv::Webserv() {
	createEpoll();
}

Webserv::~Webserv() {}

void	Webserv::addServer(Server& server) {
	m_servers.push_back(std::move(server));
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

void	Webserv::addClient(int n, struct epoll_event &events, Server *server) {
	std::unique_ptr<Client> newClient;
	struct sockaddr_in		address;
	socklen_t				addreslen;
	int conn_sock;
	int flags;

	addreslen = sizeof(address);
	conn_sock = accept(n, (struct sockaddr *)&address, &addreslen);
	if (conn_sock == -1)
		throw std::runtime_error("Accept failed");
	flags = fcntl(conn_sock, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl error");
	fcntl(conn_sock, F_SETFL, O_NONBLOCK);

	newClient = std::make_unique<Client>(n, server);
	newClient->setIp(address.sin_addr.s_addr);
	
	// IN = data available and ET = Edge-Triggered, only when something changes EPOLLET
	events.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	events.data.fd = conn_sock;
	newClient->setFd(conn_sock);
	if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, conn_sock, &events) == -1)
	{
		close(conn_sock);
		throw std::runtime_error("Epoll_ctl: conn_sock failed");
	}
	if (fcntl(conn_sock, F_SETFL, O_NONBLOCK) < 0)
	{
    	close(conn_sock);
		throw std::runtime_error("fcntl error");
	}
	server->addClient(newClient);
}

int	Webserv::mainLoop(void)
{
	
	struct epoll_event	events[MAX_EVENTS];
	struct epoll_event	ev;
	int					timeout;
	int					nfds;
	Server*				currentServer;
	Client*				currentClient;

	
	ev.events = EPOLLIN;
	timeout = 1000;
	while(keep_alive)
	{
		nfds = epoll_wait(m_epoll_fd, &ev, MAX_EVENTS, timeout);
		if (nfds <= 0)
		{
			if (nfds == -1)
				std::perror(RED "Epoll wait failed" RES);
			continue ;
		}
		for (int n = 0; n < nfds; ++n)
		{
			try
			{
				currentServer = getServer(n);
				if (currentServer != nullptr) // fd from server comes in, means that a new Client wants to join
					addClient(n, events[n], currentServer);
				else
				{
					currentClient = getClient(n);
					if (!currentClient)
						std::cout << "Not a server or client" << std::endl;
					else
						currentClient->parseData(n);
				}
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
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

