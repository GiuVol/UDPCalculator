/*
 ============================================================================
 Name        : EsoneroUDP_Server.c
 Author      : GiuseppeVolpe
 Version     :
 Copyright   : Your copyright notice
 Description : Calculator Server
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

void clearwinsock();
int startedCorrectly();
int main(int argc, char *argv[]);
void handleConnection(int serverSocket);
void executeOperation(char operator, int firstOperand, int secondOperand, struct serverAnswer* result);
void add(int firstOperand, int secondOperand, struct serverAnswer* result);
void mult(int firstOperand, int secondOperand, struct serverAnswer* result);
void sub(int firstOperand, int secondOperand, struct serverAnswer* result);
void division(int firstOperand, int secondOperand, struct serverAnswer* result);

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
		printf("Startup failed.\n");
		return ENDEDWITHERROR;
	}

	//Defining Server Port

	int definedPort = DEFINEDSERVERPORT;

	if(argc > 1){
		definedPort = atoi(argv[1]);
	}

	//

	int serverSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	struct sockaddr_in serverAddress;

	if(serverSocket < 0){
		printf("Socket creation failed.\n");
		return ENDEDWITHERROR;
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(definedPort);
	serverAddress.sin_addr.s_addr = inet_addr(SERVERIPADDRESS);

	int bindResult = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));

	if(bindResult < 0){
		printf("Binding failed.\n");
		return ENDEDWITHERROR;
	}

	printf("Server started succesfully.\n");

	while(1){
		printf("Waiting for a new request...\n");
		handleConnection(serverSocket);
	}

	closesocket(serverSocket);
	clearwinsock();
	return 0;
}

void handleConnection(int serverSocket){
	struct sockaddr_in clientAddress;
	int clientAddressLen = sizeof(clientAddress);

	struct clientRequest request;
	memset(&request, 0, sizeof(request));

	int recievedBytes = recvfrom(serverSocket, (char*) &request, sizeof(request), 0, (struct sockaddr*) &clientAddress, &clientAddressLen);

	if(recievedBytes != sizeof(request)){
		printf("Recieving failed.\n");
		return;
	}

	request.firstOperand = ntohl(request.firstOperand);
	request.secondOperand = ntohl(request.secondOperand);

	struct hostent* clientInformations = gethostbyaddr((char*) &clientAddress.sin_addr, 4, AF_INET);
	char hostName[MAXSTRINGSIZE];
	char hostAddress[MAXSTRINGSIZE];
	strcpy(hostName, clientInformations->h_name);
	strcpy(hostAddress, inet_ntoa(clientAddress.sin_addr));

	printf("Requested operation '%c %d %d' from client %s, IP: %s.\n", request.operator, request.firstOperand, request.secondOperand, hostName, hostAddress);

	struct serverAnswer result;
	memset(&result, 0, sizeof(result));
	executeOperation(request.operator, request.firstOperand, request.secondOperand, &result);

	int sentBytes = sendto(serverSocket, (char*) &result, sizeof(result), 0, (struct sockaddr*) &clientAddress, sizeof(clientAddress));

	if(sentBytes != sizeof(result)){
		printf("Sending failed.\n");
		return;
	}
}

void executeOperation(char operator, int firstOperand, int secondOperand, struct serverAnswer* result){

	result->success = htonl(1);
	strcpy(result->message, "");

	switch(operator){
		case ADDOPERATOR:
			add(firstOperand, secondOperand, result);
			break;
		case MULTOPERATOR:
			mult(firstOperand, secondOperand, result);
			break;
		case SUBOPERATOR:
			sub(firstOperand, secondOperand, result);
			break;
		case DIVOPERATOR:
			division(firstOperand, secondOperand, result);
			break;
		default:
			strcpy(result->message, "Wrong operator inserted.");
			result->success = htonl(0);
			break;
	}
}

void add(int firstOperand, int secondOperand, struct serverAnswer* result){
	int resultValue = firstOperand + secondOperand;
	sprintf(result->message, "%d + %d = %d", firstOperand, secondOperand, resultValue);
	result->success = htonl(1);
}

void mult(int firstOperand, int secondOperand, struct serverAnswer* result){
	int resultValue = firstOperand * secondOperand;
	sprintf(result->message, "%d x %d = %d", firstOperand, secondOperand, resultValue);
	result->success = htonl(1);
}

void sub(int firstOperand, int secondOperand, struct serverAnswer* result){
	int resultValue = firstOperand - secondOperand;
	sprintf(result->message, "%d - %d = %d", firstOperand, secondOperand, resultValue);
	result->success = htonl(1);
}

void division(int firstOperand, int secondOperand, struct serverAnswer* result){
	if(secondOperand == 0){
		strcpy(result->message, "Cannot divide by 0.");
		result->success = htonl(0);
	} else {
		float resultValue = (float) firstOperand / (float) secondOperand;
		sprintf(result->message, "%d / %d = %f", firstOperand, secondOperand, resultValue);
		result->success = htonl(1);
	}
}
