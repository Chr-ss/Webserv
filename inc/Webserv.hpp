#pragma once

#include "Client.hpp"
#include "sock.hpp"
#include "Epoll.hpp"
#include "SharedFd.hpp"
#include "ConfigParser.hpp"
#include "Config.hpp"

#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <string>


class Webserv {
public:
	Webserv(const std::string& confPath);
	Webserv(const Webserv& other) = delete;
	Webserv& operator=(const Webserv& other) = delete;
	~Webserv();

	void	mainLoop();

private:
	std::unordered_map<SharedFd, std::vector<Config>>	_servers;
	std::unordered_map<SharedFd, Client> 				_clients;
	Epoll												_ep;

	void	_addClient(const SharedFd& clientSock, const SharedFd& servSock);
	void	_delClient(const SharedFd& fd);
};
