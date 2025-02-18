#pragma once

#include "Client.hpp"
#include "Headers.hpp"

class Client;

class Server {
	private:
		int										m_fd;
		uint16_t								m_port;
		std::vector<std::unique_ptr<Client>>	m_clients;
	public:
		Server( uint16_t port );
		~Server( void );
		Server( Server&& ) noexcept = default;
		Server& operator=( Server&& ) noexcept = default;

		void initFd( void );
		void setSocketOptions( void );
		void bindSocket( void );
		void listenSocket( int backlog );
		
		void			addClient( std::unique_ptr<Client> &client );
		uint16_t		getPort( void ) const;
		int				getFd( void ) const;
		Client*			getCurrentClient( int fd );
};
