# include "ConfigParser.hpp"

// ========== Getters ==========

int 								Parser::getPort() { return _port; }
std::string 						Parser::getHost() { return _host; }
std::string 						Parser::getServerName() { return _server_name; }
std::string 						Parser::getRoot() { return _root; }
std::string 						Parser::getIndex() {return _index; }
int 								Parser::getClientMaxBody() { return _client_max_body_size; }
std::map<int, std::string> 			Parser::getErrorPages() { return _errorPages; }
std::map<std::string, t_location> 	Parser::getLocations() { return _locations; }
int 								Parser::getNbServers() { return _nbServers; }

// ========== Setters ==========

void	Parser::setNbServers(int nb ) { _nbServers = nb; }


// ========== Functions ==========

void Parser::processValue(std::istringstream &iss, std::string &dest) {
    iss >> dest;
    if (dest.empty()) {
      std::cerr << "Error: empty value in the config file.\n";
      return;
    }
    if (!dest.empty() && dest[dest.size() - 1] == ';')
          dest = dest.erase(dest.size() - 1);
}

void Parser::processIntValue(std::istringstream &iss, int &dest) {
    std::string buffer;
    iss >> buffer;
    if (buffer.empty()) {
      std::cerr << "Error: empty value in the config file.\n";
      return;
    }
    if (!buffer.empty() && buffer[buffer.size() - 1] == ';')
        buffer = buffer.erase(buffer.size() - 1);
    dest = atoi(buffer.c_str());
}

void Parser::processSizeValue(std::istringstream &iss, size_t &dest) {
    std::string buffer;
    iss >> buffer;
    if (buffer.empty()) {
      std::cerr << "Error: empty value in the config file.\n";
      return;
    }
    if (!buffer.empty() && buffer[buffer.size() - 1] == ';')
        buffer = buffer.erase(buffer.size() - 1);
    dest = static_cast<size_t>(std::strtoul(buffer.c_str(), NULL, 10));
}

void Parser::processMethValue(std::istringstream &iss, std::string (&dest)[9]) {
    std::string buffer;
    int i = 0;

    while (iss >> buffer && i < 6) {
    	if (buffer.empty() && i == 0) {
          	std::cerr << "Error: empty value in the config file.\n";
            return;
    	}
    	if (!buffer.empty() && buffer[buffer.size() - 1] == ';')
    	    buffer.erase(buffer.size() - 1);

    	dest[i++] = buffer;
    }
}

void Parser::parseLine(std::istringstream &iss, std::string &key, int *locInd, t_location *location) {
    if (key == "location") {
        (*locInd)++;
	}
	if (*locInd == 0) {
		if (key == "listen") {
			_port = -1;
			processIntValue(iss, _port);
			if (_port == -1) {
				std::cerr << "[ERROR] _port null in config file.\n";
				exit(1);	
			}
		}
		else if (key == "host") {
			processValue(iss, _host);
			if (_host.empty()) {
				std::cerr << "[ERROR] _host empty in config file.\n";
				exit(1);	
			}
		}
		else if (key == "server_name") { processValue(iss, _server_name); }
		else if (key == "root") { processValue(iss, _root); }
		else if (key == "index") { processValue(iss, _index); }
		else if (key == "client_max_body_size") { processIntValue(iss, _client_max_body_size); }
		else if (key == "error_page") {
			std::string code;
			iss >> code;
			if (code.empty()) {
				std::cerr << "[Error] Empty value in the config file.\n";
				return;
			}
			if (!code.empty() && code[code.size() - 1] == ';')
				code = code.erase(code.size() - 1);

			std::string buffer;
			iss >> buffer;
			if (buffer.empty()) {
				std::cerr << "[Error] Empty value in the config file.\n";
				return;
			}
			if (!buffer.empty() && buffer[buffer.size() - 1] == ';')
				buffer = buffer.erase(buffer.size() - 1);
			
			int codeInt = atoi(code.c_str());
			if (codeInt < 1) {
				std::cerr << "[Error] parsing code error_page = " << codeInt << std::endl;
				return ;
			}
			_errorPages.insert(std::make_pair(codeInt, buffer));
		}
	}
	else {	 // parse the differents locations	
		if (key == "location") {
			processValue(iss, location->location);
		}
		else if (key == "root") { processValue(iss, location->root); }
		else if (key == "methods") { processMethValue(iss, location->methods); }
		else if (key == "redirect") { processValue(iss, location->redirect); }
		else if (key == "index") { processValue(iss, location->index); }
		else if (key == "autoindex") {
			std::string buffer;
			iss >> buffer;
			location->autoindex = (buffer == "on");
		}
	}
}

bool Parser::parseElements(std::map<int, std::string> file_map, int n_server) {
    int    it_serv = -1;
    int    locInd = 0;
    std::map<int, std::string>::iterator it = file_map.begin();

	t_location location;

	// check the server index
    while (it_serv < n_server && it != file_map.end()) {
        if (it->second.rfind("server {") == 0) {
          it_serv++;
        }
        it++;
    }

    for (; it != file_map.end(); it++) {
        if (it->second.rfind("server {") == 0) break;
        std::istringstream iss(it->second);
        std::string key;
        iss >> key;
		
		if (key.find("#") != std::string::npos)
			continue;
		
		if (key == "location") {
			if (!location.location.empty()) {
				_locations.insert(make_pair(location.location, location));
			}
			location = t_location();
		}
        parseLine(iss, key, &locInd, &location);
    }

	if (!location.location.empty()) {
		_locations.insert(make_pair(location.location, location));
	}
	
	location = t_location();
	    
	return true;
}
