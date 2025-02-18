# include "../Includes/Headers.hpp"
# include "../Includes/Config.hpp"

Config::Config(void) : m_block(MAIN) {}

Config::Config(BLOCK block) : m_block(block) {}

Config::~Config() {}

void Config::addDirective(std::string key, std::string value) {
	m_directives.push_back(std::make_pair(key, value));
}

void Config::addBlock(Config block) {
	m_InnerBlocks.push_back(block);
}

BLOCK	Config::getBlockType(void) {
	return (m_block);
}

std::vector<std::pair<std::string, std::string>> Config::getDirectives(void) {
	return (m_directives);
}

std::vector<Config>	Config::getInnerBlocks(void) {
	return (m_InnerBlocks);
}