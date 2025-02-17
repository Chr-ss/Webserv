#pragma once

#include "Headers.hpp"

enum BLOCK {
	MAIN,
	EVENTS,
	HTTP,
	MAIL,
	STREAM,
	SERVER,
	LOCATION
};

class Config {
	private: 
		BLOCK	m_block;
		std::vector<std::pair<std::string, std::string>> m_directives;
		std::vector<Config> m_InnerBlocks;
	public:
		Config( void );
		Config( BLOCK block );
		~Config();

		void addDirective( std::string key, std::string value );
		void addBlock( Config block ); // recursive?

		BLOCK	getBlockType(void);
		std::vector<Config>	getInnerBlocks(void);
		std::vector<std::pair<std::string, std::string>> getDirectives(void);
};
