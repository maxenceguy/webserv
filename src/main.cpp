#include "includes.hpp"
#include "ConfigParser/ConfigParser.hpp"
#include "Server/Server.hpp"
#include "ServerManager/ServerManager.hpp"

int main(int argc, char** argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " <optional_config_file>" << std::endl;
        return 1;
    }
    
    try {
        // Parse config file
		std::string configFile;
		if (argc == 2)
			configFile = argv[1];
        std::map<int, Parser> config = createParser(configFile);
        if (config.empty()) {
			std::cerr << "Config file Error" << std::endl;
			return 1;
		}
		// Run all servers
		ServerManager serverManager(config);
		serverManager.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
