/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Method.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kellen <kellen@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/11 23:13:55 by kellen            #+#    #+#             */
/*   Updated: 2025/06/24 01:05:17 by kellen           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "WebServ.hpp"
#include <pthread.h>

struct SaveFileArgs {
    std::string request;
    size_t      contentStart;
    size_t      contentLength;
    std::string filePath;
};

static void* saveFileThread(void* arg) {
    SaveFileArgs* args = static_cast<SaveFileArgs*>(arg);
    writeFileToServer(args->request, args->contentStart, args->contentLength,
                      args->filePath);
    delete args;
    return NULL;
}

void handleGet(int fd, const std::string& path, const LocationConfig& location, const ServerConfig& config) {
	std::cout << "📥 Handling GET request for " << path << std::endl;

	// First: Check if this is a CGI request FIRST (highest priority)
	if (path.find("/cgi-bin/") == 0) {
		std::cout << "🔧 This is a CGI GET request, calling handleSimpleCGI" << std::endl;

		// Create a simple GET request object for CGI
		std::string requestLine = "GET " + path + " HTTP/1.1\r\n\r\n";
		Request req(requestLine);

		// Call our improved handleSimpleCGI function
		handleSimpleCGI(fd, req, path, config);
		return;
	}

	// NEW: Handle API endpoint for listing uploaded files
	if (path == "/api/photos" || path == "/api/files") {
		std::string uploadDir = "www/upload"; // or use location.upload_path
		std::string jsonResponse = generateJsonDirectoryListing(uploadDir);

		// Send JSON response
		std::string response = Response::build(200, jsonResponse, "application/json");
		ssize_t sent = send(fd, response.c_str(), response.size(), 0);
		if (sent != (ssize_t)response.size()) {
			std::cerr << "❌ Failed to send JSON response\n";
		}
		return;
	}

	// Check if path is a directory and autoindex is enabled
	std::string fullPath = location.root + path;
	std::cout << "🔧 DEBUG: fullPath = '" << fullPath << "'" << std::endl;


	if (isDirectory(fullPath)) {
		std::cout << "📁 Path is directory" << std::endl;
		if (location.autoindex) {
			std::cout << "📁 Serving directory listing for " << path << std::endl;
			// For now, send a simple directory listing instead of complex function
			std::string body = generateSimpleDirectoryListing(fullPath, path);
			sendHtmlResponse(fd, 200, body);
		} else {
			// Try to serve index file
			std::string indexPath = fullPath + "/" + config.index;
			if (fileExists(indexPath)) {
				serveStaticFile(indexPath, fd, config);
			} else {
				std::cout << "❌ Directory access forbidden: " << path << std::endl;
				std::string body = getErrorPageBody(403, config);
				sendHtmlResponse(fd, 403, body);
			}
		}
	} else{
		// Serve static file
		std::cout << "📄 Calling serveStaticFile for: " << path << std::endl;
		serveStaticFile(path, fd, config);
		std::cout << "✅ serveStaticFile call completed" << std::endl;
	}
}

void handlePost(int fd, const Request& req, const std::string& path, const LocationConfig& location, const ServerConfig& config) {
	std::cout << "📤 Handling POST request for " << path << std::endl;

	(void)location; // Suppress unused warning, as location is not used in this example

	// Check if this is a file upload
	if (path == "/upload" || path.find("/upload") == 0) {
		std::cout << "📁 This is a file upload request" << std::endl;
		// Simple file upload handling - you can expand this
		std::string rawRequest = req.getRawRequest();
		handleSimpleUpload(rawRequest, fd, config);
		return;
	}

	// Check if this is a CGI script
	if (path.find("/cgi-bin/") == 0) {
		std::cout << "🔧 This is a CGI POST request, calling handleSimpleCGI" << std::endl;
		// Simple CGI execution - you can expand this
		handleSimpleCGI(fd, req, path, config);
		return;
	}

	// Default POST handling
	// std::string body = "POST request received for: " + path;

	std::cout << "📝 Default POST handling for: " << path << std::endl;
	std::string body = "<html><body><h1>POST Request Received</h1>";
	body += "<p>Path: " + path + "</p>";
	body += "<p>Method: POST</p>";

	// Show some request info
	std::string requestBody = req.getBody();
	if (!requestBody.empty()) {
		body += "<p>Body size: " + intToStr(requestBody.size()) + " bytes</p>";
	}

	body += "<p><a href='/'>← Back to Home</a></p>";
	body += "</body></html>";
	sendHtmlResponse(fd, 200, body);
}

void handlePut(int fd, const Request& req, const std::string& path, const LocationConfig& location, const ServerConfig& config) {
	std::cout << "📝 Handling PUT request for " << path << std::endl;

        // Optional rename feature using header "X-Rename-To"
        const std::map<std::string, std::string>& headers = req.getHeaders();
        std::map<std::string, std::string>::const_iterator renameIt = headers.find("x-rename-to");
        std::string uploadPath = location.upload_path;
        if (uploadPath.empty())
                uploadPath = "www/upload"; // Default upload directory

        if (renameIt != headers.end() && !renameIt->second.empty()) {
                std::string oldName = path;
                if (oldName.find_last_of('/') != std::string::npos)
                        oldName = oldName.substr(oldName.find_last_of('/') + 1);
                std::string newName = renameIt->second;

                if (newName.find("..") != std::string::npos || newName.find('/') != std::string::npos) {
                        std::cout << "❌ Invalid rename target: " << newName << std::endl;
                        std::string body = getErrorPageBody(400, config);
                        sendHtmlResponse(fd, 400, body);
                        return;
                }

                std::string oldPath = uploadPath + "/" + oldName;
                std::string newPath = uploadPath + "/" + newName;
                if (std::rename(oldPath.c_str(), newPath.c_str()) == 0) {
                        std::cout << "✅ File renamed via PUT: " << oldPath << " -> " << newPath << std::endl;
                        sendHtmlResponse(fd, 200, "File renamed successfully");
                } else {
                        std::cout << "❌ Failed to rename file: " << oldPath << std::endl;
                        std::string body = getErrorPageBody(500, config);
                        sendHtmlResponse(fd, 500, body);
                }
                return;
        }
	std::string filename = path;
	if (filename.find_last_of('/') != std::string::npos) {
		filename = filename.substr(filename.find_last_of('/') + 1);
	}

	// Construct full file path

	std::string fullPath = uploadPath + "/" + filename;

	// Validate file path (security check)
	if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
		std::cout << "❌ Invalid filename in PUT request: " << filename << std::endl;
		std::string body = getErrorPageBody(400, config);
		sendHtmlResponse(fd, 400, body);
		return;
	}

	// Create upload directory if it doesn't exist
	createDirectoryIfNotExists(uploadPath);

	// Write file content
	std::string body = req.getBody();
	std::ofstream file(fullPath.c_str(), std::ios::binary);

	if (!file.is_open()) {
		std::cout << "❌ Cannot create file: " << fullPath << std::endl;
		std::string errorBody = getErrorPageBody(500, config);
		sendHtmlResponse(fd, 500, errorBody);
		return;
	}

	file.write(body.c_str(), body.size());
	file.close();

	std::cout << "✅ File uploaded via PUT: " << fullPath << " (" << body.size() << " bytes)" << std::endl;

	// Send success response
	std::string responseBody = "File uploaded successfully: " + filename;
	sendHtmlResponse(fd, 201, responseBody); // 201 Created
}

void handleDelete(int fd, const std::string& path, const LocationConfig& location, const ServerConfig& config) {
	std::cout << "🗑️ Handling DELETE request for " << path << std::endl;

	// Extract filename from path
	std::string filename = path;
	if (filename.find_last_of('/') != std::string::npos) {
		filename = filename.substr(filename.find_last_of('/') + 1);
	}

	// Construct full file path
	std::string uploadPath = location.upload_path;
	if (uploadPath.empty()) {
		uploadPath = "www/upload"; // Default upload directory
	}

	std::string fullPath = uploadPath + "/" + filename;

	// Validate file path (security check)
	if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
		std::cout << "❌ Invalid filename in DELETE request: " << filename << std::endl;
		std::string body = getErrorPageBody(400, config);
		sendHtmlResponse(fd, 400, body);
		return;
	}

	// Check if file exists
	if (!fileExists(fullPath)) {
		std::cout << "❌ File not found for deletion: " << fullPath << std::endl;
		std::string body = getErrorPageBody(404, config);
		sendHtmlResponse(fd, 404, body);
		return;
	}

	// Delete the file
	if (std::remove(fullPath.c_str()) != 0) {
		std::cout << "❌ Failed to delete file: " << fullPath << std::endl;
		std::string body = getErrorPageBody(500, config);
		sendHtmlResponse(fd, 500, body);
		return;
	}

	std::cout << "✅ File deleted: " << fullPath << std::endl;

	// Send success response
	std::string responseBody = "File deleted successfully: " + filename;
	sendHtmlResponse(fd, 200, responseBody); // 200 OK
}

void handleHead(int fd, const std::string& path, const LocationConfig& location, const ServerConfig& config) {
	std::cout << "📋 Handling HEAD request for " << path << std::endl;
	//for now
	(void)config;  // Add this line to suppress warning

	// HEAD is like GET but without the response body
	std::string fullPath = location.root + path;

	if (!fileExists(fullPath)) {
		// Send 404 headers only (no body)
		std::string headers = Response::buildHeader(404, 0, "text/html");
		send(fd, headers.c_str(), headers.size(), 0);
		return;
	}

	// Get file info
	std::ifstream file(fullPath.c_str(), std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::string headers = Response::buildHeader(500, 0, "text/html");
		send(fd, headers.c_str(), headers.size(), 0);
		return;
	}

	size_t fileSize = file.tellg();
	file.close();

	// Determine content type
	std::string contentType = Response::getContentType(fullPath);

	// Send headers only (no body for HEAD request)
	std::string headers = Response::buildHeader(200, fileSize, contentType);
	send(fd, headers.c_str(), headers.size(), 0);

	std::cout << "✅ HEAD response sent for " << path << " (size: " << fileSize << ")" << std::endl;
}

// Helper functions

bool fileExists(const std::string& path) {
	std::ifstream file(path.c_str());
	return file.good();
}

bool isDirectory(const std::string& path) {
	struct stat statbuf;
	if (::stat(path.c_str(), &statbuf) != 0) {  // Use :: to call global stat function
		return false;
	}
	return S_ISDIR(statbuf.st_mode);
}

void createDirectoryIfNotExists(const std::string& path) {
	struct stat st;
	if (::stat(path.c_str(), &st) == -1) {  // Use :: to call global stat function
		::mkdir(path.c_str(), 0755);  // Use :: to call global mkdir function
	}
}

// Simple directory listing
std::string generateSimpleDirectoryListing(const std::string& dirPath, const std::string& urlPath) {
	std::ostringstream html;

	html << "<!DOCTYPE html>\n";
	html << "<html><head><title>Directory: " << urlPath << "</title></head>\n";
	html << "<body><h1>Directory listing for " << urlPath << "</h1>\n";
	html << "<ul>\n";

	// Add parent directory link if not root
	if (urlPath != "/") {
		html << "<li><a href=\"../\">../</a></li>\n";
	}

	// List directory contents (simplified version)
	DIR* dir = opendir(dirPath.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			if (name != "." && name != "..") {
				html << "<li><a href=\"" << name << "\">" << name << "</a></li>\n";
			}
		}
		closedir(dir);
	}

	html << "</ul></body></html>\n";
	return html.str();
}

std::string rewriteURL(const std::string& path, const ServerConfig& config, const std::string& method) {
	std::cout << "🔄 Rewriting URL: " << path << " with method: " << method << std::endl;

	// Handle root path
	if (path == "/") {
		return "/" + config.index;
	}

	// For POST requests to /upload, don't rewrite
	if (method == "POST" && path == "/upload") {
		return path;
	}

	// URL mapping for clean URLs
	std::map<std::string, std::string> urlMap;
	urlMap["/home"] = "/index.html";
	urlMap["/gallery"] = "/gallery.html";
	urlMap["/upload"] = "/upload.html";
	urlMap["/interactive"] = "/interactive.html";
	urlMap["/cookies"] = "/cookie-demo.html";
	urlMap["/about"] = "/about.html";
	urlMap["/contact"] = "/contact.html";
	urlMap["/error"] = "/error/404.html";
	urlMap["/help"] = "/help.html";

	// Check if this is a clean URL that needs rewriting
	std::map<std::string, std::string>::const_iterator it = urlMap.find(path);
	if (it != urlMap.end()) {
		return it->second;
	}

	// Handle directory-style URLs (without trailing slash)
	std::string pathWithSlash = path + "/";
	it = urlMap.find(pathWithSlash);
	if (it != urlMap.end()) {
		return it->second;
	}

	// Check if file exists as-is (for files with extensions)
	std::string fullPath = config.root + path;
	if (fileExists(fullPath)) {
		return path; // File exists, use as-is
	}

	// Try adding .html extension
	std::string htmlPath = path + ".html";
	std::string fullHtmlPath = config.root + htmlPath;
	if (fileExists(fullHtmlPath)) {
		return htmlPath;
	}

	// Try as directory with index.html
	std::string indexPath = path + "/index.html";
	std::string fullIndexPath = config.root + indexPath;
	if (fileExists(fullIndexPath)) {
		return indexPath;
	}

	// Return original path if no rewrite rule found
	return path;
}

void handleSimpleUpload(const std::string& request, int client_fd, const ServerConfig& config) {
	std::cout << "🚀 Starting file upload process..." << std::endl;

	// Step 1: Extract filename
	std::string filename;
	if (!extractFilenameFromRequest(request, filename)) {
		std::cerr << "❌ Failed to extract filename" << std::endl;
		sendHtmlResponse(client_fd, 400, getErrorPageBody(400, config));
		return;
	}
	std::cout << "📁 Extracted filename: " << filename << std::endl;

	// Step 2: Find content boundaries
	size_t contentStart, contentEnd;
	if (!findFileContentBoundaries(request, filename, contentStart, contentEnd)) {
		std::cerr << "❌ Failed to find content boundaries" << std::endl;
		sendHtmlResponse(client_fd, 400, getErrorPageBody(400, config));
		return;
	}

	size_t contentLength = contentEnd - contentStart;
	std::cout << "📏 Content length: " << contentLength << " bytes" << std::endl;

	// Step 3: Validate file size
	if (!validateUploadFileSize(contentLength, config)) {
		std::cerr << "❌ File too large: " << contentLength << " bytes" << std::endl;
		sendHtmlResponse(client_fd, 413, getErrorPageBody(413, config));
		return;
	}
	std::cout << "✅ File size validation passed" << std::endl;

	// // Step 4: Save file to server
	// std::string filePath = config.root + "/upload/" + filename;
	// if (!writeFileToServer(request, contentStart, contentLength, filePath)) {
	// 	std::cerr << "❌ Failed to save file to: " << filePath << std::endl;
	// 	sendHtmlResponse(client_fd, 500, getErrorPageBody(500, config));
	// 	return;
	// }
	// std::cout << "✅ File saved successfully: " << filePath << std::endl;

	// // Step 5: Send success response
	// std::string successResponse = loadAndProcessSuccessTemplate(config, filename);
	// sendHtmlResponse(client_fd, 200, successResponse);

	// std::cout << "📤 Success response sent!" << std::endl;

	// Step 4: SEND SUCCESS RESPONSE IMMEDIATELY (before writing file!)
	std::cout << "⚡ Sending immediate success response..." << std::endl;
        std::string successResponse = loadAndProcessSuccessTemplate(config, filename);
        sendHtmlResponse(client_fd, 200, successResponse);
        shutdown(client_fd, SHUT_WR);
        std::cout << "✅ Success response sent! Saving file asynchronously..." << std::endl;

        std::string filePath = config.root + "/upload/" + filename;
        SaveFileArgs* args = new SaveFileArgs();
        args->request = request;
        args->contentStart = contentStart;
        args->contentLength = contentLength;
        args->filePath = filePath;

        pthread_t tid;
        if (pthread_create(&tid, NULL, saveFileThread, args) == 0) {
                pthread_detach(tid);
        } else {
                std::cerr << "❌ Failed to create save thread, saving synchronously" << std::endl;
                writeFileToServer(request, contentStart, contentLength, filePath);
                delete args;
        }
}

void handleSimpleCGI(int fd, const Request& req, const std::string& path, const ServerConfig& config) {
	std::cout << "🚀 Starting Simple CGI execution for: " << path << std::endl;

	// Step 1: Find the interpreter for this script
	std::string interpreter = getInterpreter(path, config);
	if (interpreter.empty()) {
		std::cout << "❌ No interpreter found for " << path << std::endl;
		std::string errorBody = getErrorPageBody(500, config);
		sendHtmlResponse(fd, 500, errorBody);
		return;
	}
	std::cout << "✅ Found interpreter: " << interpreter << std::endl;

	// Step 2: Build the full path to the script
	std::string scriptPath = config.root + path;

	// Remove query string from script path if present
	size_t queryPos = scriptPath.find('?');
	if (queryPos != std::string::npos) {
		scriptPath = scriptPath.substr(0, queryPos);
	}
	std::cout << "📁 Script path: " << scriptPath << std::endl;
	std::cout << "🔍 Query string: " << req.getQuery() << std::endl;

	// Step 3: Check if the script file exists
	if (!fileExists(scriptPath)) {
		std::cout << "❌ Script file not found: " << scriptPath << std::endl;
		std::string errorBody = getErrorPageBody(404, config);
		sendHtmlResponse(fd, 404, errorBody);
		return;
	}

	if (access(scriptPath.c_str(), X_OK) != 0) {
		std::cout << "⚠️ Script may not be executable, but continuing..." << std::endl;
	}

	// Step 4: Execute the script and capture output
	std::string scriptOutput = executeScript(interpreter, scriptPath, req);

	if (scriptOutput.empty()) {
		std::cout << "❌ Script execution failed or returned empty output" << std::endl;
		std::string errorBody = getErrorPageBody(500, config);
		sendHtmlResponse(fd, 500, errorBody);
		return;
	}

	// Step 5: Send the script output directly to the client
	std::cout << "📤 Sending script output to client" << std::endl;
	ssize_t sent = send(fd, scriptOutput.c_str(), scriptOutput.size(), 0);
	if (sent != (ssize_t)scriptOutput.size()) {
		std::cerr << "❌ Failed to send complete CGI response" << std::endl;
	} else {
		std::cout << "✅ CGI response sent successfully!" << std::endl;
	}
}

// Helper function to execute the script
std::string executeScript(const std::string& interpreter, const std::string& scriptPath, const Request& req) {
	std::cout << "⚙️ Executing: " << interpreter << " " << scriptPath << std::endl;

	// Create pipes for communication
	int outputPipe[2];
	int inputPipe[2];

	if (pipe(outputPipe) == -1 || pipe(inputPipe) == -1) {
		std::cerr << "❌ Failed to create pipes" << std::endl;
		return "";
	}

	// Fork a new process
	pid_t pid = fork();
	if (pid < 0) {
		std::cerr << "❌ Fork failed" << std::endl;
		close(outputPipe[0]);
		close(outputPipe[1]);
		close(inputPipe[0]);
		close(inputPipe[1]);
		return "";
	}

	if (pid == 0) {
		// Child process: execute the script
		std::cout << "👶 Child process: executing script" << std::endl;

		// Redirect stdin and stdout
		dup2(inputPipe[0], STDIN_FILENO);
		dup2(outputPipe[1], STDOUT_FILENO);

		// Close unused pipe ends
		close(outputPipe[0]);
		close(outputPipe[1]);
		close(inputPipe[0]);
		close(inputPipe[1]);

		// Prepare environment variables
		std::vector<std::string> envStrings;
		envStrings.push_back("REQUEST_METHOD=" + req.getMethod());
		envStrings.push_back("QUERY_STRING=" + req.getQuery());
		envStrings.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		envStrings.push_back("CONTENT_LENGTH=" + intToStr(req.getBody().length()));
		envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
		envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
		envStrings.push_back("SCRIPT_NAME=" + scriptPath);
		if (scriptPath.find(".php") != std::string::npos) {
			envStrings.push_back("SCRIPT_FILENAME=" + scriptPath);
			envStrings.push_back("REDIRECT_STATUS=200");
		}

		// HTTP headers
		const std::map<std::string, std::string>& headers = req.getHeaders();
		for (std::map<std::string, std::string>::const_iterator it = headers.begin();
			it != headers.end(); ++it) {
			std::string httpVar = "HTTP_" + it->first;
			// Convert to uppercase and replace - with _
			for (size_t i = 5; i < httpVar.length(); ++i) {
				if (httpVar[i] == '-') httpVar[i] = '_';
				httpVar[i] = std::toupper(httpVar[i]);
			}
			envStrings.push_back(httpVar + "=" + it->second);
		}

		// Convert to char* array for execve
		std::vector<char*> envp;
		for (size_t i = 0; i < envStrings.size(); ++i) {
			envp.push_back(const_cast<char*>(envStrings[i].c_str()));
		}
		envp.push_back(NULL);

		// Prepare command arguments
		char* args[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(scriptPath.c_str()),
			NULL
		};

		// Execute the script
		execve(interpreter.c_str(), args, &envp[0]);

		// If we reach here, execve failed
		std::cerr << "❌ execve failed: " << strerror(errno) << std::endl;
		exit(1);
	} else {
		// Parent process: read the output
		std::cout << "👨‍👩‍👧‍👦 Parent process: reading script output" << std::endl;
		// Close unused pipe ends
		close(inputPipe[0]);
		close(outputPipe[1]);

		// Send POST data to script if any
		std::string body = req.getBody();
		if (!body.empty() && req.getMethod() == "POST") {
			std::cout << "📤 Sending POST data to script (" << body.size() << " bytes)" << std::endl;
			write(inputPipe[1], body.c_str(), body.size());
		}
		close(inputPipe[1]); // Close input pipe

		// Read all output from the script
		std::string output;
                // Use a larger buffer when reading CGI output
                char buffer[8192];
		ssize_t bytesRead;

		while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
			output.append(buffer, bytesRead);
		}

		close(outputPipe[0]);

		// Wait for child process to finish
		int status;
		waitpid(pid, &status, 0);

		if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			std::cout << "✅ Script executed successfully" << std::endl;
		} else {
			std::cout << "⚠️ Script exited with status: " << WEXITSTATUS(status) << std::endl;
		}

		// Format the output as a proper HTTP response
		return formatCGIResponse(output);
	}
}

// Helper function to format CGI output as HTTP response
std::string formatCGIResponse(const std::string& scriptOutput) {
	if (scriptOutput.empty()) {
		std::cout << "⚠️ Script output is empty" << std::endl;
		return "";
	}

	std::cout << "📋 Formatting CGI response (" << scriptOutput.size() << " bytes)" << std::endl;

	// Check if the script already included HTTP headers
	// if (scriptOutput.find("Content-Type:") != std::string::npos) {
        size_t headerEnd = scriptOutput.find("\r\n\r\n");
        size_t altEnd = scriptOutput.find("\n\n");
        if (headerEnd == std::string::npos || (altEnd != std::string::npos && altEnd < headerEnd))
                headerEnd = altEnd;

        size_t ctPos = std::string::npos;
        if (headerEnd != std::string::npos) {
                std::string headerSection = scriptOutput.substr(0, headerEnd);
                std::string lower = headerSection;
                for (size_t i = 0; i < lower.size(); ++i)
                        lower[i] = std::tolower(lower[i]);
                ctPos = lower.find("content-type:");
        }

        if (headerEnd != std::string::npos && ctPos != std::string::npos) {
		// Script provided its own headers, just add HTTP status line
		std::cout << "✅ Script provided its own headers" << std::endl;
		// Script provided its own headers, just add HTTP status line
		return "HTTP/1.1 200 OK\r\n" + scriptOutput;
	} else {
		// Script didn't provide headers, add them
		std::cout << "📝 Adding HTTP headers to script output" << std::endl;
		std::ostringstream response;
		response << "HTTP/1.1 200 OK\r\n";
		response << "Content-Type: text/html\r\n";
		response << "Content-Length: " << scriptOutput.size() << "\r\n";
		response << "Connection: close\r\n";
		response << "\r\n";
		response << scriptOutput;
		return response.str();
	}
}