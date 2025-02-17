# include "Headers.hpp"

class Epoll {
	private:
		int m_epoll_fd;
		int m_events_mask;
	public:
		Epoll( void );
		~Epoll( void );
		void registerFd( int fd );
		void unregisterFd( int fd ) noexcept;
		std::vector<std::unique_ptr<Epoll>> Epoll::poll(int timeoutMs);
};