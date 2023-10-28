#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {
    //to be edited later to 6
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << "<serverIP:port>  <sourceCodeFileTobeGraded>  <loopNum> <sleepTimeSeconds> <timeout-seconds>" << std::endl;
        return 1;
    }

    std::string IPPORTSERVER = argv[1];
    std::string sourceFileName = argv[2];
    int numIterations = std::atoi(argv[3]);
    int sleepTime = std::atoi(argv[4]);
    int timeoutTime = std::atoi(argv[5]);
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
    int totNo = 0;
       

    for (int i = 0; i < numIterations; ++i){
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

        // Measure the start time
        struct timeval start, end, timeout;
        gettimeofday(&start, NULL);

        // Read the content of the source code file
        std::ifstream sourceFile(sourceFileName);
        if (!sourceFile) {
            std::cerr << "Error opening source code file" << std::endl;
            close(SocketForClient);
            return 1;
        }

        std::string sourceCodeContent((std::istreambuf_iterator<char>(sourceFile)),
                                       std::istreambuf_iterator<char>());

        // Send the request and source code content to the server
        std::string request = sourceCodeContent;
        send(SocketForClient, request.c_str(), request.size(), 0);

        // Receive and display the server response
        
        char buffer[1024];
        timeout.tv_sec = timeoutTime;
        if (setsockopt(SocketForClient,SOL_SOCKET,SO_RCVTIMEO,(struct timeval *)&timeout,sizeof(struct timeval)) < 0){
            std::cerr << "Timeout has occured"<<std::endl;
        }
        
        ssize_t bytesRead = recv(SocketForClient, buffer, sizeof(buffer), 0);
        
        // Measure the end time
        gettimeofday(&end, NULL);

        if (bytesRead <= 0) {
            std::cerr << "Error in Receiving" << std::endl;
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                totNo++; 
                std::cout<< "Timeout Occured";
            }
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
        close(SocketForClient);

        // Sleep between iterations
        if((i+1) != numIterations)
        sleep(sleepTime);
    }
    
        

   
    std::cout << "Average Response Time: " << (totalTime / successfulResponses) << " seconds" << std::endl;
    std::cout << "Number of Successful Responses: " << successfulResponses << std::endl;

    return 0;
}
