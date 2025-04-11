#include "../../inc/CGI/CGI.hpp"

/// @brief creating a vector<char*> for transfor to an array of * to strings ascommand-line arguments for execve.
std::vector<char*>	CGI::createArgvVector( const std::string &executable) {
	std::vector<char*>	result;
	result.push_back(const_cast<char *>(executable.c_str()));
	result.push_back(NULL);
	return (result);
}

