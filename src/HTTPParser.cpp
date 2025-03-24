# include "../inc/HTTPParser.hpp"

HTTPParser::HTTPParser(void) : \
	current_state_(COLLECT_HEADER), content_length_(0), chunked_(false) {
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
void	HTTPParser::verifyBodyCompletion(void) {
	if (current_state_ == DONE)
		return ;
	if (content_length_ > 0)
	{
		if (result_.body.size() == content_length_)
			current_state_ = DONE;
	}
	else
		current_state_ = DONE;
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
bool	HTTPParser::isValidHeader(void) {
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
static std::string trim_string(const std::string &str) {
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
	result_.headers[key] = trim_string(str.substr(split + 1));
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
		if (result_.method.empty() && str.find("HTTP/") != std::string::npos)
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
	return (isValidHeader());	
}

/**
 * @brief saves chunks and checks when chunksize == 0 (ready);
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::addIfProcessIsChunked(const std::string &buff) {
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
			current_state_ = DONE;
			return ;
		}
		pos = found + 2;
		if (pos + chunk_size <= raw_body.size()) {
			result_.body += raw_body.substr(pos, chunk_size);
			pos += chunk_size + 2;
		}
	}
}

/**
 * @brief process readbuffer by state;
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::addBufferToParser(std::string &buff) {
	if (current_state_ == COLLECT_HEADER)
	{
		rawRequest_ += buff;
		buff = "";
		if (isHeadersParsed())
		{
			splitHeaderBody();
			result_.invalidRequest = parseRequest();
			if (result_.invalidRequest == true)
				current_state_ = DONE;
			else
				current_state_ = COLLECT_BODY;
		}
		else
			return ;
	}
	if (current_state_ == COLLECT_BODY)
	{
		if (chunked_)
			addIfProcessIsChunked(buff);
		else
			result_.body += buff;
	}
	verifyBodyCompletion();
}

bool	HTTPParser::isRequestFullyParsed(void)
{
	if (current_state_ == DONE)
		return (true);
	else
		return (false);
}

void	HTTPParser::clearParser(void) {
	current_state_ = COLLECT_HEADER;
	header_.clear();
	rawRequest_.clear();
	content_length_ = 0;
	chunked_ = false;
}

const HTTPRequest HTTPParser::getParsedRequest(void) { return (result_); }

std::string HTTPParser::getRawData(void) { return (rawRequest_);}
