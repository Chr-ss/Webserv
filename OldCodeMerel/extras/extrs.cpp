#include "networking/ClientConnection.hpp"

#include <cassert>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>
#include <stdexcept>
#include <tuple>

#include "Headers.hpp"

static std::tuple<int, uint32_t> doAccept(int serverFd) {
    sockaddr_in address;
    socklen_t address_length;
    int fd = ::accept(serverFd, reinterpret_cast<sockaddr *>(&address),
                      &address_length);
    if (fd < 0)
        throw std::runtime_error("accept error");
    assert(address_length == sizeof(address));
    return {fd, address.sin_addr.s_addr};
}

void ClientConnection::setNonBlocking() {
    if (fcntl(m_fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl error");
}

void ClientConnection::processReadEvent() {
    if (!m_isReceiving)
        return;
    // TODO move this on heap and to constructor+destructor
    char buffer[m_recvBufferSIze];
    ssize_t bytesRead = ::recv(m_fd, buffer, m_recvBufferSIze, 0);
    if (bytesRead < 0)
        throw std::runtime_error("recv error");
    if (bytesRead == 0) {
        onStopReceiving();
        return;
    }
    std::string toFeed(buffer, bytesRead);
    m_client->feedData(std::move(toFeed));
}

void ClientConnection::processWriteEvent() {
    if (!m_messageQueue.empty())
        processFirstMessage();
    else if (!m_isSending)
        m_connectionsObserver.onFinishedConnection(m_fd);
}

void ClientConnection::processFirstMessage() {
    auto message = m_messageQueue.front();
    ssize_t sent = ::send(m_fd, message.c_str(),
                          std::min(m_sendBufferSIze, message.length()), 0);
    if (sent < 0)
        throw std::runtime_error("send error");
    if ((size_t)sent < message.length())
        message = message.substr(sent);
    else
        m_messageQueue.pop();
}

ClientConnection::~ClientConnection() {
    std::cout << "Closing connection" << std::endl;
    ::close(m_fd);
}

bool ClientConnection::checkShouldProcessEvent(const IPollEvent &event) {
    return getFd() == event.getFd();
}

void ClientConnection::processEvent(const IPollEvent &event) {
    if (event.isReadable())
        processReadEvent();
    else if (event.isWritable())
        processWriteEvent();
    else
        m_connectionsObserver.onFinishedConnection(m_fd);
}

int ClientConnection::getFd() const { return m_fd; }

uint16_t ClientConnection::getServerPort() const { return m_serverPort; }

uint32_t ClientConnection::getClientIpAddress() const {
    return m_clientIpAddress;
}

std::unique_ptr<ClientConnection> ClientConnection::acceptFromServerSocket(
    int serverFd, uint16_t serverPort,
    IConnectionsObserver &connectionsObserver) {
    id_t fd;
    uint32_t ip;
    std::tie(fd, ip) = doAccept(serverFd);

    // std::cout << "accepting" << std::endl;

    auto result = std::unique_ptr<ClientConnection>(
        new ClientConnection(fd, ip, serverPort, connectionsObserver));
    result->setNonBlocking();
    return result;
}

void ClientConnection::onStopSending() { m_isSending = false; }
