#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "rpc.h"     /* struct header file*/

#define ECHOMAX 255     /* Longest string to echo */

void DieWithError(char *errorMessage); /* External error handling function */

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
	int num1, num2; //operands
	char operation; //char for operator

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
		printf("Enter first number:"); /*Ask user for 1st number*/
		scanf("%d", &num1); /*Read 1st number*/
		printf("Enter operation: "); /*Ask user for operation*/
		scanf("%c", &operation); /*Read operation*/
		printf("Enter second number:"); /*Ask user for 2nd number*/
		scanf("%d", &num2); /*Read 2nd number*/

		//set request object
		request.messageType = htonl(Request);
		request.RPCId = htonl(rpcid++);
		request.arg1 = htonl(num1);
		request.arg2 = htonl(num2);
		if (operation == '+')
			request.procedureId = htonl(ADD_OP);
		if (operation == '-')
			request.procedureId = htonl(SUB_OP);
		if (operation == '*')
			request.procedureId = htonl(MULT_OP);
		if (operation == '/')
			request.procedureId = htonl(DIV_OP);
		if (operation == '%')
			request.procedureId = htonl(REM_OP);

		/* Send the string to the server */
		if (sendto(sock, &request, sizeof(request), 0,
				(struct sockaddr *) &echoServAddr, sizeof(echoServAddr))
				!= sizeof(request))
			DieWithError(
					"sendto() sent a different number of bytes than expected");

		/* Recv a response */
		fromSize = sizeof(fromAddr);
		if ((respStringLen = recvfrom(sock, &reply, sizeof(reply), 0,
				(struct sockaddr *) &fromAddr, &fromSize)) != sizeof(reply))
			DieWithError("recvfrom() failed");

		//re-storing from network byte order to byte order
		reply.RPCId = ntohl(reply.RPCId);
		reply.messageType = ntohl(reply.messageType);
		reply.procedureId = ntohl(reply.procedureId);
		reply.arg1 = ntohl(reply.arg1);
		reply.arg2 = ntohl(reply.arg2);

		if (ntohl(request.RPCId) != reply.RPCId) {
			fprintf(stdout, "RPCid mismatch %d != %d\n", ntohl(request.RPCId),
					reply.RPCId);
			//exit(1);
		}
		if (reply.arg2 != OK) {
			fprintf(stdout, "Illegal op %d\n", reply.arg2);
			//exit(1);
		}
		if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
			fprintf(stderr, "Error: received a packet from unknown source.\n");
			//exit(1);
		}

		/* null-terminate the received data */
		printf("Result = : %d\n", reply.arg1); /* Print the echoed arg */
	}
	close(sock);
	exit(0);
}
