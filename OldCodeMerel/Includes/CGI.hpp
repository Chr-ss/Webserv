#pragma once

#include "Headers.hpp"

class CGI {
	private:
		std::string m_path;
		int			m_in_fd;
		int			m_out_fd;
	public:
		CGI( const std::string &script_path, std::vector<char *> env_vector );
		~CGI( void );

		void		sendBuffer( const std::string &buffer );
		std::string	receiveBuffer( size_t size );
		void		sendEof( void );
		bool		isCgiScript( const std::string &path );
		std::string	getScriptExecutable( const std::string &path );
};