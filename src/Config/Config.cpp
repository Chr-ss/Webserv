#include "../../inc/Config/Config.hpp"
#include "../../inc/Config/ConfigParser.hpp"

std::unordered_map<std::string, std::vector<std::string>> Config::_mimeTypes;

Config::Config() {}

Config::~Config() {}

void	Config::setMimeTypes(const std::unordered_map<std::string, std::vector<std::string>> &mimeTypes) {
	_mimeTypes = mimeTypes;
}

int Config::setDirective(const std::string key, std::vector<std::string> values) {
	if (key != "error_page" && _directives.find(key) != _directives.end())
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
const std::unordered_map<std::string, std::vector<std::string>> &Config::getDirectives() const{
	return (this->_directives);
}
const std::unordered_map<std::string, Location>	&Config::getLocations() const{
	return (this->_locations);
}
const std::unordered_map<std::string, std::vector<std::string>> &Config::getMimeTypes() const{
	return (this->_mimeTypes);
}
const std::unordered_map<std::string, std::vector<std::string>> Config::getLocDirectives(const std::string locKey) const{
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


int	Config::getPort() const{
	int port;
	std::string strPort;
	auto it = this->_directives.find("listen");
	if (it != this->_directives.end()) {
		strPort = it->second[0];
		size_t pos = strPort.find(':');
		if (pos != std::string::npos) {
			strPort = strPort.substr(pos + 1);
		}
		if (strPort.find_first_not_of("0123456789") != std::string::npos) {
			throw Config::ConfigException("Port is not a number!");
		}
		if (strPort.empty()) {
			throw Config::ConfigException("Port is empty!");
		}
		port = stoi(strPort);
	} else {
		port = DEFAULT_PORT;
	}
	return (port);
}

const std::string	Config::getHost() const{
	auto it = this->_directives.find("host");
	if (it != this->_directives.end()) {
		if (it->second.size() > 0) {
			return (it->second[0]);
		}
	}
	else {
		auto it = this->_directives.find("listen");
		if (it != this->_directives.end()) {
			std::string strHost = it->second[0];
			size_t pos = strHost.find(':');
			if (pos != std::string::npos) {
				return (strHost.substr(0, pos));
			}
		}
	}
	return ("");
}

const std::string	Config::getServerName() const{
	auto it = this->_directives.find("server_name");
	if (it != this->_directives.end()) {
		if (it->second.size() > 0) {
			return (it->second[0]);
		}
	}
	return ("");
}

// client_max_body_size 10M;
std::uint64_t	Config::getClientBodySize(const std::string locKey) const{
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
const std::vector<std::string>	Config::getRedirect(const std::string locKey) const{
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
const std::vector<std::string>	Config::getRoot(const std::string locKey) const{
	std::unordered_map<std::string, std::vector<std::string>> dirMap = this->getLocDirectives(locKey);
	auto it = dirMap.find("root");
	if (it != dirMap.end()) {
		if (it->second.size() > 0) {
			return it->second;
		}
	}
	return (std::vector<std::string>());
}

// allow_methods  DELETE POST GET;
const std::vector<std::string>	Config::getMethods(const std::string locKey) const{
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
const std::vector<std::string>	Config::getIndex(const std::string locKey) const{
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
bool	Config::getAutoindex(const std::string locKey) const{
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
const std::string	Config::getErrorPage(int errorCode) const {
	auto it = this->_directives.find("error_page");
	if (it != this->_directives.end()) {
		for (size_t i = 0; i < it->second.size() && (i + 1) < it->second.size() ; i += 2) {
			if (it->second[i] == std::to_string(errorCode)) {
				return (it->second[i + 1]);
			}
		}
	}
	return ("");
}