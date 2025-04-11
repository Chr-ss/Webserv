#pragma once

# include <iostream>
# include <cstring>
# include <vector>
# include <regex>
# include <cassert>

# include <unistd.h>
# include <sys/wait.h>
# include <sys/epoll.h>

# include "../HTTP/HTTPRequest.hpp"
# include "../Webserv/SharedFd.hpp"
#include "CGIPipes.hpp"

#ifndef READ
# define READ 0
#endif

#ifndef WRITE
# define WRITE 1
#endif

# define TIMEOUT 3

enum e_cgi_state {
	START_CGI,
	SEND_TO_CGI,
	RCV_FROM_CGI,
	CRT_RSPNS_CGI
};

class CGI {
	private:
		std::string 		path_;
		CGIPipes			pipes_;
		pid_t				pid_;
		int					status_;
		std::string 		response_;
		std::string			post_data_;
		epoll_event			epoll_event_pipe_[2];
		e_cgi_state			CGI_STATE_;
		time_t				start_time_;
		bool				timeout_;
		std::function<void(int)> delFromEpoll_cb_;

		// START_CGI
		std::vector<char*>	createEnv(HTTPRequest &request );
		void				execCGI(HTTPRequest &request);
		bool				isCGIProcessFinished( void );
		bool				isCGIProcessSuccessful( void );
		bool				hasCGIProcessTimedOut(void);
		void				cleanupProcess(void);

		// SEND_TO_CGI
		void				sendDataToCGI( const SharedFd &fd );

		// RCV_FROM_CGI
		void				getResponseFromCGI( const SharedFd &fd );
		std::string			receiveBuffer( const SharedFd &fd );
		int					getStatusCodeFromResponse( void );

		// CGI UTILS
		std::vector<char*>	createArgvVector( const std::string &executable );

	public:
		explicit CGI( const std::string &post_data, CGIPipes pipes, std::function<void(int)> delFromEpoll_cb );
		~CGI( void );
		
		std::string			getResponse( void );
		bool				isReady( void );
		bool				isTimeout(void);
		void				handle_cgi( HTTPRequest &request, const SharedFd &fd );
		bool				isNPHscript( const std::string &executable );
		void				rewriteResonseFromCGI( void );
		
		static bool			isCgiScript( const std::string &path );
		static std::string	getScriptExecutable( const std::string &path );
		

		
};
