#ifndef WEBSERV_HPP
#define WEBSERV_HPP

// STD libs
# include <iostream>
# include <fstream>
# include <string>
# include <vector>
# include <exception>
# include <algorithm>
# include <map>
# include <numeric>
# include <algorithm>
# include <regex>
# include <unordered_map>
# include <cctype>
# include <sstream>

// Colors
# include "Colors.hpp"

// DEFAULTS
# define DEFAULT_PORT 8080
# define DEFAULT_CLIENT_BODY_SIZE 1048576 // 1m or 1MB
# define DEFAULT_INDEX "index.html"
# define DEFAULT_AUTOINDEX false
# define DEFAULT_LOCATION "/"
# define DEFAULT_SERVERS_CONFIG "configs/default.conf"
# define DEFAULT_MIME_TYPES "configs/default.conf"


#endif // WEBSERV_HPP
