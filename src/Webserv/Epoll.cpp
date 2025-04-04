#include "../../inc/Webserv/Epoll.hpp"

Epoll::Epoll() : 
	_epFd(epoll_create(EPOLL_START_SIZE)),
	_timeout(TIMEOUT_DFLT)
{
	if (_epFd == -1) {
		throw std::runtime_error(std::string("epoll_create(): ") + strerror(errno));
	}
	_events.reserve(MAX_EVENTS);
}

void	Epoll::add(int fd, u_int32_t events) const {
	epoll_event	ev;

	std::cerr << "add fd: " << fd << std::endl;
	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(_epFd.get(), EPOLL_CTL_ADD, fd, &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(add): ") + strerror(errno));
	}
}

void	Epoll::mod(int fd, u_int32_t events) const {
	epoll_event	ev;

	ev.events = events;
	ev.data.fd = fd;
	if (epoll_ctl(_epFd.get(), EPOLL_CTL_MOD, fd, &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(mod): ") + strerror(errno));
	}
}

void	Epoll::del(int fd) const {
	std::cerr << "del fd: " << fd << std::endl;
	if (epoll_ctl(_epFd.get(), EPOLL_CTL_DEL, fd, nullptr) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(del): ") + strerror(errno));
	}
}

const std::vector<struct epoll_event>&	Epoll::wait() {
	int	ready;

	if ((ready = epoll_wait(_epFd.get(), _events.data(), MAX_EVENTS, _timeout)) == -1) {
		throw std::runtime_error(std::string("epoll_wait(): ") + strerror(errno));
	}
	_events.erase(_events.begin() + ready, _events.end());
	return (_events);
}

void	Epoll::setTimeout(int timeout) noexcept {
	_timeout = timeout;
}


int		Epoll::getEpFd() const {
	return (_epFd.get());
}

Epoll::~Epoll() noexcept {
}
