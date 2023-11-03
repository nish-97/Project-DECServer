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
#include <assert.h>
#include <queue>
#include <map>

using namespace std;

int poolReady[100];
pthread_mutex_t lock[100],clr;
pthread_cond_t poolRdy[100];
std::queue<int> sharedq;
std::map<int,int> index_sockfd;


std::string CompileAndRun(const std::string& sourceCode,int threadid) {
    std::string response;
    

    // Create a temporary source file to store the received source code
    std::string rfilename="received_fd"+std::to_string(threadid)+".cpp";
    
    std::ofstream sourceFile(rfilename);
    if (!sourceFile) {
        response = "COMPILER ERROR\nFailed to create a source file for compilation.";
        return response;
    }

    sourceFile << sourceCode;
    sourceFile.close();
    std::string compilefile="compile_output"+std::to_string(threadid)+".txt";
    std::string outputfile="program_output"+std::to_string(threadid)+".txt";
    std::string execfile="executable"+std::to_string(threadid);
    std::string expoutput="exp_output"+std::to_string(threadid)+".txt";
    std::string diffoutput="diff_output"+std::to_string(threadid)+".txt";

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
        int id=*(int *)arg;

            pid_t tid = gettid();
            
            std::cout<<"debugs "<<tid<<std::endl;
            pthread_mutex_init(&lock[id],NULL);
            pthread_cond_init(&poolRdy[id],NULL);

            cout << "Thread %d created and waiting for grading task "<< tid <<endl;

            while(1){    
                pthread_mutex_lock(&lock[id]);
                while(poolReady[id] == 0)
                    pthread_cond_wait(&poolRdy[id],&lock[id]);
                pthread_mutex_unlock(&lock[id]);
                char buffer[1024];
                memset(buffer, 0, sizeof(buffer));
                int fd = index_sockfd.find(id)->second;  
                ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
            
                std::cout<<"debuge "<<tid<<std::endl;
                if(bytesRead>0){
                std::string receivedData(buffer);
                std::string response = CompileAndRun(receivedData,tid);
                
                send(fd, response.c_str(), response.size(), 0);
                }
                pthread_mutex_lock(&clr);
                    close(fd);
                    index_sockfd.erase(id);
                    poolReady[id] = 0;
                pthread_mutex_unlock(&clr);
            }
}

int if_zero(int threads){
    for(int i=0;i<threads;i++){
        if(poolReady[i] == 0)
            return i;
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <thread_pool_size>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    int noThreads = std::atoi(argv[2]);
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

    // int clients[100];
    // int counter=0;
    pthread_t threads[noThreads];
    memset(poolReady, -1, sizeof(poolReady));
    
    for(int i=0;i<noThreads;i++)
    {
        poolReady[i] = 0;
        int rc = pthread_create(&threads[i], NULL, ClientHandler, (void *)&i);
        assert(rc == 0);
    }
    
    pthread_mutex_init(&clr,NULL);

    
    int zero_index;
    while (true) {    
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }

        sharedq.push(clientSocket);
        zero_index=if_zero(noThreads);

        if(zero_index != -1){
            index_sockfd.insert(pair<int,int>(zero_index,clientSocket));
            pthread_mutex_lock(&lock[zero_index]);
                poolReady[zero_index]=1;
            pthread_cond_signal(&poolRdy[zero_index]);
            pthread_mutex_unlock(&lock[zero_index]);
            sharedq.pop();
        }
        else 
            cout<<"All threads are working on grading requests, your grading request has to wait"<<endl;
            continue;
            //wait logic
    }

    close(serverSocket);
    return 0;
}
