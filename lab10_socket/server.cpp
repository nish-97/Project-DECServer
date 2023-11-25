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

map<string, string> idStatusMap;

void CompileAndRun(const string &requestID)
{
    string response;

    string folderPath = "./" + requestID + "/";
    string sourcePath = folderPath + "received_code.cpp";
    string execPath = folderPath + "executable > "+folderPath +"program_output.txt";

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
            // response = "exectuable made output error or not dont know";
            // Rest of your code for comparison and handling output errors...
            // Program executed successfully, compare its output with the expected output
            ifstream programOutputFile(folderPath +"program_output.txt");
            ostringstream programOutputContent;
            programOutputContent << programOutputFile.rdbuf();
            string programOutput = programOutputContent.str();
            string copyCmd = "cp expected_output.txt " + folderPath+ "expected_output.txt";
            system(copyCmd.c_str());
            ifstream expectedOutputFile(folderPath +"expected_output.txt");
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
                ofstream programOutputFile(folderPath +"program_output.txt");
                programOutputFile << programOutput;
                programOutputFile.close();
                
                string diffCmd = "diff "+folderPath+"program_output.txt "+folderPath+"expected_output.txt > "+folderPath+"diff_output.txt";
                system(diffCmd.c_str());

                ifstream diffOutputFile(folderPath +"diff_output.txt");
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
        responseFile << response << endl;
        responseFile.close();
    }

    string keyToFind = requestID;
    auto it = idStatusMap.find(keyToFind);

    if (it != idStatusMap.end())
    {
        it->second = "done";
    }
}

string generateUniqueID()
{
    time_t timestamp = time(nullptr);
    return "ID_" + to_string(timestamp);
}

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

    // go to the folder to recieve file
    // string cd = "cd " + folderPath;
    // system(cd.c_str());

    // // Receive client request

    // char buffer[1024];
    // string recievedCode;

    // // memset(buffer, 0, sizeof(buffer));
    // ssize_t bytesRead = 0;
    // while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0))>0)
    // {
    //     recievedCode.append(buffer,bytesRead);
    // }
    // if (bytesRead < 0) {
    //     // Handle receive error
    //     std::cerr << "Error receiving data" << std::endl;
    // }
    // else {
    //     // Process received data (in 'receivedData' string)
    //     // std::cout << "Received data: " << recievedCode << std::endl;
    // }
    // receive file size

    size_t fileSize;
    ssize_t sizeReceived = recv(clientSocket, &fileSize, sizeof(fileSize), 0);

    if (sizeReceived != sizeof(fileSize))
    {
        std::cerr << "Error receiving file size" << std::endl;
        close(clientSocket);
        return;
    }

    //recieve file

    std::ofstream outputFile(filePath, std::ios::binary);
    if (!outputFile.is_open())
    {
        std::cerr << "Error opening file for writing." << std::endl;
        close(clientSocket);
        return;
    }

    const int bufferSize = 1024;
    char buffer[bufferSize];
    size_t remainingBytes = fileSize;

    while (remainingBytes > 0)
    {
        int bytesReceived = recv(clientSocket, buffer, std::min(static_cast<size_t>(bufferSize), remainingBytes), 0);


        if (bytesReceived <= 0)
        {
            std::cerr << "Error receiving data" << std::endl;
            break;
        }

        outputFile.write(buffer, bytesReceived);
        remainingBytes -= bytesReceived;
    }

    // Loop to continuously receive data until there's no more to receive
    // ssize_t bytesRead = 0;
    // while ((bytesRead = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
    //     receivedCode.append(buffer, bytesRead);
    //     // Check for the "EOF" marker to end the loop
    //     if (receivedCode.find("EOF") != std::string::npos) {
    //         // Remove the "EOF" marker from the received code
    //         receivedCode.erase(receivedCode.find("EOF"));
    //         break;
    //     }

    // }

    if (fileSize < 0)
    {
        // Handle receive error
        std::cerr << "Error receiving data" << std::endl;
    }
    else
    {
        // Process received data (in 'receivedCode' string)
        // std::cout << "Received data: " << receivedCode << std::endl;
    }

    // cout << "Chechking what byteread recieving\n";
    // cout << buffer;
    // cout << "Checking done ";

    // std::ofstream outputFile(filePath, ios::out | ios::binary);

    // Check if the file is opened successfully
    if (outputFile.is_open())
    {
        // Write the data from 'buffer' to the file
        // outputFile.write(buffer, bytesRead);
        // outputFile << buffer;

        // Check if the write operation was successful
        if (outputFile.good())
        {
            string recievedFileDOne = folderPath + "recieveDone.txt";
            cout << "Data written to file successfully." << endl;
            ofstream recieveDone(recievedFileDOne);
            send(clientSocket, requestID.c_str(), requestID.size(), 0);
            idStatusMap.insert({requestID, "Inqueue"});

            // send(clientSocket, response.c_str(), response.size(), 0)
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

    // std::istringstream request(buffer);
    // std::string serverIPPort, sourceFileName;
    // request >> serverIPPort >> sourceFileName;

    // Convert the received data to a string
    // std::string receivedData(buffer);

    // Process the request (compile and run)
    // std::string response = CompileAndRun(receivedData);

    // Send the response back to the client
    // send(clientSocket, response.c_str(), response.size(), 0);
    for (const auto &pair : idStatusMap)
    {
        std::cout << "ID: " << pair.first << ", Status: " << pair.second << std::endl;
    }
    // CompileAndRun(requestID);
    close(clientSocket);
}

void handleStatusRequest(int clientSocket, string requestId)
{

    // std::cout << "Inside handleStatusRequest" << endl;
    // char receivedRequestId[1024];
    // memset(receivedRequestId, 0, sizeof(receivedRequestId));
    // ssize_t receivedRequestId_size = recv(clientSocket, receivedRequestId, sizeof(receivedRequestId), 0);

    string keyToCheck = requestId;
    auto it = idStatusMap.find(keyToCheck);

    if (it != idStatusMap.end())
    {
        std::cout << "Status for key " << keyToCheck << ": " << it->second << std::endl;
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
            // assert(rc == 0);
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

void crashControl(){
    fs::path currentDir = fs::current_path();
    for (const auto &entry : fs::directory_iterator(currentDir))
    {
        if (entry.is_directory())
        {
            string directoryName = entry.path().filename().string();
            fs::path responseFile = entry.path() / "response.txt";
            fs::path recieveFileDone = entry.path() / "recieveDone.txt";
            cout << responseFile;
            if (fs::exists(responseFile))
            {
                cout<<"check for response"<< endl;
                idStatusMap[directoryName] = "done";
            }
            else if (fs::exists(recieveFileDone))
            {   
                cout << "check for recieveDone"<<endl;
                idStatusMap[directoryName] = "Inqueue";
            }
            else
            {
                cout << fs::exists(responseFile)<<endl;
                cout << fs::exists(recieveFileDone)<<endl;
                cout<<"going to remove the file "<<endl;
                string removeCommand = "rm -rf \"" + directoryName + "\"";
                int rc = system(removeCommand.c_str());
            }
        }
    }
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

    crashControl();

    // Listen for incoming connections
    if (listen(serverSocket, 1) == -1)
    {
        perror("Listen error");
        close(serverSocket);
        return 1;
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
        cout << "recieving Request Type "
             << endl;

        char requestType[1024];
        memset(requestType, 0, sizeof(requestType));
        ssize_t requestType_size = recv(clientSocket, requestType, sizeof(requestType), 0);
        cout << requestType << endl;
        size_t hyphenPos = std::string(requestType).find('-');
        string request;
        string requestId;
        if (hyphenPos == std::string::npos)
        {
            request = requestType;
        }
        else
        {
            request = "status";
            // cout << request << endl;
            requestId = std::string(requestType).substr(hyphenPos + 1);
            cout << requestId << endl;
            // std::cout << "First part: " << firstPart << std::endl;
            // std::cout << "Second part: " << secondPart << std::endl;
            // Continue processing with firstPart and secondPart
        }

        cout << request << endl;

        cout << "Request Type  recieved" << endl;

        if (requestType_size <= 0)
        {
            // cout << "entering Here" << endl;
            close(clientSocket);
            continue;
        }
        // cout << "Iam here"<<endl;
        // cout << request << endl;

        // action according to request type
        if (strcmp(request.c_str(), "new") == 0)
        {
            handleNewRequest(clientSocket);
        }

        else if (request == "status")
        {
            cout << "after comparing" << endl;
            handleStatusRequest(clientSocket, requestId);
        }
        else
        {
            cerr << "Invalid Request";
            close(clientSocket);
        }
        // cout << request << endl;
    }

    close(serverSocket);
    return 0;
}