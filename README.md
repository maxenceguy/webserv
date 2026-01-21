<!-- # Webserv -->
<p align="center">
  <img src="./doc/title.png" alt="Webserv Title">
</p>


A high-performance HTTP server implemented in C++ 98, compliant with the 42 School curriculum.

![Webserv Interface](./doc/preview.png)

## Description

Webserv is a fully functional HTTP server capable of handling multiple client connections simultaneously. It supports standard HTTP methods, CGI execution, and is highly configurable via a configuration file.

## Features

- **HTTP Methods**: Supports `GET`, `POST`, and `DELETE` requests.
- **I/O Multiplexing**: Uses `epoll` for non-blocking I/O operations.
- **CGI Support**: Executes CGI scripts (e.g., Python) for dynamic content.
- **Configuration**: Customizable server settings via `.conf` files (ports, roots, error pages, etc.).
- **Static Website Hosting**: Serves static HTML, CSS, and image files.
- **File Management**: Allows file uploads and deletions via the web interface.
- **Error Handling**: Custom default error pages (404, 500, etc.).

## Installation

To compile the project, simply run:

```bash
make
```

This will generate the `webserv` executable.

## Usage

Start the server by providing a configuration file:

```bash
./webserv config/default.conf
```

Once started, you can access the server in your browser (default: `http://localhost:8080`).

## Configuration

The server is configured using a file (e.g., `config/default.conf`). Key configuration options include:

- `listen`: Port to listen on.
- `host`: Host address (e.g., 127.0.0.1).
- `server_name`: Name of the server.
- `root`: Root directory for the website.
- `index`: Default index file.
- `error_page`: Custom error pages.
- `client_max_body_size`: Limit for client body size.
- `location`: specific routing rules (methods allowed, CGI paths, etc.).

## Project Structure

- `src/`: Source code files (.cpp).
- `include/`: Header files (.hpp).
- `config/`: Configuration files.
- `websites/`: Website content and CGI scripts.
- `Makefile`: Compilation rules.

## Authors

- [maxenceguy](https://github.com/maxenceguy)
- [toji-42](https://github.com/toji-42)
