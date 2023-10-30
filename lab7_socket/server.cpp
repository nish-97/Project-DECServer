#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

    // Compile the received source code
    std::string compileCommand = "g++ -o executable received_source.cpp > compile_output.txt 2>&1";
    int compileExitCode = system(compileCommand.c_str());

    if (compileExitCode != 0) {
        std::ifstream compileOutputFile("compile_output.txt");
        std::ostringstream compileOutputContent;
        compileOutputContent << compileOutputFile.rdbuf();
        response = "COMPILER ERROR\n" + compileOutputContent.str();
    } else {
        // Execute the compiled program and capture both stdout and stderr
        int runExitCode = system("./executable > program_output.txt 2>&1");

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

    if (listen(serverSocket, SOMAXCONN) == -1) {
        perror("Listen error");
        close(serverSocket);
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
        }

        int error; 
        socklen_t len = sizeof(error);
        
        while(true){
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        int res = getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, &error,&len);
        if (bytesRead == 0) {
        //std::cout<<"i am here"<<std::endl;
            break;
        }

        if(bytesRead < 0){
            continue;
        }

        // Convert the received data to a string
        std::string receivedData(buffer);

        // Process the request (compile and run) with the received source code
        std::string response = CompileAndRun(receivedData);

        // Send the response back to the client
        send(clientSocket, response.c_str(), response.size(), 0);
    }
        close(clientSocket);
        //std::cout<<"i am here2"<<std::endl;

  }
    close(serverSocket);
    return 0;
}
