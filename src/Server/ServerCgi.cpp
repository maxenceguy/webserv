/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCgi.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mguy <mguy@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/01 19:55:26 by mguy              #+#    #+#             */
/*   Updated: 2025/03/17 16:53:05 by mguy             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

void Server::handleCGIRequest(int clientfd, Request& req, const char* buffer, int bytes_read) {
    std::string method = req.getMethod();
    std::string response;
    
	// get the value of "Content-length"
    stringstream ss;
    ss << req.getContentLength();
    std::string contentLengthStr = ss.str();

	// get the location
	std::map<std::string, t_location> loc = _config.getLocations();
	t_location tempLoc;
	std::map<std::string, t_location>::iterator it = loc.begin();
	for (;it != loc.end(); ++it) {
		if (it->first.find("cgi") != std::string::npos) {
			tempLoc = it->second;
			break ;
		}
	}
	if (it == loc.end()) {
		std::cout << "[INFO] CGI location can't be finded." << std::endl;
	}

	std::ostringstream oss;
	for (int i = 0; i < 9; ++i) {
		if (i > 0) oss << " ";
		oss << tempLoc.methods[i];
	}
    
    // configure cgi env
    std::map<std::string, std::string> env;
    env["REQUEST_METHOD"] = method;
    env["CONTENT_TYPE"] = req.getContenttype();
    env["CONTENT_LENGTH"] = contentLengthStr;
    env["QUERY_STRING"] = req.getQueryString();
    env["SCRIPT_FILENAME"] = _config.getRoot() + "/cgi-bin/default.py";
    env["PATH_INFO"] = req.getLocation();
	env["METHODS_ALLOWED"] = oss.str();

    // check action (upload, download, delete, list)
    std::string action = "";
    if (req.getQueryString().find("action=upload") != std::string::npos) {
        action = "upload";
    } else if (req.getQueryString().find("action=download") != std::string::npos) {
        action = "download";
    } else if (req.getQueryString().find("action=delete") != std::string::npos) {
        action = "delete";
    } else if (req.getQueryString().find("action=list") != std::string::npos) {
		action ="list";
	}
    env["ACTION"] = action;
    
    // Execute CGI
    int ret = executeCGI(clientfd, env, buffer, bytes_read);
    if (ret != 0) {
        response = "HTTP/1.1 500 Internal Server Error\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: 21\r\n\r\n";
        response += "<h1>CGI Error</h1>";
		if (send(clientfd, response.c_str(), response.length(), 0) <= 0)
		{
			std::cerr << "[ERROR] Failed to send data to client. " << std::endl;
			close(clientfd);
			return ;
		}
    }
}

volatile sig_atomic_t timeoutOccurred = 0;

// Timeout signal handler
static void timeoutHandler(int signum) {
	(void)signum;
    timeoutOccurred = 1;
}

bool Server::createPipes(int inputPipe[2], int outputPipe[2]) {
    if (pipe(inputPipe) < 0 || pipe(outputPipe) < 0) {
        std::cerr << "[ERROR] Pipe error." << std::endl;
        return false;
    }
    return true;
}

char** Server::createEnvArray(const std::map<std::string, std::string>& env) {
    char** envp = new char*[env.size() + 1];
    int i = 0;
    for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        envp[i] = strdup(entry.c_str());
        i++;
    }
    envp[i] = NULL;
    return envp;
}

void Server::executeCGIChild(int inputPipe[2], int outputPipe[2], const std::map<std::string, std::string>& env) {	
    close(inputPipe[1]);
    close(outputPipe[0]);

    dup2(inputPipe[0], STDIN_FILENO);  // Redirect stdin
    dup2(outputPipe[1], STDOUT_FILENO); // Redirect stdout

    char** envp = createEnvArray(env);
    std::string scriptPath = _config.getRoot() + "/cgi-bin/default.py";
    char* argv[] = { strdup(scriptPath.c_str()), NULL };

    execve(argv[0], argv, envp);

    // Cleanup if execve fails
    delete[] envp;
    std::cerr << "[ERROR] Execve error." << std::endl;
    exit(1);
}

std::pair<std::string, int> Server::handleCGIParent(int clientfd, int inputPipe[2], int outputPipe[2], pid_t pid, const char* requestBody, int bodyLength) {
    (void)clientfd;
	close(inputPipe[0]);
    close(outputPipe[1]);

    signal(SIGALRM, timeoutHandler);
    alarm(TIMEOUT);

    if (bodyLength > 0) {
        write(inputPipe[1], requestBody, bodyLength);
    }
    close(inputPipe[1]);

    // Read the response
    char buffer[4096];
    int bytesRead;
    std::string response;
    
    while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
        response.append(buffer, bytesRead);
    }
    close(outputPipe[0]);

    // Wait for child process
    int status;
    pid_t res = waitpid(pid, &status, WNOHANG);
    while (res == 0 && !timeoutOccurred) {
        res = waitpid(pid, &status, WNOHANG);
        usleep(1000);
    }

    if (timeoutOccurred) {
        std::cerr << "[ERROR] CGI execution timed out. Killing child process." << std::endl;
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        response = "CGI execution timeout.\n";
    }
    timeoutOccurred = 0;
    alarm(0);

    return std::make_pair(response, WIFEXITED(status) ? WEXITSTATUS(status) : 1);
}

int Server::executeCGI(int clientfd, std::map<std::string, std::string>& env, const char* requestBody, int bodyLength) {
    int inputPipe[2], outputPipe[2];

    if (!createPipes(inputPipe, outputPipe))
        return 1;

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[ERROR] Fork error." << std::endl;
        return 1;
    }

    if (pid == 0) {
        executeCGIChild(inputPipe, outputPipe, env);
    } else {
        std::pair<std::string, int> result = handleCGIParent(clientfd, inputPipe, outputPipe, pid, requestBody, bodyLength);
        std::string response = result.first;
        int exitStatus = result.second;

        // Send response to client
        size_t totalSent = 0;
        size_t toSend = response.length();
        const char* data = response.c_str();
        while (totalSent < toSend) {
            ssize_t sent = send(clientfd, data + totalSent, toSend - totalSent, 0);
            if (sent <= 0) {
                std::cerr << "[ERROR] send() error in CGI execution.\n";
                break;
            }
            totalSent += sent;
        }

        return exitStatus;
    }
	return 0;
}
