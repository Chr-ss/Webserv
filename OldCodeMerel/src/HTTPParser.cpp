# include "../Includes/Headers.hpp"
# include "../Includes/HTTPParser.hpp"

HTTPParser::HTTPParser( void ) : m_current_state(COLLECT_HEADER), \
	m_invalidRequest(false), m_chunked(false) {}

HTTPParser::~HTTPParser( void ) {}

bool	HTTPParser::isHeadersParsed( void ) {
	if (m_rawRequest.find("\r\n\r\n") == std::string::npos)
		return (false);
	return (true);
}

/// @brief splits the header and the (first part of) the body;
void	HTTPParser::splitHeaderBody( void ) {
	const size_t header_end = m_rawRequest.find("\r\n\r\n");

	m_header = m_rawRequest.substr(0, header_end);
	if (header_end != m_rawRequest.size())
		m_body = m_rawRequest.substr(header_end + 4, m_rawRequest.size() - (header_end + 4));
}

/// @brief if available, checks if content length is same as size body
void	HTTPParser::checkIfBodyComplete( void ) {
	if (m_current_state == DONE)
		return ;
	if (m_content_length.size() == 1)
	{
		if (m_body.size() == m_content_length[0])
			m_current_state = DONE;
	}
	else
		m_current_state = DONE;
}

bool	HTTPParser::extractContentLength( std::string str ) {
	size_t	split;
	
	split = str.find("Content-Length:");
	if (split == str.size())
		return (true);
	std::string strNr = str.substr(split + 1);
	size_t start = strNr.find_first_not_of(" \n\r\t");
	size_t end = strNr.find_last_not_of(" \n\r\t");
	if (start >= end)
		return (true);
	int	nr = std::stol(strNr.substr(start, end - start + 1));
	m_content_length.push_back(nr);
	return (false);
} 

bool	HTTPParser::parseRequest( void ) {
	std::istringstream 	is(m_header);
	std::string			str;
	size_t				split;

	while (std::getline(is, str))
	{	
		if (m_method.empty() && str.find("HTTP/") != std::string::npos)
		{
			split = str.find(" ");
			if (split == std::string::npos)
				return (true);
			m_method = str.substr(0, split);
			size_t secondSpace = str.find(" ", split + 1);
			if (secondSpace == std::string::npos)
				return (true);
			m_request_target = str.substr(split + 1, secondSpace - split - 1);
			m_protocol = str.substr(secondSpace + 1);
		}
		else if (str.find("Transfer-Encoding:") != std::string::npos && \
				str.find("chunked") != std::string::npos)
			m_chunked = true;
		else if (str.find("Content-Length:") != std::string::npos)
		{
			if (extractContentLength(str))
				return (true);
		}
		else
		{
			split = str.find(":");
			if (split == std::string::npos || split == str.size())
				return (true);
			m_request[str.substr(0, split)] = str.substr(split + 1);
		}
	}
	if (m_method.empty() || m_request_target.empty() || m_protocol.empty())
		return (true);
	return (false);	
}

/**
 * @brief saves chunks and checks when chunksize == 0 (ready);
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::processChunked( std::string &buff ) {
	std::string	raw_body = m_body + buff;
	std::string	chunk_size_str;
	size_t found;
	size_t chunk_size;
	size_t pos = 0;

	while ((found = raw_body.find("\r\n", pos)) != std::string::npos)
	{
		chunk_size_str = raw_body.substr(pos, found - pos);
		chunk_size = std::stoi(chunk_size_str, nullptr, 16);
		
		if (chunk_size == 0)
		{
			m_current_state = DONE;
			return ;
		}
		pos = found + 2;
		if (pos + chunk_size <= raw_body.size()) {
			m_body += raw_body.substr(pos, chunk_size);
			pos += chunk_size + 2;
		}
	}
}

/**
 * @brief process readbuffer by state;
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::expandWithBuffer( std::string &buff ) {
	if (m_current_state == COLLECT_HEADER)
	{
		m_rawRequest += buff;
		if (isHeadersParsed())
		{
			splitHeaderBody();
			m_invalidRequest = parseRequest();
			m_current_state = COLLECT_BODY;
		}
		else
			return ;
		buff.clear();
	}
	if (m_current_state == COLLECT_BODY)
	{
		if (m_chunked)
			processChunked(buff);
		else
			m_body += buff;
	}
	checkIfBodyComplete();
}

bool	HTTPParser::isBadRequest( void ) {
	return (m_invalidRequest);
}

bool	HTTPParser::isRequestFullyParsed( void )
{
	if (m_current_state == DONE)
		return (true);
	return (false);
}

// TODO change internal implementation to keep the interface
HTTPRequest HTTPParser::getParsedRequest() {
	HTTPRequest result;
	result.method = m_method;
	result.request_target = m_request_target;
	result.protocol = m_protocol;
	result.headers = m_request;
	result.body = m_body;
	return (result);
}

void	HTTPParser::clearParser( void ) {
	m_current_state = COLLECT_HEADER;
	m_header.clear();
	m_body.clear();
	m_rawRequest.clear();
	m_request.clear();
	m_content_length.clear();
	m_invalidRequest = false;
	m_chunked = false;
}