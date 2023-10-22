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

int k = 1;
pthread_mutex_t lock1;
pthread_mutex_t lock2;
std::string CompileAndRun(const std::string& sourceCode) {
    std::string response;
    

    // Create a temporary source file to store the received source code
    std::ofstream sourceFile("received_source.cpp");
    if (!sourceFile) {
        response = "COMPILER ERROR\nFailed to create a source file for compilation.";
        return response;
    }

    sourceFile << sourceCode;
    sourceFile.close();

    pthread_mutex_lock(&lock1);
    std::string execfile="executable"+std::to_string(k);
    //std::cout<<"i am here4"<<std::endl;
    std::string newstring="g++ -o " + execfile + " received_source.cpp > compile_output.txt 2>&1";
    std::cout<<newstring<<std::endl;
    // Compile the received source code
    //std::string compileCommand = "g++ -o executable received_source.cpp > compile_output.txt 2>&1";
    int compileExitCode = system(newstring.c_str());
    k++;
    pthread_mutex_unlock(&lock1);
    if (compileExitCode != 0) {
        std::ifstream compileOutputFile("compile_output.txt");
        std::ostringstream compileOutputContent;
        compileOutputContent << compileOutputFile.rdbuf();
        response = "COMPILER ERROR\n" + compileOutputContent.str();
    } else {
        // Execute the compiled program and capture both stdout and stderr
        std::string execfile1="./"+ execfile + " > program_output.txt 2>&1";
        int runExitCode = system(execfile1.c_str());

        if (runExitCode != 0) {
            std::ifstream runOutputFile("program_output.txt");
            std::ostringstream runOutputContent;
            runOutputContent << runOutputFile.rdbuf();
            response = "RUNTIME ERROR\n" + runOutputContent.str();
        } else {
            // Program executed successfully, compare its output with the expected output
            std::ifstream programOutputFile("program_output.txt");
            std::ostringstream programOutputContent;
            programOutputContent << programOutputFile.rdbuf();
            std::string programOutput = programOutputContent.str();

            std::ifstream expectedOutputFile("expected_output.txt");
            std::ostringstream expectedOutputContent;
            expectedOutputContent << expectedOutputFile.rdbuf();
            std::string expectedOutput = expectedOutputContent.str();

            if (programOutput == expectedOutput) {
                response = "PASS\n" + programOutput;
            } else {
                // Handle output error
                std::ofstream programOutputFile("program_output.txt");
                programOutputFile << programOutput;
                programOutputFile.close();

                system("diff program_output.txt expected_output.txt > diff_output.txt");

                std::ifstream diffOutputFile("diff_output.txt");
                std::ostringstream diffOutputContent;
                diffOutputContent << diffOutputFile.rdbuf();
                response = "OUTPUT ERROR\n" + programOutput + "\n" + diffOutputContent.str();
            }
        }
    }

    return response;
}

void* ClientHandler(void* arg) {
    int clientSocket = *(int*)arg;
    //std::cout<<clientSocket<<std::endl;
    //std::cout<<"i am here"<<std::endl;
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout<<buffer<<std::endl;
    
    if (bytesRead > 0) {
        std::string receivedData(buffer);
        std::string response = CompileAndRun(receivedData);
        send(clientSocket, response.c_str(), response.size(), 0);
    }
    std::cout<<"end"<<clientSocket<<std::endl;
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

    if (listen(serverSocket, 10) == -1) {
        perror("Listen error");
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;
    pthread_mutex_init(&lock1,NULL);

    while (true) {    
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }
        std::cout<<"start"<<clientSocket<<std::endl;
        // Create a new thread to handle the client request
        pthread_t thread;
        //std::cout<<"i am here2"<<std::endl;
        if (pthread_create(&thread, NULL, ClientHandler, &clientSocket) != 0) {
            std::cout<<"i am here2"<<std::endl;
            perror("Thread creation error");
            close(clientSocket);
        }

    }

    close(serverSocket);
    return 0;
}
