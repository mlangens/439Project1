#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "rpc.h"     /* struct header file*/

#define ECHOMAX 255     /* Longest string to echo */

void DieWithError(char *errorMessage); /* External error handling function */

void chooseOperator(char operation, RPCMessage* request) {
	if (operation == '+')
		request.procedureId = htonl(ADD_OP);
	else if (operation == '-')
		request.procedureId = htonl(SUB_OP);
	else if (operation == '*')
		request.procedureId = htonl(MULT_OP);
	else if (operation == '/')
		request.procedureId = htonl(DIV_OP);
	else if (operation == '%')
		request.procedureId = htonl(REM_OP);
}

int setRequest(int rpcid, int operand1, int operand2, RPCMessage* request) {
	//set request object
	request.messageType = htonl(Request);
	request.RPCId = htonl(rpcid++);
	request.arg1 = htonl(operand1);
	request.arg2 = htonl(operand2);
	return rpcid;
}

void processNetworkByteOrder(RPCMessage* reply) {
	//re-storing from network byte order to byte order
	reply.RPCId = ntohl(reply.RPCId);
	reply.messageType = ntohl(reply.messageType);
	reply.procedureId = ntohl(reply.procedureId);
	reply.arg1 = ntohl(reply.arg1);
	reply.arg2 = ntohl(reply.arg2);
}

void handleErrors(const RPCMessage* request, const struct sockaddr_in* echoServAddr, const struct sockaddr_in* fromAddr,
		RPCMessage* reply) {
	if (ntohl(request.RPCId) != reply.RPCId) {
		fprintf(stdout, "RPCid mismatch %d != %d\n", ntohl(request.RPCId), reply.RPCId);
		exit(1);
	}
	if (reply.arg2 != OK) {
		fprintf(stdout, "Illegal op %d\n", reply.arg2);
		exit(1);
	}
	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
		fprintf(stderr, "Error: received a packet from unknown source.\n");
		exit(1);
	}
}

void sendToServer(int sock, const RPCMessage* request, const struct sockaddr_in* echoServAddr) {
	/* Send the string to the server */
	if (sendto(sock, &*request, sizeof(*request), 0, (struct sockaddr*) &*echoServAddr, sizeof(*echoServAddr))
			!= sizeof(*request))
		DieWithError("sendto() sent a different number of bytes than expected");
}

void recvResponse(unsigned int fromSize, int respStringLen, int sock, struct sockaddr_in* fromAddr, RPCMessage* reply) {
	/* Recv a response */
	fromSize = sizeof(*fromAddr);
	if ((respStringLen = recvfrom(sock, &*reply, sizeof(*reply), 0, (struct sockaddr*) &*fromAddr, &fromSize))
			!= sizeof(*reply))
		DieWithError("recvfrom() failed");
}

int processInput(char inputExpr[255], int* operand1, char* operation, int* operand2, int* rpcid, RPCMessage* request) {
	int cont = 0;
	printf("Enter Expression: ");
	//flush input
	fflush(stdout);
	fgets(inputExpr, 255, stdin);
	if (isdigit(inputExpr[0])) {
		sscanf(inputExpr, "%d %c %d", &*operand1, &*operation, &*operand2);
	} else {
		sscanf(inputExpr, "%c %d", &*operation, &*operand2);
		cont = 1;
	}
	//set request object
	*rpcid = setRequest(*rpcid, *operand1, *operand2, &*request);
	chooseOperator(*operation, &*request);
	//if there are more operations
	if (cont) {
		request.procedureId |= htonl(CONT_OP);
	}
	return cont;
}

int main(int argc, char *argv[]) {
	int sock; /* Socket descriptor */
	struct sockaddr_in echoServAddr; /* Echo server address */
	struct sockaddr_in fromAddr; /* Source address of echo */
	unsigned short echoServPort; /* Echo server port */
	unsigned int fromSize; /* In-out of address size for recvfrom() */
	char *servIP; /* IP address of server */
	RPCMessage request, reply;
	int echoStringLen; /* Length of string to echo */
	int respStringLen; /* Length of received response */
	int rpcid = 0;
	int operand1;
	int operand2;
	char operation;
	char inputExpr[255];

	if ((argc < 2) || (argc > 3)) /* Test for correct number of arguments */
	{
		fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
		exit(1);
	}

	servIP = argv[1]; /* First arg: server IP address (dotted quad) */

	if (argc == 3)
		echoServPort = atoi(argv[2]); /* Use given port, if any */
	else
		echoServPort = 3333; /* 7 is the well-known port for the echo service */

	/* Create a datagram/UDP socket */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	/* Zero out structure */
	echoServAddr.sin_family = AF_INET; /* Internet addr family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP); /* Server IP address */
	echoServAddr.sin_port = htons(echoServPort); /* Server port */

	while (1) {
		int cont = processInput(inputExpr, &operand1, &operation, &operand2, &rpcid, &request);
		/* Send the string to the server */
		sendToServer(sock, &request, &echoServAddr);

		/* Recv a response */
		recvResponse(fromSize, respStringLen, sock, &fromAddr, &reply);
		//re-storing from network byte order to byte order
		processNetworkByteOrder(&reply);
		handleErrors(&request, &echoServAddr, &fromAddr, &reply);
		/* null-terminate the received data */
		printf("Result = : %d\n", reply.arg1); /* Print the echoed arg */
	}
	close(sock);
	exit(0);
}
