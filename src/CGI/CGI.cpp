#include "../../inc/CGI/CGI.hpp"
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>

// #######################     PUBLIC     ########################
// ###############################################################

CGI::CGI(const std::string &post_data,
		 CGIPipes pipes,
		 std::function<void(int)> delFromEpoll_cb)
	:
	post_data_(post_data), 
	pipes_(pipes),
	delFromEpoll_cb_(delFromEpoll_cb),
	CGI_STATE_(START_CGI)
{
}

CGI::~CGI(void) {}

std::string CGI::getResponse(void) { return (response_); }

bool	CGI::isReady(void) { return (CGI_STATE_ == CRT_RSPNS_CGI); }

bool	CGI::isTimeout(void) { return (timeout_); }

void	CGI::handle_cgi(HTTPRequest &request, const SharedFd &fd) {

	switch (CGI_STATE_) {
		case START_CGI:
			execCGI(request);
			return ;
		case SEND_TO_CGI:
			sendDataToCGI(fd);
			if (CGI_STATE_ == SEND_TO_CGI)
				return ;
		case RCV_FROM_CGI:
			getResponseFromCGI(fd);
			if (CGI_STATE_ == RCV_FROM_CGI)
				return ;
		case CRT_RSPNS_CGI:
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
 * @param request HTTP request
 */
void	CGI::execCGI(HTTPRequest& request) {
	std::cout << std::flush;
	start_time_ = time(NULL);
	std::string executable = request.request_target;

	pid_ = fork();
	if (pid_ < 0)
		throw std::runtime_error("fork()");
	else if (pid_ == 0) 
	{
		// reset refcount to 1 in child since independent process
		for(int i = 0; i < 4; ++i) {
			pipes_[i].resetRefCount();
		}

		auto redir = [](int fdA, int fdB) {
			if (dup2(fdA, fdB) < 0)
				throw std::runtime_error("dup2(): " + std::to_string(fdA) + ": " + strerror(errno));
		};
		try {
			redir(pipes_[TO_CGI_READ].get(), STDIN_FILENO);
			redir(pipes_[TO_CGI_WRITE].get(), STDOUT_FILENO);
			std::vector<char*>	argv_vector = createArgvVector(executable);
			std::vector<char*>	env_vector = createEnv(request);
			if (execve(executable.c_str(), argv_vector.data(), env_vector.data()) == -1)
				throw std::runtime_error("execve(): " +  executable + ": " + std::strerror(errno));
		} catch (std::runtime_error) {
			exit(1); // TODO: how to handle this case
		}
	}
	pipes_[TO_CGI_READ] = -1;
	pipes_[FROM_CGI_WRITE] = -1;
	CGI_STATE_ = SEND_TO_CGI;
}

// ###############################################################
// ###################### SEND_TO_CGI ############################

/**
 * @brief writes body to stdin for CGI and closes write end pipe
 * @param post_data string with body
 */
void	CGI::sendDataToCGI(const SharedFd &fd) {
	ssize_t				write_bytes;

	// check that ready fd is write end
	if (fd.get() != pipes_[TO_CGI_WRITE].get())
		return ;

	if (!post_data_.empty())
	{
		write_bytes = write(fd.get(), post_data_.c_str(), post_data_.size());
		if (write_bytes == -1) {
			throw std::runtime_error("CGI write(): " + std::to_string(fd.get()) + " : " + strerror(errno));
		} else if (write_bytes != (ssize_t)post_data_.size()) {
			post_data_.erase(0, write_bytes);
			std::cerr << "Could not written everything in once, remaining bytes:" <<  write_bytes << std::endl;
			// TODO: probably needs to be handled differently
			return ;
		}
	}
	delFromEpoll_cb_(pipes_[TO_CGI_WRITE].get());
	CGI_STATE_ = RCV_FROM_CGI;
}

/**
 * @brief creates a vector<char*> to store all the current request info as env variable
 * @param request struct with all information that is gathered. 
 * @return vector<char*> with all env information
 */
std::vector<char*> CGI::createEnv(HTTPRequest &request) {
	std::vector<std::string> envStrings;
	if (request.method == "DELETE")
	{
		envStrings.push_back("DELETE_FILE=" + request.request_target);
		request.request_target = "data/www/cgi-bin/nph_CGI_delete.py"; // TODO: why overwriting? and should this be hardcoded?
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

	// create result vector with char*
	std::vector<char*> result;
	result.push_back(const_cast<char*>("GATEWAY=CGI/1.1"));
	result.push_back(const_cast<char*>("SERVER_PROTOCOL=HTTP/1.1"));
	for (auto& str : envStrings)
		result.push_back(const_cast<char*>(str.c_str()));
	result.push_back(NULL);
	return (result);
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

void	CGI::cleanupPipes(void) {
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
	int status_code;

	// check that fd ready is pipe read end
	if (fd.get() != pipes_[FROM_CGI_READ].get())
		return ;

	std::string buffer = receiveBuffer(fd);
	response_ += buffer;
	bool isFinished = isCGIProcessFinished();
	bool timedOut = hasCGIProcessTimedOut();
	if (!buffer.empty() || (!isFinished && !timedOut)) {
		return;
	}
	if (isFinished && !isCGIProcessSuccessful()) {
		response_ = CGI_ERR_RESPONSE;
		status_code = getStatusCodeFromResponse();
		std::cerr << "status_code; " << status_code << std::endl;
		// TODO: compare with configfile error pages.
	} else if (timedOut) {
		std::cerr << "TIMEOUT, shutting down CGI...\n";
		timeout_ = true;
		if (kill(pid_, SIGKILL) == -1) // TODO: maybe use sigterm instead
			throw std::runtime_error("kill() " + std::to_string(pid_) + " : " + strerror(errno));
	}
	cleanupPipes();
	CGI_STATE_ = CRT_RSPNS_CGI;
}

/**
 * @brief read from pipe to CGI
 * @return string with read buffer
 * @THROW throws exception if read fails
 */
std::string	CGI::receiveBuffer(const SharedFd &fd) {
	std::string buffer;
	
	// TODO: replace 1024 by macro
	ssize_t bytesRead = read(fd.get(), buffer.data(), 1024 - 1);
	if (bytesRead == -1) {
		throw std::runtime_error(std::string("CGI read(): ") + strerror(errno));
	} 
	return (buffer);
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
