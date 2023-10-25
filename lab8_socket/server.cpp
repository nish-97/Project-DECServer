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



/*Add .gitignore*/

std::string CompileAndRun(const std::string& sourceCode,int clsocket) {
    std::string response;
    

    // Create a temporary source file to store the received source code
    std::string rfilename="received_fd"+std::to_string(clsocket)+".cpp";
    
    std::ofstream sourceFile(rfilename);
    if (!sourceFile) {
        response = "COMPILER ERROR\nFailed to create a source file for compilation.";
        return response;
    }

    sourceFile << sourceCode;
    sourceFile.close();
    std::string compilefile="compile_output"+std::to_string(clsocket)+".txt";
    std::string outputfile="program_output"+std::to_string(clsocket)+".txt";
    std::string execfile="executable"+std::to_string(clsocket);
    std::string expoutput="exp_output"+std::to_string(clsocket)+".txt";
    std::string diffoutput="diff_output"+std::to_string(clsocket)+".txt";

    std::string copy_cmd="cp expected_output.txt " + expoutput;
    system(copy_cmd.c_str());

    std::string execstring="g++ -o " + execfile + " " + rfilename + ">" + compilefile +" 2>&1";
    // std::cout<<execstring<<std::endl;
    // Compile the received source code
    //std::string compileCommand = "g++ -o executable received_source.cpp > compile_output.txt 2>&1";
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
        int clientSocket=*(int *)arg;

            pid_t tid = gettid();
            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            std::cout<<"rs"<<clientSocket<<" "<<tid<<std::endl;
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
          
            std::cout<<"re"<<clientSocket<<" "<<tid<<std::endl;
            if(bytesRead>0){
            std::string receivedData(buffer);
            std::string response = CompileAndRun(receivedData,clientSocket);
            
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

    int port = std::atoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation error");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Bind error");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 20) == -1) {
        perror("Listen error");
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    int clients[100];
    int counter=0;

    while (true) {    
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }
        // Create a new thread to handle the client request
        pthread_t thread;
        std::cout<<"threadcreate"<<clientSocket<<std::endl;
            clients[counter]=clientSocket;

            if (pthread_create(&thread, NULL, ClientHandler, &clients[counter]) != 0) {
                std::cout<<"i am here2"<<std::endl;
                perror("Thread creation error");
            }
            counter++;
    }

    close(serverSocket);
    return 0;
}
