#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")

// Define default global constants
#define BUFFERSIZE 256
#define ACTIVATIONFILENAME "actFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
void cleanup(SOCKET socket);

void createFile(char *machineID, bool activation);

int main(int argc, char *argv[])
{
    WSADATA wsaData;            // structure to hold info about Windows sockets implementation
    SOCKET theSocket;          // socket for communication with the server
    SOCKADDR_IN serverAddr;            // structure to hold server address information
    char buffer[BUFFERSIZE];    // buffer to hold message received from server
    string serialNumstring;         // serial number from user
    int iResult;            // resulting code from socket functions
    int port;                // server's port number
    bool done;                // variable to control communication loop
    bool moreData;            // variable to control receive data loop

    // If user types in a port number on the command line use it,
    // otherwise, use the default port number
    if (argc > 1)
        port = atoi(argv[1]);
    else
        port = DEFAULTPORT;

    string machineIDstring;
    bool activation = true;
    cout << "Enter your machine id: \n";
    getline(cin, machineIDstring);
    char *machineID = new char[machineIDstring.length() + 1];
    std::strcpy(machineID, machineIDstring.c_str());
    createFile(machineID, activation);

    // WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        cerr << "WSAStartup failed with error: " << iResult << endl;
        return 1;
    }

    // Create a new socket for communication with the server
    theSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (theSocket == INVALID_SOCKET)
    {
        cerr << "Socket failed with error: " << WSAGetLastError() << endl;
        cleanup(theSocket);
        return 1;
    }

    //***Attempting to connect***
    // Setup a SOCKADDR_IN structure which will be used to hold address
    // and port information for the server. Notice that the port must be converted
    // from host byte order to network byte order.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);

    // Try to connect to server
    iResult = connect(theSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR)
    {
        cerr << "Connect failed with error: " << WSAGetLastError() << endl;
        cleanup(theSocket);
        return 1;
    }

    //***Connected***
    // Always make sure there's a '\0' at the end of the buffer
    buffer[BUFFERSIZE - 1] = '\0';

    done = false;
    do
    {
        // SEND and then RECV

        //Get the serial number from the user and send it to the server.
        cout << "Enter your serial number: \n";
        getline(cin, serialNumstring);

        iResult = send(theSocket, serialNumstring.c_str(), (int) serialNumstring.size() + 1, 0);
        if (iResult == SOCKET_ERROR)
        {
            cerr << "Send failed with error: " << WSAGetLastError() << endl;
            cleanup(theSocket);
            return 1;
        }

    } while (!done);
    return 0;
}

void cleanup(SOCKET socket)
{
    if (socket != INVALID_SOCKET)

        closesocket(socket);


    WSACleanup();

}

void createFile(char *machineID, bool activation)
{
    if (activation)
    {
        std::ofstream actFile;
        actFile.open("actFile.txt", std::ofstream::out | std::ofstream::trunc);
        actFile << machineID;
        actFile.close();
    }
}
