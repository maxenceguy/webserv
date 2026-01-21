#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "includes.hpp"

class Request {
	private:
		std::string _method;
		std::string _location;
		std::string _versionHttp;
		std::string _host;
		std::string _userAgent;
		std::string _accept;
		std::string _acceptLang;
		std::string _acceptEncod;
		std::string _connection;
		std::string _upgradeIncReq;
		int _contentLength;
		std::string _contentType;
		std::string _queryString;
		
		std::string _body;

		int			_statusCode;
		
	public:
		Request();

		int parseHeader(char buffer[]);
		void setupValues(std::string key, std::string value);

		void setHost(std::string value);
		void setUserAgent(std::string value);
		void setConnection(std::string value);
		void setContentLength(std::string value);
		void setStatusCode(int value);
		void setContentType(std::string value);
		void setQueryString(const std::string& queryString);
		void setBody(const std::string& body);

		std::string getMethod();
		std::string getLocation();
		std::string getVersionHttp();
		std::string getHost();
		std::string getUserAgent();
		std::string getConnection();
		int			getContentLength();
		int			getStatusCode();
		std::string getContenttype();
		std::string getBody();
		std::string getQueryString() const;
		
		std::string readFullRequest(int clientfd, int contentLength, int firstBytesRead, char firstBuffer[]);

};

#endif
