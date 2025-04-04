# include "../../inc/HTTP/HTTPParser.hpp"

HTTPParser::HTTPParser(void) : \
	content_length_(0), chunked_(false) {
	result_.status_code = 200;
	result_.invalidRequest = false;
	result_.body = "";
	result_.method = "";
	result_.request_target = "";
	PARSE_STATE_ = RCV_HEADER;
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
	if (content_length_ > 0)
	{
		if (result_.body.size() == content_length_)
		PARSE_STATE_ = DONE_PARSING;
	}
	else
		PARSE_STATE_ = DONE_PARSING;
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
			PARSE_STATE_ = DONE_PARSING;
			return ;
		}
		pos = found + 2;
		if (pos + chunk_size <= raw_body.size()) {
			result_.body += raw_body.substr(pos, chunk_size);
			pos += chunk_size + 2;
		}
	}
}

bool	HTTPParser::validWithConfig(HTTPClient *client) {
	const Config				*config = client->getConfig();
	std::vector<std::string>	allow_method;

	// METHOD CHECK
	allow_method = config->getMethods(result_.request_target);
	if (std::find(allow_method.begin(), allow_method.end(), result_.method) == allow_method.end())
	{
		result_.status_code = 405;
		return (true);
	}
	return (false);
}

bool	HTTPParser::isRedirection(std::string &endpoint, const std::vector<std::string> &redir) {
	if (redir.size() == 2)
	{
		try
		{
			if (std::all_of(redir[0].begin(), redir[0].end(), ::isdigit))
				result_.status_code = std::stoi(redir[0]);
			else
				return (false);
		}
		catch (...)
		{
			return (false);
		}
		endpoint = redir[1];
		result_.body = \
			"HTTP/1.1 " + redir[0] + " Found\r\nLocation: " + result_.request_target + \
			"\r\nContent-Type: text/html\r\n\r\n";
		PARSE_STATE_ = DONE_PARSING;
		return (true);
	}
	return (false);
}

static std::string	addDir_Folder(std::string path, std::string folder) {
	std::string	full_path = "";
	if (path.back() == '/')
		path.pop_back();
	if (folder.front() != '/')
		full_path = path + '/' + folder;
	else
		full_path = path + folder;
	if (full_path.back() == '/')
		full_path.pop_back();
	return (full_path);
}

static bool	folderExists(const std::string &path, const std::string &dir, std::string &fullpath) {
	struct stat statbuf;

	fullpath = addDir_Folder(path, dir);
	if (stat(fullpath.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode))
		return (true);
	return (false);
}

static void	getSubdirRecursive(const std::string &path, std::vector<std::string> &subdirs) {
	struct dirent 	*d;
	DIR 			*dir;
	std::string		fullpath;

	dir = opendir(path.c_str());
	if (!dir)
		return ;
	while (true)
	{
		d = readdir(dir);
		if (d == nullptr)
			break ;
		else if (d->d_name[0] == '.')
			continue ;
		else if (folderExists(path, std::string(d->d_name), fullpath))
		{
			subdirs.push_back(fullpath);
			getSubdirRecursive(fullpath, subdirs);
		}
	}
	closedir(dir);
}

static bool	isAccesseble(const std::string &path, int &status_code) {
	if (access(path.c_str(), F_OK) != -1)
	{
		if (access(path.c_str(), R_OK) == -1)
		{
			std::cerr << "No permission" << std::endl;
			status_code = 403;
			return (false);
		}
		status_code = 200;
		return (true);
	}
	status_code = 404;
	return (false);
}

void	HTTPParser::handleRootDir(const Config *config) {
	if (config->getAutoindex("/") == false) // AUTOINDEX OFF, so '/' returns 403 
	{
		result_.status_code = 403;
		std::cerr << "auto index off, request / so statuscode 403\n";
		return ;
	}

	const std::vector<std::string> indices = config->getIndex("/");
	result_.dir_list = false;
	if (indices.empty()) // if no indices givens body = list of dir
	{
		result_.dir_list = true;
		std::cerr << "no indices given so printing dir\n";
		return ;
	}

	std::string	folder_file;
	struct stat	statbuf;
	for (const std::string &index : indices)
	{
		for (const std::string &dir : result_.subdir)
		{
			std::string fullpath = addDir_Folder(dir, index);
			if (stat((fullpath).c_str(), &statbuf) == 0)
			{ 
				if (S_ISDIR(statbuf.st_mode))
					result_.dir_list = true;
				else if (isAccesseble(fullpath, result_.status_code))
					result_.request_target = fullpath;
				return ;
			}
		}
	}
	std::cerr << "no good index found in root so 404" << std::endl;
	result_.status_code = 404;
	return ;
}

static const std::vector<std::string>	getSubdirectories(const std::vector<std::string> &roots) {
	std::vector<std::string>	subdir;
	
	for (const std::string &root: roots)
	{
		subdir.push_back(root);
		getSubdirRecursive(root, subdir);
	}
	return (subdir);
}

/**
 * @brief checks for paths (redir, root, access)
 * @param config pointer to Config 
 */
void	HTTPParser::generatePath(const Config *config) {
	struct stat	statbuf;
	std::string	full_path;

	result_.subdir = getSubdirectories(config->getRoot(result_.request_target));
	if (isRedirection(result_.request_target, config->getRedirect(result_.request_target)))
		return ;
	else if (result_.request_target == "/")
		return (handleRootDir(config));
	for (const std::string &folder : result_.subdir)
	{
		full_path = addDir_Folder(folder, result_.request_target);
		if (stat(full_path.c_str(), &statbuf) == 0)
		{
			result_.request_target = full_path;
			return ;
		}
	}
}

static bool	isBiggerMaxBodyLength(size_t content_length, uint64_t max_size) {
	return (content_length > max_size);
}

bool	HTTPParser::checkBodySizeLimit(size_t body_size, const Config *config, std::string path) {
	size_t			length;
	std::uint64_t	max_content_length = config->getClientBodySize(path);

	if (max_content_length != 0)
		length = max_content_length;
	else
		length = body_size;
	if (isBiggerMaxBodyLength(length,  max_content_length))
	{
		result_.status_code = 413;
		return (true);
	}
	if (body_size > max_content_length)
		return (true);
	else
		return (false);
}

/**
 * @brief process readbuffer by state;
 * @param buff std::string with readbuffer;
 */
void	HTTPParser::addBufferToParser(std::string &buff, HTTPClient *client) {
	if (PARSE_STATE_ == RCV_HEADER)
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
			generatePath(client->getConfig());

			// Validate with config and results from parsing
			if (!result_.invalidRequest)
				result_.invalidRequest = isValidHeader(client);
			if (result_.invalidRequest == true)
				PARSE_STATE_ = DONE_PARSING;
			else
				PARSE_STATE_ = RCV_BODY;
		}
		else
			return ;
	}
	if (PARSE_STATE_ == RCV_BODY)
	{
		if (chunked_)
			addIfProcessIsChunked(buff);
		else
			result_.body += buff;
	}
	if (checkBodySizeLimit(result_.body.size(), client->getConfig(), result_.request_target))
	{
		result_.status_code = 413;
		PARSE_STATE_ = DONE_PARSING;
	}
	verifyBodyCompletion();
}

void	HTTPParser::clearParser(void) {
	header_.clear();
	rawRequest_.clear();
	content_length_ = 0;
	chunked_ = false;
}

const HTTPRequest HTTPParser::getParsedRequest(void) { return (result_); }
