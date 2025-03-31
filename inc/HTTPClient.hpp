#pragma once

# include <iostream>
# include <bitset>
# include <climits>
# include <cstring>
# include <cstdio>
# include <fstream>
# include <sstream>
# include <string>
# include <unordered_map>
# include <map>
# include <vector>
# include <cstdlib>
# include <regex>
# include <memory>
# include <any>

# include <errno.h>
# include <unistd.h>
# include <fcntl.h>
# include <poll.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <string.h>
# include <sys/epoll.h>

# include "HTTPRequest.hpp"
# include "HTTPRequestConfig.hpp"
# include "HTTPResponse.hpp"
# include "HTTPParser.hpp"
# include "CGI.hpp"
# include "Epoll.hpp"
# include "Server.hpp"
# include "SharedFd.hpp"
# include "CGIPipes.hpp"
# include "Config.hpp"

# define READSIZE 100
# define READY true


enum e_state {
	RECEIVING,
	PROCESS_CGI,
	RESPONSE,
	DONE
};

enum e_get_conf {
	AUTOINDEX,
	BODYSIZE,
	REDIRECT,
	ROOT,
	METHODS,
	INDEX
};

class HTTPClient {
	public:
		explicit HTTPClient(
			SharedFd clientFd,
			SharedFd serverFd,
			std::function<void(struct epoll_event)> addToEpoll_cb,
			std::function<const Config* (const SharedFd& serverSock, const std::string& serverName)> getConfig_cb
		);
		~HTTPClient( void );

		void	handle( epoll_event &event );
		bool	isDone( void );
		void	setServer(std::vector<std::string> host);

		void		writeTo( int fd );
		std::string	readFrom( int fd );

		bool	parsing( int fd );
		bool	cgi( int fd );
		void	responding( bool cgi_used, int fd );
		void	cgiresponse( void );
		const Config	*getConfig( void );

	private:
		e_state						STATE_;

		SharedFd					clientSock_;
		SharedFd					serverSock_;
		std::vector<std::string>	message_que_;
		HTTPParser					parser_;
		HTTPRequest					request_;
		std::unique_ptr<CGI> 		cgi_;
		CGIPipes					pipes_;
		HTTPResponse				responseGenerator_;
		const Config				*config_;

		std::function<void(const std::string& serverName)> _setConfig_cb;
		std::function<const Config* (const SharedFd& serverSock, const std::string& serverName)> getConfig_cb_;
};
