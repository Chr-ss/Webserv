#pragma once

# include <string>
# include <unordered_map>
# include <regex>
# include <vector>
# include <string>
# include <iostream>
# include <sstream>
# include <functional>
# include <filesystem>

# include <sys/stat.h>

# include "HTTPRequest.hpp"

enum e_parse_state {
	RCV_HEADER,
	RCV_BODY,
	DONE_PARSING
};

class HTTPClient;
class Config;

class HTTPParser
{
	public:
		HTTPParser( void );
		~HTTPParser( void );

		void				clearParser( void );
		void				processData( std::string &buff, HTTPClient *client );
		const HTTPRequest	getParsedRequest( void );

		inline bool isDone() {
			return (PARSE_STATE_ == DONE_PARSING);
		}

	private:
		HTTPRequest		result_;
		std::string		rawRequest_;
		std::string		header_;
		size_t			content_length_;
		bool			chunked_;
		e_parse_state	PARSE_STATE_;
		
		bool	isHeaderRead( void );
		bool	isBodyComplete( void );

		void	splitHeaderBody( void );
		void	addIfProcessIsChunked( const std::string &buff );
		bool	validWithConfig( HTTPClient *client );
		bool	checkBodySizeLimit( size_t body_size, const Config *config, std::string path );
		bool	isRedirection(std::string &endpoint, const std::vector<std::string> &redir);
		std::string	handleRootDir( const Config *config );
		std::string	generatePath(const Config *config);
		std::string searchThroughIndices(std::vector<std::string> indices, bool autoindex);

		// Parsing header
		bool	tryParseContentLength( std::string str );
		void	parseExtraHeaderInformation( const std::string &str );
		bool	parseHTTPline( const std::string &str );
		bool	parseRequest( void );
		bool	isValidHeader( HTTPClient *client );
};

# include "HTTPClient.hpp"
