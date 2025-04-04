#include "../../inc/HTTP/HTTPClient.hpp"


HTTPClient::HTTPClient(
	SharedFd clientFd, 
	SharedFd serverFd, 
	std::function<void(struct epoll_event, const SharedFd&)>  addToEpoll_cb,
	std::function<const Config* (const SharedFd&, const std::string&)> getConfig_cb
	) : clientSock_(clientFd), \
		serverSock_(serverFd), \
		responseGenerator_(), \
		addToEpoll_cb_(addToEpoll_cb), \
		getConfig_cb_(getConfig_cb) {
	addToEpoll_cb_ = addToEpoll_cb;
	STATE_ = RECEIVING;
	config_ = NULL;
}

HTTPClient::HTTPClient(const HTTPClient& other) : \
	STATE_(other.STATE_),
	clientSock_(other.clientSock_), \
	serverSock_(other.serverSock_), \
	responseGenerator_(other.responseGenerator_), \
	config_(other.config_), \
	addToEpoll_cb_(other.addToEpoll_cb_), \
	getConfig_cb_(other.getConfig_cb_) 
{}

HTTPClient::~HTTPClient(void) { }

bool	HTTPClient::isDone(void) {
	if (STATE_ == DONE)
		return (true);
	return (false);
}

/**
 * @brief get right Config corresponding with servername from the HTTP request
 * @param host vector string with hostname and if set port
 */
void	HTTPClient::setServer(std::vector<std::string> host) {
	config_ = getConfig_cb_(serverSock_, host[0]);
}

void	HTTPClient::writeTo(int fd) {
	size_t		bytes_write;
	std::string	response;

	if (message_que_.empty())
		return ;
	response = message_que_.front();
	message_que_.erase(message_que_.begin());
	bytes_write = write(fd, response.c_str(), response.size());
	if (bytes_write != response.size())
		throw std::runtime_error("write(): " + std::string(strerror(errno)));
	
	// check if everything can be written in once?
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
	
	switch (STATE_) {
		case RECEIVING:
			data = readFrom(event.data.fd);
			parser_.processData(data, this);
			if (!parser_.isDone()) {
				return;
			}
			setRequestDataAndConfig();
			is_cgi_request = isCGI(event.data.fd); // TODO: rename
		case PROCESS_CGI:
			if (is_cgi_request && cgi(event.data.fd) != READY)
				return ;
		case RESPONSE:
			responding(is_cgi_request, event);
		case DONE:
			return ;
	}
}

void	printRequest(HTTPRequest request) {
	std::cerr << "------------------\nRequest:\n";
	std::cerr << "Method: " << request.method << "\n";
	std::cerr << "Target: " << request.request_target << "\n";
	std::cerr << "Host: " << request.host[0] << "\n";
	std::cerr << "StatusCode: " << request.status_code << "\n------------------\n";
}

void	HTTPClient::setRequestDataAndConfig(void) {
	request_ = parser_.getParsedRequest();
	printRequest(request_);
	responseGenerator_.setConfig(config_);
}
bool	HTTPClient::isCGI(int fd) {
	if (!responseGenerator_.isCGI(request_))
	{
		STATE_ = RESPONSE;
		return (false);
	}
	else
	{
		STATE_ = PROCESS_CGI;
		cgi(fd);
		return (true);
	}
}

/// @brief regenerates response and add this one to the que.
void	HTTPClient::responding(bool cgi_used, const epoll_event &ev) {
	int fd;

	if (cgi_used)
	{
		cgiresponse();
		fd = ev.data.u32;
	}
	else
	{
		responseGenerator_.generateResponse(request_);
		message_que_.push_back(responseGenerator_.loadResponse());
		fd = ev.data.fd;
	}
	writeTo(fd);
	STATE_ = DONE;
}

/// @brief creates a CGI class, checks the method/target, starts the cgi
bool	HTTPClient::cgi(int fd) {
	std::vector<std::string>	env_strings;

	if (cgi_ == NULL)
	{
		pipes_.setCallbackFunction(addToEpoll_cb_, clientSock_);
		pipes_.addNewPipes();
		cgi_ = std::make_unique<CGI>(request_.body, pipes_.getPipes());
	}
	cgi_->handle_cgi(request_, fd);
	return (cgi_->isReady());
}

/// @brief checks if cgi header has to be rewritten and add response to que.
void	HTTPClient::cgiresponse(void) {
	if (!cgi_->isNPHscript(request_.request_target))
		cgi_->rewriteResonseFromCGI();
	message_que_.push_back(cgi_->getResponse());
}

const Config	*HTTPClient::getConfig( void ) const { return (config_); }
