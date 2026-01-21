/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParserConfig.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mguy <mguy@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/04 16:04:27 by jd                #+#    #+#             */
/*   Updated: 2025/02/10 20:02:52 by mguy             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

# include "includes.hpp"
// Add in ParserConfig.hpp a array of struct parser to stock the differents parsers.

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <sstream> 
#include <cstdlib>
#include <vector>

using namespace std;

typedef struct s_location {
	std::string 				location;
	std::string 				methods[9];
	std::string 				root;
	std::string 				redirect;
	bool						autoindex;
	std::string					index;
} t_location;

class Parser {
	private :
		std::map<int, std::string> 			file_map;
		int 								_nbServers;
		std::map<std::string, t_location> 	_locations;
		
		int 						_port;
		std::string 				_host;
		std::string 				_server_name;
		std::string 				_root;
		std::string 				_index;
		int 						_client_max_body_size;
		std::map<int, std::string> 	_errorPages;

	public:
		Parser();
		~Parser();

		void 	createDeafaultConfig();

		bool	parseElements(std::map<int, std::string> file_map, int n_server);
		void	parseLine(std::istringstream &iss, std::string &key, int *locInd, t_location *location);
		void	processIntValue(std::istringstream &iss, int &dest);
		void	processValue(std::istringstream &iss, std::string &dest);
		void	processMethValue(std::istringstream &iss, std::string (&dest)[9]);
		void	processSizeValue(std::istringstream &iss, size_t &dest);

		void	setNbServers(int nb);
		int		getNbServers();

		// Getters
		int 								getPort();
		std::string 						getHost();
		std::string 						getServerName();
		std::string 						getRoot();
		std::string 						getIndex();
		int 								getClientMaxBody();
		std::map<int, std::string> 			getErrorPages();
		std::map<std::string, t_location> 	getLocations();

};

std::map<int, Parser> createParser(std::string config_file);

#endif
