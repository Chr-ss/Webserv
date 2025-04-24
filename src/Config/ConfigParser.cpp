#include "../../inc/Config/ConfigParser.hpp"

void debugConfigPrint(std::vector<Config> &configs) {
	for (auto &config : configs) {
		config.printConfig();
		std::cout << BOLD << "getHostPort():";
		std::unordered_map<std::string, std::vector<int>> hostPort = config.getHostPort();
		for (const auto &entry : hostPort) {
			std::cout << "\n\t\t" << entry.first << " >> ";
			for (auto it = entry.second.begin(); it != entry.second.end(); ++it) {
				std::cout << entry.first << ":" << *it;
				if (std::next(it) != entry.second.end()) {
					std::cout << ", ";
				}
			}
		}
		std::cout << RESET << std::endl;
		std::cout << BOLD << "getServerName(): " << config.getServerName() << RESET << std::endl;
		std::cout << BOLD << "getErrorPage(404): " << config.getErrorPage(404) << RESET << std::endl;
		std::cout << BOLD << "getAutoindex(\"/\"): " << config.getAutoindex("/") << RESET << std::endl;
		std::cout << BOLD << "getAutoindex(\"data/www/images/\"): " << config.getAutoindex("data/www/images/") << RESET << std::endl;
		std::cout << BOLD << "getClientBodySize(\"/\"): " << config.getClientBodySize("/") << RESET << std::endl;
		std::cout << BOLD << "getRoot(\"/\"): ";
		std::vector<std::string> roots = config.getRoot("/");
		for (std::vector<std::string>::iterator it = roots.begin(); it != roots.end(); ++it) {
			std::cout << *it << " ";
		}
		std::cout << RESET << std::endl;
		std::cout << BOLD << "getMethods(\"/\"): ";
		std::vector<std::string> methods = config.getMethods("/");
		for (std::vector<std::string>::iterator it = methods.begin(); it != methods.end(); ++it) {
			std::cout << *it << " ";
		}
		std::cout << RESET << std::endl;
		std::cout << BOLD << "getIndex(\"/\"): ";
		std::vector<std::string> index = config.getIndex("/");
		for (std::vector<std::string>::iterator it = index.begin(); it != index.end(); ++it) {
			std::cout << *it << " ";
		}
		std::cout << RESET << std::endl;
		std::cout << BOLD << "getRedirect(\"/http/\"): ";
		std::vector<std::string> redirect = config.getRedirect("/http/");
		for (std::vector<std::string>::iterator it = redirect.begin(); it != redirect.end(); ++it) {
			std::cout << *it << " ";
		}
		std::cout << RESET << std::endl;
		std::cout << BOLD << "getRedirect(\"/old_file\"): ";
		std::vector<std::string> redirect1 = config.getRedirect("/old-page");
		for (std::vector<std::string>::iterator it = redirect1.begin(); it != redirect1.end(); ++it) {
			std::cout << *it << " ";
		}
		std::cout << RESET << std::endl;
	}
}

ConfigParser::ConfigParser(const std::string& filepath) {
	this->_filepath = filepath;
	std::cout << filepath << std::endl;
	readConfigToInput();
	parseInputToTokens();
	// printTokens(this->_tokens);
	parseTokenToConfig();
	this->_configs[0].setMimeTypes(this->_mimeTypes);
	// debugConfigPrint(this->_configs);
}

ConfigParser::~ConfigParser() {
}

ConfigParser::ConfigParser(const ConfigParser &toCopy) {
	*this = toCopy;
}

ConfigParser& ConfigParser::operator=(const ConfigParser &other) {
	if (this != &other) {
		this->_filepath = other._filepath;
		this->_input = other._input;
	}
	return *this;
}


// ###############################################################
// ################     READ AND FILEHANDLING     ################
std::ifstream openFile(const std::string& filepath) {
	if (filepath.empty() || filepath.length() == 0) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: filepath is empty");
	}
	std::ifstream file;
	file.open(filepath.c_str());
	if (!file || !file.is_open()) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: could not open file");
	}
	return (file);
}

std::string readFile(std::ifstream& file) {
	std::streampos original_pos = file.tellg();
	file.seekg(0, std::ios::end);
	std::streamsize fileSize = file.tellg();
	if (fileSize < 0) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: Failed to determine config file size");
	}
	if (fileSize > MAX_CONFIG_FILE_SIZE) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: Config file too large (limit is MAX_FILE_SIZE)");
	}
	file.seekg(0, std::ios::beg);
	std::string fileContent(static_cast<size_t>(fileSize), '\0');
	if (!file.read(&fileContent[0], fileSize)) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: Failed to read config file");
	}
	file.seekg(original_pos);
	return (fileContent);
}

void closeFile(std::ifstream& file) {
	file.close();
	if (file.fail()) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: could not close config file.");
	} else if (file.bad()) {
		throw ConfigParser::ConfigParserException("ERROR PARSING: Critical I/O error! Bad config file");
	}
}

void ConfigParser::readConfigToInput() {
	try {
		std::ifstream config_file = openFile(this->_filepath);
		this->_input = readFile(config_file);
		closeFile(config_file);
	} catch (std::exception &e) {
		throw ConfigParser::ConfigParserException(e.what());
	}
}

void ConfigParser::readMimeToInput(const std::string &filepath) {

	try {
		std::ifstream file = openFile(filepath);
		this->_inputMime = readFile(file);
		closeFile(file);
	} catch (std::exception &e) {
		throw ConfigParser::ConfigParserException(e.what());
	}
}
// ################     READ AND FILEHANDLING     ################
// ###############################################################


void ConfigParser::printInput() {
	std::cout << this->_input << std::endl;
}

void ConfigParser::printTokens(std::vector<token> &tokens) {
	std::unordered_map<tokenType, std::string> tokenColorMap = {
		{INIT, BG_BRIGHT_WHITE},
		{WHITE_SPACE, BG_BRIGHT_WHITE},
		{TYPES, BG_PINK},
		{COMMENT, BG_GRAY},
		{HTTP, BG_MAGENTA},
		{INCLUDE, BG_ORANGE},
		{SERVER, BG_BRIGHT_BLUE},
		{LOCATION, BG_BLUE},
		{BLOCK_OPEN, BG_BRIGHT_CYAN},
		{BLOCK_CLOSE, BG_BRIGHT_GREEN},
		{SEMICOLON, BG_BRIGHT_YELLOW},
		{STRING, BG_BRIGHT_MAGENTA},
		{NUMBER, BG_YELLOW},
		{PATH, BG_GREEN},
		{OPERATOR, BG_PURPLE},
		{EOF_TOKEN, BG_RED}
	};
	for (const auto &it : tokens) {
		std::string color = tokenColorMap.count(it.type) ? tokenColorMap[it.type] : BG_RED;
		if (it.value != "\n")
			std::cout << color << it.value << RESET;
		else
			std::cout << it.value << RESET;
	}
}

void	ConfigParser::errorToken(token token, std::string msg) {
	int line;
	int col;
	getTokenPos(token, line, col);
	throw ConfigParser::ConfigParserException("Unexpected token at Ln " + std::to_string(line) + ", Col " + std::to_string(col) + " " + msg);
}

// bool isValidIPv4(const std::string& ip) {
// 	std::istringstream ss(ip);
// 	std::string token;
// 	int count = 0;

// 	while (std::getline(ss, token, '.')) {
// 		++count;
// 		if (count > 4)
// 			return (false);
// 		if (token.empty() || token.length() > 3)
// 			return (false);
// 		for (char c : token) {
// 			if (!isdigit(static_cast<unsigned char>(c)))
// 				return (false);
// 		}
// 		if (token.length() > 1 && token[0] == '0')
// 			return (false);
// 		int num = std::stoi(token);
// 		if (num < 0 || num > 255)
// 			return (false);
// 	}
// 	return (true);
// }

void	ConfigParser::checkConfig(Config &config) {
	size_t indexConf = _configs.size() + 1;
	try {
		if (config.getHostPort().empty())
			throw ConfigParser::ConfigParserException("Host and port are not defined.");
		for (std::vector<Config>::iterator it = this->_configs.begin(); it != this->_configs.end(); ++it) {
			if (it->getServerName() == config.getServerName()) {
				for (auto &host: it->getHostPort()) {
					for (auto &port: host.second) {
						for (auto &host2: config.getHostPort()) {
							if (host.first == host2.first) {
								for (auto &port2: host2.second) {
									if (port == port2) {
										throw ConfigParser::ConfigParserException("Duplicate server block with same host and port.");
									}
								}
							}
						}
					}
				}
			}
		}
		if (config.getRoot("/").empty())
			throw ConfigParser::ConfigParserException("Root is not defined.");
	} catch (ConfigParser::ConfigParserException &e) {
		throw ConfigParser::ConfigParserException("ERROR CONFIG: Server-" + std::to_string(indexConf) + ": " + std::string(e.what()));
	}
}

token ConfigParser::getNextToken(token &lastToken, const std::regex &path_regex, const std::regex &op_regex) {
	token newToken;
	newToken.itStart = lastToken.itEnd;
	if (*newToken.itStart == '#') {
		newToken.type = COMMENT;
		newToken.itEnd = std::find(newToken.itStart, this->_input.end(), '\n');
		newToken.value = std::string(newToken.itStart, newToken.itEnd);
		return newToken;
	}
	newToken.itEnd = std::find_if(newToken.itStart, this->_input.end(), [](char c) {
		return (c == ' ' || c == '\n' || c == '\t' || c == '{' || c == '}' || c == ';' || c == '#');
	});
	if (newToken.itStart != this->_input.end() && newToken.itStart == newToken.itEnd) {
		newToken.itEnd++;
	}
	newToken.value = std::string(newToken.itStart, newToken.itEnd);
	if (newToken.itStart == this->_input.end()) {
		newToken.type = EOF_TOKEN;
	} else if (newToken.value == "server") {
		newToken.type = SERVER;
	} else if (newToken.value == "location") {
		newToken.type = LOCATION;
	} else if (newToken.value == "http") {
		newToken.type = HTTP;
	} else if (newToken.value == "include") {
		newToken.type = INCLUDE;
	} else if (newToken.value == "{") {
		newToken.type = BLOCK_OPEN;
	} else if (newToken.value == "}") {
		newToken.type = BLOCK_CLOSE;
	} else if (newToken.value == ";") {
		newToken.type = SEMICOLON;
	} else if (newToken.value == " " || newToken.value == "\t" || newToken.value == "\n") {
		newToken.type = WHITE_SPACE;
	} else if (!newToken.value.empty() && std::all_of(newToken.value.begin(), newToken.value.end(), ::isdigit)) {
		newToken.type = NUMBER;
	} else if (std::regex_match(newToken.value, op_regex)) {
		newToken.type = OPERATOR;
	} else if (std::regex_match(newToken.value, path_regex)) {
		newToken.type = PATH;
	} else {
		newToken.type = STRING;
	}
	return newToken;
}

void ConfigParser::parseInputToTokens() {
	token newToken;
	static const std::regex path_regex(R"(^(/[a-zA-Z0-9._~!$&'()*+,;=:@-]*)*$)");
	static const std::regex op_regex(R"([=~!^][=~!^|*]*)");
	newToken.type = INIT;
	newToken.itEnd = this->_input.begin();
	while (newToken.type != EOF_TOKEN) {
		newToken = getNextToken(newToken, path_regex, op_regex);
		this->_tokens.push_back(newToken);
	}
}


token ConfigParser::getNextMimeToken(token &lastToken) {
	token newToken;
	newToken.itStart = lastToken.itEnd;
	newToken.itEnd = std::find_if(newToken.itStart, this->_inputMime.end(), [](char c) {
		return (c == ' ' || c == '\n' || c == '\t' || c == '{' || c == '}' || c == ';' || c == '#');
	});
	if (newToken.itStart == newToken.itEnd) {
		newToken.itEnd++;
	}
	newToken.value = std::string(newToken.itStart, newToken.itEnd);
	if (newToken.itEnd == this->_inputMime.end()) {
		newToken.type = EOF_TOKEN;
	} else if (newToken.value == "types") {
		newToken.type = TYPES;
	} else if (newToken.value == "{") {
		newToken.type = BLOCK_OPEN;
	} else if (newToken.value == "}") {
		newToken.type = BLOCK_CLOSE;
	} else if (newToken.value == ";") {
		newToken.type = SEMICOLON;
	} else if (newToken.value == " " || newToken.value == "\t" || newToken.value == "\n") {
		newToken.type = WHITE_SPACE;
	} else {
		newToken.type = STRING;
	}
	return newToken;
}

void ConfigParser::parseMimeToTokens() {
	token newToken;
	newToken.type = INIT;
	newToken.itEnd = this->_inputMime.begin();
	while (newToken.type != EOF_TOKEN) {
		newToken = getNextMimeToken(newToken);
		this->_tokensMime.push_back(newToken);
	}
}

void	ConfigParser::getTokenPos(token token, int &line, int &col) {
	line = 1;
	col = 1;
	for (std::string::iterator it = this->_input.begin(); it != this->_input.end() && it != token.itStart; it++) {
		col++;
		if (*it == '\n') {
			line++;
			col = 0;
		}
	}
}

void ConfigParser::parseTokenToDirective(std::vector<token>::iterator &it, Config &newServer) {
	std::string key = it->value;
	std::vector<token>::iterator key_it = it;
	std::vector<std::string> values;
	moveOneTokenSafly(this->_tokens, it);
	for (;it != this->_tokens.end(); it++) {
		if (it->type < STRING)
			break;
		values.push_back(it->value);
	}
	if (it->type != SEMICOLON)
		errorToken(*it, "Expected: ;");
	if (newServer.setDirective(key, values) == -1) {
		errorToken(*key_it, "Info: duplicate directive key.");
	}
}

void	ConfigParser::moveOneTokenSafly(std::vector<token> &tokens, std::vector<token>::iterator &it) {
	it++;
	if (it == tokens.end() || it->type == EOF_TOKEN)
		errorToken(*it, "Unexpected EOF!");
}

void ConfigParser::parseTokenToLocDir(std::vector<token>::iterator &it, Location &loc) {
	std::string key = it->value;
	std::vector<token>::iterator key_it = it;
	std::vector<std::string> values;
	if (key == "index")
		errorToken(*it, "Invalid argument in location");
	moveOneTokenSafly(this->_tokens, it);
	for (;it != this->_tokens.end(); it++) {
		if (it->type < STRING)
			break;
		values.push_back(it->value);
	}
	if (it->type != SEMICOLON) {
		errorToken(*it, "Expected: ;");
	}
	if (loc.directives.find(key) != loc.directives.end()) {
		errorToken(*key_it, "Info: duplicate directive key.");
	}
	loc.directives[key] = values;
}

void ConfigParser::parseTokenToLocation(std::vector<token>::iterator &it, Config &newServer) {
	moveOneTokenSafly(this->_tokens, it);
	Location newloc;
	std::string path_key;

	if (it->value == "=") {
		newloc.strict_match = 1;
		moveOneTokenSafly(this->_tokens, it);
	} else {
		newloc.strict_match = 0;
	}
	if (it->type == PATH) {
		path_key = it->value;
		moveOneTokenSafly(this->_tokens, it);
	} else {
		errorToken(*it, "Expected: PATH");
	}
	if (it->type == BLOCK_OPEN) {
		moveOneTokenSafly(this->_tokens, it);
	} else {
		errorToken(*it, "Expected: {");
	}
	for (;it != this->_tokens.end(); it++) {
		if (it->type == STRING) {
			parseTokenToLocDir(it, newloc);
		} else if (it->type == BLOCK_CLOSE) {
			break;
		} else {
			errorToken(*it, "Expected STRING or }");
		}
	}
	newServer.setLocation(path_key, newloc);
}

void ConfigParser::parseTokenToServer(std::vector<token>::iterator &it) {
	moveOneTokenSafly(this->_tokens, it);
	if (it == this->_tokens.end() || it->type != BLOCK_OPEN)
		errorToken(*it, "{");
	else if (it->type == BLOCK_OPEN)
		moveOneTokenSafly(this->_tokens, it);
	Config newServer;
	for (;it != this->_tokens.end(); ++it) {
		if (it->type == STRING) {
			parseTokenToDirective(it, newServer);
		} else if (it->type == LOCATION) {
			parseTokenToLocation(it, newServer);
		} else if (it->type == BLOCK_CLOSE) {
			it++;
			break;
		} else {
			errorToken(*it, "Expected STRING, LOCATION or }");
		}
	}
	// newServer.printConfig();
	checkConfig(newServer);
	this->_configs.push_back(newServer);
}

void	ConfigParser::eraseToken(std::vector<token> &tokens, enum tokenType type) {
	for (std::vector<token>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
		if (it->type == type) {
			tokens.erase(it);
			it--;
		}
	}
}

void	ConfigParser::printMimeTypes() {
	std::cout << BOLD << BG_LIGHT_GRAY << BLACK << "\n CONFIG PRINT - MIME TYPES:" << RESET << std::endl;
	for (auto it = this->_mimeTypes.begin(); it != this->_mimeTypes.end(); it++) {
		std::cout << "\t" << it->first << " : ";
		for (const std::string &str : it->second)
			std::cout << str << " ";
		std::cout << "\n";
	}
}

std::unordered_map<std::string, std::vector<std::string>>	ConfigParser::parseMimeToken() {
	eraseToken(this->_tokensMime, WHITE_SPACE);
	std::unordered_map<std::string, std::vector<std::string>>	mapReturn;
	for (std::vector<token>::iterator it = this->_tokensMime.begin(); it != this->_tokensMime.end(); it++) {
		if (it == this->_tokensMime.begin()) {
			if (it->type != TYPES)
				throw ConfigParser::ConfigParserException("Unexpected token in http. Expected: TYPES");
			moveOneTokenSafly(this->_tokensMime, it);
			if (it->type != BLOCK_OPEN)
				throw ConfigParser::ConfigParserException("Unexpected token in http. Expected: BLOCK_OPEN");
		}
		else if (it->type == STRING) {
			std::string key = it->value;
			moveOneTokenSafly(this->_tokensMime, it);
			for (; it != this->_tokensMime.end(); ++it) {
				if (it->type == SEMICOLON) {
					break ;
				}
				else if (it->type != STRING) {
					throw ConfigParser::ConfigParserException("Unexpected token in http. Expected: STRING1");
				}
				mapReturn[key].push_back(it->value);
			}
		}
		else if (it->type == BLOCK_CLOSE) {
			break ;
		}
		else {
			std::cout << it->type << std::endl;
			throw ConfigParser::ConfigParserException("Unexpected token in http. Expected: STRING2");
		}
	}
	return (mapReturn);
}

void	ConfigParser::parseTokenToConfig() {
	eraseToken(this->_tokens, WHITE_SPACE);
	eraseToken(this->_tokens, COMMENT);
	bool inHttp = false;
	bool mimeSet = false;
	for (std::vector<token>::iterator it = this->_tokens.begin(); it != this->_tokens.end(); ++it) {
		if (it->type == HTTP) {
			if (inHttp) {
				errorToken(*it, "Unexpected: http");
			}
			inHttp = true;
			moveOneTokenSafly(this->_tokens, it);
			if (it->type != BLOCK_OPEN) {
				errorToken(*it, "Expected: {");
			}
		}
		else if (it->type == INCLUDE) {
			if (!inHttp || mimeSet) {
				errorToken(*it, "Unexpected: include");
			}
			moveOneTokenSafly(this->_tokens, it);
			if (it->type != STRING && it->type != PATH) {
				errorToken(*it, "Expected: STRING or PATH");
			}
			readMimeToInput(it->value);
			parseMimeToTokens();
			this->_mimeTypes = parseMimeToken();
			mimeSet = true;
			moveOneTokenSafly(this->_tokens, it);
			if (it->type != SEMICOLON) {
				errorToken(*it, "Expected: ;");
			}
		}
		else if (it->type == SERVER) {
			if (!inHttp) {
				errorToken(*it, "Unexpected: server");
			}
			parseTokenToServer(it);
			it--;
		}
		else if (!inHttp && it->type == BLOCK_CLOSE) {
			moveOneTokenSafly(this->_tokens, it);
		}
		else if (inHttp && it->type == BLOCK_CLOSE) {
			inHttp = false;
			it++;
			if (it->type != EOF_TOKEN) {
				errorToken(*it, "Expected: EOF");
			}
			break ;
		}
		else {
			errorToken(*it, "Unexpected token");
		}
	}
	if (this->_configs.empty())
		throw ConfigParser::ConfigParserException("Missing any Server config in .conf file.");
	if (!mimeSet) {
		readMimeToInput(DEFAULT_MIME_TYPES);
		parseMimeToTokens();
		this->_mimeTypes = parseMimeToken();
	}
}