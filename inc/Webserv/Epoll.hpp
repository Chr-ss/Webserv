
#pragma once

#include "SharedFd.hpp"

#include <sys/types.h>
#ifndef CONFTESTER
	#include <sys/epoll.h>
#endif
#include <string.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>

#define	TIMEOUT_DFLT 3
#define	MAX_EVENTS 5

// since Linux 2.6.8 ignored anyways
#define	EPOLL_START_SIZE 5

// referenced by void* in epoll_event.data
// - fd: fd that the event belongs to
// - data: additional data - can be used to map to other fd
struct epollEventData {
	int	fd;
	int	data;
};

class Epoll {
public:
	Epoll();
	Epoll(const Epoll &toCopy) = delete;
	Epoll& operator=(const Epoll& toAssign) = delete;
	~Epoll() noexcept;

	void	add(int fd, int data, u_int32_t events);
	void	add(int fd, u_int32_t events);
	void	mod(int fd, int data, u_int32_t events);
	void	mod(int fd, u_int32_t events);
	void	del(int fd);
	const std::vector<struct epoll_event>&	wait();
	static const struct epollEventData&	getEventData(const struct epoll_event& ev);

	void	setTimeout(int timeout) noexcept;
	int		getEpFd() const;

	// TODO: maybe add pwait() from epoll_pwait()

private:
	SharedFd										_epFd;
	int												_timeout;
	std::vector<struct epoll_event>					_events;
	std::unordered_map<int, struct epollEventData>	_eventDataStorage;
};

