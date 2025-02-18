#include "../Includes/Headers.hpp"
#include "../Includes/CGI.hpp"

CGI::CGI(const std::string &script_path, std::vector<char *> env_vector) : m_path(script_path)
{
	std::string executable = getScriptExecutable(m_path);

	int fildes_to[2];
	int fildes_from[2];
	if (pipe(fildes_to) < 0)
	{
		std::perror("Pipe failed");
		throw std::exception();
	}
	if (pipe(fildes_from) < 0)
	{
		std::perror("Pipe failed");
		throw std::exception();
	}
	pid_t pid = fork();
	if (pid < 0)
	{
		std::perror("Fork failed");
		throw std::exception();
	}
	if (pid == 0) 
	{
		close(fildes_to[1]);
		close(fildes_from[0]);
		if (dup2(fildes_to[0], STDIN_FILENO) < 0)
		{
			std::perror("dup2 failed");
			exit(EXIT_FAILURE);
		}
		if (dup2(fildes_from[1], STDOUT_FILENO) < 0)
		{
			std::perror("dup2 failed");
			exit(EXIT_FAILURE);
		}
		std::vector<char*>	argv_vector;
		std::string			scriptname;

		argv_vector.push_back(const_cast<char *>(executable.c_str()));
		argv_vector.push_back(const_cast<char *>(script_path.c_str()));
		argv_vector.push_back(NULL);

		if (execve(const_cast<char *>(executable.c_str()), argv_vector.data(), env_vector.data()) == -1)
			std::perror("execve failed");
		exit(1);
	}
	close(fildes_to[0]);
	close(fildes_from[1]);
	m_out_fd = fildes_to[1];
	m_in_fd = fildes_from[0];
}

CGI::~CGI()
{
	close(m_out_fd);
	close(m_in_fd);
}

void CGI::sendBuffer(const std::string &buffer)
{
	write(m_out_fd, buffer.c_str(), buffer.size());
}

std::string CGI::receiveBuffer(size_t size)
{
	std::string result(size, '\0');
	int bytes_read = read(m_in_fd, &result[0], size);
	return std::string(result, 0, bytes_read);
}

void CGI::sendEof()
{
	close(m_out_fd);
}

bool CGI::isCgiScript(const std::string &path)
{
	std::string executable = getScriptExecutable(path);
	return !executable.empty();
}

std::string CGI::getScriptExecutable(const std::string &path)
{
	if (path.ends_with(".py"))
		return "/usr/bin/python3";
	if (path.ends_with(".php"))
		return "/usr/bin/php";
	return "";
}
