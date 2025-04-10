#include "../../inc/CGI/CGI.hpp"

void	CGI::throwException(const char *msg) {
	std::cerr << "Error: " << msg << std::endl;
	std::perror(msg);
	throw std::exception();
}

void	CGI::throwExceptionExit(const char *msg) {
	std::cerr << "Error: " << msg << std::endl;
	exit(EXIT_FAILURE);
}

/// @brief creating a vector<char*> for transfor to an array of * to strings ascommand-line arguments for execve.
void	CGI::createArgvVector(std::vector<char*> &argv_vector, const std::string &executable) {
	argv_vector.push_back(const_cast<char *>(executable.c_str()));
	argv_vector.push_back(NULL);
}

/**
 * @brief fills the vector with the variables as env and add also a gateway key for the cgi protocol
 * @param env_c_vector vector<char*> with env variables
 * @param env_vector vector<string,string> with env variables
 */
void	CGI::createEnvCharPtrVector(std::vector<char*> &env_c_vector, std::vector<std::string> &env_vector) {
	env_vector.push_back(const_cast<char*>("GATEWAY=CGI/1.1"));
	env_vector.push_back(const_cast<char*>("SERVER_PROTOCOL=HTTP/1.1"));
	for (auto& str : env_vector)
		env_c_vector.push_back(const_cast<char*>(str.c_str()));
	env_c_vector.push_back(NULL);
}
