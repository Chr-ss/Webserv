#pragma once

# include "Headers.hpp"

class HTTPParser {
	private:
		enum State {
			COLLECT_HEADER,
			COLLECT_BODY,
			DONE
		};

		State m_current_state;
		std::string m_rawRequest;
		std::string	m_header;
		std::string	m_body;
		std::string m_method;
		std::string m_request_target;
		std::string m_protocol;
		std::unordered_map<std::string, std::string>  m_request;
		std::vector<size_t> m_content_length;
		bool m_invalidRequest;
		bool m_chunked;

		bool isHeadersParsed( void );
		void splitHeaderBody( void );
		void checkIfBodyComplete( void );
		void processChunked( std::string &buff );
		bool extractContentLength( std::string str );
		bool parseRequest( void );

	public:
		HTTPParser( void );
		~HTTPParser( void );

		void		clearParser( void );
		void		expandWithBuffer( std::string &buff );
		bool		isBadRequest( void );
		bool		isRequestFullyParsed( void );
		HTTPRequest	getParsedRequest( void );
};