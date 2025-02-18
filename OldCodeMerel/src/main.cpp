#include "../Includes/Headers.hpp"
#include "../Includes/Webserv.hpp"

std::atomic<bool> keep_alive(true);

void	signalHandler(int signum)
{
	if (signum == SIGINT)
		keep_alive = false;

	// maybe not nessasary anymore because of send() with MSG_NOSIGNAL instead of write 
	if (signum == SIGPIPE)
		std::perror("SIGPIPE receiving, ignored");
}

std::ifstream	openConfigfile(std::string filename) {
	std::ifstream 	config;
	
	config.open(filename);
	if (!config.good())
	{
		std::perror("can not open configfile");
		exit (EXIT_FAILURE);
	}
	return (config);
}

void	handleConfigfile(std::string filename, Webserv &webserv)
{
	
	std::ifstream config = openConfigfile(filename);
	// serverConfig = parse_configfile(config);
	// for (int i = 0; i < serverConfig.size(); i++)
	//	webserv.addServer(Server(serverConfig[i]))
	(void) webserv;
}

int	secureMain(int argc, char **argv)
{
	std::ifstream		config;
	Webserv				webserv;
	std::string			str;

	keep_alive = true;
	std::signal(SIGINT, signalHandler);
	
	if (argc == 1)
		handleConfigfile(DEFAULT_PATH, webserv);
	else
	{
		std::string	strFilename(argv[1]);
		handleConfigfile(strFilename, webserv);
	}
	return (webserv.mainLoop());
}

int main(int argc, char **argv)
{
	int	exit_code;

	exit_code = 0;
	try
	{
		exit_code = secureMain(argc, argv);
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return (exit_code);
}