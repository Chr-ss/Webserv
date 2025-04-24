
# include "../../inc/HTTP/HTTPResponse.hpp"

// #######################     PUBLIC     ########################
// ###############################################################

HTTPResponse::HTTPResponse(void) {
	header_ = "";
	body_ = "";
	content_type_ = "application/octet-stream";
	status_code_ = 200;
	response_ = "";
	dir_list_ = false;
	config_ = NULL;
}

HTTPResponse::~HTTPResponse(void) { }

void	HTTPResponse::setConfig(const std::shared_ptr<Config> config) { config_ = config; }

static bool	isRedirectStatusCode(int status_code) { return (status_code >= 300 && status_code <= 308); }

/**
 * @brief generates Response by processing the request Header
 * @param request struct that contains all the parsed header info
 */
void	HTTPResponse::generateResponse(HTTPRequest request) {
	std::string	filename(request.request_target);

	if (request.dir_list)
	{
		// std::cerr << "filename: " << filename << std::endl;
		// std::cerr << "autoindex on/off: " << config_->getAutoindex(filename) << std::endl;
		if (!config_->getAutoindex(filename))
			request.status_code = 404;
		else
			dir_list_ = true ;
		// std::cerr << dir_list_ << "dirlist is on " << request.status_code << std::endl;
	}

	if (!request.invalidRequest && request.status_code == 200)
		filename_ = request.request_target;
	else if (isRedirectStatusCode(request.status_code))
	{
		header_ = request.body;
		// std::cerr << "header: " << header_ << std::endl;
	}
	else if (isCustomErrorPages(filename, request.status_code))
	{
		filename_ = searchErrorPage(request.subdir, filename);
		// std::cerr << "is customerrorpage" << std::endl;
	}
	status_code_ = request.status_code;
}

/**
 * @brief insert "key: value\r\n" after first linebreak or first line
 */
void	HTTPResponse::insertHeader(const std::string& key, const std::string& value, std::string& response) {
	size_t	insertPos;
	size_t	firstLineBreak = response.find("\r\n");
	if (firstLineBreak != std::string::npos) {
		insertPos = firstLineBreak + 2;
	} else {
		response += "\r\n";
		insertPos = response.size();
	}

	std::string headerLine = key + ": " + value + "\r\n";
	response.insert(insertPos, headerLine);
}

std::string	HTTPResponse::loadResponse(void) {
	if (isRedirectStatusCode(status_code_))
		return (header_);
	getBody();
	getContentType();
	getHttpStatusMessages();
	createHeader();
	// std::cerr << "Response: \n" << header_ << "body size: " << body_.size() << "\n" << std::endl;
	return (header_ + body_);
}

// ##############     PRIVATE GENERATE RESPONSE     ##############
// ###############################################################

/**
 * @brief search for the fullpath for the custom errorpage
 * @param dir vector<string> with all the subdirectories
 * @param errorpage string with the filename of the errorpage
 * @return fullpath if found, else empty
 */
std::string	HTTPResponse::searchErrorPage(const std::vector<std::string> &dir, const std::string &errorpage) {
	struct stat	statbuf;

	for (const std::string &path : dir)
	{
		std::string fullpath = path + errorpage;
		// std::cerr << "path: " << path << " page: " << errorpage << std::endl;
		if (stat(fullpath.c_str(), &statbuf) == 0)
		{
			if (access(fullpath.c_str(), F_OK) != -1)
			{
				if (access(fullpath.c_str(), R_OK) == -1)
				{
					// std::cerr << "No permission" << std::endl;
					status_code_ = 403;
					return "";
				}
				// std::cerr << "fullpath: " << fullpath << std::endl;
				return (fullpath);
			}
			break ;
		}
	}
	status_code_ = 404;
	return ("");
}

bool	HTTPResponse::isCustomErrorPages(std::string &filename, int status_code) {
	std::string error_page = config_->getErrorPage(status_code);
	if (error_page.empty())
		return (false);

	// std::cerr << "status code: " << status_code << " filename: " << error_page << std::endl;
	filename = error_page;
	status_code_ = status_code;
	return (true);
}

// ################     PRIVATE LOAD RESPONSE     ################
// ###############################################################

/**
 * @brief sums up all the directives from the given directory
 * @param path const char * with dir name
 * @return string with all the directories, seperated with a '\n'
 */
static std::string	getAllDirNames(const char *path) {
	struct dirent 	*d;
	std::string		list;
	DIR 			*dir = opendir(path);

	if (!dir)
		return ("");
	list = "<!DOCTYPE html><head><title>" + std::string(path) + \
		"Folder:</title></head><body><p>" \
		+ std::string(path) + "</p><br><ul>";
	while (true)
	{
		d = readdir(dir);
		if (d != nullptr)
			list += "<li>" + std::string(d->d_name) + "</li>";
		else
			break ;
	}
	closedir(dir);
	list += "</ul></body></html>";
	return (list);
}

/// @brief get body from request target
void	HTTPResponse::getBody(void) {
	if (dir_list_ == true)
		body_ = getAllDirNames(filename_.c_str());
	else if (!filename_.empty())
	{
		std::ifstream 		myfile(filename_, std::ios::binary);
		std::ostringstream	buffer;

		buffer.clear();
		if (!myfile)
			throw std::runtime_error("Failed to open file: " + filename_);
		body_.assign((std::istreambuf_iterator<char>(myfile)), std::istreambuf_iterator<char>());
	}
	else
		body_ = "";
}

/// @brief search through mimetypes to set the right content type based on the extension
void	HTTPResponse::getContentType( void )
{
	std::string	extension;
	size_t		index;

	if (dir_list_)
	{
		content_type_ = "text/html";
		// std::cerr << "dir list on\n";
		return ;
	}
	index = filename_.find_last_of('.');
	if (index == std::string::npos)
		return ;

	extension = filename_.substr(index + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
	for (const auto &type : config_->getMimeTypes())
	{
		for(const std::string &mimetype : type.second)
		{
			if (mimetype == extension)
			{
				content_type_ = type.first;
				return ;
			}
		}
	}
}

/// @brief checks the status and get the right httpstatus message to add it to the default error page
void	HTTPResponse::getHttpStatusMessages(void) {
	static const std::unordered_map<int, std::string> httpStatusMessages = {
		{100, "100 Continue"}, {101, "101 Switching Protocols"}, {102, "102 Processing"},
		{200, "200 OK"}, {201, "201 Created"}, {202, "202 Accepted"}, {204, "204 No Content"},
		{301, "301 Moved Permanently"}, {302, "302 Found"}, {304, "304 Not Modified"},
		{307, "307 Temporary Redirect"}, {308, "308 Permanent Redirect"},
		{400, "400 Bad Request"}, {401, "401 Unauthorized"}, {403, "403 Forbidden"},
		{404, "404 Not Found"}, {405, "405 Method Not Allowed"}, {408, "408 Request Timeout"},
		{409, "409 Conflict"}, {410, "410 Gone"}, {413 ,"413 Payload Too Large"}, {415, "415 Unsupported Media Type"},
		{429, "429 Too Many Requests"}, {500, "500 Internal Server Error"},
		{501, "501 Not Implemented"}, {502, "502 Bad Gateway"}, {503, "503 Service Unavailable"},
		{504, "504 Gateway Timeout"}, {505, "505 HTTP Version Not Supported"}
	};
	auto it = httpStatusMessages.find(status_code_);
	if (it != httpStatusMessages.end())
		httpStatusMessages_ = it->second;
	else
		httpStatusMessages_ = "500 Internal Server Error";

	if (status_code_ != 200 && status_code_ != 404 && !body_.empty()) // Do we want this?
	{
		size_t index = body_.find("Error 404 Not found");
		if (index != std::string::npos)
			body_.replace(index, 19, "Error " + httpStatusMessages_);
		content_type_ = "text/html";
	}
}

/// @brief combines all parts of HTTP header and adds right values
void	HTTPResponse::createHeader(void) {
	header_ = "HTTP/1.1 " + httpStatusMessages_ + "\r\n"
			+ "Content-Type: " + content_type_ + "\r\n";
	if (!body_.empty())
		header_ += "Content-Length: " + std::to_string(body_.size()); // what if chunked?
	header_ += "\r\n\r\n";
}
