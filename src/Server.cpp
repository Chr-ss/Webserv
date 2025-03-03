
#include "../inc/Server.hpp"

Server::Server(in_addr_t addr, in_port_t port) {
	_listen.sBind(addr, port);
	_listen.sListen(5);
	_listen.setNonBlock();
	_ep.add(_listen.getFd(), EPOLLIN);
}

Server::~Server() {
}

void Server::addClient() {
	int clientFd = _listen.sAccept(); // throws exception if no pending connection request
	_ep.add(clientFd, EPOLLIN | EPOLLOUT);
	_clients.emplace(clientFd, Client(clientFd));

	#ifdef DEBUG
	std::cout << "|SERVER| New client registered: " << clientFd << std::endl;
	#endif
}

void	Server::handleClient(u_int32_t events, int fd) {
	auto it = _clients.find(fd);
	if (it == _clients.end()) {
		throw std::invalid_argument("handleClient() called with unadded client");
	}

	if (events & EPOLLIN) {
		it->second.readFrom();
	} else if (events & EPOLLOUT) {
		it->second.writeTo();
	} else if (events & (EPOLLHUP | EPOLLERR)) {
		this->delClient(fd);
	}
}

void	Server::delClient(int fd) {
	_ep.del(fd);
	_clients.erase(fd);
	close(fd);

	#ifdef DEBUG
	std::cout << "|SERVER| Client connection closed: " << fd << std::endl;
	#endif
}


void	Server::runServer() {
	while (true) {
		auto events = _ep.wait();
		for (auto ev : events) {

			#ifdef DEBUG
			std::cout << "|SERVER| client: " << ev.data.fd << " "
				<< ((ev.events & EPOLLIN) ? "EPOLLIN " : " ")
				<< ((ev.events & EPOLLOUT) ? "EPOLLOUT " : " ")
				<< ((ev.events & (EPOLLHUP | EPOLLERR)) ? "EPOLLHUP | EPOLLERR" : "") << std::endl;
			#endif

			if (ev.data.fd == _listen.getFd()) {
				this->addClient();
			} else {
				this->handleClient(ev.events, ev.data.fd);
			}
		}
	}
}
