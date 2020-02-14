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
#define ACTIVATIONFILENAME "actFile.txt"
#define IPADDRESS "127.0.0.1"
#define GOODMSG "good"
#define BADMSG "invalid"
#define DEFAULTPORT 6000


// Function to close the specified socket and perform DLL cleanup (WSACleanup)
int initNetworking();

SOCKET makeSocket(SOCKADDR_IN &serverAddr, int port);

void activate(const string machineId);

bool checkActivation(string machineId);

void cleanup(SOCKET socket);

string trim(const string &str);

int main(int argc, char *argv[])
{
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

    // Checking if the software is activated....
    string machineId;

    cout << "Enter your machine id: \n";
    getline(cin, machineId);

    // Clean user input
    trim(machineId);

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
        auto theSocket = makeSocket(serverAddr, port);
        if (theSocket == INVALID_SOCKET)
        {
            cout << "An error occurred while creating a socket! We cannot continue, sorry." << std::endl;
            return INVALID_SOCKET;
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
                cerr << "Send failed with error: " << WSAGetLastError() << std::endl;
                cleanup(theSocket);
                return 1;
            }

        } while (!done);
    }
    else
    {
        cout << "The client is already activated!" << std::endl;
    }

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
    // Setup a SOCKADDR_IN structure which will be used to hold address
    // and port information for the server. Notice that the port must be converted
    // from host byte order to network byte order.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, IPADDRESS, &serverAddr.sin_addr);

    // Try to connect to server
    auto iResult = connect(theSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR)
    {
        cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
        cleanup(theSocket);
        return INVALID_SOCKET;
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
 * Checks if the client has been activated or not.
 * @param machineId
 * @return
 */
bool checkActivation(const string machineId)
{
    std::ifstream actFile(ACTIVATIONFILENAME);

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

/**
 * Activates the client.
 * @param machineId
 * @return
 */
void activate(const string machineId)
{
    std::ofstream actFile;
    actFile.open("actFile.txt", std::ofstream::out | std::ofstream::trunc);
    actFile << machineId;
    actFile.close();
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