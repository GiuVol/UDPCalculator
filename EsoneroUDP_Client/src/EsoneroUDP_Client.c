/*
 ============================================================================
 Name        : EsoneroUDP_Client.c
 Author      : GiuseppeVolpe
 Version     :
 Copyright   : Your copyright notice
 Description : Calculator Client
 ============================================================================
 */

#if defined WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#endif

#include "ClientServerAgreement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENDEDWITHERROR -1
#define MAXHOSTNAMEANDPORTSIZE 250

void clearwinsock();
int startedCorrectly();
int main(int argc, char *argv[]);
void parseAddress(char* hostNameAndPort, unsigned long* address, int* port);
void handleConnection(int clientSocket, struct sockaddr_in serverAddress, int* exit);
int composeRequest(struct clientRequest* request);

void clearwinsock() {
#if defined WIN32
	WSACleanup();
#endif
}

int startedCorrectly(){
	//...
#if defined WIN32
	// Initialize Winsock
	WSADATA wsa_data;
	int result = WSAStartup(MAKEWORD(2,2), &wsa_data);
	if (result != NO_ERROR) {
		return 0;
	}
#endif

	return 1;
}

int main(int argc, char *argv[]) {

	setvbuf(stdout, NULL, _IONBF, 0);

	if(startedCorrectly() == 0){
		printf("Startup Failed.\n");
		return ENDEDWITHERROR;
	}

	//Defining address & port

	unsigned long address = inet_addr(SERVERIPADDRESS);
	int port = DEFINEDSERVERPORT;

	if(argc > 1){
		parseAddress(argv[1], &address, &port);
	}

	//

	int clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in serverAddress;

	if(clientSocket < 0){
		printf("Socket creation failed.\n");
		return ENDEDWITHERROR;
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = address;

	int exit = 0;

	do{
		handleConnection(clientSocket, serverAddress, &exit);
	} while(exit == 0);

	printf("Client terminated.\n");
	closesocket(clientSocket);
	clearwinsock();
	return 0;
}

void parseAddress(char* hostNameAndPort, unsigned long* address, int* port){

	char nameAndPort[MAXHOSTNAMEANDPORTSIZE];
	strcpy(nameAndPort, hostNameAndPort);

	for(int i = 0; i < strlen(nameAndPort); i++){
		if(nameAndPort[i] == ':'){
			nameAndPort[i] = ' ';
		}
	}

	char hostName[MAXHOSTNAMEANDPORTSIZE];
	int hostPort;

	int correctValuesEntered = sscanf(nameAndPort, "%s %d", hostName, &hostPort);

	if(correctValuesEntered < 2){
		printf("Entered wrong values. Connection will start with default values.\n");
		return;
	}

	struct hostent* hostInformations;
	memset(&hostInformations, 0, sizeof(hostInformations));
	hostInformations = gethostbyname(hostName);

	if(hostInformations == NULL){
		printf("Problems with name resolution. Connection will start with default values.\n");
	} else {
		struct in_addr* hostAddress = (struct in_addr*) hostInformations->h_addr_list[0];
		*address = inet_addr(inet_ntoa(*hostAddress));
		*port = hostPort;
	}
}

void handleConnection(int clientSocket, struct sockaddr_in serverAddress, int* exit){

	struct clientRequest request;
	memset(&request, 0, sizeof(request));

	int correctInsertion = 0;

	do{
		correctInsertion = composeRequest(&request);
	} while(!correctInsertion);

	if(request.operator == ENDOPERATOR){
		*exit = 1;
		return;
	}

	int sentBytes = sendto(clientSocket, (char*) &request, sizeof(request), 0, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

	if(sentBytes != sizeof(request)){
		printf("Sending failed.\n");
		return;
	}

	struct sockaddr_in fromAddress;
	int fromAddressLen = sizeof(fromAddress);
	memset(&fromAddress, 0, fromAddressLen);
	struct serverAnswer answer;
	memset(&answer, 0, sizeof(answer));


	int recievedBytes = recvfrom(clientSocket, (char*) &answer, sizeof(answer), 0, (struct sockaddr*) &fromAddress, &fromAddressLen);

	if(recievedBytes != sizeof(answer)){
		printf("Recieving failed.\n");
		*exit = 1;
		return;
	}

	if(fromAddress.sin_addr.s_addr != serverAddress.sin_addr.s_addr){
		printf("Received a packet from unknown source.\n");
		*exit = 1;
		return;
	}

	char serverName[MAXSTRINGSIZE];
	struct hostent* serverInformations = gethostbyaddr((char*) &fromAddress.sin_addr, 4, AF_INET);
	strcpy(serverName, serverInformations->h_name);

	printf("Answer recieved from the server %s, IP: %s.\n", serverName, inet_ntoa(fromAddress.sin_addr));

	int success = ntohl(answer.success);

	if(success){
		printf("Operation executed succesfully!\n%s\n", answer.message);
	} else {
		printf("Errors with the execution.\n%s\n", answer.message);
	}
}

int composeRequest(struct clientRequest* request){

	char command[MAXSTRINGSIZE];

	printf("Enter the operation you want to execute using prefix notation: ");
	fgets(command, MAXSTRINGSIZE - 1, stdin);
	fflush(stdin);

	if(command[0] == ENDOPERATOR){
		request->operator = ENDOPERATOR;
		request->firstOperand = htonl(0);
		request->secondOperand = htonl(0);
		return 1;
	}

	char operator;
	int firstOperand, secondOperand;

	int valuesEntered = sscanf(command, "%c %d %d", &operator, &firstOperand, &secondOperand);
	fflush(stdin);

	int error = 0;

	if(valuesEntered != 3){
		printf("Wrong Insertion Format.\n");
		error = 1;
	}

	switch(operator){
		case ADDOPERATOR:
		case MULTOPERATOR:
		case SUBOPERATOR:
		case DIVOPERATOR:
			break;
		default:
			printf("Wrong Operator Inserted.\n");
			error = 1;
			break;
	}

	if(error){
		printf("Retry.\n");
		return 0;
	}

	request->operator = operator;
	request->firstOperand = htonl(firstOperand);
	request->secondOperand = htonl(secondOperand);

	return 1;
}
