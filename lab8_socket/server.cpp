#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <thread>
#include <sys/types.h>
#include <vector>

using namespace std;

// Function to compile and execute the source code
std::string CompileAndRun(const std::string& sourceCode,int tid) {
    std::string response;
    

    // Create a temporary source file to store the received source code
    std::string rfilename="received_fd"+std::to_string(tid)+".cpp";
    
    std::ofstream sourceFile(rfilename);
    if (!sourceFile) {
        response = "COMPILER ERROR\nFailed to create a source file for compilation.";
        return response;
    }

    sourceFile << sourceCode;
    sourceFile.close();
    std::string compilefile="compile_output"+std::to_string(tid)+".txt";  //compile output file
    std::string outputfile="program_output"+std::to_string(tid)+".txt";  //program output file
    std::string execfile="executable"+std::to_string(tid); //executable
    std::string expoutput="exp_output"+std::to_string(tid)+".txt";  //expected output file
    std::string diffoutput="diff_output"+std::to_string(tid)+".txt";  //diff_output file

    //copying the expected output file contents to each exp_ouptut file based for a particular thread
    std::string copy_cmd="cp expected_output.txt " + expoutput;
    system(copy_cmd.c_str());

    // Compile the received source code
    std::string execstring="g++ -o " + execfile + " " + rfilename + ">" + compilefile +" 2>&1";
    int compileExitCode = system(execstring.c_str());
 
    if (compileExitCode != 0) {
        std::ifstream compileOutputFile(compilefile);
        std::ostringstream compileOutputContent;
        compileOutputContent << compileOutputFile.rdbuf();
        response = "COMPILER ERROR\n" + compileOutputContent.str();
    } else {
        // Execute the compiled program and capture both stdout and stderr
        std::string runop="./"+ execfile + ">"+ outputfile + " 2>&1";
        int runExitCode = system(runop.c_str());

        if (runExitCode != 0) {
            //handling runtime error
            std::ifstream runOutputFile(outputfile);
            std::ostringstream runOutputContent;
            runOutputContent << runOutputFile.rdbuf();
            response = "RUNTIME ERROR\n" + runOutputContent.str();
        } else {
            // Program executed successfully, compare its output with the expected output
            std::ifstream programOutputFile(outputfile);
            std::ostringstream programOutputContent;
            programOutputContent << programOutputFile.rdbuf();
            std::string programOutput = programOutputContent.str();

            std::ifstream expectedOutputFile(expoutput);
            std::ostringstream expectedOutputContent;
            expectedOutputContent << expectedOutputFile.rdbuf();
            std::string expectedOutput = expectedOutputContent.str();

            if (programOutput == expectedOutput) {
                response = "PASS\n" + programOutput;
            } else {
                // Handle output error
                std::ofstream programOutputFile(outputfile);
                programOutputFile << programOutput;
                programOutputFile.close();
                std:: string diffstring = "diff " + outputfile + " " + expoutput + ">" + diffoutput;
                system(diffstring.c_str());

                std::ifstream diffOutputFile(diffoutput);
                std::ostringstream diffOutputContent;
                diffOutputContent << diffOutputFile.rdbuf();
                response = "OUTPUT ERROR\n" + programOutput + "\n" + diffOutputContent.str();
            }
        }
    }

    return response;
}

void* ClientHandler(void* arg) {
        int clientSocket=*(int *)arg;  //extracting socketFD

            pid_t tid = gettid();  //thread ID
            
            //receiving file size
            size_t filesize;
            ssize_t size_received=recv(clientSocket,&filesize,sizeof(filesize),0);
            cout<<"File size is :"<<filesize<<endl;

            // Receiving file
            char buffer[filesize+1];
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if(bytesRead>0){
            cout<<"Received file"<<endl;
      
            // Convert the received data to a string
            std::string receivedData(buffer);
      
            // Process the request (compile and run) with the received source code
            std::string response = CompileAndRun(receivedData,tid);
            
            // Send the response back to the client
            send(clientSocket, response.c_str(), response.size(), 0);
            }
    close(clientSocket);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);   //extracting port no

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation error");
        return 1;
    }

    // Bind to the specified port  
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Bind error");
        close(serverSocket);
        return 1;
    }

    // Listen for incoming connections
    if (listen(serverSocket, SOMAXCONN) == -1) {
        perror("Listen error");
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;
    //server listening on one port as a listener socket & creates new socket everytime it accepts a connection

    int clients[100];
    int counter=0;

    while (true) {    
        // Accept a connection from a client
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }
        // Create a new thread to handle the client request
        pthread_t thread;
        std::cout<<"thread created"<<clientSocket<<std::endl;
            clients[counter]=clientSocket; //keeping track of the socket FDs
            //create the threads
            if (pthread_create(&thread, NULL, ClientHandler, &clients[counter]) != 0) {
                perror("Thread creation error");
            }
            counter++;
    }

    close(serverSocket);
    return 0;
}
