#include "../inc/Config.hpp"

Config::Config(std::unordered_map<std::string, std::vector<std::string>> const &mimeTypes) : _mimeTypes(mimeTypes) {}

Config::~Config() {
}

int Config::setDirective(const std::string key, std::vector<std::string> values) {
	if (key != "listen" && key != "error_page" && _directives.find(key) != _directives.end())
		return (-1);
	for (std::string str: values) {
		_directives[key].push_back(str);
	}
	return (0);
}

int Config::setLocation(const std::string key, Location loc) {
	if (_locations.find(key) != _locations.end())
		return (-1);
	_locations[key] = loc;
	return (0);
}

void Config::printConfig() {
	std::cout << BOLD << BG_LIGHT_GRAY << BLACK << "\n CONFIG PRINT - MIME TYPES:" << RESET << std::endl;
	for (auto it = _mimeTypes.begin(); it != _mimeTypes.end(); it++) {
		std::cout << "\t" << it->first << " : ";
		for (const std::string &str : it->second)
			std::cout << str << " ";
		std::cout << "\n";
	}
	std::cout << BOLD << BG_LIGHT_GRAY << BLACK << "\n CONFIG PRINT - DIRECTIVES:" << RESET << std::endl;
	for (auto it = _directives.begin(); it != _directives.end(); it++) {
		std::cout << "\t" << it->first << " : ";
		for (const std::string &str : it->second)
			std::cout << str << " ";
		std::cout << "\n";
	}
	std::cout << BOLD << BG_LIGHT_GRAY << BLACK << "\n CONFIG PRINT - LOCATIONS:" << RESET << std::endl;
	for (const auto &pair: _locations)
	{
		std::cout << BOLD << pair.first << ":\n" << RESET;
		std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(pair.first);
		for (const auto &str: dirMap)
		{
			std::cout << "\t" << str.first << " : ";
			for (const std::string &str2: str.second)
				std::cout << str2 << " ";
			std::cout << "\n";
		}
		std::cout << "\n";
	}
}


// ###############################################################
// ####################     GET RAW DATA     #####################
const std::unordered_map<std::string, std::vector<std::string>> &Config::getDirectives() {
	return (this->_directives);
}
const std::unordered_map<std::string, Location>	&Config::getLocations() {
	return (this->_locations);
}
const std::unordered_map<std::string, std::vector<std::string>> &Config::getMimeTypes() {
	return (this->_mimeTypes);
}
const std::unordered_map<std::string, std::vector<std::string>> Config::getLocDirectives(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> locMap = this->_directives;
	size_t pos = 0;
	std::string key = "/";
	while (pos < locKey.size()) {
		auto it = this->_locations.find(key);
		if (it != this->_locations.end()) {
			Location loc = it->second;
			if (loc.strict_match == false || pos + 1 == locKey.size()) {
				for (auto it_loc = loc.directives.begin(); it_loc != loc.directives.end(); it_loc++) {
					locMap[it_loc->first] = it_loc->second;
				}
			}
		}
		pos = locKey.find('/', pos + 1);
		key = locKey.substr(0, pos + 1);
	}
	return (locMap);
}
// ####################     GET RAW DATA     #####################
// ###############################################################


int	Config::getPort() {
	int port;
	std::string strPort;
	auto it = this->_directives.find("listen");
	if (it != this->_directives.end()) {
		if (it->second.size() > 0) {
			strPort = it->second[0];
		}
		port = stoi(strPort);
	} else {
		port = DEFAULT_PORT;
	}
	// try {
	// } catch (std::exception &e) {
	// 	throw Config::ConfigException(e.what());
	// }
	// if (port < 0 || port > 65535)
	// 	throw Config::ConfigException("Port out of range!");
	return (port);
}

const std::string	Config::getHost() {
	// return ((*(this->_directives.find("host"))).second.front());
	auto it = this->_directives.find("host");
	if (it != this->_directives.end()) {
		if (it->second.size() > 0) {
			return (it->second[0]);
		}
	}
	return ("localhost");
}

const std::string	Config::getServerName() {
	// return ((*(this->_directives.find("server_name"))).second.front());
	auto it = this->_directives.find("server_name");
	if (it != this->_directives.end()) {
		if (it->second.size() > 0) {
			return (it->second[0]);
		}
	}
	return ("");
}

// client_max_body_size 10M;
std::uint64_t	Config::getClientBodySize(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("client_max_body_size");
	std::string strSize;
	std::uint64_t	size;
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			strSize = it->second[0];
		}
		size = stoi(strSize);
		if (tolower(strSize.back()) == 'k')
			size *= 1024;
		else if (tolower(strSize.back()) == 'm')
			size *= (1024*1024);
		else if (tolower(strSize.back()) == 'g')
			size *= (1024*1024*1024);
	} else {
		size = DEFAULT_CLIENT_BODY_SIZE;
	}
	return (size);
}

// return 301 http://example.com/newpage;
const std::vector<std::string>	Config::getRedirect(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("return");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			return it->second;
		}
	}
	return (std::vector<std::string>());
}

// root /tmp/www;
const std::vector<std::string>	Config::getRoot(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("index");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			return it->second;
		}
	}
	return (std::vector<std::string>());
}

// allow_methods  DELETE POST GET;
const std::vector<std::string>	Config::getMethods(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("allow_methods");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			return it->second;
		}
	}
	return (std::vector<std::string>{"GET", "POST", "DELETE"});
}

// index index.html index.php;
const std::vector<std::string>	Config::getIndex(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("index");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			return it->second;
		}
	}
	return (std::vector<std::string>{DEFAULT_INDEX});
}

// autoindex on;
bool	Config::getAutoindex(const std::string locKey) {
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("autoindex");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			if (it->second[0] == "on")
				return (true);
			else if (it->second[0] == "off")
				return (false);
		}
	}
	return (DEFAULT_AUTOINDEX);
}

// error_page 404 /tmp/www/404.html;
const std::string	Config::getErrorPage(int errorCode) {
	auto it = this->_directives.find("error_page");
	if (it != this->_directives.end()) {
		for (size_t i = 0; i < it->second.size(); i++) {
			if (std::stoi(it->second[i]) == errorCode) {
				return (it->second[i + 1]);
			}
		}
	}
	return ("");
}