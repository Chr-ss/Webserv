#pragma once

#include <exception>
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

# include "../Config/Config.hpp"
# include "../Webserv/Epoll.hpp"
# include "../Webserv/SharedFd.hpp"
# include "../CGI/CGIPipes.hpp"
# include "../CGI/CGI.hpp"
# include "HTTPResponse.hpp"
# include "HTTPRequest.hpp"
# include "HTTPParser.hpp"

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
		HTTPClient(
			SharedFd clientFd,
			SharedFd serverFd,
			std::function<void(struct epoll_event, const SharedFd&)> addToEpoll_cb,
			std::function<const Config* (const SharedFd& serverSock, const std::string& serverName)> getConfig_cb,
			std::function<void(const SharedFd&)> delFromEpoll_cd
		);
		HTTPClient(const HTTPClient &other);
		~HTTPClient( void );

		void	handle( const epoll_event &event );
		bool	isDone( void );
		void	setServer(std::vector<std::string> host);

		void		writeTo( const SharedFd &fd );
		std::string	readFrom( int fd );

		void			cgiResponse( void );
		const Config	*getConfig( void ) const;
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
		std::string					remaining_write_;

		std::function<void(struct epoll_event, const SharedFd&)> addToEpoll_cb_;
		std::function<const Config* (const SharedFd& serverSock, const std::string& serverName)> getConfig_cb_;
		std::function<void(const SharedFd&)> delFromEpoll_cb_;

		void	setRequestDataAndConfig( void );
		void	responding( bool cgi_used, const SharedFd &fd);
		bool	isCGI();
		bool	cgi( const SharedFd &fd );

};

///@brief Exception specific to a client - not critical for server
class ClientException : public std::exception {
public:
	explicit ClientException(const std::string& msg) : message_(msg) {}
	const char* what() const noexcept override  { return message_.c_str(); }

private:
	std::string	message_;
};

