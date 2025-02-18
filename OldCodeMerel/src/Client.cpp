
# include "../Includes/Client.hpp"

Client::Client( void ) { }

Client::Client( int fd, Server *server ): m_fd(fd), m_server(server)
{
	m_header_complet = false;
	m_length_available = false;
	std::cout << m_server->getFd() << std::endl;
}

Client::~Client()
{
	// close fd
}

void	Client::parseData(int fd) {
	std::string buffer;
	// open file
	(void)fd;

	m_parser.expandWithBuffer(buffer);
}

Client& Client::operator=(const Client& src)
{
	m_header_complet = src.m_header_complet;
	m_length_available = src.m_length_available;
	m_fd = src.m_fd;
	std::copy(src.m_buff, src.m_buff + 4000, m_buff);
	m_env = src.m_env;
	return (*this);
}

void	Client::setIp(uint32_t ip_addr)
{
	m_env["REMOTE_ADDR"] = \
		std::to_string((ip_addr >> 24) & 0xFF) + "." + \
		std::to_string((ip_addr >> 16) & 0xFF) + "." + \
		std::to_string((ip_addr >> 8) & 0xFF) + "." + \
		std::to_string(ip_addr & 0xFF);
}

void	Client::setFd( int fd ) { m_fd = fd; }

int	Client::getFd( void ) { return (m_fd); };

void	Client::setSocketBuffer (std::string buff)
{
	if (m_socket_buffer.empty())
		m_socket_buffer = buff;
	else
		m_socket_buffer += buff;
}

std::string	Client::getSocketBuffer( void ) { return (m_socket_buffer); }

bool	Client::getHeaderComplet( void ) { return (m_header_complet); }

bool	Client::getLengthAvailable( void ) { return (m_length_available); }

void	Client::setEnv(std::string request)
{
	std::map<std::string, std::regex>::iterator		it;
	std::smatch										match;
	std::string										key;
	std::regex										re;
	std::map<std::string, std::regex> 				regexMap = { \
		{"REQUEST_METHOD", std::regex(R"((GET|POST|DELETE|PUT|PATCH|OPTIONS|HEAD)\s+(\S+)\s+HTTP/\d\.\d)")},\
		{"SCRIPT_NAME", std::regex(R"(\S+\s+(/\S*))")}, \
		{"SERVER_PROTOCOL", std::regex(R"(HTTP/\d\.\d)")}, \
		{"QUERY_STRING", std::regex(R"(\?(.+)$")")}, \
		{"SERVER_NAME", std::regex(R"(Host:\s*(\S+))")}, \
		{"SERVER_PORT", std::regex(R"(Host:\s*\S+:(\d+))")}, \
		{"HTTP_USER_AGENT", std::regex(R"(User-Agent:\s*(.*))")}, \
		{"HTTP_ACCEPT", std::regex(R"(Accept:\s*(.*))")}, \
		{"CONTENT_TYPE", std::regex(R"(Content-Type:\s*(.*))")}, \
		{"CONTENT_LENGTH", std::regex(R"(Content-Length:\s*(\d+))")}, \
		{"SERVER_SOFTWARE" , std::regex(R"(Server:\s*(.*))")}, \
		{"HTTP_REFERER", std::regex(R"(Referer:\s*(.*))")}, \
		{"HTTP_ACCEPT_LANGUAGE", std::regex(R"(Accept-Language:\s*(.*))")}, \
		{"HTTP_ACCEPT_ENCODING", std::regex(R"(Accept-Encoding:\s*(.*))")}
		};

	for (it = regexMap.begin(); it != regexMap.end(); ++it)
	{
		key = it->first;
		re = it->second;
		if (std::regex_search(request, match, re) && match.size() > 1)
		{
			m_env[key] = match[1].str();
			if (key == "CONTENT_LENGTH")
				m_length_available = true;
		}
	}
	m_header_complet = true;
}

std::string	Client::getEnvItem(std::string key) { return (m_env[key]); }

std::vector<char*>	Client::getEnv()
{
	std::vector<char*>									env_vector;
	std::map<std::string, std::string>::const_iterator	it;
	std::string											env_var;
	char												*str;
	std::vector<char*>::size_type						i;

	for (it = m_env.begin(); it != m_env.end(); ++it)
	{
		env_var = it->first + "=" + it->second;
		str = strdup(env_var.c_str());
		if (str == NULL)
		{
			std::perror("Strdup get env");
			for (i = 0; i < env_vector.size(); ++i)
                free(env_vector[i]);
            env_vector.clear();
            return env_vector;
		}
		env_vector.push_back(str);
	}
	env_vector.push_back(nullptr);
	return (env_vector);
}

void	Client::reset()
{
	std::map<std::string, std::string>::iterator it;

	m_env.clear();
	close(m_fd);
	m_header_complet = false;
	m_length_available = false;
	m_socket_buffer.erase();
}

void	Client::printEnv( void )
{
	std::map<std::string, std::string>::const_iterator it;

	std::cout << "ENV:" << std::endl;
	for (it = m_env.begin(); it != m_env.end(); ++it)
		std::cout << it->first << ": " << (it->second.empty() ? "(empty)" : it->second) << std::endl;
}

