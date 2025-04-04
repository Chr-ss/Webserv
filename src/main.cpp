#include "../inc/Webserv/Webserv.hpp"
#include <cstdlib>

std::atomic<bool> keep_alive(true);

void	signal_handler(int signum)
{
	if (signum == 13 || signum == SIGINT)
		keep_alive = false;
}

int	save_main(int argc, char **argv) {
	if (argc > 2) {
		std::cerr << "Usage: ./webserv <path_to_config>" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string path = (argc == 2) ? argv[1] : DEFAULT_CONFIGFILE;

	std::signal(SIGINT, signal_handler);

	std::cerr << "Webserver starting ... with config: " << path << std::endl;
	Webserv	webserver(path);
	std::cerr << "Webserver has started" << std::endl;
	webserver.eventLoop();
	return (0);
}

int main (int argc, char **argv) {
	try
	{
		return (save_main(argc, argv));
	}
	catch (std::runtime_error &e) {
		std::cerr << "Runtime error occurred: " << e.what() << std::endl;
	}
	catch (std::invalid_argument &e) {
		std::cerr << "Invalid argument error occurred: " << e.what() << std::endl;
	}
	catch (std::exception &e) {
		std::cerr << "Error occurred: " << e.what() << std::endl;
	}
	catch (...) {
		std::cerr << "Error: Something else went wrong" << std::endl;
	}
	return (01);
}
