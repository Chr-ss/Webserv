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
		int					pipe_to_CGI_[2];
		int					pipe_from_CGI_[2];
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
		std::vector<char*>	createEnv( std::vector<std::string> &envStrings, HTTPRequest &request );
		void				forkCGI( const std::string &executable, std::vector<std::string> env_vector );
		bool				isCGIProcessFinished( void );
		bool				isCGIProcessSuccessful( void );

		// SEND_TO_CGI
		bool				sendDataToStdinReady( const SharedFd &fd );

		// RCV_FROM_CGI
		bool				getResponseFromCGI( const SharedFd &fd );
		std::string			receiveBuffer( const SharedFd &fd );
		int					getStatusCodeFromResponse( void );

		// CGI UTILS
		void	createArgvVector( std::vector<char*> &argv_vector, const std::string &executable );
		void	createEnvCharPtrVector( std::vector<char*> &env_c_vector, std::vector<std::string> &env_vector );
		void	throwException( const char *msg );
		void	throwExceptionExit( const char *msg );

	public:
		explicit CGI( const std::string &post_data, std::vector<SharedFd> pipes, std::function<void(int)> delFromEpoll_cb );
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
