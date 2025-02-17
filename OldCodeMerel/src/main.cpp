#include "Headers.hpp"
#include "Webserv.hpp"

std::atomic<bool> keep_alive(true);

void	signal_handler(int signum)
{
	if (signum == SIGINT)
		keep_alive = false;

	// maybe not nessasary anymore because of send() with MSG_NOSIGNAL instead of write 
	if (signum == SIGPIPE)
		std::perror("SIGPIPE receiving, ignored");
}

std::ifstream	open_configfile(char *filename) {
	std::ifstream 	config;
	
	config.open(filename);
	if (!config.good())
	{
		std::perror(filename);
		exit (EXIT_FAILURE);
	}
	return (config);
}

void	handle_configfile(char *filename, Webserv &webserv)
{
	
	std::ifstream config = open_configfile(filename);
	// serverConfig = parse_configfile(config);
	// for (int i = 0; i < serverConfig.size(); i++)
	//	webserv.addServer(Server(serverConfig[i]))
}

int main(int argc, char **argv)
{
	std::ifstream		config;
	Webserv				webserv;
	int					exit_code;

	keep_alive = true;
	std::signal(SIGINT, signal_handler);
	
	if (argc == 1)
		handle_configfile(DEFAULT_PATH, webserv);
	else
		handle_configfile(argv[1], webserv);
	exit_code = webserv.mainLoop();
	return (exit_code);
}