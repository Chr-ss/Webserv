#include "../../inc/HTTP/HTTPClient.hpp"

HTTPClient::HTTPClient(
	SharedFd clientFd, 
	SharedFd serverFd, 
	std::function<void(struct epoll_event, const SharedFd&)>  addToEpoll_cb,
	std::function<const Config* (const SharedFd&, const std::string&)> getConfig_cb,
	std::function<void(const SharedFd&)> delFromEpoll_cb
	) : clientSock_(clientFd), \
		serverSock_(serverFd), \
		responseGenerator_(), \
		addToEpoll_cb_(addToEpoll_cb), \
		getConfig_cb_(getConfig_cb), \
		delFromEpoll_cb_(delFromEpoll_cb) {
	STATE_ = RECEIVING;
	config_ = NULL;
	response_ = "";
}

HTTPClient::HTTPClient(const HTTPClient& other) : \
	STATE_(other.STATE_),
	clientSock_(other.clientSock_), \
	serverSock_(other.serverSock_), \
	responseGenerator_(other.responseGenerator_), \
	config_(other.config_), \
	addToEpoll_cb_(other.addToEpoll_cb_), \
	getConfig_cb_(other.getConfig_cb_), \
	delFromEpoll_cb_(other.delFromEpoll_cb_)
{}

HTTPClient::~HTTPClient(void) { }

bool	HTTPClient::isDone(void) { return (STATE_ == DONE); }

/**
 * @brief get right Config corresponding with servername from the HTTP request
 * @param host vector string with hostname and if set port
 */
void	HTTPClient::setServer(std::vector<std::string> host) {
	config_ = getConfig_cb_(serverSock_, host[0]);
}

/**
 * @brief checks state and processes event. Write or Read action
 * @param event epoll_event of the current event
 */
void	HTTPClient::handle(const epoll_event &event) {
	std::string	data;
	HTTPRequest	request;
	static bool	is_cgi_request = false;

	auto eventData = Epoll::getEventData(event);
	
	switch (STATE_) {
		case RECEIVING:
			data = readFrom(eventData.fd);
			parser_.processData(data, this);
			if (!parser_.isDone())
				return;
			setRequestDataAndConfig();
			is_cgi_request = isCGI();
		case PROCESS_CGI:
			if (is_cgi_request && cgi(eventData.fd) != READY)
				return ;
		case RESPONSE:
			if (eventData.fd != clientSock_.get())
				return ;
			responding(is_cgi_request, eventData.fd);
		case DONE:
			return ;
	}
}

void	printRequest(HTTPRequest request) {
	std::cerr << "------------------\nRequest:\n";
	std::cerr << "Method: " << request.method << "\n";
	std::cerr << "Target: " << request.request_target << "\n";
	std::cerr << "Host: " << request.host[0] << "\n";
	std::cerr << "StatusCode: " << request.status_code << "\n";
	std::cerr <<  "Dir on/off: " << request.dir_list << "\n------------------\n";
}

void	HTTPClient::setRequestDataAndConfig(void) {
	request_ = parser_.getParsedRequest();
	printRequest(request_); // REMOVE
	responseGenerator_.setConfig(config_);
}
bool	HTTPClient::isCGI() {
	if (!responseGenerator_.isCGI(request_))
	{
		STATE_ = RESPONSE;
		return (false);
	}
	STATE_ = PROCESS_CGI;
	return (true);
}

bool	isRedirection(const std::string &response) {
	return (response.rfind("HTTP/1.1 302 Found") == 0);
}

/// @brief regenerates response and add this one to the que.
void	HTTPClient::responding(bool cgi_used, const SharedFd &fd) {
	static bool	send_first_msg = true ;

	if (send_first_msg)
	{
		if (cgi_used)
			cgiResponse();
		else
		{
			responseGenerator_.generateResponse(request_);
			message_que_.push_back(responseGenerator_.loadResponse());
		}
		writeToClient(fd, send_first_msg, cgi_used);
		send_first_msg = false;
	}
	else
		writeToClient(fd, send_first_msg, cgi_used);
	if (STATE_ == DONE)
		send_first_msg = true;
}

/// @brief creates a CGI class, checks the method/target, starts the cgi
bool	HTTPClient::cgi(const SharedFd &fd) {
	std::vector<std::string>	env_strings;

	if (cgi_ == NULL)
	{
		pipes_.setCallbackFunctions(clientSock_, addToEpoll_cb_, delFromEpoll_cb_);
		pipes_.addNewPipes();
		cgi_ = std::make_unique<CGI>(request_.body, pipes_.getPipes(), delFromEpoll_cb_);
	}
	cgi_->handle_cgi(request_, fd);
	return (cgi_->isReady());
}

/// @brief checks if cgi header has to be rewritten and add response to que.
void	HTTPClient::cgiResponse(void) {
	std::cerr << "is cgi response...\n";
	if (cgi_->isTimeout())
	{
		std::cerr << "is cgi timeout...\n";
		HTTPRequest	timeout_request;
		std::memset(&timeout_request, 0, sizeof(timeout_request));
		timeout_request.request_target = "timeout";
		timeout_request.status_code = 500;
		responseGenerator_.generateResponse(timeout_request);
		message_que_.push_back(responseGenerator_.loadResponse());
	}
	else
	{	if (!cgi_->isNPHscript(request_.request_target))
			cgi_->rewriteResonseFromCGI();
		else
			std::cerr << "is cgi nph...\n";
		message_que_.push_back(cgi_->getResponse());
	}
	std::cerr << "-------------------\nResponse: " << message_que_.back() \
	<< "\n-----------------" << std::endl;
	// cgi_.reset(); // NEED?
}

const Config*	HTTPClient::getConfig( void ) const { return (config_); }
