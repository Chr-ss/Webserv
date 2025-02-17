#pragma once

# include <poll.h>
# include <iostream>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <string.h>
# include <bitset>
# include <climits>
# include <cstring>
# include <sys/epoll.h>
# include <errno.h>
# include <unistd.h>
# include <fcntl.h>
# include <cstdio>
# include <fstream>
# include <sstream>
# include <string>
# include <unordered_map>
# include <map>
# include <vector>
# include <cstdlib>
# include <regex>
# include <sys/types.h>
# include <sys/wait.h>
# include <csignal>
# include <atomic>
# include <utility>

# define BUFF_SIZE 4000
# define IP_ZERO "000.000.000.000"
# define ADDRESS_FAMILY AF_INET
# define PORT 8080
# define MAX_EVENTS 100
# define SA struct sockaddr
# define DEFAULT_PATH "data/www/index.html"
# define BUFF_SIZE 4000

# define RES "\033[0m"
# define RED "\033[31m"
# define GRE "\033[32m"
# define BLU "\033[34m"
# define YEL "\033[33m"

struct HTTPRequest
{
	std::string method;
	std::string request_target;
	std::string protocol;
	std::string body;
	bool		invalidRequest;
	std::unordered_map<std::string, std::string> headers;
};

