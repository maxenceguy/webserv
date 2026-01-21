#include "Server.hpp"


Server::Server(Parser config) {
	_config = config;
}

Server::~Server() {}



// ============================ Utils ========================

void printRequest(char *buffer) {
    std::cout << "========== Request ==========\n";
    std::cout << buffer << std::endl;
    std::cout << "=============================\n\n\n";
}

void printResponse(std::string buffer) {
    std::cout << "========== Response ==========\n";
    std::cout << buffer << std::endl;
    std::cout << "==============================\n\n\n";
}


// ====================== Functions =======================


int Server::newClientConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
	// get the client fd with accept()
    int clientfd = accept(_sockfd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientfd == -1) {
        std::cerr << "[ERROR] New client connection failed: " << _sockfd << std::endl;
        return 1;
    }

	// add client to the client list
	if (std::find(_clientFds.begin(), _clientFds.end(), clientfd) != _clientFds.end()) {
        std::cerr << "[ERROR] Duplicate client connection detected!" << std::endl;
		std::string response = generateErrorPage(400);
        if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return 1;
		}
        close(clientfd);
        return 1;
    }
    _clientFds.push_back(clientfd);

	// retrieves options for a given socket
    int sockType;
    socklen_t len = sizeof(sockType);
    if (getsockopt(clientfd, SOL_SOCKET, SO_TYPE, &sockType, &len) == -1) {
        std::cerr << "[ERROR] getsockopt failed" << std::endl;
		std::string response = generateErrorPage(500);
        if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return 1;
		}
        close(clientfd);
        return 1;
    }

    setNonBlocking(clientfd);

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = clientfd;

	// register clientfd with epoll 
    if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, clientfd, &event) == -1) {
        std::cerr << "[ERROR] Epoll_ctl failed: (fd: " << clientfd << ")" << std::endl;
		std::string response = generateErrorPage(500);
        if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return 1;
		}
        close(clientfd);
        return 1;
    }

    std::cout << "[INFO] New client connection accepted, fd: " << clientfd << "." << std::endl;
    return 0;
}


void Server::addToEpoll(int epollfd) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = _sockfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, _sockfd, &ev) == -1) {
        std::cerr << "[ERROR] Failed to add server socket to epoll" << std::endl;
        exit(1);
    }
}

void Server::handleClientRequest(int clientfd) {
    char buffer[4096];
    int bytes_read = read(clientfd, buffer, sizeof(buffer) - 1);

	// check if the client is disconnected
    if (bytes_read <= 0) {
		std::cout << "[INFO] Client disconnected, fd: " << clientfd << std::endl;
		
		std::vector<int>::iterator it = std::find(_clientFds.begin(), _clientFds.end(), clientfd);
		if (it != _clientFds.end()) {
			_clientFds.erase(it);
			std::cout << "[INFO] Client fd " << clientfd << " removed from the list." << std::endl;
		} else {
			std::cout << "[INFO] Client fd " << clientfd << " not found in the list." << std::endl;
		}

		close(clientfd);
		return;
	}

	// =========== IF THE CLIENT IS CONNECTED ===========
    buffer[bytes_read] = '\0';
    Request req;
    if (req.parseHeader(buffer)) {
        return ;
	}

	std::cout << "[INFO] Request recevied / " << req.getMethod() << " " << req.getLocation() << " (" << req.getVersionHttp() << ")" << std::endl;
	std::cout << "  ↳ Body size : (" << req.getContentLength() << ")" << std::endl;
	
	// Manage Client body size max
	if (req.getContentLength() > _config.getClientMaxBody()) {
		std::cerr << "[ERROR] Body too large, max allowed: " << _config.getClientMaxBody() << " bytes." << std::endl;
		std::string response = generateErrorPage(413);
		if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return ;
		}
		return ;
	}

	// read the entire body
	if (req.getContentLength() > 0) {
		std::string body = req.readFullRequest(clientfd, req.getContentLength(), bytes_read, buffer);
		req.setBody(body);
	}

	// Request management: standard request / CGI
    if (req.getLocation().find("/cgi-bin/") != std::string::npos) {
		t_location temploc = findLocation("/cgi-bin/", clientfd);
		std::string fullCgiPath = temploc.location + temploc.index;
		if (req.getLocation() == fullCgiPath && fileExists(req.getLocation()))
        	handleCGIRequest(clientfd, req, req.getBody().c_str(), req.getContentLength());
		else {
			std::cerr << "[ERROR] " << req.getLocation() << " can't be finded." << std::endl;
			std::string response = generateErrorPage(404);
            if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
            {
                std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
                close(clientfd);
                return ;
            }
		}
    } else {
        handleRegularRequest(clientfd, req);
    }
}



// ========== Standard Request ==========


bool Server::isMethodAllowed(const std::string& method, const t_location& loc) {
    for (int i = 0; i < 9; ++i) {
        if (loc.methods[i].find(method.c_str()) != std::string::npos)
            return true;
    }
    return false;
}

t_location Server::findLocation(const std::map<std::string, t_location>& locations, const std::string& path) {
    std::map<std::string, t_location>::const_iterator bestMatch = locations.end();
    
    for (std::map<std::string, t_location>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        if (path.find(it->first) == 0) { 
            std::string loc = it->first;

            if (path.length() == loc.length() || (path[loc.length()] == '/')) {
                if (bestMatch == locations.end() || loc.length() > bestMatch->first.length()) {
                    bestMatch = it;
                }
            }
        }
    }
    
    if (bestMatch != locations.end())
        return bestMatch->second;
    
    return t_location();
}

std::string Server::handleGetRequest(const std::string& fullPath) {
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == 0) {
        std::ifstream file(fullPath.c_str(), std::ios::binary);
        if (file) {
            file.seekg(0, std::ios::end);
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            char* buffer = new char[size];
            std::string response;
            if (file.read(buffer, size)) {
                std::string contentType = getMimeType(fullPath);

                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: " + contentType + "\r\n";
                response += "Connection: keep-alive\r\n";

                std::stringstream ss;
                ss << size;
                response += "Content-Length: " + ss.str() + "\r\n\r\n";
                response.append(buffer, size);
            }
            delete[] buffer;
            return response;
        }
    }
    return generateErrorPage(404);
}

std::string Server::handleDeleteRequest(const std::string& fullPath) {
    if (access(fullPath.c_str(), F_OK) == 0) {
        if (remove(fullPath.c_str()) == 0) {
            return "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 23\r\n\r\n<h1>File Deleted</h1>";
        }
        return generateErrorPage(500);
    }
    return generateErrorPage(404);
}

void Server::handleRegularRequest(int clientfd, Request& req) {
    std::string method = req.getMethod();
    std::string path = req.getLocation();
    std::string response;

    // Find the corresponding location
    t_location location = findLocation(_config.getLocations(), path);

    if (location.location.empty()) {
        response = generateErrorPage(404);
        if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
        {
            std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
            close(clientfd);
            return ;
        }
        return;
    }

    // Check if the method is allowed
    if (!isMethodAllowed(method, location)) {
        response = generateErrorPage(405);
		if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return ;
		}
        return;
    }

    // Construct full path
    std::string fullPath = location.root + (location.index.empty() ? _config.getIndex() : location.index);

    // Handle methods
    if (method == "GET") {
        response = handleGetRequest(fullPath);
    } else if (method == "POST") {
        response = generateErrorPage(405);
    } else if (method == "DELETE") {
        response = handleDeleteRequest(fullPath);
    } else {
        response = generateErrorPage(501);
    }

    if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
    {
        std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
        close(clientfd);
        return ;
    }
}



// ==================== Generate Error Page ===================

std::string Server::generateErrorPage(int errorCode) {
    std::string errorPage;
	
    // check if the personal error page is configured
    std::map<int, std::string> errorPages = _config.getErrorPages();
	std::map<int, std::string>::iterator it = errorPages.find(errorCode);
    if (it != errorPages.end()) {
		std::string errorPagePath = _config.getRoot() + "errors_pages/" + errorPages[errorCode];
        std::ifstream file(errorPagePath.c_str());
        
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            
            // Convert errorCode into string
            std::stringstream ss;
            ss << errorCode;
            
            errorPage = "HTTP/1.1 " + ss.str() + " " + getStatusMessage(errorCode) + "\r\n";
            errorPage += "Content-Type: text/html\r\n";
            
            // Convert buffer.str().length() into string
            std::stringstream ss2;
            ss2 << buffer.str().length();
            
            errorPage += "Content-Length: " + ss2.str() + "\r\n\r\n";
            errorPage += buffer.str();
            return errorPage;
        }
    }
	
    // Default error page
    std::stringstream ss;
    ss << errorCode;
	
    std::string defaultPage = "<html><head><title>Error " + ss.str() + "</title></head><body>";
    defaultPage += "<h1>Error " + ss.str() + " - " + getStatusMessage(errorCode) + "</h1>";
    defaultPage += "</body></html>";
    
    errorPage = "HTTP/1.1 " + ss.str() + " " + getStatusMessage(errorCode) + "\r\n";
    errorPage += "Content-Type: text/html\r\n";

    
    // Convert defaultPage.length() into string
    std::stringstream ss2;
    ss2 << defaultPage.length();
    
    errorPage += "Content-Length: " + ss2.str() + "\r\n\r\n";
    errorPage += defaultPage;
	
    
    return errorPage;
}
