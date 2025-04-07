#include "../../inc/CGI/CGI.hpp"

// #######################     PUBLIC     ########################
// ###############################################################

CGI::CGI(const std::string &post_data, std::vector<int> pipes) : post_data_(post_data) {
	pipe_from_CGI_[READ] = pipes[0];
	pipe_from_CGI_[WRITE] = pipes[1];
	pipe_to_CGI_[READ] = pipes[2];
	pipe_to_CGI_[WRITE] = pipes[3];
	CGI_STATE_ = START_CGI;
}

void	CGI::printPipes(void) const {
	std::cerr << "pipe from cgi READ\t" << pipe_from_CGI_[READ] << std::endl;
	std::cerr << "pipe from cgi WRITE\t" << pipe_from_CGI_[WRITE] << std::endl;
	std::cerr << "pipe to cgi READ\t" << pipe_to_CGI_[READ] << std::endl;
	std::cerr << "pipe to cgi WRITE\t" << pipe_to_CGI_[WRITE] << std::endl;
}

CGI::~CGI(void) {}

std::string CGI::getResponse(void) { return (response_); }

bool	CGI::isReady( void ) { return (CGI_STATE_ == CRT_RSPNS_CGI); }

void print_epoll_events(uint32_t events) {
    if (events & EPOLLIN) std::cout << "EPOLLIN " << std::endl;
    if (events & EPOLLOUT) std::cout << "EPOLLOUT " << std::endl;
    if (events & EPOLLERR) std::cout << "EPOLLERR " << std::endl;
    if (events & EPOLLHUP) std::cout << "EPOLLHUP " << std::endl;
    if (events & EPOLLET) std::cout << "EPOLLET " << std::endl;
    if (events & EPOLLONESHOT) std::cout << "EPOLLONESHOT " << std::endl;
}

void	CGI::handle_cgi(HTTPRequest &request, const epoll_event &event) {
	std::vector<std::string>	env_strings;

	switch (CGI_STATE_) {
		case START_CGI:
			if (request.method == "DELETE")
				request.request_target = "data/www/cgi-bin/nph_CGI_delete.py";
			createEnv(env_strings, request);
			forkCGI(request.request_target, env_strings);
			CGI_STATE_ = SEND_TO_CGI;
			return ;
		case SEND_TO_CGI:
			if (event.data.fd != pipe_to_CGI_[WRITE])
				return ;
			if (!sendDataToStdinReady(event.data.fd))
				return ;
			CGI_STATE_ = RCV_FROM_CGI;
			return ;
		case RCV_FROM_CGI:
			if (event.data.fd != pipe_from_CGI_[READ])
				return ;
			// print_epoll_events(event.events);
			if (getResponseFromCGI(event.data.fd) == false)
				return ;
			CGI_STATE_ = CRT_RSPNS_CGI;
		case CRT_RSPNS_CGI:
			return ;
		default:
			return ;
	}
}

/// @brief Set up a response for the client after receiving the header from the CGI
/// saves the result again in response_
void	CGI::rewriteResonseFromCGI(void) {
	std::smatch	match;
	std::string	new_response = "";
	std::regex	r_content_type = std::regex(R"(Content-Type:\s+([^\r\n]+)\r\n)");
	std::regex	r_status = std::regex(R"(Status:\s+([^\r\n]+)\r\n)");
	std::regex	r_location = std::regex(R"(Location:\s+([^\r\n]+)\r\n)");
	
	if (std::regex_match(response_, match, r_status) && match.size() == 2)
		new_response += "HTTP/1.1 " + std::string(match[1]) + "\r\n";
	if (std::regex_match(response_, match, r_location) && match.size() == 2)
		new_response += "Location: " + std::string(match[1]) + "\r\n";
	if (std::regex_match(response_, match, r_content_type) && match.size() == 2)
		new_response += "Content-Type: " + std::string(match[1]) + "\r\n";
	if (new_response.empty())
	{
		std::cerr << "Error: Received wrong formated header from CGI" << std::endl;
		response_ = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
		return ;
	}
	new_response += "\r\n";

	std::size_t	index = response_.find("\r\n\r");
	if (index != std::string::npos)
	{
		if (index + 4 < response_.size())
			new_response += response_.substr(index + 4) + "\r\n";
	}
	response_ = new_response;
}

/**
 * @brief checks if the executable file starts with nph_
 * @param executable absolute path to request target
 * @return true if nhp_ file, else false
 */
bool	CGI::isNPHscript( const std::string &executable ) {
	size_t		index;
	std::string	filename = "";

	index = executable.find_last_of('/');
	if (index != std::string::npos)
		filename = executable.substr(index + 1);
	else
		filename = executable;
	if (filename.size() > 4 && filename.substr(0, 4) == "nph_")
		return (true);
	else
		return (false);
}

/**
 * @brief static function that checks if executable is allowed and valid
 * @param path string with path and filename
 * @return bool if cgi script is valid
 */
bool CGI::isCgiScript(const std::string &path)
{
	std::string executable = getScriptExecutable(path);
	return (!executable.empty());
}

/**
 * @brief static function that compares executable extension with the allowed cgi scripts
 * @param path string with path and filename
 * @return string with path to executable program or empty when not correct
 */
std::string CGI::getScriptExecutable(const std::string &path)
{
	if (path.size() >= 3 && path.substr(path.size() - 3) == ".py")
		return "/usr/bin/python3";
	// if (path.size() >= 4 && path.substr(path.size() - 4) == ".php")
	// 	return "/usr/bin/php";
	return "";
}

// ##################     PRIVATE START_CGI     ##################
// ###############################################################

/**
 * @brief forks the process to execve the CGI, with POST sends buffer to child
 * @param executable const string CGI filename
 * @param env_vector char* vector with key-values as env argument to CGI
 */
void	CGI::forkCGI(const std::string &executable, std::vector<std::string> env_vector) {
	std::cout << std::flush;
	start_time_ = time(NULL);

	pid_ = fork();
	if (pid_ < 0)
		throwException("Fork failed");
	else if (pid_ == 0) 
	{
		// std::cerr << "IN CHILD: \n";
		// closeSave(pipe_to_CGI_[WRITE]);
		// closeSave(pipe_from_CGI_[READ]);
		if (dup2(pipe_to_CGI_[READ], STDIN_FILENO) < 0)
			throwExceptionExit("dub2 failed");
		if (dup2(pipe_from_CGI_[WRITE], STDOUT_FILENO) < 0)
			throwExceptionExit("dub2 failed");
		// std::cerr << "IN CHILD AFTER DUB: ";
		// closeSave(pipe_to_CGI_[READ]);
		// closeSave(pipe_from_CGI_[WRITE]);
		std::vector<char*>	argv_vector;
		std::vector<char*>	env_c_vector;

		createArgvVector(argv_vector, executable);
		createEnvCharPtrVector(env_c_vector, env_vector);

		if (execve(executable.c_str(), argv_vector.data(), env_c_vector.data()) == -1)
			std::cerr << "Error: Execve failed: " << std::strerror(errno) << std::endl;
		exit(1);
	}
	std::cerr << "IN PARENT: ";
	closeSave(pipe_to_CGI_[READ]);
	closeSave(pipe_from_CGI_[WRITE]);
	// watchDog();
}

/// @brief forks a new process that checks the time of the CGI + timout time
void	CGI::watchDog(void) {
	pid_t	pid;
	time_t	start;
	time_t	now;
	bool	timeout = false;

	pid = fork();
	if (pid < 0)
	{
		std::perror("Error: fork failed");
		throwException("Fork failed");
	}
	else if (pid == 0)
	{
		start = time(NULL);
		std::cerr << "wait..." << std::endl;
		while (waitpid(pid_, &status_, WNOHANG) == 0)
		{
			now = time(NULL);
			if (now - start > TIMEOUT)
			{
				timeout = true;
				break ;
			}
			usleep(100000);
		}
		if (timeout)
		{
			std::cerr << "timeout CGI\n";
			if (kill(pid_, SIGKILL) == -1)
				std::perror("Error: failed to kill child process");
		}
		std::cerr << "IN WACHDOG: ";
		closeSave(pipe_from_CGI_[READ]);
		closeSave(pipe_to_CGI_[WRITE]);
		exit(0);
	}
}

// ###############################################################
// ###################### SEND_TO_CGI ############################
#include <sys/socket.h>
/**
 * @brief writes body to stdin for CGI and closes write end pipe
 * @param post_data string with body
 */
bool	CGI::sendDataToStdinReady(int fd) {
	ssize_t	readBytes;

	if (fd != pipe_to_CGI_[WRITE])
		return (false);

	if (!post_data_.empty())
	{
		readBytes = write(fd, post_data_.c_str(), post_data_.size());
		if (readBytes != (ssize_t)post_data_.size())
		{
			if (readBytes == -1)
				std::cerr << "Error write: " << std::strerror(errno) << std::endl;
			else
				std::cerr << "Error write: not written right amount:" <<  readBytes << std::endl;
		}
	}
	std::cerr << "AFTER SEND DATA to CGI close: ";
	closeSave(pipe_to_CGI_[WRITE]);
	return (true);
}

/**
 * @brief creates a vector<char*> to store all the current request info as env variable
 * @param envStrings all info from the HTTP header
 * @param request struct with all information that is gathered. 
 * @return vector<char*> with all env information
 */
std::vector<char*> CGI::createEnv(std::vector<std::string> &envStrings, const HTTPRequest request) {
	envStrings.push_back("REQUEST_METHOD=" + request.method);
	envStrings.push_back("REQUEST_TARGET=" + request.request_target);
	envStrings.push_back("CONTENT_LENGTH=" + std::to_string(request.body.size()));

	for (const auto& pair : request.headers)
	{
		if (*pair.second.end() == '\n')
			envStrings.push_back(pair.first + "=" + pair.second.substr(0, pair.second.size() - 1));
		else
			envStrings.push_back(pair.first + "=" + pair.second);
	}

	std::vector<char*>	env;
	for (auto &str : envStrings)
		env.push_back(const_cast<char*>(str.c_str()));
	env.push_back(nullptr);
	return (env);
}


bool	CGI::isCGIProcessFinished(void) {
	time_t	current_time;
	pid_t	result;

	result = waitpid(pid_, &status_, WNOHANG);
	if (result == pid_)
		return (true);

	timeout_ = false;
	current_time = time(NULL);
	if (current_time - start_time_ > TIMEOUT)
		timeout_ = true;
	return (false);
}

bool	CGI::isCGIProcessSuccessful(void) {
	if (WIFEXITED(status_) && WEXITSTATUS(status_) == 0)
		return (true);
	return (false);
}

// ####################     RCV_FROM_CGI     #####################
// ###############################################################

/**
 * @brief checks response status from CGI and receives header (and body) from pipe
 * if statuscode is not set it wil generate a Internal Server Error
 */
bool	CGI::getResponseFromCGI(int fd) {
	int return_value;
	int status_code;

	if (!isCGIProcessFinished())
		return (false);
	if (isCGIProcessSuccessful())
	{
		assert(fd == pipe_from_CGI_[READ]);
		return_value = WEXITSTATUS(status_);
		std::cerr << "Reading response ... \n";
		response_ = receiveBuffer(fd);
		if (response_.empty())
			return (false);

		if (return_value != 0) {
			status_code = getStatusCodeFromResponse();
			std::cerr << "status_code; " << status_code << std::endl;
			// compare with configfile error pages.
			std::cerr << "response" << response_ << std::endl;
		}
	}
	else
		response_ = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	return (true);
}

/**
 * @brief receives a response from child with a headerfile and body to return to the client
 * @return string buffer with response from child or error msg that something went wrong
 */
std::string	CGI::receiveBuffer(int fd) {
	char	buffer[1024];
	
	std::cerr << "READ from " << fd << std::endl;
	ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

	if (bytesRead > 0)
		buffer[bytesRead] = '\0';
	else
	{
		if (bytesRead == -1)
		{
			std::cerr << "Error read?: " << std::strerror(errno) << std::endl;
			return (""); // again?
		}
		else
			std::cerr << "Error: no output read"; // what now?
		return std::string("HTTP/1.1 500 Internal Server Error\nContent-Type: text/html\n\r\n<html>\n") +
			"<head><title>Server Error</title></head><body><h1>Something went wrong</h1></body></html>";
	}
	closeSave(pipe_from_CGI_[READ]);
	return (buffer); // also in chunked form...
}

/**
 * @brief extract statuscode from CGI response
 * @return int with statuscode or zero if not found
 */
int	CGI::getStatusCodeFromResponse(void) {
	std::regex	status_code_regex(R"(HTTP/1.1 (\d+))");
	std::smatch	match;
	int			status_code = 0;

	if (!response_.empty() && std::regex_search(response_, match, status_code_regex))
	{
		std::string to_string = match[1];
		if (to_string.size() < 9)
			status_code = std::stoi(match[1]);
		else
			status_code = 500;
	}
	else
		std::cerr << "Error: No response or statuscode is found in response" << std::endl;
	return (status_code);
}
