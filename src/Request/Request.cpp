#include "Request.hpp"
#include <cerrno>

Request::Request() {};

// =========== GETTERS ===========

std::string Request::getMethod() {
	return _method;
}

std::string Request::getLocation() {
	return _location;
}

std::string Request::getVersionHttp() {
	return _versionHttp;
}

std::string Request::getHost() {
	return _host;
}

std::string Request::getUserAgent() {
	return _userAgent;
}

std::string Request::getConnection() {
	return _connection;
}

int	Request::getContentLength() {
	return _contentLength;
}

int Request::getStatusCode() {
	return _statusCode;
}

std::string Request::getContenttype() {
	return _contentType;
}

std::string Request::getBody() {
	return _body;
}

std::string Request::getQueryString() const {
	return _queryString;
}


// ========== SETTERS ===========

void Request::setHost(std::string value) {
	_host = value;
}

void Request::setUserAgent(std::string value) {
	_userAgent = value;
}

void Request::setConnection(std::string value) {
	_connection = value;
}

void Request::setContentLength(std::string value) {
	_contentLength = atoi(value.c_str());
}

void Request::setContentType(std::string value) {
	_contentType = value;
}

void Request::setStatusCode(int value) {
	_statusCode = value;
}

void Request::setQueryString(const std::string& queryString) {
	_queryString = queryString;
}

void Request::setBody(const std::string& body) {
    _body = body;
}


// ========== FUNCTIONS ==========

std::string Request::readFullRequest(int clientfd, int contentLength, int firstBytesRead, char firstBuffer[]) {
    std::string body;
    std::string rawData(firstBuffer, firstBytesRead);
    
    size_t headerEnd = rawData.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        headerEnd += 4;
    } else {
        headerEnd = rawData.find("\n\n");
        if (headerEnd != std::string::npos) {
            headerEnd += 2;
        } else {
            headerEnd = 0;
        }
    }

    if (headerEnd < rawData.size()) {
        body = rawData.substr(headerEnd);
    }

    int currentBodySize = body.size();
    char buffer[4096];

    while (currentBodySize < contentLength) {
        int bytesRead = read(clientfd, buffer, sizeof(buffer));

        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000); // 10ms
                continue;
            }
            std::cerr << "read failed: " << strerror(errno) << std::endl;
			break;
        }
        if (bytesRead == 0) {
            std::cerr << "Connection closed before full read" << std::endl;
			break;
        }

        body.append(buffer, bytesRead);
        currentBodySize += bytesRead;
    }

    _contentLength = currentBodySize;
    return body;
}

void Request::setupValues(std::string key, std::string value) {
	if (key == "Host")
		setHost(value);
	else if (key == "User-Agent")
		setUserAgent(value);
	else if (key == "Connection")
		setConnection(value);
	else if (key == "Content-Length")
		setContentLength(value);
	else if (key == "Content-Type")
		setContentType(value);
}


int Request::parseHeader(char buffer[]) {
    std::string buff = buffer;
    std::string line;
    std::istringstream stream(buff);
    bool firstLine = true;

    _contentLength = 0;
    _statusCode = 0;

    // Read HTTP header
    bool isBody = false;
    std::stringstream bodyStream;
    
    while (std::getline(stream, line)) {
        if (line == "\r") {
            isBody = true;
            continue;
        }

        if (isBody) {
            bodyStream << line << "\n";
        } else {
            if (firstLine) {
                std::stringstream firstLineStream(line);
                firstLineStream >> _method >> _location >> _versionHttp;

                size_t queryPos = _location.find('?');
                if (queryPos != std::string::npos) {
                    _queryString = _location.substr(queryPos + 1);
                    _location = _location.substr(0, queryPos);
                } else {
                    _queryString = "";
                }
                firstLine = false;
            } else {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);

                    size_t start = key.find_first_not_of(" \t");
                    size_t end = key.find_last_not_of(" \t");
                    key = key.substr(start, end - start + 1);

                    start = value.find_first_not_of(" \t");
                    end = value.find_last_not_of(" \t");
                    value = value.substr(start, end - start + 1);

                    setupValues(key, value);
                }
            }
        }
    }
    
    return 0;
}
