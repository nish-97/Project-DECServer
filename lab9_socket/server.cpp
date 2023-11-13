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
#include "server.h"

using namespace std;
int poolReady[THREAD_POOL_MAX];
pthread_mutex_t lock[THREAD_POOL_MAX],clr,sharedq_lock;
pthread_cond_t poolRdy[THREAD_POOL_MAX],sharedq_condvar;
std::queue<int> sharedq;
std::ofstream reqinq;
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
        // cout<<"this is the index received"<<id;
            pid_t tid = gettid();
            
            // std::cout<<"debugs "<<tid<<std::endl;
            pthread_mutex_init(&lock[id],NULL);
            pthread_cond_init(&poolRdy[id],NULL);

            cout << "Thread created and waiting for grading task "<< tid <<endl;
            //cout<<"after creating thread"<<poolReady[id]<<endl;
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

void* qCheck(void* arg) {
    int threadCount=*(int *)arg;
    //int tries=5;
    while(1){
        int zero_index=if_zero(threadCount);                
        pthread_mutex_lock(&sharedq_lock);
        while(sharedq.size()==0)              //no req in sharedq
            pthread_cond_wait(&sharedq_condvar,&sharedq_lock);
        pthread_mutex_unlock(&sharedq_lock);
           
        
            if(zero_index==-1){          //req in sharedq but no thread is free
                // sleep(5);
                continue;
            }
            else if(zero_index!=-1){    //req in sharedq and atleast a thread is available
                index_sockfd.insert(pair<int,int>(zero_index,sharedq.front()));
                pthread_mutex_lock(&lock[zero_index]);
                    poolReady[zero_index]=1;
                pthread_cond_signal(&poolRdy[zero_index]);
                pthread_mutex_unlock(&lock[zero_index]);
                sharedq.pop();
            }
    }        
}

void* reqinqueue(void *arg){
    
    while(1){
        pthread_mutex_lock(&sharedq_lock);
        while(sharedq.size()==0)
            pthread_cond_wait(&sharedq_condvar,&sharedq_lock);
        pthread_mutex_unlock(&sharedq_lock);

        reqinq<<"Average no. of requests in queue are: " << sharedq.size() <<endl;
        sleep(2);
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

    int arr[noThreads];
    pthread_t threads[noThreads];
    memset(poolReady, -1, sizeof(poolReady));
    memset(poolReady, 0, noThreads*sizeof(int));
    
    for(int i=0;i<noThreads;i++)
        arr[i]=i;

    for(int i=0;i<noThreads;i++)
    {
        //cout<<"before calling thread"<<poolReady[i]<<endl;
        int rc = pthread_create(&threads[i], NULL, ClientHandler, (void *)&arr[i]);
        assert(rc == 0);
    }
    
    reqinq.open("avg_req_in_queue.txt");
    if(!reqinq){
        cerr<<"File could not be opened"<<endl;
        exit(1);
    }

    pthread_mutex_init(&clr,NULL);
    pthread_mutex_init(&sharedq_lock,NULL);
    pthread_cond_init(&sharedq_condvar,NULL);

    pthread_t qcheck,req_in_q;
    int rc = pthread_create(&qcheck, NULL, qCheck, (void *)&noThreads);
    assert(rc == 0);
    rc = pthread_create(&req_in_q, NULL, reqinqueue, NULL);
    assert(rc == 0);

    int zero_index;
    while (true) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }
        if(sharedq.size()==0){
        pthread_mutex_lock(&sharedq_lock);
            sharedq.push(clientSocket);
        pthread_cond_broadcast(&sharedq_condvar);
        pthread_mutex_unlock(&sharedq_lock);
        }
        else 
        sharedq.push(clientSocket);
    }

    close(serverSocket);
    return 0;
}
