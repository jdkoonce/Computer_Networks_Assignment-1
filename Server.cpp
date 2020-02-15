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

string receiveString(SOCKET connection);

void sendString(SOCKET connection, const string &toSend);

void printTableOutput(const string &serialNumber, const string &machineId, bool serialValid, bool machineIdValid);

void cleanup(SOCKET socket);

void activate(const string &serialNumber, const string &machineId);

string checkSerialActivation(const string &serialNumber);

bool checkSerialValidity(const string &SerialNumber);

int main(int argc, char *argv[])
{
    SOCKADDR_IN serverAddr;
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
            WSACleanup();
            return 1;
        }

        cout << "Client connected. Awaiting response.\n\n";

        string serialNumber;
        string machineId;
        bool moreData = false;

        try
        {
            bool activationProcess;

            cout << "Waiting for serialNumber...\n";
            serialNumber = receiveString(clientConnection);

            auto isValid = checkSerialValidity(serialNumber);
            if (!isValid)
            {
                printTableOutput(serialNumber, "", false, false);
                sendString(clientConnection, BADMSG);
                cleanup(clientConnection);
                continue;
            }

            // Serial was valid, move foward!
            sendString(clientConnection, GOODMSG);

            cout << "Waiting for machineId...\n";
            machineId = receiveString(clientConnection);

            // Check activations
            auto checkedMachineId = checkSerialActivation(serialNumber);

            // If no machine Id was found associated with this serial number
            if (checkedMachineId.empty())
            {
                // Sweet, we can activate it
                printTableOutput(serialNumber, machineId, true, true);
                activate(serialNumber, machineId);
                sendString(clientConnection, GOODMSG);
            }
            else
            {
                // Gotta make sure the machine id matches the one the client sent
                if (checkedMachineId == machineId)
                {
                    // Reactivate
                    printTableOutput(serialNumber, machineId, true, true);
                    sendString(clientConnection, GOODMSG);
                }
                else
                {
                    // Nope, they tried to use a serial on a different machine
                    printTableOutput(serialNumber, machineId, true, false);
                    sendString(clientConnection, BADMSG);
                }
            }
        }
        catch (string ex)
        {
            cerr << ex << std::endl;
            cleanup(clientConnection);
            continue;
            // Exception occurred while talking to the client. Ignore this client and continue listening.
            // receiveString automatically deals with the connection.
        }
    }

    closesocket(listenSocket);
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

string receiveString(SOCKET connection)
{
    char buffer[BUFFERSIZE];
    bool moreData = false;
    string receiveString;

    buffer[BUFFERSIZE - 1] = '\0';

    do
    {
        auto iResult = recv(connection, buffer, BUFFERSIZE - 1, 0);

        if (iResult > 0)
        {
            // Received data; need to determine if there's more coming
            if (buffer[iResult - 1] != '\0')
                moreData = true;
            else
                moreData = false;

            // Concatenate received data onto end of string we're building
            receiveString = receiveString + (string) buffer;
        }
        else if (iResult == 0)
        {
            // Need to close clientSocket; listenSocket was already closed
            throw "Connection closed!\n";
        }
        else
        {
            throw "Recv failed with error: " + std::to_string(WSAGetLastError()) + "\n";
        }
    } while (moreData);

    return receiveString;
}

void sendString(SOCKET connection, const string &toSend)
{
    auto iResult = send(connection, toSend.c_str(), (int) toSend.size() + 1, 0);
    if (iResult == SOCKET_ERROR)
    {
        throw "Send failed with error: " + std::to_string(WSAGetLastError()) + "\n";
    }
}

void printTableOutput(const string &serialNumber, const string &machineId, bool serialValid, bool machineIdValid)
{
    if (!serialNumber.empty())
        cout << ("Received SerialNumber: " + serialNumber + "\t\t" + (serialValid ? GOODMSG : BADMSG) + "\n");

    if (!machineId.empty())
        cout << ("Received MachineId: " + serialNumber + "\t\t" + (machineIdValid ? GOODMSG : BADMSG) + "\n");
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
 * Activates a client.
 * @param machineId The machineId of the client
 * @param serialNumber The client's serial number
 */
void activate(const string &serialNumber, const string &machineId)
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
 * @return the machineId or an empty string if the serial hasn't been activated yet.
 */
string checkSerialActivation(const string &serialNumber)
{
    std::ifstream dataFile(DATAFILENAME);
    //File doesn't exist, no check needed.
    if (dataFile.fail())
    {
        dataFile.close();
        return "";
    }

    string contents;
    if (dataFile.is_open())
    {
        string line;
        while (getline(dataFile, line))
        {
            contents = line;
            if (contents == serialNumber)
            {
                string machineId = "";

                // If there is a machineId directly associated with this serialNumber
                if (getline(dataFile, line))
                {
                    dataFile.close();
                    machineId = line;
                }

                return machineId;
            }
        }
        dataFile.close();
        return "";
    }
}

bool checkSerialValidity(const string &serialNumber)
{
    return (serialNumber.find_first_not_of("0123456789") == std::string::npos);
}