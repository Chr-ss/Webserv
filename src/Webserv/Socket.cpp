#include "../../inc/Webserv/Socket.hpp"

Socket::Socket() : _fd(::socket(AF_INET, SOCK_STREAM, 0)) {
	if (_fd == -1) {
		throw std::runtime_error(std::string("socket(): ") + strerror(errno));
	}
	// REMOVE!
	int opt = 1;
	if (setsockopt(_fd.get(), SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
		// std::cerr << "Setsockopt failed\n";
		throw std::runtime_error(std::string("socket(): ") + strerror(errno));
	}
}

Socket::Socket(int sockFd) : _fd(sockFd) {
	if (_fd < 0) {
		throw std::runtime_error(std::string("socket(int sockFd): sockFd < 0"));
	}
}

Socket& Socket::operator=(int sockFd) {
	if (_fd < 0) {
		throw std::runtime_error(std::string("socket(int sockFd): sockFd < 0"));
	}

	_fd = sockFd;
	return (*this);
}

Socket::Socket(const Socket& other) {
	if (this != &other) {
		*this = other;
	}
}

Socket& Socket::operator=(const Socket& other) {
	_fd = other._fd;
	return (*this);
}

Socket::~Socket() {
	// std::cerr << "close fd: " << _fd.get() << std::endl;
	try {
		close(_fd.get());
	}
	catch (std::exception &e) {
		std::cerr << "close(): " << e.what() << std::endl;
	}
}

void	Socket::bind(in_addr_t ipv4Addr, in_port_t port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipv4Addr;
	addr.sin_port = port;

	if (::bind(_fd.get(), (const struct sockaddr*)&addr, sizeof(addr)) == -1) {
		throw std::runtime_error(std::string("bind(): ") + strerror(errno));
	}
}

void	Socket::listen(int backlog) {
	if (::listen(_fd.get(), backlog) == -1) {
		throw std::runtime_error(std::string("listen(): ") + strerror(errno));
	}
}

int	Socket::accept() {
	int	newClient;

	if ((newClient = ::accept(_fd.get(), nullptr, nullptr)) == -1) {
		throw std::runtime_error(std::string("accept(): ") + strerror(errno));
	}
	return (newClient);
}

void	Socket::connect(in_addr_t ipv4Addr, in_port_t port) {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ipv4Addr;
	addr.sin_port = port;

	if (::connect(_fd.get(), (const struct sockaddr*)&addr, sizeof(addr)) == -1) {
		throw std::runtime_error(std::string("connect(): ") + strerror(errno));
	}
}
