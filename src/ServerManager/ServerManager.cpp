#include "ServerManager.hpp"

ServerManager::ServerManager(const std::map<int, Parser>& configs) {
	// Create the main epoll
	epollfd = epoll_create(1024);
	if (epollfd == -1) {
		std::cerr << "[ERROR] epoll_create failed." << std::endl;
		exit(1);
	}

	// Init the servers Map
	std::map<int, Parser>::const_iterator it;
	for (it = configs.begin(); it != configs.end(); ++it) {
		_servers.push_back(Server(it->second));
		Server &srv = _servers.back();

		srv.setupSocket();
		if (srv.setupBind() == 1 || srv.setupListen() == 1)
			continue;
		srv.setNonBlocking(srv.getSockFd());
		srv.addToEpoll(epollfd);
		srv.setupEpoll(epollfd);
	}
	_config = configs;
}

ServerManager::~ServerManager() {}

// === Global variable for the server(s) loop ===
bool loop = true;
// ==============================================

static void signalHandler(int signum) {
    std::cout << "\nSignal recevied: " << signum << ", closing server..." << std::endl;
	loop = false;
}

void ServerManager::run() {
    struct epoll_event events[1024];

    std::cout << "[INFO] Starting all servers..." << std::endl << std::endl;

	signal(SIGINT, signalHandler);

	size_t servIndex = 1;
    for (std::map<int, Parser>::iterator it = _config.begin(); it != _config.end(); ++it) {
        int port = it->second.getPort();
        std::string host = it->second.getHost();

        std::cout << "[INFO] Server " << servIndex << " listening on port " << port << " [...]" << std::endl;
        std::cout << "[INFO] Web Address: http://" << host << ":" << port << std::endl;
		std::cout << std::endl;
		servIndex++;
    }

	std::cout << "==============================\n";

	// main loop of the webserv
    while (loop) {
		// wait requests from the client
		int numEvents = epoll_wait(epollfd, events, 1024, -1);
		if (numEvents == -1) {
			if (loop) {
				std::cerr << "[ERROR] epoll_wait failed." << std::endl;	
			}
			break;
		}

		// manage the requests: newconnection / other requests
		for (int i = 0; i < numEvents; i++) {
			int fd = events[i].data.fd;
			for (size_t j = 0; j < _servers.size(); j++) {
				if (fd == _servers[j].getSockFd()) {
					if (_servers[j].newClientConnection() == 0) {
						std::cout << "[INFO] New client connection accepted on port " << _servers[j].getPort() << "." << std::endl;
					}
					goto next_event;
				}
			}

			for (size_t j = 0; j < _servers.size(); j++) {
				if (_servers[j].isClientFd(fd)) {
					_servers[j].handleClientRequest(fd);
					goto next_event;
				}
			}

			std::cerr << "[WARNING] Unhandled event for fd: " << fd << std::endl;
			next_event:;
		}
	}

	// clean close
	for (size_t  i = 0; i < _servers.size(); ++i) {
		for (size_t j = 0; j < _servers[i].getClientFds().size(); ++j) {
			_servers[i].closeClientFd(j);
		}
		close(_servers[i].getSockFd());
	}
    close(epollfd);
}
