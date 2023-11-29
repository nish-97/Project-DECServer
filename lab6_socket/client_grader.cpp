#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
using namespace std;


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <serverIP:port> <sourceCodeFileTobeGraded>" << std::endl;
        return 1;
    }

    std::string serverIPPort = argv[1];  //IP:port
    std::string sourceFileName = argv[2];  //source file name

    // Extract server IP and port from the command line argument
    size_t colonPos = serverIPPort.find(':');
    if (colonPos == std::string::npos) {
        std::cerr << "Invalid server IP:port format" << std::endl;
        return 1;
    }

    //extracting IP and port number
    std::string serverIP = serverIPPort.substr(0, colonPos);
    int port = std::atoi(serverIPPort.substr(colonPos + 1).c_str());

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation error");
        return 1;
    }

    // Data structure for socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0) {
        perror("Invalid server address");
        close(clientSocket);
        return 1;
    }

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Connection error");
        close(clientSocket);
        return 1;
    }

      // Read the content of the source code file
        std::ifstream sourceFile(sourceFileName,ios::binary | ios::ate);
        if (!sourceFile) {
            std::cerr << "Error opening source code file" << std::endl;
            close(clientSocket);
            return -1;
        }

        //get file size
        size_t filesize= sourceFile.tellg();
        sourceFile.seekg(0,std::ios::beg);
        std::cout<<filesize<<std::endl;
        
        //send file size to server
        send(clientSocket,&filesize, sizeof(filesize),0);

        //read contents of sourceFile linebyline until EOF and copy it to sourceCodeContent
        std::string sourceCodeContent((std::istreambuf_iterator<char>(sourceFile)),
                                       std::istreambuf_iterator<char>());

        // Send the request and source code content to the server
        std::string request = sourceCodeContent;
        send(clientSocket, request.c_str(), request.size(), 0);
        cout<<"File sent to server for grading"<<endl;

    // Receive and display the server response
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        std::cerr << "Error receiving response from server" << std::endl;
    } else {
        buffer[bytesRead] = '\0';
        std::cout << buffer << std::endl;
    }

    close(clientSocket);
    return 0;
}

