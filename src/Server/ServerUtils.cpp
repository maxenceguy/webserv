/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fsulvac <fsulvac@student.42lyon.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/25 15:43:35 by mguy              #+#    #+#             */
/*   Updated: 2025/03/14 14:26:06 by fsulvac          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

// ========== SET ==========

void Server::setupSocket() {
	_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	memset(&_serv_addr, 0, sizeof(_serv_addr));
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_port = htons(_config.getPort());
	_serv_addr.sin_addr.s_addr = INADDR_ANY;
}

int Server::setupBind() {
	int opt = 1;
	if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		std::cerr << "[ERROR] setsockopt failed." << std::endl;
		close(_sockfd);
		return 1;
	}

	if (bind(_sockfd, (struct sockaddr *)& _serv_addr, sizeof(_serv_addr)) == -1) {
		std::cerr << "[ERROR] Bind failed." << std::endl;
		close(_sockfd);
		return 1;
	}
	return 0;
}

int Server::setupListen() {
	if (listen(_sockfd, 10) < 0) {
		std::cerr << "[ERROR] Listen failed." << std::endl;
		return 1;
	}
	return 0;
}

void Server::setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		std::cerr << "[ERROR] Fcntl failed." << std::endl;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		std::cerr << "[ERROR] Fcntl non blockin failed." << std::endl;
	}
}

int Server::setupEpoll(int fd) {
	_epollfd = fd;
	return 0;
}



// ========== GET ==========


std::string Server::getWebFile(const std::string& path) {
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "File Website can't be open" << std::endl;
		return "<html><body><h1>404 - File Not Found</h1></body></html>";
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

int Server::getSockFd() {
	return _sockfd;
}

int Server::getEpollFd() {
	return _epollfd;
}

int Server::getPort() {
	return _config.getPort();
}

std::vector<int> Server::getClientFds() {
	return _clientFds;
}



// ================ UTILS FUNCTIONS ===================

bool Server::isClientFd(int fd) {
    return std::find(_clientFds.begin(), _clientFds.end(), fd) != _clientFds.end();
}

void Server::closeClientFd(int client) {
	if (_clientFds[client] != -1) {
		close(_clientFds[client]);
		_clientFds[client] = -1;
	}
}

bool Server::fileExists(const std::string &filename) {
	std::string path = _config.getRoot() + filename;
    std::ifstream file(path.c_str());
    return file.good();
}

// Return the type of the element
std::string Server::getMimeType(const std::string& path) {
    std::string extension;
    size_t pos = path.find_last_of(".");
    if (pos != std::string::npos) {
        extension = path.substr(pos + 1);
    }
    
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "png") return "image/png";
    if (extension == "gif") return "image/gif";
    if (extension == "svg") return "image/svg+xml";
    if (extension == "pdf") return "application/pdf";
    if (extension == "txt") return "text/plain";
    if (extension == "xml") return "application/xml";
    if (extension == "json") return "application/json";
    
    return "application/octet-stream";
}

// return Message status
std::string Server::getStatusMessage(int statusCode) {
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown Status";
    }
}

t_location Server::findLocation(std::string locPath, int clientfd) {
	std::map<std::string, t_location> loc = _config.getLocations();
	t_location tempLoc;
	std::map<std::string, t_location>::iterator it1 = loc.begin();
	while (it1 != loc.end()) {
		if (it1->first == locPath) {
			tempLoc = it1->second;
			break ;
		}
		++it1;
	}
	if (it1 == loc.end()) {
		std::cout << "[INFO] location can't be finded." << std::endl;
		std::string response = generateErrorPage(404);
		if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return tempLoc;
		}
	}
	return tempLoc;
}
