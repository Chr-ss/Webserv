#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "Webserv.hpp"

// make a static for the mime.types conf
// make the htpp wrapper maybe??
// read and store the mime.type map
struct Location {
	bool														strict_match;
	std::unordered_map<std::string, std::vector<std::string>>	directives;
};

class Config {
	private:
		std::unordered_map<std::string, std::vector<std::string>>		_directives;
		std::unordered_map<std::string, std::vector<std::string>> const	&_mimeTypes;
		std::unordered_map<std::string, Location>						_locations;

		// UTILS
	public:
		Config() = delete;
		Config(std::unordered_map<std::string, std::vector<std::string>> const &mimeTypes);
		~Config();

		// SETTERS
		int	setLocation(std::string key, Location loc);
		int	setDirective(std::string key, std::vector<std::string> values);

		// GET RAW DATA
		const std::unordered_map<std::string, std::vector<std::string>>	&getDirectives();
		const std::unordered_map<std::string, Location>					&getLocations();
		const std::unordered_map<std::string, std::vector<std::string>>	&getMimeTypes();
		const std::unordered_map<std::string, std::vector<std::string>>	getLocDirectives(const std::string locKey);

		//	GETTER need for WebServer/Client Class
		int						getPort();
		const std::string		getHost();
		const std::string		getServerName();
		const std::string		getErrorPage(int errorCode);	// error_page 404 /tmp/www/404.html;

		bool							getAutoindex(const std::string locKey);			// autoindex on;
		std::uint64_t					getClientBodySize(const std::string locKey);	// client_max_body_size 10M;
		const std::vector<std::string>	getRedirect(const std::string locKey);			// return 301 http://example.com/newpage;
		const std::vector<std::string>	getRoot(const std::string locKey);				// root /tmp/www;
		const std::vector<std::string>	getMethods(const std::string locKey);			// allow_methods  DELETE POST GET;
		const std::vector<std::string>	getIndex(const std::string locKey);				// index index.html index.php;

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

#endif // SERVERCONFIG_HPP
