#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "rpc.h"     /* struct header file*/

#define ECHOMAX 255     /* Longest string to echo */

void DieWithError(char *errorMessage); /* External error handling function */

int divide(int x, int y, int *z) {
	if (y == 0) {
		return DIV_ZERO;
	} else {
		*z = x / y;
		return OK;
	}
}

int add(int x, int y, int *z) {
	*z = x + y;
	return OK;
}

int subtract(int x, int y, int *z) {
	*z = x - y;
	return OK;
}

int multiply(int x, int y, int *z) {
	*z = x * y;
	return OK;
}

int rem(int x, int y, int *z) {
	if (y == 0) {
		return DIV_ZERO;
	} else {
		*z = x % y;
		return OK;
	}
}

int processOperation(int lastResult, RPCMessage* request, int* status) {
	//check for request type
	if (request->messageType == Request) {
		//bit and 0x80 for continuation
		if (request->procedureId & CONT_OP)
			request->arg1 = lastResult;
		if (request->procedureId & ADD_OP)
			*status = add(request->arg1, request->arg2, &lastResult);
		if (request->procedureId & SUB_OP)
			*status = subtract(request->arg1, request->arg2, &lastResult);
		if (request->procedureId & MULT_OP)
			*status = multiply(request->arg1, request->arg2, &lastResult);
		if (request->procedureId & DIV_OP)
			*status = divide(request->arg1, request->arg2, &lastResult);
		if (request->procedureId & REM_OP)
			*status = rem(request->arg1, request->arg2, &lastResult);
	}
	return lastResult;
}

void processNetworkByteOrder(RPCMessage* request) {
	//re-storing from network byte order to byte order
	request->RPCId = ntohl(request->RPCId);
	request->messageType = ntohl(request->messageType);
	request->procedureId = ntohl(request->procedureId);
	request->arg1 = ntohl(request->arg1);
	request->arg2 = ntohl(request->arg2);
}

void populateReply(const RPCMessage* request, int lastResult, int status, RPCMessage* reply) {
	//populate reply, don't need to populate procedureId
	memset(&reply, 0, sizeof(reply));
	reply->RPCId = htonl(request->RPCId);
	reply->messageType = htonl(Reply);
	reply->arg1 = htonl(lastResult);
	reply->arg2 = htonl(status);
}

void sendDataToClient(int sock, const RPCMessage* reply, const struct sockaddr_in* echoClntAddr, int recvMsgSize) {
	/* Send received datagram back to the client */
	if (sendto(sock, &*reply, sizeof(*reply), 0, (struct sockaddr*) &*echoClntAddr, sizeof(*echoClntAddr))
			!= recvMsgSize)
		DieWithError("sendto() sent a different number of bytes than expected");
}

int blockUntilMsgRecv(int recvMsgSize, int sock, unsigned int cliAddrLen, RPCMessage* request,
		struct sockaddr_in* echoClntAddr) {
	/* Block until receive message from a client */
	if ((recvMsgSize = recvfrom(sock, &*request, sizeof(*request), 0, (struct sockaddr*) &*echoClntAddr, &cliAddrLen))
			< 0)
		DieWithError("recvfrom() failed");

	return recvMsgSize;
}

int main(int argc, char **argv) {
	int sock; /* Socket */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in echoClntAddr; /* Client address */
	unsigned int cliAddrLen; /* Length of incoming message */
	RPCMessage request, reply;
	unsigned short echoServPort; /* Server port */
	int recvMsgSize; /* Size of received message */
	int lastResult = 0;
	int status;

	if (argc != 2) /* Test for correct number of parameters */
	{
		fprintf(stderr, "Usage:  %s <UDP SERVER PORT>\n", argv[0]);
		exit(1);
	}

	echoServPort = atoi(argv[1]); /* First arg:  local port */

	/* Create socket for sending/receiving datagrams */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	/* Zero out structure */
	echoServAddr.sin_family = AF_INET; /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
	echoServAddr.sin_port = htons(echoServPort); /* Local port */

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed");

	for (;;) { /* Run forever */
		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);

		/* Block until receive message from a client */
		recvMsgSize = blockUntilMsgRecv(recvMsgSize, sock, cliAddrLen, &request, &echoClntAddr);

		//re-storing from network byte order to byte order
		processNetworkByteOrder(&request);
		//check for request type
		lastResult = processOperation(lastResult, &request, &status);

		//populate reply, don't need to populate procedureId
		populateReply(&request, lastResult, status, &reply);
		printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

		/* Send received datagram back to the client */
		sendDataToClient(sock, &reply, &echoClntAddr, recvMsgSize);
	}
	/* NOT REACHED */
}
