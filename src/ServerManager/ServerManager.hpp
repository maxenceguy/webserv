#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include "includes.hpp"
#include "../Server/Server.hpp"

class ServerManager {
	private:
		std::vector<Server> 	_servers;
		int 					epollfd;
		std::map<int, Parser> 	_config;
	
	public:
		ServerManager(const std::map<int, Parser>& configs);
		~ServerManager();
		void run();
};

#endif
