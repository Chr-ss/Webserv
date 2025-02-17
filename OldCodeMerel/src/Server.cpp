#include "Headers.hpp"
#include "Server.hpp"

Server::Server(uint16_t port) : m_port(port) {
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
		throw std::runtime_error("Socket creation error");

	setSocketOptions();
	bindSocket();
	listenSocket(5);
}

Server::~Server() {}

// int	Server::start_server() {
// 	int	exit_code = 0;

// 	return (exit_code);
// }

void	Server::addClient(std::unique_ptr<Client> &Client) {

	m_clients.push_back(Client);
}

void Server::initFd(void) {
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
		throw std::runtime_error("Socket creation error");
}

const uint16_t Server::getPort( void ) const {
	return (m_port);
}

const int	Server::getFd( void ) const {
	return (m_fd);
}

void Server::setSocketOptions( void ) {
	const int option = 1;
	if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
		throw std::runtime_error("setsockopt error");
	if (fcntl(m_fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl error");
}

void Server::bindSocket( void ) {
	sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(m_port);
	if (bind(m_fd, reinterpret_cast<const sockaddr *>(&address),
			 sizeof(address)) != 0)
		throw std::runtime_error("bind error");
}

void Server::listenSocket(int backlog) {
	if (listen(m_fd, backlog) != 0)
		throw std::runtime_error("listen error");
}

Client* Server::getCurrentClient(int fd) {
	for (auto it = m_clients.begin(); it != m_clients.end(); it++)
	{
		if ((*it).get_fd() == fd)
			return &(*it);
	}
	return (nullptr);
}
