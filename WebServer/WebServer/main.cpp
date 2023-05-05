#include <winsock2.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

const std::string WEBROOT_DIR = "C:/WebRoot";

std::string getContentType(const std::string& fileName) {
    if (fileName.find(".html") != std::string::npos) {
        return "text/html";
    }
    else if (fileName.find(".css") != std::string::npos) {
        return "text/css";
    }
    else if (fileName.find(".js") != std::string::npos) {
        return "application/javascript";
    }
    else {
        return "application/octet-stream";
    }
}

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void sendResponse(SOCKET clientSocket, const std::string& content, const std::string& contentType, int statusCode = 200) {
    std::stringstream response;
    response << "HTTP/1.1 " << statusCode << " OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "\r\n";
    response << content;
    send(clientSocket, response.str().c_str(), response.str().length(), 0);
}

int main() {
    // Initialize WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Bind socket to port 8888
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(8888);

    result = bind(listenSocket, (SOCKADDR*)&service, sizeof(service));
    if (result == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    result = listen(listenSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Accept incoming connections and serve files
    while (true) {
        // Accept incoming connection
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        // Receive request from client
        char buffer[4096] = {};
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            continue;
        }

        // Print received message
        std::cout << "Received message: " << buffer << std::endl;

        // Parse request
        std::stringstream request(buffer);
        std::string method, url, httpVersion;
        request >> method >> url >> httpVersion;

        std::cout << method << " " << url << " " << httpVersion << std::endl;

        // Serve file
        if (method == "GET") {
            // Remove leading slash from URL
            url = url.substr(1);

            // If URL is empty, set it to "index.html"
            if (url.empty()) {
                url = "index.html";
            }

            // Append URL to web root directory
            std::string filePath = WEBROOT_DIR + "/" + url;

          

            // Check if file exists
            std::ifstream file(filePath);
            if (file.good()) {
                // Read file content
                std::string content = readFile(filePath);

                // Send file content to client
                std::string contentType = getContentType(filePath);
                sendResponse(clientSocket, content, contentType);
            }
            else {
                // File not found
                sendResponse(clientSocket, "File not found", "text/plain", 404);
            }
        }
        else {
            // Unsupported method
            sendResponse(clientSocket, "Method not supported", "text/plain", 405);
        }

        // Close connection
        closesocket(clientSocket);
    }

    // Cleanup WinSock
    closesocket(listenSocket);
    WSACleanup();

    return 0;
}