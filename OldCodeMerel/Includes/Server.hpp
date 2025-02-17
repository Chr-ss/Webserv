#pragma once

#include "Headers.hpp"
#include "Client.hpp"

class Server {
	private:
		int										m_fd;
		uint16_t								m_port;
		std::vector<std::unique_ptr<Client>>	m_clients;
	public:
		Server( uint16_t port );
		~Server( void );

		void initFd( void );
		void setSocketOptions( void );
		void bindSocket( void );
		void listenSocket( int backlog );
		
		void			addClient( std::unique_ptr<Client> &Client );
		const uint16_t	getPort( void ) const;
		const int		getFd( void ) const;
		Client*			getCurrentClient( int fd );
};
