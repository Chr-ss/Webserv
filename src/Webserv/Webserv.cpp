#include "../../inc/Webserv/Webserv.hpp"

std::atomic<bool> keepalive(true);

void	signalhandler(int signum)
{
	if (signum == 13 || signum == 2)
		keepalive = false;
}

Webserv::Webserv(const std::string& confPath) {
	std::unordered_map<std::string, SharedFd> sockets;

	for (auto& config : ConfigParser(confPath).getConfigs()) {
		SharedFd	serverFd;

		std::string key = config.getHost() + std::to_string(config.getPort());
		const auto& it = sockets.find(key);
		if (it == sockets.end()) {
			// no fitting socket existing
			serverFd = sock::create();
			sock::bind(serverFd.get(), inet_addr(config.getHost().c_str()), htons(config.getPort()));
			sock::listen(serverFd.get(), 5);

			sockets[key] = serverFd;
		} else {
			// use existing socket
			serverFd = it->second;
		}

		_ep.add(serverFd.get(), EPOLLIN);
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

void	Webserv::mainLoop() {
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
				// server socket ready
				SharedFd clientSock = sock::accept(fd.get());
				this->_addClient(clientSock, fd);
			} else if (_clients.find(fd) != _clients.end()) {
				//  client socket ready
				_clients.find(fd)->second.handle(ev);
			} else if (_clients.find(ev.data.u32) != _clients.end()) {
				_clients.find(ev.data.u32)->second.handle(ev);
				if (_clients.find(ev.data.u32)->second.isDone())
					_clients.erase(_clients.find(ev.data.u32)); // REMOVE !!!
				// client pipe ready
			} else {
				throw std::runtime_error("mainLoop(): fd not found");
			}
		}
	}
	std::cerr << "Webserver is shutting down" << std::endl;
}
