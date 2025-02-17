# include "Headers.hpp"
# include "HTTPResponse.hpp"
# include "CGI.hpp"

HTTPResponse::HTTPResponse(void) {
	m_header = "";
	m_body = "";
	m_content_type = "application/octet-stream";
	m_status_code = 200;
}

HTTPResponse::~HTTPResponse() { }

void	HTTPResponse::loadMimeTypes(const std::string& filename) {
	std::ifstream file(filename);
	if (!file)
		throw std::runtime_error("Failed to open file: " + filename);

	std::string	line;
	while (std::getline(file, line))
	{
		line.erase(0, line.find_first_not_of(" \t"));
		line.erase(line.find_last_not_of(" \t") + 1);

		if (line.empty() || line[0] == '#')
            continue;

		size_t delimiter = line.find('=');
		if (delimiter == std::string::npos && delimiter == line.length() - 1)
			continue ;
		std::string extension = line.substr(0, delimiter);
		std::string mimeType = line.substr(delimiter + 1);

		extension.erase(0, extension.find_first_not_of(" \t"));
        extension.erase(extension.find_last_not_of(" \t") + 1);
        mimeType.erase(0, mimeType.find_first_not_of(" \t"));
        mimeType.erase(mimeType.find_last_not_of(" \t") + 1);
		m_mimetypes[extension] = mimeType;
	}
}

void	HTTPResponse::getBody(void) {
	if (!m_filename.empty())
	{
		std::ifstream 	myfile(m_filename, std::ios::binary);
		std::ostringstream	buffer;

		buffer.clear();
		if (!myfile)
			throw std::runtime_error("Failed to open file: " + m_filename);
		buffer << myfile.rdbuf();
		m_body = buffer.str();
	}
	else
		m_body = "";
}

void	HTTPResponse::createHeader(void) {
	m_header = "HTTP/1.1 " + m_httpStatusMessages + "\r\n" \
			+ "Content-Type: " + m_content_type + "\r\n" \
			+ "Content-Length: " + std::to_string(m_body.size()) + "\r\n\r\n"; // what if chunked?
}

void	HTTPResponse::getContentType(void)
{
	size_t index = m_filename.find_last_of('.');
	if (index != std::string::npos)
	{
		std::string	extension = m_filename.substr(index);
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		auto it = m_mimetypes.find(extension);
		if (it != m_mimetypes.end())
			m_content_type = it->second;
	}
}

void	HTTPResponse::getHttpStatusMessages(void) {
	static const std::unordered_map<int, std::string> httpStatusMessages = {
		{100, "100 Continue"}, {101, "101 Switching Protocols"}, {102, "102 Processing"},
		{200, "200 OK"}, {201, "201 Created"}, {202, "202 Accepted"}, {204, "204 No Content"},
		{301, "301 Moved Permanently"}, {302, "302 Found"}, {304, "304 Not Modified"},
		{307, "307 Temporary Redirect"}, {308, "308 Permanent Redirect"},
		{400, "400 Bad Request"}, {401, "401 Unauthorized"}, {403, "403 Forbidden"},
		{404, "404 Not Found"}, {405, "405 Method Not Allowed"}, {408, "408 Request Timeout"},
		{409, "409 Conflict"}, {410, "410 Gone"}, {429, "429 Too Many Requests"},
		{500, "500 Internal Server Error"}, {501, "501 Not Implemented"}, {502, "502 Bad Gateway"},
		{503, "503 Service Unavailable"}, {504, "504 Gateway Timeout"}, {505, "505 HTTP Version Not Supported"}
	};
	auto it = httpStatusMessages.find(m_status_code);
	if (it != httpStatusMessages.end())
		m_httpStatusMessages = it->second;
	else
		m_httpStatusMessages = "500 Internal Server Error";
	if (m_status_code != 200 && m_status_code != 404 && !m_body.empty())
	{
		size_t index = m_body.find("<h1>Error 404: Not found</h1>");
		if (index != m_body.size())
			m_body.replace(index, 30, "Error " + m_httpStatusMessages);
	}
}

void	HTTPResponse::generateResponse( const HTTPRequest &request) {
	std::string filename = request.request_target;
	if (request.invalidRequest)
	{
		m_status_code = 400;
		filename = "custom_404.html"; // change
	}
	if (!CGIProcessor::is_cgi_script(m_filename))
	{
		m_filename = resolve_path(filename);
		if (m_filename.empty())
			m_filename = resolve_path("custom_404.html");
		// if (m_filename.empty())
		// 	return ("There went something wrong..."); // TO DO
	}
	else
	{
		// Not implemented
		// TODO redo CGIProcessor
		// CGIProcessor cgip = CGIProcessor(m_filename, m_env);
		// cgip.send_buffer(m_clients[n].get_socket_buffer());
		// cgip.send_eof();
	}
}

std::string	HTTPResponse::loadResponse(void) {
	getBody();
	getContentType();
	getHttpStatusMessages();
	createHeader();
	return (m_header + m_body);
}

