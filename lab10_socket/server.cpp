#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <map>
#include <fstream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

// Map to store the status of each request ID
map<string, string> idStatusMap;

// Mutex and condition variable for managing the task queue
pthread_mutex_t lockQueue;
pthread_cond_t cv;

// Queue to hold the request IDs awaiting processing
queue<string> taskqueue;

// Function to compile and run the received C++ code
void CompileAndRun(const string &requestID)
{
    string keyToFind = requestID;
    auto it = idStatusMap.find(keyToFind);

    // Update status map to mark the request as 'processing'
    if (it != idStatusMap.end())
    {
        it->second = "Under process";
    }

    string response;
    string folderPath = "./" + requestID + "/";
    string sourcePath = folderPath + "received_code.cpp";
    string execPath = folderPath + "executable > " + folderPath + "program_output.txt";

    // Compile the source code and capture both stdout and stderr
    string compileCommand = "g++ -o " + execPath + " " + sourcePath + " > " + folderPath + "compile_output.txt 2>&1";
    int compileExitCode = system(compileCommand.c_str());

    if (compileExitCode != 0)
    {
        ifstream compileOutputFile(folderPath + "compile_output.txt");
        ostringstream compileOutputContent;
        compileOutputContent << compileOutputFile.rdbuf();
        response = "COMPILER ERROR\n" + compileOutputContent.str();
    }
    else
    {
        // Execute the compiled program and capture both stdout and stderr
        int runExitCode = system(execPath.c_str());

        if (runExitCode != 0)
        {
            ifstream runOutputFile(folderPath + "program_output.txt");
            ostringstream runOutputContent;
            runOutputContent << runOutputFile.rdbuf();
            response = "RUNTIME ERROR\n" + runOutputContent.str();
        }
        else
        {
            // Program executed successfully, compare its output with the expected output
            ifstream programOutputFile(folderPath + "program_output.txt");
            ostringstream programOutputContent;
            programOutputContent << programOutputFile.rdbuf();
            string programOutput = programOutputContent.str();
            string copyCmd = "cp expected_output.txt " + folderPath + "expected_output.txt";
            system(copyCmd.c_str());
            ifstream expectedOutputFile(folderPath + "expected_output.txt");
            ostringstream expectedOutputContent;
            expectedOutputContent << expectedOutputFile.rdbuf();
            string expectedOutput = expectedOutputContent.str();
            if (programOutput == expectedOutput)
            {
                response = "PASS\n" + programOutput;
            }
            else
            {
                // Handle output error
                ofstream programOutputFile(folderPath + "program_output.txt");
                programOutputFile << programOutput;
                programOutputFile.close();

                string diffCmd = "diff " + folderPath + "program_output.txt " + folderPath + "expected_output.txt > " + folderPath + "diff_output.txt";
                system(diffCmd.c_str());

                ifstream diffOutputFile(folderPath + "diff_output.txt");
                ostringstream diffOutputContent;
                diffOutputContent << diffOutputFile.rdbuf();
                response = "OUTPUT ERROR\n" + programOutput + "\n" + diffOutputContent.str();
            }
        }
    }

    string responseFilePath = folderPath + "response.txt";
    ofstream responseFile(responseFilePath);
    if (responseFile.is_open())
    {
        responseFile << response;
        responseFile.close();
    }


    // Update status map to mark the request as 'done'
    if (it != idStatusMap.end())
    {
        it->second = "done";
    }
}

// Function to generate a unique request ID based on time
string generateUniqueID()
{
    auto now = chrono::system_clock::now();
    auto since_epoch = now.time_since_epoch();
    auto micros = chrono::duration_cast<chrono::microseconds>(since_epoch);

    // Convert microseconds since epoch to string
    ostringstream oss;
    oss << "ID_" << micros.count();
    return oss.str();
}

// Function to handle a new request received from a client
void handleNewRequest(int clientSocket)
{

    string requestID = generateUniqueID();
    string folderPath = "./" + requestID + "/";

    // Create a folder with the generated ID
    if (mkdir(folderPath.c_str(), 0777) == -1)
    {
        perror("Error creating folder");
        close(clientSocket);
        return;
    }

    string filePath = folderPath + "received_code.cpp";

    // receive file size

    size_t fileSize;
    ssize_t sizeReceived = recv(clientSocket, &fileSize, sizeof(fileSize), 0);
    send(clientSocket, "File Size Recieved", 18, 0);

    cout << "file size recieved is " << fileSize << endl;

    if (sizeReceived != sizeof(fileSize))
    {
        cerr << "Error receiving file size" << endl;
        close(clientSocket);
        return;
    }

    // recieve file

    ofstream outputFile(filePath, ios::binary);
    if (!outputFile.is_open())
    {
        cerr << "Error opening file for writing." << endl;
        close(clientSocket);
        return;
    }

    const int bufferSize = 1024;
    char buffer[bufferSize];
    size_t remainingBytes = fileSize;

    while (remainingBytes > 0)
    {
        int bytesReceived = recv(clientSocket, buffer, min(static_cast<size_t>(bufferSize), remainingBytes), 0);

        if (bytesReceived <= 0)
        {
            cerr << "Error receiving data" << endl;
            break;
        }

        outputFile.write(buffer, bytesReceived);
        remainingBytes -= bytesReceived;
    }

    send(clientSocket, "code Recieved", 13, 0);

    cout << "code recieved" << endl;

    if (fileSize < 0)
    {
        // Handle receive error
        cerr << "Error receiving data" << endl;
    }

    if (outputFile.is_open())
    {

        // Check if the write operation was successful
        if (outputFile.good())
        {
            string recievedFileDOne = folderPath + "recieveDone.txt";
            cout << "Data written to file successfully." << endl;
            ofstream recieveDone(recievedFileDOne);
            send(clientSocket, requestID.c_str(), requestID.size(), 0);
            idStatusMap.insert({requestID, "Inqueue"});
            
        }
        else
        {
            cerr << "Error writing data to file." << endl;
        }

        // Close the file
        outputFile.close();
    }
    else
    {
        cerr << "Error opening file for writing." << endl;
    }

    if (fileSize <= 0)
    {
        close(clientSocket);
        // continue;
    }
    pthread_mutex_lock(&lockQueue);
    taskqueue.push(requestID);
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&lockQueue);

    close(clientSocket);
}

// Function to handle status requests from clients
void handleStatusRequest(int clientSocket, string requestId)
{
    string keyToCheck = requestId;
    auto it = idStatusMap.find(keyToCheck);

    if (it != idStatusMap.end())
    {
        cout << "Status for key " << keyToCheck << ": " << it->second << endl;
        string response = it->second;
        send(clientSocket, response.c_str(), response.size(), 0);
        if (response == "done")
        {
            string folderPath = "./" + requestId + "/";
            ifstream file(folderPath + "response.txt");
            ostringstream fileContent;
            fileContent << file.rdbuf();
            string responseText = fileContent.str();
            send(clientSocket, responseText.c_str(), responseText.size(), 0);

            string removeCommand = "rm -rf \"" + requestId + "\"";
            int rc = system(removeCommand.c_str());
            idStatusMap.erase(keyToCheck);
            close(clientSocket);
        }
    }
    else
    {
        string response = "Invalid ID";
        send(clientSocket, response.c_str(), response.size(), 0);
        close(clientSocket);
    }
}

// Function to control recovery after a crash or unexpected termination
void crashControl()
{
    fs::path currentDir = fs::current_path();
    for (const auto &entry : fs::directory_iterator(currentDir))
    {
        if (entry.is_directory())
        {
            string directoryName = entry.path().filename().string();
            fs::path responseFile = entry.path() / "response.txt";
            fs::path recieveFileDone = entry.path() / "recieveDone.txt";
            if (fs::exists(responseFile))
            {
                idStatusMap[directoryName] = "done";
            }
            else if (fs::exists(recieveFileDone))
            {
                idStatusMap[directoryName] = "Inqueue";
                pthread_mutex_lock(&lockQueue);
                taskqueue.push(directoryName);
                pthread_mutex_unlock(&lockQueue);
                pthread_cond_signal(&cv);
            }
            else
            {
                cout << fs::exists(responseFile) << endl;
                cout << fs::exists(recieveFileDone) << endl;
                string removeCommand = "rm -rf \"" + directoryName + "\"";
                int rc = system(removeCommand.c_str());
            }
        }
    }
}

// Worker thread function to process tasks from the task queue
void *workerThread(void *arg)
{
    while (true)
    {
        pthread_mutex_lock(&lockQueue);
        while (taskqueue.empty())
        {
            pthread_cond_wait(&cv, &lockQueue);
        }
        string requestId = taskqueue.front();
        taskqueue.pop();
        pthread_mutex_unlock(&lockQueue);

        CompileAndRun(requestId);
        pthread_mutex_unlock(&lockQueue);

    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    int port = atoi(argv[1]);

    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Socket creation error");
        return 1;
    }

    // Bind to the specified port
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Bind error");
        close(serverSocket);
        return 1;
    }

    // Perform crash control and recovery
    crashControl();

    // Listen for incoming connections
    if (listen(serverSocket, 1) == -1)
    {
        perror("Listen error");
        close(serverSocket);
        return 1;
    }

    // Create worker threads to process tasks
    pthread_t threads[16];

    for (int i = 0; i < 16; ++i)
    {
        pthread_create(&threads[i], NULL, workerThread, NULL);
    }

    cout << "Server listening on port " << port << endl;

    // server listening on one port as a listener socket & creates new socket everytime it accepts a connection
    while (true)
    {
        // Accept a connection from a client
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1)
        {
            perror("Accept error");
            continue;
        }

        // Recieve requestType

        char requestType[30];
        memset(requestType, 0, sizeof(requestType));
        ssize_t requestType_size = recv(clientSocket, requestType, sizeof(requestType), 0);

        
        size_t hyphenPos = string(requestType).find('-');
        string request;
        string requestId;
        if (hyphenPos == string::npos)
        {
            request = requestType;
        }
        else
        {
            request = "status";
            requestId = string(requestType).substr(hyphenPos + 1);
            cout << requestId << endl;
        }

        if (requestType_size <= 0)
        {
        
            close(clientSocket);
            continue;
        }

        // action according to request type
        if (strcmp(request.c_str(), "new") == 0)
        {
            cout<<"New Request Come"<<endl;
            send(clientSocket, "new type request recieved", 25, 0);
            handleNewRequest(clientSocket);
        }

        else if (request == "status")
        {
            cout<<"Status Request Come"<<endl;
            handleStatusRequest(clientSocket, requestId);
        }
        else
        {
            cerr << "Invalid Request";
            close(clientSocket);
        }
        
    }

    close(serverSocket);
    return 0;
}