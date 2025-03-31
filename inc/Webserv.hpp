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

// C libs

// DEFAULTS
# define DEFAULT_PORT 8080
// # define DEFAULT_HOST ????
// # define DEFAULT_SERVER_NAME ????
# define DEFAULT_CLIENT_BODY_SIZE 1048576 // 1m or 1MB
# define DEFAULT_INDEX "index.html"
# define DEFAULT_AUTOINDEX false
# define DEFAULT_LOCATION "/"
# define DEFAULT_SERVERS_CONFIG "configs/default.conf"
# define DEFAULT_MIME_TYPES "configs/default.conf"

// COLORS AND TEXT FORMATING
# define RESET          "\033[0m"
# define BLACK          "\033[30m"
# define WHITE          "\033[37m"
# define BRIGHT_WHITE   "\033[97m"
# define RED            "\033[31m"
# define BRIGHT_RED     "\033[91m"
# define YELLOW         "\033[33m"
# define BRIGHT_YELLOW  "\033[93m"
# define BLUE           "\033[34m"
# define LIGHT_BLUE     "\033[38;5;123m"
# define BRIGHT_BLUE    "\033[94m"
# define MAGENTA        "\033[35m"
# define BRIGHT_MAGENTA "\033[95m"
# define CYAN           "\033[36m"
# define BRIGHT_CYAN    "\033[96m"
# define GREEN          "\033[32m"
# define BRIGHT_GREEN   "\033[92m"
# define GRAY           "\033[90m"
# define LIGHT_GRAY     "\033[37m"
# define DARK_GRAY      "\033[90m"
# define ORANGE         "\033[38;5;208m"
# define PINK           "\033[38;5;213m"
# define PURPLE         "\033[38;5;129m"
# define BOLD           "\033[1m"
# define UNDERLINE      "\033[4m"

# define BG_BLACK          "\033[40;30m"
# define BG_WHITE          "\033[47;30m"
# define BG_BRIGHT_WHITE   "\033[107;30m"
# define BG_RED            "\033[41;30m"
# define BG_BRIGHT_RED     "\033[101;30m"
# define BG_YELLOW         "\033[43;30m"
# define BG_BRIGHT_YELLOW  "\033[103;30m"
# define BG_BLUE           "\033[44;30m"
# define BG_LIGHT_BLUE     "\033[48;5;123;30m"
# define BG_BRIGHT_BLUE    "\033[104;30m"
# define BG_MAGENTA        "\033[45;30m"
# define BG_BRIGHT_MAGENTA "\033[105;30m"
# define BG_CYAN           "\033[46;30m"
# define BG_BRIGHT_CYAN    "\033[106;30m"
# define BG_GREEN          "\033[42;30m"
# define BG_BRIGHT_GREEN   "\033[102;30m"
# define BG_GRAY           "\033[100;30m"
# define BG_LIGHT_GRAY     "\033[47;30m"
# define BG_DARK_GRAY      "\033[100;30m"
# define BG_ORANGE         "\033[48;5;208;30m"
# define BG_PINK           "\033[48;5;213;30m"
# define BG_PURPLE         "\033[48;5;129;30m"


#endif // WEBSERV_HPP
