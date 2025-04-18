#include "../../inc/CGI/CGI.hpp"
#include "../../inc/HTTP/HTTPClient.hpp"
#include <csignal>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

// #######################     PUBLIC     ########################
// ###############################################################

static bool	isNPH(const std::string& path) {
	size_t		index;
	std::string	filename = "";

	index = path.find_last_of('/');
	if (index != std::string::npos)
		filename = path.substr(index + 1);
	else
		filename = path;
	if (filename.size() > 4 && filename.substr(0, 4) == "nph_")
		return (true);
	else
		return (false);
}

CGI::CGI(const HTTPRequest &request,
		 CGIPipes pipes,
		 std::function<void(int)> delFromEpoll_cb)
	:
	pipes_(pipes),
	CGI_STATE_(START_CGI),
	delFromEpoll_cb_(delFromEpoll_cb),
	request_(request),
	timeout_(false),
	finished_(false)
{
	scriptPath_ = request_.request_target;
	if (request_.method == "DELETE") {
		scriptPath_ = "data/www/cgi-bin/nph_CGI_delete.py";
	}
	nph_ = isNPH(scriptPath_);
	send_data_ = "";
}

CGI::~CGI(void) {}

std::string CGI::getResponse(void) { return (response_); }

int	CGI::getStatusCode( void ) { return (status_); };

bool	CGI::isDone(void) { return (CGI_STATE_ == CGI_DONE); }

void	CGI::handle(const SharedFd &fd, uint32_t events) {
	if (CGI_STATE_ != START_CGI && CGI_STATE_ != CGI_DONE && timedOut()) {
		handleTimeOut();
		CGI_STATE_ = CGI_DONE;
		return;
	}

	switch (CGI_STATE_) {
		case START_CGI:
			execCGI();
			return ;
		case SEND_TO_CGI:
			sendDataToCGI(fd, events);
			return ;
		case RCV_FROM_CGI:
			handleCGIResponse(fd, events);
			return;
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
 * @brief static function that checks if executable is allowed and valid
 * @param path string with path and filename
 * @return bool if cgi script is valid
 */
bool CGI::isCGIScript(const std::string &path)
{
	std::string executable = getScriptExecutable(path);
	return (!executable.empty());
}

/**
 * @brief static function that checks if request requires CGI
 * @param request to be checked
 * @return bool true if cgi required
 */
bool	CGI::isCGI(const HTTPRequest& request) {
	if (request.invalidRequest)
		return (false);

	if (CGI::isCGIScript(request.request_target) || request.method == "DELETE")
		return (true);
	return (false);
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
void	CGI::execCGI() {
	std::cout << std::flush;
	start_time_ = time(NULL);

	pid_ = fork();
	if (pid_ < 0)
		throw std::runtime_error("fork()");
	else if (pid_ == 0) 
	{
		auto redir = [](int fdA, int fdB) {
			if (dup2(fdA, fdB) < 0)
				throw std::runtime_error("dup2(): " + std::to_string(fdA) + ": " + strerror(errno));
		};
		try {
			redir(pipes_[TO_CGI_READ].get(), STDIN_FILENO);
			redir(pipes_[FROM_CGI_WRITE].get(), STDOUT_FILENO);

			std::vector<char*>	env_vector = createEnv();
			std::vector<char*>	argv_vector = std::vector<char*>{const_cast<char*>(scriptPath_.c_str()), NULL};

			if (execve(scriptPath_.c_str(), argv_vector.data(), env_vector.data()) == -1)
				throw std::runtime_error("execve(): " +  scriptPath_ + ": " + std::strerror(errno));
		} catch (std::runtime_error &e) {
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			throw::CGIException(e.what());
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
void	CGI::sendDataToCGI( const SharedFd &fd, uint32_t events ) {
	ssize_t				write_bytes;

	if (fd.get() != pipes_[TO_CGI_WRITE].get() || !(events & EPOLLOUT))
		return ;

	if (events & (EPOLLHUP | EPOLLERR)) {
		throw ClientException("sendDataToCGI(): received EPOLLHUP or EPOLLERR on fd: " + std::to_string(fd.get()));
	}

	if (!request_.body.empty())
	{
		if (send_data_.empty())
			send_data_ = request_.body;
		size_t writesize = (send_data_.size() < WRITESIZE) ? send_data_.size() : WRITESIZE;

		write_bytes = write(fd.get(), send_data_.c_str(), writesize);
		if (write_bytes == -1) {
			throw std::runtime_error("CGI write(): " + std::to_string(fd.get()) + " : " + strerror(errno));
		} else if (write_bytes < (ssize_t)send_data_.size()) {
			send_data_.erase(0, write_bytes);
			std::cerr << "Remaining bytes after CGI write: " <<  write_bytes << std::endl;
			return ;
		}
	}
	delFromEpoll_cb_(pipes_[TO_CGI_WRITE].get());
	pipes_[TO_CGI_WRITE] = -1;
	CGI_STATE_ = RCV_FROM_CGI;
}

/**
 * @brief creates a vector<char*> to store all the current request info as env variable
 * @param request struct with all information that is gathered. 
 * @return vector<char*> with all env information
 */
std::vector<char*> CGI::createEnv() {
	envStrings_.push_back("GATEWAY=CGI/1.1");
	envStrings_.push_back("SERVER_PROTOCOL=HTTP/1.1");
	if (request_.method == "DELETE")
	{
		envStrings_.push_back("DELETE_FILE=" + request_.request_target);
	}
	envStrings_.push_back("REQUEST_TARGET=" + scriptPath_); // TODO: why hardcoded?
	envStrings_.push_back("REQUEST_METHOD=" + request_.method);
	envStrings_.push_back("CONTENT_LENGTH=" + std::to_string(request_.body.size()));

	for (const auto& pair : request_.headers)
	{
		if (*pair.second.end() == '\n')
			envStrings_.push_back(pair.first + "=" + pair.second.substr(0, pair.second.size() - 1));
		else
			envStrings_.push_back(pair.first + "=" + pair.second);
	}

	// create result vector with char*
	std::vector<char*> result;
	result.push_back(const_cast<char*>("GATEWAY=CGI/1.1"));
	result.push_back(const_cast<char*>("SERVER_PROTOCOL=HTTP/1.1"));
	for (auto& str : envStrings_)
		result.push_back(const_cast<char*>(str.c_str()));
	result.push_back(NULL);
	return (result);
}

bool	CGI::isCGIProcessFinished(void) {
	pid_t	result;

	if (finished_)
		return (true);

	result = waitpid(pid_, &status_, WNOHANG);
	if (result == pid_) {
		finished_ = true;
		return (true);
	}
	return (false);
}

void	CGI::handleTimeOut() {
	std::cerr << "TIMEOUT, shutting down CGI...\n";
	timeout_ = true;
	status_ = 500;
	finished_ = true;
	if (!isCGIProcessFinished()) {
		if (kill(pid_, SIGKILL) == -1) // TODO: maybe use sigterm instead
			throw std::runtime_error("kill() " + std::to_string(pid_) + " : " + strerror(errno));
	}
}

bool	CGI::timedOut(void) {
	if (timeout_)
		return (true);
	if (isCGIProcessFinished())
		return (false);

	time_t current_time = time(NULL);
	if (current_time - start_time_ > TIMEOUT) {
		timeout_ = true;
		return (true);
	}
	return (false);
}


bool	CGI::isCGIProcessSuccessful(void) {
	if (WIFEXITED(status_) && WEXITSTATUS(status_) == 0)
		return (true);
	return (false);
}

// ####################     RCV_FROM_CGI     #####################
// ###############################################################

static std::string	receiveBuffer(int fd) {
	char buffer[READSIZE + 1] = {'\0'};
	

	ssize_t bytesRead = read(fd, buffer, READSIZE);
	if (bytesRead == -1) {
		throw std::runtime_error(std::string("CGI read(): ") + strerror(errno));
	} 
	return (buffer);
}

/**
 * @brief checks response status from CGI and receives header (and body) from pipe
 * if statuscode is not set it wil generate a Internal Server Error
 */
void	CGI::getResponseFromCGI(const SharedFd &fd, uint32_t events) {
	if (fd.get() != pipes_[FROM_CGI_READ].get() || !(events & (EPOLLIN | EPOLLHUP)))
		return ;

	if (events & EPOLLERR) {
		throw ClientException("getResponseFromCGI(): received EPOLLERR on fd: " + std::to_string(fd.get()));
	}

	std::string buffer = receiveBuffer(fd.get());
	response_ += buffer;
	if (!isCGIProcessFinished()) {
		return;
	}

	if (!isCGIProcessSuccessful()) {
		response_ = CGI_ERR_RESPONSE;
		status_ = 500;
	}	
	delFromEpoll_cb_(pipes_[FROM_CGI_READ].get());
	pipes_[FROM_CGI_READ] = -1;
	CGI_STATE_ = CGI_DONE;
}

void	CGI::handleCGIResponse(const SharedFd &fd, uint32_t events) {
	getResponseFromCGI(fd, events);
	if (CGI_STATE_ == RCV_FROM_CGI)
		return ;
	if (!isNPHscript())
		rewriteResonseFromCGI();
	CGI_STATE_ = CGI_DONE;
}


/**
 * @brief extract statuscode from CGI response
 * @return int with statuscode or zero if not found
 */
int	CGI::getStatusCodeFromResponse(void) {
	std::regex	status_code_regex(R"(HTTP/1.1 (\d+))");
	std::smatch	match;
	int			status_code = 500;

	if (!response_.empty() && std::regex_search(response_, match, status_code_regex))
	{
		std::string to_string = match[1];
		if (to_string.size() < 9)
			status_code = std::stoi(match[1]);
	}
	else
		std::cerr << "Error: No response or statuscode is found in response" << std::endl;
	return (status_code);
}
