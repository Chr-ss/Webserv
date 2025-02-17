#include "Server.hpp"

#include <filesystem>

#include "CGI.hpp"

// TODO: move this into a webserv method working with config
// to resolve the correct path
static std::string resolve_path(std::string endpoint)
{
	std::string folders[] = {
		".",
		"data",
		"data/www",
		"data/images",
		"data/www/pages",
	};

	if (endpoint == "/")
		endpoint = "index.html";
	std::string fileName;
	if (!endpoint.empty() && endpoint.front() != '/')
		fileName = "/" + endpoint;
	else
		fileName = endpoint;
	for (const auto &folder : folders)
	{
		std::filesystem::path path = folder + fileName;
		if (std::filesystem::is_directory(path))
			path = path / "index.html";
		if (std::filesystem::exists(path))
			return path;
	}
	return "";
}

// BASIC PROCES
// 1. Get infomation out of the HTTP page
// 2. Search for correct respons (errorpage / page / cgi script)
// 3. Start cgi script with execve
// 		The CGI script has to perform the following tasks in order to retrieve 
// 		the necessary information:
// 		If the QUERY_STRING environment variable is not set, it reads 
// 		CONTENT_LENGTH characters from its standard input. 
// 		- create HTTP response header -> function to count the content-length
// 		- add body with content (html)
// 		- response back to the server
// 4. Server sending response to the client

int	GetContentPage(std::string fileName, std::string &response)
{
	std::ifstream 		myfile;
	std::ostringstream	buffer;

	myfile.open(fileName, std::ios::binary);
	buffer.clear();
	buffer << myfile.rdbuf();
	response = buffer.str();
	myfile.close();
	return (0);
}

std::string	CalculateContentLength(std::string response)
{
	int	index;
	int	bodySize;

	index = response.find("\n\n");
	index += 3;
	bodySize = response.size() - index;
	index = response.find("Content-Length:");
	index += 15;
	response.replace(index, 0, std::to_string(bodySize));
	return (response);
}

int	writeToClient(std::string response, int fd)
{
	ssize_t	bytes_written;

	// bytes_written = write(fd, response.c_str(), response.size());
	bytes_written = send(fd, response.c_str(), response.size(), MSG_NOSIGNAL);
	if (bytes_written == -1)
	{
		// std::perror("writing fails"); // connection is closed
		return (1);
	}
	return (0);
}

static std::string prepare_response(std::string body)
{
	std::string	header = "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: ";
	return header + std::to_string(body.size()) + "\r\n\r\n" + body;
}

int	Server::get_request(int n)
{
	std::string response;
	int			fd;
	std::string	filename;
	
	fd = m_clients[n].get_fd();
	filename = resolve_path(m_clients[n].get_env_item("SCRIPT_NAME"));
	if (filename.empty())
		filename = resolve_path("custom_404.html");
	if (GetContentPage(filename, response))
	{
		m_socket_buffers.erase(fd);
		m_clients[n].reset();
		return (1);
	}
	writeToClient(prepare_response(response), fd);

	m_socket_buffers.erase(fd);
	m_clients[n].reset();
	return (0);
}

int Server::post_request(bool ready, int n)
{
	std::string	header;
	std::string	body;

	if (ready)
	{
		body = "<html><body><h1>Upload Successful</h1><p>Your file has been uploaded successfully!</p></body></html>";
		header = "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length:" + std::to_string(body.size()) + "\r\n"
			"\r\n" + body;
		write(m_clients[n].get_fd(), header.c_str(), header.size()); 
		m_socket_buffers.erase(m_clients[n].get_fd());
		m_clients[n].reset();
	}
	else
	{
		header = "HTTP/1.1 100 OK\r\nContinue\r\n\r\n";
		write(m_clients[n].get_fd(), header.c_str(), header.size()); 
	}
	return (0);
}

int Server::delete_request(int n)
{
	// syntax: DELETE <request-target>["?"<query>] HTTP/1.1
	// Page: error: 204 No Content
	std::string response;

	response = "Page not found\n";
	if(GetContentPage("delete.html", response))
		response = "Deleted request";
	writeToClient(response, m_clients[n].get_fd());
	m_socket_buffers.erase(m_clients[n].get_fd());
	m_clients[n].reset();
	return (0);
}

int	Server::start_cgi(int n)
{
	int		status = 0;
	pid_t	pid;

	pid = fork();
	if (pid == -1)
	{
		std::perror("Fork fails");
		return (1);
	}
	if (pid == 0) 
	{
		std::vector<char*>	env_vector;
		std::vector<char*>	argv_vector;
		std::string			scriptname;

		env_vector = m_clients[n].get_env();
		scriptname = m_clients[n].get_env_item("SCRIPT_NAME");
		argv_vector.push_back(const_cast<char*>(scriptname.data()));
		argv_vector.push_back(NULL);

		if (execve("www/cgi-bin/", argv_vector.data(), env_vector.data()) == -1)
			std::perror("execve fails");
		exit(1);
	}
	else
		waitpid(0, &status, 0);
	// if (status == 256)
	// 	return (1); // nothing to write to client or 404 page ect
	std::cout << status << std::endl; // action?
	return (0);
}

int	Server::process_buffer(int n, char buff[BUFF_SIZE])
{
	std::string	s(buff);
	size_t		header_end;
	size_t		body_length;
	std::string	body;
	std::smatch	match;

	m_clients[n].set_socket_buffer(s);
	header_end = m_clients[n].get_socket_buffer().find("\r\n\r\n");
	if (header_end == std::string::npos)
		return (0);
		
	if (m_clients[n].get_header_complet() == false) // header now complete
	{	
		if (std::regex_search(s, match, std::regex(R"(HTTP/\d\.\d)")))
			m_clients[n].set_env(m_clients[n].get_socket_buffer().substr(0, header_end));
	}
	if (m_clients[n].get_length_available())
	{
		try
		{
			body_length = std::stoi(m_clients[n].get_env_item("CONTENT_LENGTH"));
			body = m_clients[n].get_socket_buffer().substr(header_end + 4);
			if (body.size() < body_length)
				return (1);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			return (1);
		}
	}
	return (0);
}

int Server::process_epoll_event(int n)
{
	char		buff[BUFF_SIZE] = {'\0'};
	int			len;

	if (m_clients[n].get_fd() == 0)
		return (0);
	try
	{
		len = ::recv(m_clients[n].get_fd(), buff, BUFF_SIZE - 1, MSG_NOSIGNAL);
	}
	catch(const std::exception& e)
	{
		std::cerr << "Recv fails: " << e.what() << std::endl;
		return (0);
	}

	if (len >= 1)
	{
		if (process_buffer(n, buff))
			return (0);
	}
	else // client have closed connection
		return (0);
	
	std::string filename = resolve_path(m_clients[n].get_env_item("SCRIPT_NAME"));
	if (filename.empty())
		filename = resolve_path("custom_404.html");
	if (!CGIProcessor::is_cgi_script(filename))
		return (get_request(n));
	CGIProcessor cgip = CGIProcessor(filename, m_clients[n].get_env());
	cgip.send_buffer(m_clients[n].get_socket_buffer());
	cgip.send_eof();
	writeToClient(prepare_response(cgip.receive_buffer(4096)), m_clients[n].get_fd());
	m_clients[n].reset();
	return 0;
	// possibly create better way to put it
	// m_clients[n].reset();
}
