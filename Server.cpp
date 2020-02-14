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

void activate(const string machineId);

string checkSerialActivation(int serialNumber);

void cleanup(SOCKET socket);

int main(int argc, char *argv[])
{
    int port;

    // If user types in a port number on the command line use it,
    // otherwise, use the default port number
    if (argc > 1)
        port = atoi(argv[1]);
    else
        port = DEFAULTPORT;

    auto initCode = initNetworking();

    if (initCode != 0)
    {
        cout << "An error occurred while initializing networking! We cannot continue, sorry." << std::endl;
        return initCode;
    }

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
void activate(string machineId, int serialNumber)
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

