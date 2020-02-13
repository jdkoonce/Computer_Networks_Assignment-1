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
void createFile(char* machineID, bool activation);


int main(int argc, char* argv[])
{
	int port;

	// If user types in a port number on the command line use it,
	// otherwise, use the default port number
	if (argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULTPORT;
	string machineIDstring;
	bool activation = true;
	cout << "Enter your machine id: ";
	getline(cin, machineIDstring);
	char* machineID = new char[machineIDstring.length() + 1];
	std::strcpy(machineID, machineIDstring.c_str());
	createFile(machineID, activation);
	
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
		FILE* actFile = fopen("actFile.txt", "w");

		fputs(machineID,actFile);
		fclose(actFile);
	}
}