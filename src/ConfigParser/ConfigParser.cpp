# include "ConfigParser.hpp"

Parser::Parser() {}

Parser::~Parser() {}

void Parser::createDeafaultConfig() {
	_port = 8080;
	_host = "127.0.0.1";
	_server_name = "Default_Server";
	_root = "websites/1/";
	_index = "index.html";
	_client_max_body_size = 10485760;
	
	t_location location;
	location.location = "/";
	location.methods[0] = "GET";
	location.methods[1] = "POST";
	location.methods[2] = "DELETE";
	location.root = "websites/1/";
	location.autoindex = false;
	_locations.insert(make_pair(location.location, location));

	location = t_location();
	location.location = "/cgi-bin/";
	location.methods[0] = "GET";
	location.methods[1] = "POST";
	location.methods[2] = "DELETE";
	location.root = "websites/1/cgi-bin/";
	location.autoindex = false;
	location.index = "default.py";
	_locations.insert(make_pair(location.location, location));
}


static std::map<int, std::string> getFile(std::string config_file) {
	std::map<int, std::string> file_map;
	
	std::ifstream file(config_file.c_str());
	if (!file) {std::cerr << "Error: can't open the folder.\n"; return file_map;}
	
	std::string line;
	int 		line_number = -1;
	while (std::getline(file, line)) {
		file_map[++line_number] = line;
	}
	file.close();
	return file_map;
}

static int countServers(std::map<int, std::string> file_map) {
	std::map<int, std::string>::iterator it;
	int	count = 0;
	for (it = file_map.begin(); it != file_map.end(); it++) {
		if (it->second.find("server {") != std::string::npos)
			count++;
	}
	return count;
}

bool isConfFile(const std::string& filename) {
    return filename.length() > 5 && filename.rfind(".conf") == filename.length() - 5;
}

std::map<int, Parser> createParser(std::string config_file) {
	std::map<int, Parser> configs;
	std::map<int, Parser> empty;
	
	// check if it's not empty
	if (config_file == "") {
		Parser parser;
		parser.createDeafaultConfig();
		configs.insert(std::make_pair(0, parser));
		return configs;
	}
	
	// open the config file
	std::map<int, std::string> file_map = getFile(config_file);
	if (file_map.empty()) { return empty; }

	// check .conf
	if (!isConfFile(config_file)) {
        std::cout << "[ERROR] Invalid file extension\n";
		return empty;
    }

	int nb_servers = countServers(file_map);
	if (nb_servers < 0) {std::cerr << "Error: Not enough servers in the config file.\n"; return empty;}
	// parser elements
	for (int i = 0; i < nb_servers; i++) {
		Parser parser;
		parser.setNbServers(nb_servers);
		if (parser.parseElements(file_map, i) == false) {return empty;}
		configs.insert(std::make_pair(i, parser));	
	}
	for (std::map<int, Parser>::iterator it = configs.begin(); it != configs.end(); ++it)
	{
		for (std::map<int, Parser>::iterator ij = it; ij != configs.end(); )
		{
			++ij;  // Increment ij to start at the next element from 'it'
			if (ij != configs.end() && ij->second.getPort() == it->second.getPort()) {
				std::cerr << "[Error] Same port on different server !" << std::endl;
				exit(1);
			}
		}
	}
	return configs;	
}
