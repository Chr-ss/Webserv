#pragma once

// #include "../Webserv/Webserv.hpp"
# include <cstdint>
# include <vector>
# include <unordered_map>
# include <iostream>
# include <regex>


class Webserv;

// make a static for the mime.types conf
// make the htpp wrapper maybe??
// read and store the mime.type map
struct Location {
	bool														strict_match;
	std::unordered_map<std::string, std::vector<std::string>>	directives;
};

class Config {
	private:
		std::unordered_map<std::string, std::vector<std::string>>			_directives;
		std::unordered_map<std::string, Location>							_locations;
		static std::unordered_map<std::string, std::vector<std::string>>	_mimeTypes;

	public:
		Config();
		~Config();

		// SETTERS
		int		setLocation(std::string key, Location loc);
		int		setDirective(std::string key, std::vector<std::string> values);
		void	setMimeTypes(const std::unordered_map<std::string, std::vector<std::string>> &mimeTypes);

		// GET RAW DATA
		const std::unordered_map<std::string, std::vector<std::string>>	&getDirectives() const ;
		const std::unordered_map<std::string, Location>					&getLocations()const ;
		const std::unordered_map<std::string, std::vector<std::string>>	&getMimeTypes() const;
		const std::unordered_map<std::string, std::vector<std::string>>	getLocDirectives(const std::string locKey) const;

		//	GETTER need for WebServer/Client Class
		int						getPort() const;
		const std::string		getHost() const;
		const std::string		getServerName() const;

		bool							getAutoindex(const std::string locKey) const;		// autoindex on;
		std::uint64_t					getClientBodySize(const std::string locKey) const;	// client_max_body_size 10M;
		const std::vector<std::string>	getRedirect(const std::string locKey) const;		// return 301 http://example.com/newpage;
		const std::vector<std::string>	getRoot(const std::string locKey) const;			// root /tmp/www;
		const std::vector<std::string>	getMethods(const std::string locKey) const;			// allow_methods  DELETE POST GET;
		const std::vector<std::string>	getIndex(const std::string locKey) const;			// index index.html index.php;
		const std::string				getErrorPage(int errorCode) const;	// error_page 404 /tmp/www/404.html;
		// UTILS
		void	printConfig();

		// EXCEPTIONS
		class ConfigException : public std::exception {
			private:
				std::string message;
			public:
				ConfigException(const std::string &msg) : message(msg) {}
				virtual const char* what() const throw() {
					return message.c_str();
				}
		};
};
