#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iostream>
#include <string>
#include <fstream>

using std::string;
using std::cout;
using std::cin;
using std::cerr;

// Make sure build environment links to Winsock library file
#pragma comment(lib, "Ws2_32.lib")

// Define default global constants
#define BUFFERSIZE 256
#define DATAFILENAME "dataFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)

int initNetworking();

SOCKET makeSocket(SOCKADDR_IN &serverAddr, int port);

void activate(const string machineId);

string checkSerialActivation(int serialNumber);

void cleanup(SOCKET socket);

int main(int argc, char *argv[])
{
    SOCKADDR_IN serverAddr;
    char buffer[BUFFERSIZE];
    int port;

    // If user types in a port number on the command line use it,
    // otherwise, use the default port number
    if (argc > 1)
        port = atoi(argv[1]);
    else
        port = DEFAULTPORT;

    // Initialize WSA networking
    auto initCode = initNetworking();
    if (initCode != 0)
    {
        cout << "An error occurred while initializing networking! We cannot continue, sorry." << std::endl;
        return initCode;
    }

    auto listenSocket = makeSocket(serverAddr, port);
    if (listenSocket == INVALID_SOCKET)
    {
        cout << "An error occurred while creating a socket! We cannot continue, sorry." << std::endl;
        return INVALID_SOCKET;
    }

    // Start listening for incoming connections
    auto iResult = listen(listenSocket, 1);
    if (iResult == SOCKET_ERROR)
    {
        cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        cleanup(listenSocket);
        return 1;
    }


    bool listening = true;

    while (listening)
    {
        cout << "\nWaiting for connections...\n";

        // Accept an incoming connection; Program pauses here until a connection arrives
        auto clientConnection = accept(listenSocket, NULL, NULL);
        if (clientConnection == INVALID_SOCKET)
        {
            cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            // Need to close listenSocket; clientConnection never opened
            cleanup(listenSocket);
            return 1;
        }

        cout << "Connected...\n\n";

        buffer[BUFFERSIZE - 1] = '\0';
        string serialNumber;
        string machineId;
        bool moreData = false;

        do
        {
            iResult = recv(clientConnection, buffer, BUFFERSIZE - 1, 0);

            if (iResult > 0)
            {
                // Received data; need to determine if there's more coming
                if (buffer[iResult - 1] != '\0')
                    moreData = true;
                else
                    moreData = false;

                // Concatenate received data onto end of string we're building
                serialNumber = serialNumber + (string) buffer;
            }
            else if (iResult == 0)
            {
                cout << "Connection closed\n";
                // Need to close clientSocket; listenSocket was already closed
                cleanup(clientConnection);
                return 0;
            }
            else
            {
                cerr << "Recv failed with error: " << WSAGetLastError() << std::endl;
                cleanup(clientConnection);
                return 1;
            }
        } while (moreData);

        do
        {
            iResult = recv(clientConnection, buffer, BUFFERSIZE - 1, 0);

            if (iResult > 0)
            {
                // Received data; need to determine if there's more coming
                if (buffer[iResult - 1] != '\0')
                    moreData = true;
                else
                    moreData = false;

                // Concatenate received data onto end of string we're building
                machineId = machineId + (string) buffer;
            }
            else if (iResult == 0)
            {
                cout << "Connection closed\n";
                // Need to close clientSocket; listenSocket was already closed
                cleanup(clientConnection);
                return 0;
            }
            else
            {
                cerr << "Recv failed with error: " << WSAGetLastError() << std::endl;
                cleanup(clientConnection);
                return 1;
            }
        } while (moreData);

        cout << "Received Serial Number: " << serialNumber << std::endl;
        cout << "Received Machine Id: " << machineId << std::endl;
    }

    closesocket(listenSocket);

    string machineIDstring;
    char machineID[20] = "abd";
    int serialNumber = 122;
    bool activation = true;

    return 0;
}

/**
 * Initializes WSA for networking.
 */
int initNetworking()
{
    // WSAStartup loads WS2_32.dll (Winsock version 2.2) used in network programming
    WSADATA wsaData;
    auto iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        return 1;
    }

    return 0;
}

SOCKET makeSocket(SOCKADDR_IN &serverAddr, int port)
{
    // Create a new socket for communication with the server
    auto theSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (theSocket == INVALID_SOCKET)
    {
        cerr << "Socket failed with error: " << WSAGetLastError() << std::endl;
        cleanup(theSocket);
        return 1;
    }

    //***Attempting to connect***
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);

    // Attempt to bind a the local network address to the socket
    auto iResult = bind(theSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR)
    {
        cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        cleanup(theSocket);
        return 1;
    }

    return theSocket;
}


/**
 * Cleanup the socket.
 * @param socket
 */
void cleanup(SOCKET socket)
{
    if (socket != INVALID_SOCKET)
        closesocket(socket);

    WSACleanup();
}

/**
 * Activates a client.
 * @param machineId The machineId of the client
 * @param serialNumber The client's serial number
 */
void activate(int serialNumber, string machineId)
{
    std::ofstream dataFile;
    dataFile.open(DATAFILENAME, std::ofstream::out | std::ofstream::app);
    dataFile << serialNumber;
    dataFile << "\n";
    dataFile << machineId;
    dataFile << "\n";
    dataFile.close();
}

/**
 * Checks if a serialNumber has been activated or not.
 * @param serialNumber The serialNumber that is being checked.
 * @return the machineId or null if the serial hasn't been activated yet.
 */
string checkSerialActivation(int serialNumber)
{
    return "";
}