#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>" << std::endl;
        return 1;
    }

    std::string IPPORTSERVER = argv[1];
    std::string sourceFileName = argv[2];
    int numIterations = std::atoi(argv[3]);
    int sleepTime = std::atoi(argv[4]);

    // Extract server IP and port from the command line argument
    size_t COLON = IPPORTSERVER.find(':');
    if (COLON == std::string::npos) {
        std::cerr << "Invalid server IP:port format" << std::endl;
        return 1;
    }

    std::string serverIP = IPPORTSERVER.substr(0, COLON);
    int port = std::atoi(IPPORTSERVER.substr(COLON + 1).c_str());

    double totalTime = 0.0;
    int successfulResponses = 0;

    // Create a socket
    int SocketForClient = socket(AF_INET, SOCK_STREAM, 0);
        if (SocketForClient == -1) {
            perror("ERROR IN Creating SOCKET");
            return 1;
        }

        // Connect to the server
        struct sockaddr_in AdrrForServer;
        AdrrForServer.sin_family = AF_INET;
        AdrrForServer.sin_port = htons(port);
        if (inet_pton(AF_INET, serverIP.c_str(), &AdrrForServer.sin_addr) <= 0) {
            perror("Server Address Invalid");
            close(SocketForClient);
            return 1;
        }

        if (connect(SocketForClient, (struct sockaddr*)&AdrrForServer, sizeof(AdrrForServer)) == -1) {
            perror("Connection error");
            close(SocketForClient);
            return 1;
        }
    for (int i = 0; i < numIterations; ++i) {

        // Measure the start time
        struct timeval start, end;
        gettimeofday(&start, NULL);

        // Read the content of the source code file
        std::ifstream sourceFile(sourceFileName);
        if (!sourceFile) {
            std::cerr << "Error opening source code file" << std::endl;
            close(SocketForClient);
            return 1;
        }
        //read contents of sourceFile linebyline until EOF and copy it to sourceCodeContent
        std::string sourceCodeContent((std::istreambuf_iterator<char>(sourceFile)),
                                       std::istreambuf_iterator<char>());

        // Send the request and source code content to the server
        std::string request = sourceCodeContent;
        send(SocketForClient, request.c_str(), request.size(), 0);

        // Receive and display the server response
        char buffer[1024];
        ssize_t bytesRead = recv(SocketForClient, buffer, sizeof(buffer), 0);

        // Measure the end time
        gettimeofday(&end, NULL);

        if (bytesRead <= 0) {
            std::cerr << "Error in Receiving" << std::endl;
        } else {
            buffer[bytesRead] = '\0';
            std::cout << buffer << std::endl;

            // Calculate and accumulate the response time
            double responseTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
            totalTime += responseTime;
            successfulResponses++;
            std::cout << "Response Time: " << responseTime << " seconds" << std::endl;
        }

        // Close the socket for this iteration
        

        // Sleep between iterations
        if((i+1) != numIterations){
            sleep(sleepTime);
            }
    }

    close(SocketForClient);
    
    std::cout << "Average Response Time: " << (totalTime / successfulResponses) << " seconds" << std::endl;
    std::cout << "Number of Successful Responses: " << successfulResponses << std::endl;

    return 0;
}

