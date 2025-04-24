#include "../../inc/CGI/CGI.hpp"
#include "../../inc/HTTP/HTTPClient.hpp"

static bool	isNPH(const std::string& path) {
	size_t		index;
	std::string	filename = "";

	index = path.find_last_of('/');
	if (index != std::string::npos)
		filename = path.substr(index + 1);
	else
		filename = path;
	if (filename.size() > 4 && filename.substr(0, 4) == "nph_")
		return (true);
	else
		return (false);
}

CGI::CGI(const HTTPRequest &request,
		 CGIPipes pipes,
		 std::function<void(int)> delFromEpoll_cb)
	:
	timeout_(false),
	finished_(false),
	pipes_(pipes),
	CGI_STATE_(START_CGI),
	delFromEpoll_cb_(delFromEpoll_cb),
	request_(request)
{
	scriptPath_ = request_.request_target;
	if (request_.method == "DELETE") {
		scriptPath_ = "data/www/cgi-bin/nph_CGI_delete.py";
	}
	nph_ = isNPH(scriptPath_);
	send_data_ = "";
}

CGI::~CGI(void) {}

// ###############################################################
// ####################### IS BOOL ###############################

bool	CGI::isDone(void) { return (CGI_STATE_ == CGI_DONE); }

/**
 * @brief static function that checks if executable is allowed and valid
 * @param path string with path and filename
 * @return bool if cgi script is valid
 */
bool CGI::isCGIScript(const std::string &path)
{
	std::string executable = getScriptExecutable(path);
	return (!executable.empty());
}

/**
 * @brief static function that checks if request requires CGI
 * @param request to be checked
 * @return bool true if cgi required
 */
bool	CGI::isCGI(const HTTPRequest& request) {
	if (request.invalidRequest)
		return (false);

	if (CGI::isCGIScript(request.request_target) || request.method == "DELETE")
		return (true);
	return (false);
}

// ###############################################################
// ###################### HANDLE CGI #############################

void	CGI::handle(const SharedFd &fd, uint32_t events) {
	if (CGI_STATE_ != START_CGI && CGI_STATE_ != CGI_DONE && timedOut()) {
		handleTimeOut();
		CGI_STATE_ = HANDLE_RSPNS_CGI;
	}
	switch (CGI_STATE_) {
		case START_CGI:
			execCGI();
			return ;
		case SEND_TO_CGI:
			sendDataToCGI(fd, events);
			return ;
		case RCV_FROM_CGI:
			getResponseFromCGI(fd, events);
			[[fallthrough]];
		case HANDLE_RSPNS_CGI:
			handleCGIResponse();
			return;
		default:
			return ;
	}
}

void	CGI::handleCGIResponse() {
	if (!isCGIProcessFinished())
		return ;

	if (!isCGIProcessSuccessful()) {
		status_ = 500;
	} else {
		status_ = getStatusCodeFromResponse();
		if (!isNPHscript()) {
			rewriteResponseFromCGI();
		}
	}
	CGI_STATE_ = CGI_DONE;
}

void	CGI::handleTimeOut() {
	std::cerr << "TIMEOUT, shutting down CGI...\n";
	timeout_ = true;
	status_ = 500;
	finished_ = true;
	if (!isCGIProcessFinished()) {
		if (kill(pid_, SIGKILL) == -1) // TODO: maybe use sigterm instead
			throw std::runtime_error("kill() " + std::to_string(pid_) + " : " + strerror(errno));
	}
}

// ###############################################################
// ######################## GETTERS ##############################

std::string CGI::getResponse(void) { return (response_); }

int	CGI::getStatusCode( void ) { return (status_); };

/**
 * @brief static function that compares executable extension with the allowed cgi scripts
 * @param path string with path and filename
 * @return string with path to executable program or empty when not correct
 */
std::string CGI::getScriptExecutable(const std::string &path)
{
	if (path.size() >= 3 && path.substr(path.size() - 3) == ".py")
		return "/usr/bin/python3";
	if (path.size() >= 3 && path.substr(path.size() - 3) == ".rb")
		return "/usr/bin/ruby";
	// if (path.size() >= 4 && path.substr(path.size() - 4) == ".php")
	// 	return "/usr/bin/php";
	return "";
}
