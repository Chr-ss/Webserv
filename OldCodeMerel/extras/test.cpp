#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 10

// Helper function to set a socket to non-blocking mode
void setNonBlocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_fd, client_fd, epfd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    // Set non-blocking mode
    setNonBlocking(server_fd);

    // Bind and listen
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Create epoll instance
    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1 failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Add server socket to epoll instance
    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN; // Monitor for incoming connections
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl failed");
        close(server_fd);
        close(epfd);
        return EXIT_FAILURE;
    }

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int num_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // Accept new client connection
                client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
                if (client_fd == -1) {
                    perror("accept failed");
                    continue;
                }
                setNonBlocking(client_fd);

                // Add client socket to epoll instance
                struct epoll_event client_event;
                client_event.data.fd = client_fd;
                client_event.events = EPOLLIN | EPOLLET; // Edge-triggered
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event);
                std::cout << "New client connected: " << client_fd << std::endl;
            } else {
                // Handle client input
                char buffer[1024] = {0};
                int bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    std::cout << "Received: " << buffer << std::endl;
                    write(events[i].data.fd, buffer, bytes_read); // Echo back
                } else {
                    // Close connection if client disconnected
                    close(events[i].data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    std::cout << "Client disconnected: " << events[i].data.fd << std::endl;
                }
            }
        }
    }

    close(server_fd);
    close(epfd);
    return 0;
}
