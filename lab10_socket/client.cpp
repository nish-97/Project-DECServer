#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;



int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID>" << endl;
        return 1;
    }

    string requestType = argv[1];
    string serverIPPort = argv[2];
    string sourceFileOrRquestId = argv[3];

    // Extract server IP and port from the command line argument
    size_t colonPos = serverIPPort.find(':');
    if (colonPos == string::npos)
    {
        cerr << "Invalid server IP:port format" << endl;
        return 1;
    }

    string serverIP = serverIPPort.substr(0, colonPos);
    int port = atoi(serverIPPort.substr(colonPos + 1).c_str());

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1)
        {
            perror("Socket creation error");
            return 1;
        }

        // Connect to the server
        // Data structure for socket
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);

        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0)
        {
            perror("Invalid server address");
            close(clientSocket);
            return 1;
        }
        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        {
            perror("Connection error");
            close(clientSocket);
            return 1;
        }

    if (requestType == "new")
    // if (strcmp(requestType, "done") == 0)
    {
        
        send(clientSocket, "new", 3, 0);
        
        char buffer_status_recieved[25];
        ssize_t sizeReceived = recv(clientSocket, &buffer_status_recieved, sizeof(buffer_status_recieved), 0);

        cout << buffer_status_recieved << endl;

        // Read the content of the source code file
        ifstream sourceFile(sourceFileOrRquestId,ios::binary | ios::ate);
        if (!sourceFile.is_open())
        {
            cerr << "Error opening source code file" << endl;
            close(clientSocket);
            return 1;
        }
        // get file size 

        size_t fileSize = sourceFile.tellg();
        sourceFile.seekg(0, ios::beg);

        char buffer_after_sending_size[20];

        //send the file size to the server

        send(clientSocket,&fileSize,sizeof(fileSize),0);
        
        ssize_t sizeReceived_after_sending_size = recv(clientSocket, &buffer_after_sending_size, sizeof(buffer_after_sending_size), 0);
        cout << buffer_after_sending_size <<endl;

        // read contents of sourceFile linebyline until EOF and copy it to sourceCodeContent
        string sourceCodeContent((istreambuf_iterator<char>(sourceFile)),
                                 istreambuf_iterator<char>());

        // Send the request and source code content to the server
        string request = sourceCodeContent;
        send(clientSocket, request.c_str(), request.size(), 0);
        string eofMarker = "EOF";
        send(clientSocket, eofMarker.c_str(), eofMarker.size(), 0);
        cout << "file sent "<<endl;

        char buffer_after_file_send[13];
        ssize_t sizeReceived_afetrFileSend = recv(clientSocket, &buffer_after_file_send, sizeof(buffer_after_file_send), 0);
        cout<<buffer_after_file_send<<endl;


        // Receive and display the server response
        char buffer[1024];
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead <= 0)
        {
            cerr << "Error receiving response from server" << endl;
        }
        else
        {
            buffer[bytesRead] = '\0';
            cout << buffer << endl;
        }

        close(clientSocket);
    }

    else if (requestType == "status")
    {
        // while (true)
        // {
            // int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            // if (clientSocket == -1)
            // {
            //     perror("Socket creation error");
            //     return 1;
            // }

            // // Connect to the server
            // // Data structure for socket
            // struct sockaddr_in serverAddress;
            // serverAddress.sin_family = AF_INET;
            // serverAddress.sin_port = htons(port);

            // if (inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr) <= 0)
            // {
            //     perror("Invalid server address");
            //     close(clientSocket);
            //     return 1;
            // }
            // if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
            // {
            //     perror("Connection error");
            //     close(clientSocket);
            //     return 1;
            // }

            string statusRequest = requestType + "-" + sourceFileOrRquestId;
            send(clientSocket, statusRequest.c_str(), statusRequest.size(), 0);
            // sleep(1);
            // send(clientSocket, sourceFileOrRquestId.c_str(), sourceFileOrRquestId.size(), 0);

            char buffer[1024];
            memset(buffer,0,sizeof(buffer));
            ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesRead <= 0)
            {
                cerr << "Error receiving response from server" << endl;
            }
            else
            {
                buffer[bytesRead] = '\0';
                cout << buffer << endl;
            }

            if (string(buffer)== "done")
            {
                cout << "your grading is done"<<endl;
                char response[1024];
                memset(response,0,sizeof(response));
                ssize_t bytesRead = recv(clientSocket, response, sizeof(response), 0);

                if (bytesRead <= 0)
                {
                    cerr << "Error receiving response from server" << endl;
                }
                else
                {
                    response[bytesRead] = '\0';
                    cout << response << endl;
                }

                close(clientSocket);
                return 0;
            }

            else
            {
                close(clientSocket);
                // return 0;
                sleep(2);
            }
        // }
    }

    else
    {
        cerr << "Invalid Request";
    }
    

    return 0;
}
