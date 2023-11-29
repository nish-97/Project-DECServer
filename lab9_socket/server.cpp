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
int poolReady[THREAD_POOL_MAX]; //keeps track of when a thread is free
//set of locks for each thread, and handling sharedq
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
    std::string compilefile="compile_output"+std::to_string(threadid)+".txt";  //compile output file
    std::string outputfile="program_output"+std::to_string(threadid)+".txt";   //program output file
    std::string execfile="executable"+std::to_string(threadid);    //executable
    std::string expoutput="exp_output"+std::to_string(threadid)+".txt";  //expected output file
    std::string diffoutput="diff_output"+std::to_string(threadid)+".txt";  //diff_output file

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
        int id=*(int *)arg;  //extracting index of sharedq
        
            pid_t tid = gettid();  //thread ID

            pthread_mutex_init(&lock[id],NULL);
            pthread_cond_init(&poolRdy[id],NULL);

            cout << "Thread created and waiting for grading task "<< tid <<endl;
            while(1){    
                pthread_mutex_lock(&lock[id]);
                while(poolReady[id] == 0) //wait on the condition until this thread is free
                    pthread_cond_wait(&poolRdy[id],&lock[id]);
                pthread_mutex_unlock(&lock[id]);

                //receiving file size
                
                int fd = index_sockfd.find(id)->second;   //extracting the sockfd   
                size_t filesize;
                ssize_t size_received=recv(fd,&filesize,sizeof(filesize),0);
                cout<<"File size is :"<<filesize<<endl;

                // Receiving file
                char buffer[filesize+1];
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
            
                
                if(bytesRead>0){
                std::cout<<"Received file"<<endl;
                // Convert the received data to a string
                std::string receivedData(buffer);
                // Process the request (compile and run) with the received source code
                std::string response = CompileAndRun(receivedData,tid);
                // Send the response back to the client
                send(fd, response.c_str(), response.size(), 0);
                }
                pthread_mutex_lock(&clr);
                    close(fd);                 //close the client sockFD
                    index_sockfd.erase(id);    //erase this entry from the map
                    poolReady[id] = 0;    //alert the entry of this index that this thread is free
                pthread_mutex_unlock(&clr);
            }
}

void* qCheck(void* arg) {
    //this function manages the requests in queue by continually
    //tracking if any thread is free which can be assigned to a req in sharedq
    int threadCount=*(int *)arg; //no of threads
    while(1){
        int zero_index=if_zero(threadCount);                
        pthread_mutex_lock(&sharedq_lock);   
        while(sharedq.size()==0)              //wait until there's a req in sharedq
            pthread_cond_wait(&sharedq_condvar,&sharedq_lock);
        pthread_mutex_unlock(&sharedq_lock);
           
        
            if(zero_index==-1){          //req in sharedq but no thread is free
                continue;
            }
            else if(zero_index!=-1){    //req in sharedq and atleast a thread is available
                index_sockfd.insert(pair<int,int>(zero_index,sharedq.front()));  //insert this as a map entry<index,sockfd>
                pthread_mutex_lock(&lock[zero_index]);
                    poolReady[zero_index]=1;  //signal the thradhandler to take up this req
                pthread_cond_signal(&poolRdy[zero_index]);
                pthread_mutex_unlock(&lock[zero_index]);
                sharedq.pop();  //remove the req from sharedq
            }
    }        
}

void* reqinqueue(void *arg){
//this function keeps track of the no. of requests in the sharedq    
    while(1){
        pthread_mutex_lock(&sharedq_lock);
        while(sharedq.size()==0)  //operate only when there is atleast one request in the sharedq
            pthread_cond_wait(&sharedq_condvar,&sharedq_lock);
        pthread_mutex_unlock(&sharedq_lock);

        reqinq<<"Average no. of requests in queue are: " << sharedq.size() <<endl;  //updating the no. of req in sharedq
        sleep(2);  //monitor after sometime
    }
}

int if_zero(int threads){
    //this function returns the index in the sharedq if that thread is free
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

    int port = std::atoi(argv[1]);  //extracting port no
    int noThreads = std::atoi(argv[2]);  //extracting thread pool size
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
    int arr[noThreads]; 
    pthread_t threads[noThreads];  //array of threads
    memset(poolReady, -1, sizeof(poolReady)); //set all entries to -1 in the thread status queue
    memset(poolReady, 0, noThreads*sizeof(int));  //initialise only those threads to 0(free) that exist
    
    for(int i=0;i<noThreads;i++)
        arr[i]=i;

    for(int i=0;i<noThreads;i++)
    {
        int rc = pthread_create(&threads[i], NULL, ClientHandler, (void *)&arr[i]);
        assert(rc == 0);
    }
    
    reqinq.open("avg_req_in_queue.txt"); //open the text file keeping count of avg reqs in queue
    if(!reqinq){
        cerr<<"File could not be opened"<<endl;
        exit(1);
    }

    pthread_mutex_init(&clr,NULL);
    pthread_mutex_init(&sharedq_lock,NULL);
    pthread_cond_init(&sharedq_condvar,NULL);

    pthread_t qcheck,req_in_q;
    //create thread to monitor the status of the thread and assigns it a req whenever free
    int rc = pthread_create(&qcheck, NULL, qCheck, (void *)&noThreads);
    assert(rc == 0);
    //create thread to monitor the no of reqs in sharedq
    rc = pthread_create(&req_in_q, NULL, reqinqueue, NULL);
    assert(rc == 0);

    int zero_index;
    while (true) {
        // Accept a connection from a client
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1) {
            perror("Accept error");
            continue;
            }

        if(sharedq.size()==0){ //new client req that updates sharedq size from 0->1
        pthread_mutex_lock(&sharedq_lock);
            sharedq.push(clientSocket);
        pthread_cond_broadcast(&sharedq_condvar); //broadcast all threads waiting on this condition variable
        pthread_mutex_unlock(&sharedq_lock);
        }
        else 
        sharedq.push(clientSocket);  //sharedq size is already > 0
    }

    close(serverSocket);
    return 0;
}
