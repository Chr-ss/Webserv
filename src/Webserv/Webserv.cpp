
#include "../../inc/Webserv/Webserv.hpp"
#include <sys/epoll.h>

std::atomic<bool> keepalive(true);

void	signalhandler(int signum)
{
	if (signum == 13 || signum == 2)
		keepalive = false;
}

static int	findExistingSocket(const std::vector<std::pair<struct sockaddr_storage, int>>& sockets, const struct addrinfo* temp) {
	for (const auto& it : sockets) {
		if (memcmp(&it.first, temp->ai_addr, temp->ai_addrlen) == 0)
		{
			return (it.second);
		}
	}
	return (-1);
}

static int	createAndBindSock(struct addrinfo* info) {
	int fd = socket(info->ai_family, info->ai_socktype | SOCK_NONBLOCK, info->ai_protocol);
	if (fd == -1) {
		return (-1);
	}

	if (bind(fd, info->ai_addr, sizeof(struct addrinfo)) == -1) {
		close(fd);
		return (-1);
	}
	return (fd);
}

static int	resolveSocket(const std::string& host, const std::string& port) {
	static std::vector<std::pair<struct sockaddr_storage, int>> sockets;
	static struct addrinfo hints = {
		// AI_PASSIVE: if !host, use wildcard address
		// AI_NUMERICSERV: port specified as numeric string
		.ai_flags = AI_PASSIVE | AI_NUMERICSERV,
		// allow IPv4 and IPv6
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_addrlen =0,
		.ai_addr = nullptr,
		.ai_canonname = nullptr,
		.ai_next = nullptr
	};

	// get addrinfo list
	struct addrinfo *results;
	int err_code = getaddrinfo(host.c_str(), port.c_str(), &hints, &results);
	if (err_code != 0)
		throw std::runtime_error("Socket could not be created for host: " + host + " and port: " 
						   + port + " : " + gai_strerror(err_code));

	// try getaddrinfo results
	int res = -1;
	for (struct addrinfo *temp = results; temp != NULL; temp = temp->ai_next) {
		// check whether temp == already existing socket
		res = findExistingSocket(sockets, temp);
		if (res != -1)
			break;

		res = createAndBindSock(temp);
		if (res != -1) {
			// copy sockaddr to storage to save it in vector
			struct sockaddr_storage addr;
			bzero(&addr, sizeof(struct sockaddr_storage));
			memcpy(&addr, temp->ai_addr, temp->ai_addrlen);
			sockets.push_back({addr, res});
			break;
		}
	}
	freeaddrinfo(results);
	if (res == -1) {
		throw std::runtime_error("Socket could not be created for host: " + host + " and port: "
						   + port + ": " + std::string(strerror(errno)));
	}
	if(listen(res, 5) == -1) {  // TODO: replace random number
		throw std::runtime_error("listen(): " + std::string(strerror(errno)));
	}
	return (res);
}

Webserv::Webserv(const std::string& confPath) {
	std::unordered_set<int>	sockets;

	for (auto& config : ConfigParser(confPath).getConfigs()) {
		SharedFd serverFd = resolveSocket(config.getHost(), std::to_string(config.getPort()));

		if (sockets.find(serverFd.get()) == sockets.end()) {
			sockets.insert(serverFd.get());
			_ep.add(serverFd.get(), EPOLLIN);
		}
		_servers[serverFd].push_back(config);
	}
}

Webserv::~Webserv() {
}

// used for lambda in Client Constructor - see addClient
static const Config* getConfig (
	const std::unordered_map<SharedFd, std::vector<Config>>& servers,
	const SharedFd& socket, 
	const std::string& servName
	)
{
	auto it = servers.find(socket);
	if (it == servers.end()) {
		throw std::invalid_argument("getServer(): invalid fd");
	}

	for (auto& server : it->second) {
		if (server.getServerName() == servName) {
			return (&server);
		}
	}
	return(&(it->second[0]));
}

void	Webserv::_addClient(const SharedFd& clientSock, const SharedFd& servSock) {
	if (_clients.find(clientSock) != _clients.end()) {
		throw std::runtime_error("addClient(): trying to add existing client");
	}
	
	_ep.add(clientSock.get(), EPOLLIN | EPOLLOUT);
	_clients.emplace(
		std::make_pair(
		clientSock, 
		HTTPClient(
			clientSock,
			servSock,
			// function to add an epoll event to epoll instance - used for callback
			[this](struct epoll_event ev, const SharedFd& clientSock) {
				// save the client fd to be able to map the pipe fd to the client
				ev.data.u32 = clientSock.get();
				_ep.add(ev.data.fd, ev.events);
			},
			// function to get ptr to config - used for callback
			[this](const SharedFd& servSock, const std::string& servName) {
				return(getConfig(_servers, servSock, servName));
			}
		))
	);
}

void	Webserv::_delClient(const SharedFd& clientSock) {
	const auto& it = _clients.find(clientSock);
	if (it == _clients.end()) {
		throw std::runtime_error("delClient(): trying to del non-existant client");
	}

	_ep.del(clientSock.get());
	_clients.erase(it);
}

void	Webserv::_handleServerReady(SharedFd fd) {
	SharedFd clientSock = accept(fd.get(), nullptr, nullptr);
	if (clientSock == -1)
		throw std::runtime_error(std::string("accept(): ") + strerror(errno));
	_addClient(clientSock, fd);
}

void	Webserv::_handleClientReady(const struct epoll_event& ev) {
	SharedFd fd = ev.data.fd;
	auto it = _clients.find(fd);
	if (it == _clients.end()) {
		// check whether client pipe is ready
		fd = ev.data.u32;
		it = _clients.find(fd);
		if (it == _clients.end()) {
			throw std::runtime_error("_handleClientReady(): unvalid fd");
		}
	}

	auto& client = it->second;
	client.handle(ev);
	// TODO: add logic to remove pipes when client is deleted - e.g. through callback invoked from client destructor
	if (client.isDone()) {
		_delClient(fd);
	}
}

void	Webserv::eventLoop() {
	std::signal(2, signalhandler);
	
	while (keepalive)
	{
		const auto& events = _ep.wait();
		for (const auto& ev : events) {
			// #ifdef DEBUG
			// std::cout << "|SERVER| client: " << ev.data.fd << " "
			// 	<< ((ev.events & EPOLLIN) ? "EPOLLIN " : " ")
			// 	<< ((ev.events & EPOLLOUT) ? "EPOLLOUT " : " ")
			// 	<< ((ev.events & (EPOLLHUP | EPOLLERR)) ? "EPOLLHUP | EPOLLERR" : "") << std::endl;
			// #endif

			SharedFd fd = ev.data.fd;
			if (_servers.find(fd) != _servers.end()) {
				_handleServerReady(fd);
			} else {
				_handleClientReady(ev);
			}
		}
	}
	std::cerr << "Webserver is shutting down" << std::endl;
}
