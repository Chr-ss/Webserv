#include "../../inc/Webserv/Epoll.hpp"

Epoll::Epoll() :
	_epFd(epoll_create1(EPOLL_CLOEXEC)),
	_timeout(TIMEOUT_DFLT)
{
	if (_epFd == -1) {
		throw std::runtime_error(std::string("epoll_create(): ") + strerror(errno));
	}
	_events.reserve(MAX_EVENTS);
}

// add a file descriptor to epoll
// - fd: 	fd to monitor
// - data:	other data - can be used to store associated fd
void	Epoll::add(int fd, int data, u_int32_t events) {
	// std::cerr << "add fd: " << fd << std::endl;
	if (fd == -1)
		throw std::runtime_error(std::string("fd == -1 in add Epoll"));
	_eventDataStorage[fd] = {fd, data};

	epoll_event	ev;
	ev.events = events;
	ev.data.ptr = &_eventDataStorage.at(fd);

	if (epoll_ctl(_epFd.get(), EPOLL_CTL_ADD, fd, &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(add): ") + strerror(errno));
	}
}

void	Epoll::add(int fd, u_int32_t events) {
	add(fd, -1, events);
}

void	Epoll::mod(int fd, int data, u_int32_t events) {
	_eventDataStorage[fd] = {fd, data};

	epoll_event	ev;
	ev.events = events;
	ev.data.ptr = &_eventDataStorage.at(fd);
	if (epoll_ctl(_epFd.get(), EPOLL_CTL_MOD, fd, &ev) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(mod): ") + strerror(errno));
	}
}

const struct epollEventData&	Epoll::getEventData(const struct epoll_event& ev) {
	if (!ev.data.ptr) {
		throw std::runtime_error("getEventData(): data.ptr is null");
	}
	return (*static_cast<epollEventData*>(ev.data.ptr));
}

void	Epoll::mod(int fd, u_int32_t events) {
	mod(fd, -1, events);
}

void	Epoll::del(int fd) {
	auto it = _eventDataStorage.find(fd);

	if (it == _eventDataStorage.end()) {
		throw std::runtime_error("Epoll::del(): trying to delete unmonitored fd: " + std::to_string(fd));
	}

	// std::cerr << "del fd: " << fd << " from client: " << it->second.data << std::endl;
	_eventDataStorage.erase(it);

	if (epoll_ctl(_epFd.get(), EPOLL_CTL_DEL, fd, nullptr) == -1) {
		throw std::runtime_error(std::string("epoll_ctl(del): ") + strerror(errno));
	}
}

const std::vector<struct epoll_event>&	Epoll::wait() {
	int	ready;

	_events.resize(MAX_EVENTS);
	if ((ready = epoll_wait(_epFd.get(), _events.data(), MAX_EVENTS, _timeout)) == -1) {
		if (errno == EINTR) {
			_events.resize(0);
			return (_events);
		}
		throw std::runtime_error(std::string("epoll_wait(): ") + strerror(errno));
	}
	_events.resize(ready);
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
