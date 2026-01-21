/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerManager.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mguy <mguy@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/10 18:29:00 by mguy              #+#    #+#             */
/*   Updated: 2025/03/13 08:50:27 by mguy             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
