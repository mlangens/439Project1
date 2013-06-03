#ifndef _RPC_H
#define _RPC_H
#include <stdint.h>

#define DIV_ZERO -1
#define OK 0

#define ADD_OP 0x1
#define SUB_OP 0x2
#define MULT_OP 0x4
#define DIV_OP 0x8
#define REM_OP 0x10
#define CONT_OP 0x80

typedef struct {
	enum {
		Request, Reply
	} messageType; /* same size as an unsigned int */
	uint32_t RPCId; /* unique identifier */
	uint32_t procedureId; /* e.g. (1, 2, 3, 4) for (+, -, *, /) */
	int32_t arg1; /* argument/ return parameter */
	int32_t arg2; /* argument/ return parameter */
} RPCMessage; /* each int (and unsigned int) is 32 bits = 4 bytes */
#endif //_RPC_H
