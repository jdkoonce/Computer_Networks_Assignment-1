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
#define DATAFILENAME "dataFile.txt"
#define IP_ADDRESS "127.0.0.1"
#define GOOD_MSG "good"
#define BAD_MSG "invalid"
#define DEFAULT_PORT 6000

/**
 * Initializes WSA for networking.
 */
int initNetworking();

/**
 * Makes a socket, and binds it to the server address and port provided.
 */
SOCKET makeSocket(SOCKADDR_IN &serverAddr, int port);

/**
 * Receives a string from the connection. (blocking)
 * @param connection
 * @return The string the connection provided.
 */
string receiveString(SOCKET connection);

/**
 * Sends a string to the connection. (blocking)
 */
void sendString(SOCKET connection, const string &toSend);

/**
 * Performs cleanup actions and then closes the socket provided.
 */
void cleanup(SOCKET socket);

/**
 * Prints a table that shows whether a serialNumber and machineId are valid or not.
 * @param serialNumber The serial number to display
 * @param machineId  The machineId to display
 * @param serialValid Is this serial valid?
 * @param machineIdValid Is this machineId valid?
 */
void printTableOutput(const string &serialNumber, const string &machineId, bool serialValid, bool machineIdValid);

/**
 * Activates the provided serialNumber and ties it to the provided machineId.
 */
void activate(const string &serialNumber, const string &machineId);

/**
 * Checks whether this serialNumber has been activated or not.
 * @param serialNumber
 * @return The machineId this serialNumber is attached to, or an empty string if the serialNumber hasn't been activated.
 */
string checkSerialActivation(const string &serialNumber);

/**
 * Checks whether this serialNumber is valid or not. (Whether it meets the criteria for validation or not)
 * @param SerialNumber
 * @return True if it is valid, false is it isn't.
 */
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
        port = DEFAULT_PORT;

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

    while (true)
    {
        cout << "\nWaiting for connections...\n";

        // Accept an incoming connection; Program pauses here until a connection arrives
        auto clientConnection = accept(listenSocket, nullptr, nullptr);
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

        try
        {
            cout << "Waiting for serialNumber...\n";
            serialNumber = receiveString(clientConnection);

            auto isValid = checkSerialValidity(serialNumber);
            if (!isValid)
            {
                printTableOutput(serialNumber, "", false, false);
                sendString(clientConnection, BAD_MSG);
                cleanup(clientConnection);
                continue;
            }

            // Serial was valid, move foward!
            sendString(clientConnection, GOOD_MSG);

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
                sendString(clientConnection, GOOD_MSG);
            }
            else
            {
                // Gotta make sure the machine id matches the one the client sent
                if (checkedMachineId == machineId)
                {
                    // Reactivate
                    printTableOutput(serialNumber, machineId, true, true);
                    sendString(clientConnection, GOOD_MSG);
                }
                else
                {
                    // Nope, they tried to use a serial on a different machine
                    printTableOutput(serialNumber, machineId, true, false);
                    sendString(clientConnection, BAD_MSG);
                }
            }
        }
        catch (std::exception &ex)
        {
            cerr << ex.what() << std::endl;
            cleanup(clientConnection);
            continue;
            // Exception occurred while talking to the client. Ignore this client and continue listening.
            // receiveString automatically deals with the connection.
        }
    }

    // Server never closes naturally, just a factor of life
}

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
    inet_pton(AF_INET, IP_ADDRESS, &serverAddr.sin_addr);

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
    char buffer[BUFFER_SIZE];
    bool moreData;
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

void printTableOutput(const string &serialNumber, const string &machineId, bool serialValid, bool machineIdValid)
{
    if (!serialNumber.empty())
        cout << ("Received SerialNumber: " + serialNumber + "\t\t" + (serialValid ? GOOD_MSG : BAD_MSG) + "\n");

    if (!machineId.empty())
        cout << ("Received MachineId: " + machineId + "\t\t" + (machineIdValid ? GOOD_MSG : BAD_MSG) + "\n");
}

void cleanup(SOCKET socket)
{
    if (socket != INVALID_SOCKET)
        closesocket(socket);
}

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
                string machineId;

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
    }

    return "";
}

bool checkSerialValidity(const string &serialNumber)
{
    return (serialNumber.find_first_not_of("0123456789") == std::string::npos);
}