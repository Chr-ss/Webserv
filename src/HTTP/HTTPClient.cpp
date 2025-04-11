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
	remaining_write_ = "";
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

void	HTTPClient::writeTo(const SharedFd &fd) {
	ssize_t		bytes_write;
	std::string	response = "";

	if (!remaining_write_.empty())
	{
		response = remaining_write_;
		remaining_write_.clear();
	}
	else if (!message_que_.empty())
	{
		response = message_que_.front();
		message_que_.erase(message_que_.begin());
	}
	if (!response.empty())
	{
		bytes_write = write(fd.get(), response.c_str(), response.size());
		if (bytes_write == -1)
			throw std::runtime_error("write(): " + std::string(strerror(errno)));
		else
		{
			remaining_write_ = response.substr(bytes_write);
			return ;
		}
	}
	STATE_ = DONE;
}

std::string	HTTPClient::readFrom(int fd) {
	char	buff[READSIZE + 1] = { '\0'};
	int		bytes_read;

	bytes_read = read(fd, buff, READSIZE);
	if (bytes_read == -1)
		throw std::runtime_error(std::string("read(): ") + strerror(errno));
	return (std::string(buff, bytes_read));
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
	if (remaining_write_.empty())
	{
		if (cgi_used)
			cgiResponse();
		else
		{
			responseGenerator_.generateResponse(request_);
			message_que_.push_back(responseGenerator_.loadResponse());
		}
	}
	writeTo(fd);
	STATE_ = DONE;
}

/// @brief creates a CGI class, checks the method/target, starts the cgi
bool	HTTPClient::cgi(const SharedFd &fd) {
	std::vector<std::string>	env_strings;

	if (cgi_ == NULL)
	{
		CGIPipes pipes;
		pipes.setCallbackFunctions(clientSock_, addToEpoll_cb_, delFromEpoll_cb_);
		pipes.addNewPipes();
		cgi_ = std::make_unique<CGI>(request_.body, pipes, delFromEpoll_cb_);
	}
	cgi_->handle_cgi(request_, fd);
	return (cgi_->isReady());
}

/// @brief checks if cgi header has to be rewritten and add response to que.
void	HTTPClient::cgiResponse(void) {
	if (cgi_->isTimeout())
	{
		HTTPRequest	timeout_request;
		std::memset(&timeout_request, 0, sizeof(timeout_request));
		timeout_request.request_target = "timeout";
		timeout_request.status_code = 408;
		responseGenerator_.generateResponse(timeout_request);
		message_que_.push_back(responseGenerator_.loadResponse());

	}
	else
	{	if (!cgi_->isNPHscript(request_.request_target))
			cgi_->rewriteResonseFromCGI();
		message_que_.push_back(cgi_->getResponse());
	}
	std::cerr << "-------------------\nResponse: " << message_que_.back() \
	<< "\n-----------------" << std::endl;
	cgi_.reset();
}

const Config*	HTTPClient::getConfig( void ) const { return (config_); }
