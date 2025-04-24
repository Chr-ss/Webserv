#include "../../inc/CGI/CGI.hpp"

// ######################     START_CGI     ######################
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
			std::cerr << "excevce: " << scriptPath_ << std::endl;
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
