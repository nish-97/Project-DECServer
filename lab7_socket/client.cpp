#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>" << std::endl;
        return 1;
    }

    std::string IPPORTSERVER = argv[1];   //IP:port
    std::string sourceFileName = argv[2];   //source file name
    int numIterations = std::atoi(argv[3]);  //no. of iterations
    int sleepTime = std::atoi(argv[4]);      //think time

    struct timeval start_cl,end_cl;
    double totalTime_cl=0.0;
    
    // Extract server IP and port from the command line argument
    size_t COLON = IPPORTSERVER.find(':');
    if (COLON == std::string::npos) {
        std::cerr << "Invalid server IP:port format" << std::endl;
        return 1;
    }

    std::string serverIP = IPPORTSERVER.substr(0, COLON);   //IP
    int port = std::atoi(IPPORTSERVER.substr(COLON + 1).c_str());  //port no

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

        struct timeval start, end;
        gettimeofday(&start_cl,NULL); //get starting time of client execution 
        
        for (int i = 0; i < numIterations; ++i) {

        // Read the content of the source code file
        std::ifstream sourceFile(sourceFileName,ios::binary | ios::ate);
        if (!sourceFile) {
            std::cerr << "Error opening source code file" << std::endl;
            close(SocketForClient);
            return -1;
        }


        //get file size
        size_t filesize= sourceFile.tellg();
        sourceFile.seekg(0,std::ios::beg);
        std::cout<<filesize<<std::endl;
        
        //send file size to server
        send(SocketForClient,&filesize, sizeof(filesize),0);


        //read contents of sourceFile linebyline until EOF and copy it to sourceCodeContent
        std::string sourceCodeContent((std::istreambuf_iterator<char>(sourceFile)),
                                       std::istreambuf_iterator<char>());

        // Send the request and source code content to the server
        gettimeofday(&start, NULL); //getting start time of sending a req
        std::string request = sourceCodeContent;
        send(SocketForClient, request.c_str(), request.size(), 0);
        cout<<"File sent to server for grading"<<endl;

        // Receive and display the server response
        char buffer[1024];
        ssize_t bytesRead = recv(SocketForClient, buffer, sizeof(buffer), 0);

        // Measure the end time of receiving a req
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

        // Sleep between iterations
        if((i+1) != numIterations){
            sleep(sleepTime);
            }
    }

        gettimeofday(&end_cl,NULL);   //get end time of end of client execution
        close(SocketForClient);
        totalTime_cl=(end_cl.tv_sec - start_cl.tv_sec) + (end_cl.tv_usec - start_cl.tv_usec) / 1000000.0;   //total client execution time
        std::cout << "Throughput: " << (successfulResponses / totalTime_cl) << " seconds" << std::endl;
        std::cout << "Average Response Time: " << (totalTime / successfulResponses) << " seconds" << std::endl;
        std::cout << "Number of Successful Responses: " << successfulResponses << std::endl;

        return 0;
}

