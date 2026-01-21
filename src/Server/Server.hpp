/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mguy <mguy@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 01:42:47 by mguy              #+#    #+#             */
/*   Updated: 2025/03/13 13:44:05 by mguy             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "includes.hpp"
#include "../Request/Request.hpp"
#include "../ConfigParser/ConfigParser.hpp"

#define TIMEOUT 5

class Server {
	private:
		Parser _config;

		std::string 				_serverName;
		std::string 				_root;
		std::map<int, std::string> 	_errorPages;

		int 				_sockfd;
		struct sockaddr_in 	_serv_addr;
		int 				_epollfd;
		struct epoll_event 	event;

		std::vector<int> 	_clientFds;

	public:
		Server(Parser config);
		~Server();
		
		// setters
		void 	setupSocket();
		int 	setupBind();
		int 	setupListen();
		void 	setNonBlocking(int fd);
		int 	setupEpoll(int fd);

		// getters
		std::string 		getWebFile(const std::string& path);
		std::string 		getStatusMessage(int statusCode);
		int 				getSockFd();
		int					getEpollFd();
		int					getPort();
		std::vector<int>	getClientFds();
		
		int 		newClientConnection();
		void 		addToEpoll(int epollfd);
		bool 		isClientFd(int fd);
		void 		closeClientFd(int client);
		bool 		fileExists(const std::string &filename);
		t_location 	findLocation(std::string loc, int clientfd);
		
		void 		handleClientRequest(int clientfd);

		// Server
		void				handleRegularRequest(int clientfd, Request& req);
		std::string 		generateErrorPage(int errorCode);
		std::string 		generateDirectoryListing(const std::string& urlPath, const std::string& fullPath);
		std::string 		getMimeType(const std::string& path);
		

		// Standard Request
		bool		isMethodAllowed(const std::string& method, const t_location& loc);
		t_location 	findLocation(const std::map<std::string, t_location>& locations, const std::string& path);
		std::string handleGetRequest(const std::string& fullPath);
		std::string handleDeleteRequest(const std::string& fullPath);

		// ServerCgi
		void 							handleCGIRequest(int clientfd, Request& req, const char* buffer, int bytes_read);
		int 							executeCGI(int clientfd, std::map<std::string, std::string>& env, const char* requestBody, int bodyLength);
		bool							createPipes(int inputPipe[2], int outputPipe[2]);
		char**							createEnvArray(const std::map<std::string, std::string>& env);
		void 							executeCGIChild(int inputPipe[2], int outputPipe[2], const std::map<std::string, std::string>& env);
		std::pair<std::string, int>		handleCGIParent(int clientfd, int inputPipe[2], int outputPipe[2], pid_t pid, const char* requestBody, int bodyLength);

	};

#endif
