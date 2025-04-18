#pragma once

#include "../Webserv/Webserv.hpp"
#include "Config.hpp"

enum tokenType {
	INIT,
	EOF_TOKEN,
	WHITE_SPACE,
	COMMENT,
	TYPES,
	HTTP,
	INCLUDE,
	SERVER,
	LOCATION,
	BLOCK_OPEN,
	BLOCK_CLOSE,
	SEMICOLON,
	OPERATOR,
	STRING,
	NUMBER,
	PATH
};

// TOKENS FOR LEXAR
struct token {
	std::string::iterator		itStart;
	std::string::iterator		itEnd;
	std::string					value;
	tokenType					type;
};

class ConfigParser {
	private:
		std::string													_filepath;
		std::string													_input;
		std::string													_inputMime;
		std::vector<Config>											_configs;
		std::vector<token>											_tokens;
		std::vector<token>											_tokensMime;
		std::unordered_map<std::string, std::vector<std::string>>	_mimeTypes;

		// PARSING
		token getNextToken(token &lastToken, const std::regex &path, const std::regex &op);
		// UTILS
		void	moveOneTokenSafly(std::vector<token> &tokens, std::vector<token>::iterator &it);
		// EXCEPTIONS
		void	errorToken(token token, std::string msg);
		void	getTokenPos(token token, int &line, int &col);

	public:
		ConfigParser() = delete;
		explicit ConfigParser(const std::string& filepath);
		~ConfigParser();
		ConfigParser(const ConfigParser &toCopy);
		ConfigParser& operator=(const ConfigParser &other);

		// PARSING
		void	readConfigToInput();
		void	parseInputToTokens();
		void	parseTokenToConfig();
		void	parseTokenToServer(std::vector<token>::iterator &it);
		void	parseTokenToDirective(std::vector<token>::iterator &it, Config &newServer);
		void	parseTokenToLocation(std::vector<token>::iterator &it, Config &newServer);
		void	parseTokenToLocDir(std::vector<token>::iterator &it, Location &loc);

		void	checkConfig(Config &config);
		void	setServerName(Config &config);

		void	readMimeToInput(const std::string &filepath);
		void	parseMimeToTokens();
		token	getNextMimeToken(token &lastToken);
		std::unordered_map<std::string, std::vector<std::string>>	parseMimeToken();

		void	checkPort();
		void	checkHost();
		void	checkServerName();		//
		void	checkClientBodySize();	// https://nginx.org/en/docs/syntax.html

		// GETTERS
		std::vector<Config> getConfigs() const { return this->_configs; }


		// UTILS
		void	printInput();
		void	eraseToken(std::vector<token> &tokens, enum tokenType type);
		void	printTokens(std::vector<token> &tokens);
		void	printMimeTypes();

		// EXCEPTIONS
		class ConfigParserException : public std::exception {
			private:
				std::string message;
			public:
				ConfigParserException(const std::string &msg) : message(msg) {}
				virtual const char* what() const throw() {
					return message.c_str();
				}
		};
};
