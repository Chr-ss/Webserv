#pragma once

# include "Headers.hpp"
# include "CGI.hpp"

class HTTPResponse {
	private:
		std::unordered_map<std::string, std::string> m_mimetypes;
		std::string m_filename;
		std::string m_header;
		std::string m_body;
		std::string m_httpStatusMessages;
		std::string m_content_type;
		int			m_status_code;
	public:
		HTTPResponse( void );
		~HTTPResponse( void );

		void		loadMimeTypes( const std::string& filename );
		void		getBody( void );
		void		createHeader( void );
		void		getContentType( void );
		void		getHttpStatusMessages( void );
		void		generateResponse( const HTTPRequest &request);
		std::string	loadResponse( void );

};
