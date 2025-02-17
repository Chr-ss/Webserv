# include "Headers.hpp"
# include "HTTPParser.hpp"
# include "HTTPResponse.hpp"
# include "Server.hpp"


class Client{
	public:
		Client( void );
		Client( int fd );
		Client( const Client& src ) = delete;
		Client& operator=( const Client& src );
		~Client( void );

		void				setIp( uint32_t ip_addr );
		void				setEnv( std::string request );
		void				setFd( int fd );
		void				setSocketBuffer (std::string buff);
		void				reset( void );
		void				printEnv( void );

		bool				getHeaderComplet( void );
		bool				getLengthAvailable( void );
		int					getFd( void );
		std::string			getSocketBuffer( void );
		std::string			getEnvItem(std::string key);
		std::vector<char*>	getEnv( void );
		void				parseData( int fd );

	private:
		bool								m_header_complet;
		bool								m_length_available;
		int								m_fd;
		char								m_buff[BUFF_SIZE] = {'\0'};
		std::string							m_socket_buffer;
		std::map<std::string, std::string>	m_env;
		Server								*m_server;
		HTTPParser							m_parser;
		HTTPResponse						m_response;
};
