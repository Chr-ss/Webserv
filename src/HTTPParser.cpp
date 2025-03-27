# include "../inc/HTTPParser.hpp"

HTTPParser::HTTPParser(void) : \
	content_length_(0), chunked_(false) {
	result_.status_code = 200;
}

HTTPParser::~HTTPParser(void) { }

bool	HTTPParser::isHeadersParsed(void) {
	if (rawRequest_.find("\r\n\r\n") == std::string::npos)
		return (false);
	return (true);
}

/// @brief splits the header and the (first part of) the body;
void	HTTPParser::splitHeaderBody(void) {
	size_t header_end;
	
	header_end = rawRequest_.find("\r\n\r\n");
	header_ = rawRequest_.substr(0, header_end);
	if (header_end != rawRequest_.size())
		result_.body = rawRequest_.substr(header_end + 4, rawRequest_.size() - (header_end + 4));
}

/// @brief if available, checks if content length is same as size body
void	HTTPParser::verifyBodyCompletion(e_state &STATE) {
	if (STATE != RECEIVEHEADER && STATE != RECEIVEBODY)
		return ;
	if (content_length_ > 0)
	{
		if (result_.body.size() == content_length_)
			STATE = PARSING;
	}
	else
		STATE = PARSING;
}

/**
 * @brief set content_length_ if possible
 * @param str string with line that contains Content-Lenght
 * @return bool if invalid content length input
 */
bool	HTTPParser::tryParseContentLength(std::string str) {
	std::string	str_nr;
	size_t		cl_pos;
	size_t		start;
	size_t		end;
	
	cl_pos = str.find("Content-Length:");
	if (cl_pos == std::string::npos)
		return (true);
	str_nr = str.substr(cl_pos + 15);
	start = str_nr.find_first_not_of(" \n\r\t");
	end = str_nr.find_last_not_of(" \n\r\t");
	if (start == std::string::npos || start == end)
		return (true);
	try
	{
		content_length_ = std::stol(str_nr.substr(start, end - start + 1));
		return (false);
	}
	catch (const std::exception& e)
	{
		std::cerr << "No valid Content-Lenght: " << e.what() << std::endl;
		return (true);
	}
	return (true);
} 

/**
 * @brief header needs a method, target or protocol
 * @return bool is one or more of these are present
 */
bool	HTTPParser::isValidHeader(HTTPClient *client) {
	if (!validWithConfig(client))
		return (false);
	if (result_.method.empty() || \
		result_.request_target.empty() || \
		result_.protocol.empty())
		return (true);
	return (false);
}

/**
 * @brief extracts HTTP protocol, request target and if available query string
 * @param str line with HTTP in it
 * @return bool if header is invalid
 */
bool	HTTPParser::parseHTTPline(const std::string &str) {
	size_t	split;
	size_t	second_space;
	size_t	query_index;

	split = str.find(" ");
	if (split == std::string::npos)
		return (true);
	result_.method = str.substr(0, split);
	second_space = str.find(" ", split + 1);
	if (second_space == std::string::npos)
		return (true);
	result_.request_target = str.substr(split + 1, second_space - split - 1);
	query_index = result_.request_target.find("?");
	if (query_index != std::string::npos)
	{
		result_.headers["QUERY_STRING"] = result_.request_target.substr(query_index + 1);
		result_.request_target = result_.request_target.substr(0, query_index);
	}
	else
		result_.headers["QUERY_STRING"] = "";
	result_.protocol = str.substr(second_space + 1);
	return (false);
}

/**
 * @brief removes spaces in begin and end from value
 * @param str raw value string
 * @return trimmed string
 */
static std::string trimString(const std::string &str) {
	size_t	start;
	size_t	end;
	
	if (str.empty()) 
		return "";
	start = str.find_first_not_of(' ');
	end = str.find_last_not_of(' ');
	if (start == std::string::npos || start == end)
		return ("");
	return (str.substr(start, end - start + 1));
}

/**
 * @brief extract key-value from header,in right format in unordmap (CAPITAL_KEY=trimmedString)
 * @param str line from header
 */
void	HTTPParser::parseExtraHeaderInformation(const std::string &str) {
	size_t		split;
	std::string	raw_key;
	std::string key("");

	split = str.find(":");
	if (split == std::string::npos || split == str.size())
		return ;
	raw_key = str.substr(0, split);
	for (size_t i = 0; i < raw_key.size(); i++)
	{
		if (raw_key[i] == '-')
			key += '_';
		else if (std::isalpha(raw_key[i]))
			key += static_cast<char>(std::toupper(raw_key[i]));
		else
			key += raw_key[i];
	}
	result_.headers[key] = trimString(str.substr(split + 1));
}

static std::vector<std::string>	getHost(std::string &host_string) {
	std::vector<std::string>	host;
	size_t						split;
	std::string					hostname;
	std::string					port;

	if (host_string.find(":") != std::string::npos)
		host_string.erase(0, 5);

	split = host_string.find(":");
	if (split != std::string::npos)
	{
		hostname = host_string.substr(0, split);
		port = host_string.substr(split + 1);
		host.push_back(hostname);
		host.push_back(port);
	}
	else
		host.push_back(host_string);
	return (host);
}

/**
 * @brief extract all info from header and validate
 * @return bool for invalidRequest
 */
bool	HTTPParser::parseRequest(void) {
	std::istringstream 	is(header_);
	std::string			str;

	while (std::getline(is, str))
	{	
		if (str.find("Host:") != std::string::npos)
			result_.host = getHost(str);
		else if (result_.method.empty() && str.find("HTTP/") != std::string::npos)
		{
			if (parseHTTPline(str))
				return (true);
		}
		else if (str.find("Transfer-Encoding:") != std::string::npos && \
				str.find("chunked") != std::string::npos)
			chunked_ = true;
		else if (str.find("Content-Length:") != std::string::npos)
		{
			if (tryParseContentLength(str))
				return (true);
		}
		else
			parseExtraHeaderInformation(str);
	}
	return (false);	
}

/**
 * @brief saves chunks and checks when chunksize == 0 (ready);
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::addIfProcessIsChunked(const std::string &buff, e_state &STATE) {
	std::string	raw_body(result_.body + buff);
	std::string	chunk_size_str;
	size_t		found;
	size_t		chunk_size;
	size_t		pos(0);

	while ((found = raw_body.find("\r\n", pos)) != std::string::npos)
	{
		chunk_size_str = raw_body.substr(pos, found - pos);
		chunk_size = std::stoi(chunk_size_str, nullptr, 16);
		
		if (chunk_size == 0)
		{
			STATE = PARSING;
			return ;
		}
		pos = found + 2;
		if (pos + chunk_size <= raw_body.size()) {
			result_.body += raw_body.substr(pos, chunk_size);
			pos += chunk_size + 2;
		}
	}
}

bool	isBiggerMaxBodyLength(size_t content_length, uint64_t max_size)
{
	if (content_length > max_size)
		return (true);
	else
		return (false);
}

bool	HTTPParser::validWithConfig(HTTPClient *client) {
	Config	*config = &client->getConfig();
	size_t	length;

	if (!client->isConfigSet())
		return (false); // WHat now?
	// MAX CLIENT CHECK
	if (content_length_ != 0)
		length = content_length_;
	else
		length = result_.body.size();
	if (isBiggerMaxBodyLength(length,  config->getClientBodySize(result_.request_target)))
	{
		// TO BIG
		result_.status_code = 413;
		return (true);
	}
	// METHOD CHECK
	std::vector<std::string>	allowed_methods = config->getMothods(result_.request_target);
	auto it = std::find(allowed_methods.begin(), allowed_methods.end(), result_.method);
	if (it == allowed_methods.end()) // not found
	{
		// WRONG METHOD
		result_.status_code = 405;
		return (true);
	}
	return (false);
}

static void	generatePath(std::string &endpoint, Config *config) {
	std::string	folder_file;
	struct stat	statbuf;

	for(const auto &pair : config->getLocations())
	{
		if (pair.first[pair.first.size() - 1] == '/')
			folder_file = pair.first + endpoint;
		else
			folder_file = pair.first + "/" + endpoint;
		if (stat(folder_file.c_str(), &statbuf) == 0)
		{
			endpoint = folder_file;
			return ;
		}
	}
}

bool	checkBodySizeLimit(size_t body_size, Config *conf, std::string path) {
	if (body_size > conf->getClientBodySize(path))
		return (true);
	else
		return (false);
}

/**
 * @brief process readbuffer by state;
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::addBufferToParser(std::string &buff, HTTPClient *client, e_state &STATE) {
	if (STATE == RECEIVEHEADER)
	{
		rawRequest_ += buff;
		buff = "";
		if (isHeadersParsed())
		{
			splitHeaderBody();
			result_.invalidRequest = parseRequest();

			// SET SERVER
			client->setServer(result_.host);

			// generate PATH
			generatePath(result_.request_target, &client->getConfig());

			// Validate with config and results from parsing
			if (!result_.invalidRequest)
				result_.invalidRequest = isValidHeader(client);
			if (result_.invalidRequest == true)
				STATE = PARSING;
			else
				STATE = RECEIVEBODY;
		}
		else
			return ;
	}
	if (STATE == RECEIVEBODY)
	{
		if (chunked_)
			addIfProcessIsChunked(buff, STATE);
		else
			result_.body += buff;
	}
	if (checkBodySizeLimit(result_.body.size(), &client->getConfig(), result_.request_target))
	{
		result_.status_code = 413;
		STATE = RESPONSE;
	}
	verifyBodyCompletion(STATE);
}

void	HTTPParser::clearParser(void) {
	header_.clear();
	rawRequest_.clear();
	content_length_ = 0;
	chunked_ = false;
}

const HTTPRequest HTTPParser::getParsedRequest(void) { return (result_); }

std::string HTTPParser::getRawData(void) { return (rawRequest_); }
