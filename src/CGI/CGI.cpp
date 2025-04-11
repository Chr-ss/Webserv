#include "../../inc/CGI/CGI.hpp"

// #######################     PUBLIC     ########################
// ###############################################################

CGI::CGI(const std::string &post_data, std::vector<SharedFd> pipes, \
		std::function<void(int)> delFromEpoll_cb) : post_data_(post_data), \
		delFromEpoll_cb_(delFromEpoll_cb) {
	pipe_from_CGI_[READ] = pipes[0].get();
	pipe_from_CGI_[WRITE] = pipes[1].get();
	pipe_to_CGI_[READ] = pipes[2].get();
	pipe_to_CGI_[WRITE] = pipes[3].get();
	(void) pipes;
	CGI_STATE_ = START_CGI;
}

CGI::~CGI(void) {}

std::string CGI::getResponse(void) { return (response_); }

bool	CGI::isReady(void) { return (CGI_STATE_ == CRT_RSPNS_CGI); }

bool	CGI::isTimeout(void) { return (timeout_); }

void	CGI::handle_cgi(HTTPRequest &request, const SharedFd &fd) {
	std::vector<std::string>	env_strings;

	switch (CGI_STATE_) {
		case START_CGI:
			createEnv(env_strings, request);
			forkCGI(request.request_target, env_strings);
			CGI_STATE_ = SEND_TO_CGI;
			return ;
		case SEND_TO_CGI:
			if (!sendDataToStdinReady(fd))
				return ;
			CGI_STATE_ = RCV_FROM_CGI;
			return ;
		case RCV_FROM_CGI:
			if (!getResponseFromCGI(fd))
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
bool	CGI::isNPHscript(const std::string &executable) {
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
		if (dup2(pipe_to_CGI_[READ], STDIN_FILENO) < 0)
			throwExceptionExit("dub2 failed");
		if (dup2(pipe_from_CGI_[WRITE], STDOUT_FILENO) < 0)
			throwExceptionExit("dub2 failed");

		std::vector<char*>	argv_vector;
		std::vector<char*>	env_c_vector;
		createArgvVector(argv_vector, executable);
		createEnvCharPtrVector(env_c_vector, env_vector);

		if (execve(executable.c_str(), argv_vector.data(), env_c_vector.data()) == -1)
			std::cerr << "Error: Execve failed: " <<  executable << std::strerror(errno) << std::endl;
		exit(1);
	}
	pipe_to_CGI_[READ] = -1;
	pipe_from_CGI_[WRITE] = -1;
}

// ###############################################################
// ###################### SEND_TO_CGI ############################

/**
 * @brief writes body to stdin for CGI and closes write end pipe
 * @param post_data string with body
 */
bool	CGI::sendDataToStdinReady(const SharedFd &fd) {
	ssize_t				write_bytes;

	if (fd.get() != pipe_to_CGI_[WRITE])
		return (false);

	if (!post_data_.empty())
	{
		write_bytes = write(fd.get(), post_data_.c_str(), post_data_.size());
		if (write_bytes != (ssize_t)post_data_.size())
		{
			if (write_bytes == -1)
				std::cerr << "Error write: " << std::strerror(errno) << std::endl;
			else 
			{
				post_data_.erase(0, write_bytes);
				std::cerr << "Could not written everything in once, remaining bytes:" <<  write_bytes << std::endl;
				return (false);
			}
		}
	}
	delFromEpoll_cb_(pipe_to_CGI_[WRITE]);
	return (true);
}

/**
 * @brief creates a vector<char*> to store all the current request info as env variable
 * @param envStrings all info from the HTTP header
 * @param request struct with all information that is gathered. 
 * @return vector<char*> with all env information
 */
std::vector<char*> CGI::createEnv(std::vector<std::string> &envStrings, HTTPRequest &request) {
	if (request.method == "DELETE")
	{
		envStrings.push_back("DELETE_FILE=" + request.request_target);
		request.request_target = "data/www/cgi-bin/nph_CGI_delete.py";
	}
	envStrings.push_back("REQUEST_TARGET=" + request.request_target);
	envStrings.push_back("REQUEST_METHOD=" + request.method);
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
	pid_t	result;

	// check finished
	result = waitpid(pid_, &status_, WNOHANG);
	if (result == pid_)
		return (true);
	return (false);
}

bool	CGI::hasCGIProcessTimedOut(void) {
	timeout_ = false;
	time_t current_time = time(NULL);
	if (current_time - start_time_ > TIMEOUT) {
		timeout_ = true;
		return (true);
	}
	return (false);
}

void	CGI::cleanupProcess(void) {
	if (kill(pid_, SIGKILL) == -1) // TODO: maybe use sigterm instead
		throw std::runtime_error("kill() " + std::to_string(pid_) + " : " + strerror(errno));

	if (pipes_[TO_CGI_WRITE] != -1)
	{
		delFromEpoll_cb_(pipes_[TO_CGI_WRITE].get());
		pipes_[TO_CGI_WRITE] = -1;
	}
	if (pipes_[FROM_CGI_READ] != -1)
	{
		delFromEpoll_cb_(pipes_[FROM_CGI_READ].get());
		pipes_[FROM_CGI_READ] = -1;
	}
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
void	CGI::getResponseFromCGI(const SharedFd &fd) {
	int return_value;
	int status_code;

	// check that fd ready is pipe read end
	if (fd.get() != pipes_[FROM_CGI_READ].get())
		return ;

	if (!isCGIProcessFinished()) {
		if (hasCGIProcessTimedOut()) {
			std::cerr << "TIMEOUT, shutting down CGI...\n";
			timeout_ = true;
			CGI_STATE_ = CRT_RSPNS_CGI;
			cleanupProcess();
		};
		return ;
	}

	if (isCGIProcessSuccessful())
	{
		// TODO: return value will be 0 if the function above returns true
		return_value = WEXITSTATUS(status_);
		response_ = receiveBuffer(fd);
		if (response_.empty())
			return ;

		if (return_value != 0) {
			status_code = getStatusCodeFromResponse();
			std::cerr << "status_code; " << status_code << std::endl;
			// compare with configfile error pages.
			std::cerr << "response" << response_ << std::endl;
		}
	}
	else
		response_ = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
	CGI_STATE_ = CRT_RSPNS_CGI;
}

/**
 * @brief receives a response from child with a headerfile and body to return to the client
 * @return string buffer with response from child or error msg that something went wrong
 */
std::string	CGI::receiveBuffer(const SharedFd &fd) {
	char	buffer[1024];
	
	ssize_t bytesRead = read(fd.get(), buffer, sizeof(buffer) - 1);
	if (bytesRead > 0)
		buffer[bytesRead] = '\0';
	else
	{
		if (bytesRead == -1)
		{
			std::cerr << "Error read: " << std::strerror(errno) << std::endl;
			return ("");
		}
		else
			std::cerr << "Error: no output read"; // what now?
		return std::string("HTTP/1.1 500 Internal Server Error\nContent-Type: text/html\n\r\n<html>\n") +
			"<head><title>Server Error</title></head><body><h1>Something went wrong</h1></body></html>";
	}
	delFromEpoll_cb_(pipe_from_CGI_[READ]);
	pipe_from_CGI_[READ] = -1;
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
