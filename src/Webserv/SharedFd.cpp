
#include "../../inc/Webserv/SharedFd.hpp"

std::unordered_map<int, int> SharedFd::_refCounts;

SharedFd::SharedFd() : SharedFd(UNVALID_FD) {
}

SharedFd::SharedFd(int fd) : SharedFd(fd, false) {
}

SharedFd::SharedFd(int fd, bool temp) : _fd(fd), _temp(temp) {
	if (!_temp)
		_refCounts[fd]++;
}

SharedFd::SharedFd(const SharedFd& other) : SharedFd() {
	if (this != &other) {
		*this = other;
	}
}

SharedFd& SharedFd::operator=(const SharedFd& other) {
	return (*this = other._fd);
}

SharedFd& SharedFd::operator=(int fd) {
	if (this->_fd != fd) {
		if (!_temp)
			_refCounts[this->_fd]--;
		if (_refCounts[_fd] == 0)
		{
			closeFd();
		}
		this->_fd = fd;
		if (!_temp)
			_refCounts[this->_fd]++;
	}
	return (*this);
}

void	SharedFd::printOpenFds() {
	std::cout << std::left << std::setw(10) << "fd" <<  "count" << std::endl;
	for (auto it : _refCounts) {
		if (it.second > 0) {
			std::cout << std::left << std::setw(10) << it.first << it.second << std::endl;
		}
	}
}

void	SharedFd::closeFd() {
	if (_fd >= 0 && !_temp)
	{
		close(_fd);
	}
}

SharedFd::~SharedFd() {
	if (!_temp)
		_refCounts[_fd]--;
	if (_refCounts[_fd] == 0)
	{
		closeFd();
	}
}

