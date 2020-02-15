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
#define BUFFER_SIZE 256
#define ACTIVATION_FILENAME "actFile.txt"
#define IP_ADDRESS "127.0.0.1"
#define GOOD_MSG "good"
#define BAD_MSG "invalid"
#define DEFAULT_PORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
int initNetworking();

SOCKET makeSocket(SOCKADDR_IN &serverAddr, int port);

string receiveString(SOCKET connection);

void sendString(SOCKET connection, const string &toSend);

void cleanup(SOCKET socket);

void activate(const string &machineId);

bool checkActivation(const string &machineId);

void onActivationFailed(SOCKET serverConnection);

string trim(const string &str);

int main(int argc, char *argv[])
{
    SOCKADDR_IN serverAddr;            // structure to hold server address information
    string serialNumstring;         // serial number from user
    int port;                // server's port number
    SOCKET serverConnection;

    // If user types in a port number on the command line use it,
    // otherwise, use the default port number
    if (argc > 1)
        port = atoi(argv[1]);
    else
        port = DEFAULT_PORT;

    // Checking if the software is activated....
    string machineId;

    cout << "Enter your machine id: \n";
    if (!getline(cin, machineId))
    {
        cout << "An error occurred! We cannot continue, sorry!" << std::endl;
        return 1;
    }

    // Clean user input
    machineId = trim(machineId);

    //Run the activation check
    if (!checkActivation(machineId))
    {
        cout << "Activation is required." << std::endl;

        // Initialize WSA networking
        auto initCode = initNetworking();
        if (initCode != 0)
        {
            cout << "An error occurred while initializing networking! We cannot continue, sorry." << std::endl;
            return initCode;
        }

        // Create the socket
        serverConnection = makeSocket(serverAddr, port);
        if (serverConnection == INVALID_SOCKET)
        {
            cout << "An error occurred while creating a socket! We cannot continue, sorry." << std::endl;
            WSACleanup();
            return INVALID_SOCKET;
        }

        // Get the serial number from the user and send it to the server.
        cout << "Enter your serial number: \n";
        if (!getline(cin, serialNumstring))
        {
            cout << "An error occurred! We cannot continue, sorry!" << std::endl;
            WSACleanup();
            return 1;
        }

        try
        {
            sendString(serverConnection, serialNumstring);

            auto serialResponse = receiveString(serverConnection);
            if (serialResponse != GOOD_MSG)
            {
                // Oops.
                onActivationFailed(serverConnection);
                return 0;
            }

            sendString(serverConnection, machineId);

            auto activationResponse = receiveString(serverConnection);
            if (activationResponse != GOOD_MSG)
            {
                // Oops.
                onActivationFailed(serverConnection);
                return 0;
            }

            // We're good to go!
            activate(machineId);
            cout << "The client has been successfully activated!" << std::endl;
        }
        catch (std::exception &ex)
        {
            cerr << ex.what() << std::endl;
            // We weren't able to communicate with the server for some reason. Let's cleanup and leave.

            cleanup(serverConnection);
            WSACleanup();
            return 0;
        }
    }
    else
    {
        cout << "The client is already activated." << std::endl;
        return 0;
    }

    cleanup(serverConnection);
    WSACleanup();
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
        cerr << "WSAStartup failed with error: " << iResult << std::endl;
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
    inet_pton(AF_INET, IP_ADDRESS, &serverAddr.sin_addr);

    // Try to connect to the server
    auto iResult = connect(theSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR)
    {
        cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        cleanup(theSocket);
        return INVALID_SOCKET;
    }

    return theSocket;
}

string receiveString(SOCKET connection)
{
    char buffer[BUFFER_SIZE];
    bool moreData = false;
    string receiveString;

    buffer[BUFFER_SIZE - 1] = '\0';

    do
    {
        auto iResult = recv(connection, buffer, BUFFER_SIZE - 1, 0);

        if (iResult > 0)
        {
            // Received data; need to determine if there's more coming
            moreData = buffer[iResult - 1] != '\0';

            // Concatenate received data onto end of string we're building
            receiveString = receiveString + (string) buffer;
        }
        else if (iResult == 0)
        {
            // Need to close clientSocket; listenSocket was already closed
            throw std::exception("Connection closed!\n");
        }
        else
        {
            throw std::exception(("Recv failed with error: " + std::to_string(WSAGetLastError()) + "\n").c_str());
        }
    } while (moreData);

    return receiveString;
}

void sendString(SOCKET connection, const string &toSend)
{
    auto iResult = send(connection, toSend.c_str(), (int) toSend.size() + 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw std::exception(("Send failed with error: " + std::to_string(WSAGetLastError()) + "\n").c_str());
    }
}

/**
 * Cleanup the socket.
 * @param socket
 */
void cleanup(SOCKET socket)
{
    if (socket != INVALID_SOCKET)
        closesocket(socket);
}

/**
 * Activates the client.
 * @param machineId
 * @return
 */
void activate(const string &machineId)
{
    std::ofstream actFile;
    actFile.open("actFile.txt", std::ofstream::out | std::ofstream::trunc);
    actFile << machineId;
    actFile.close();
}

/**
 * Checks if the client has been activated or not.
 * @param machineId
 * @return
 */
bool checkActivation(const string &machineId)
{
    std::ifstream actFile(ACTIVATION_FILENAME);

    //File doesn't exist, client needs activation
    if (actFile.fail())
    {
        actFile.close();
        return false;
    }

    string contents;
    if (actFile.is_open())
    {
        string line;
        while (getline(actFile, line))
        {
            contents += line;
        }
    }

    if (contents == machineId)
    {
        actFile.close();
        return true;
    }

    actFile.close();
    return false;
}

void onActivationFailed(SOCKET serverConnection)
{
    cout << "The activation was not successful.\n";
    cleanup(serverConnection);
    WSACleanup();
}

/**
 * Removes whitespace from both ends of a string.
 * @param str
 * @return
 */
string trim(const string &str)
{
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first) return str;

    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}