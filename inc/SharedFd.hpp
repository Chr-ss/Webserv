
#pragma once

#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <cstring>
#include <unordered_map>
#include <stdexcept>

#define UNVALID_FD -1

class	SharedFd {
public:
	SharedFd();
	SharedFd(int fd);
	SharedFd(const SharedFd& other);
	SharedFd& operator=(const SharedFd& other);
	SharedFd& operator=(int fd);
	~SharedFd();

	// comparison operators
	inline bool	operator==(const SharedFd& other) const { return (this->_fd == other._fd); }
	inline bool	operator<(const SharedFd& other) const { return (this->_fd < other._fd); }
	inline bool	operator>(const SharedFd& other) const { return (this->_fd > other._fd); }
	inline bool	operator<=(const SharedFd& other) const { return (this->_fd <= other._fd); }
	inline bool	operator>=(const SharedFd& other) const { return (this->_fd >= other._fd); }

	// conversion operator
	inline explicit operator	int() const { return (_fd); }

	inline bool	isValid() const { return (this->_fd >= 0); }
	inline int	get() const { return (_fd); }
	void		setNonBlock() const;

private:
	int	_fd;
	static std::unordered_map<int, int> _refCounts;
};

// special hash template struct to allow SharedFd as key in unordered map
template<>
struct std::hash<SharedFd> {
	size_t	operator()(const SharedFd& fd) const {
		return(std::hash<int>()(fd.get()));
	}
};
