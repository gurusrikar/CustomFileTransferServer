#include "sem.h"

#define NUM_MSGS 10
#define MSG_SIZE 10
#define NUM_PORTS 100

#define SERVER_PORT 0

// int message[MSG_SIZE];

// int** port[NUM_MSGS];
typedef struct port
{
	int portNumber;
	TCB_t* msgQ;
	Semaphore_t *full;
	Semaphore_t *empty;
	Semaphore_t *mutex;
} port;

int runningPortNumber = 0;
//Semaphore_t *mutex;
port *p[NUM_PORTS];

port* create_port() {
	port *p = (port *)malloc(sizeof(port));
	p->portNumber = runningPortNumber++;
	p->msgQ = newQueue();
	p->full = CreateSem(0);
	p->empty = CreateSem(NUM_MSGS);
	return p;
}

void init_all_ports() {
	for (int i=0; i < NUM_PORTS; i++) {
		p[i] = create_port();
		p[i]->mutex = CreateSem(1);
	}
	return;
}

void send(int portNumber, message *sendMsgItem) {
	port *sendPort = p[portNumber];
	P(sendPort->empty);
		//printf("Sending to port: %d\n", p->portNumber);
		P(sendPort->mutex);
			//printf("Writing to port buffer: %d\n", p->portNumber);
			TCB_t* msgQItem = NewItem();
			msgQItem->m = sendMsgItem;
			AddQueue(sendPort->msgQ, msgQItem);
		V(sendPort->mutex);
	V(sendPort->full);
}

void recieve(int portNumber, message **recieveMsgItem) {
	port *recievePort = p[portNumber];
	P(recievePort->full);
		//printf("Recieving from port: %d\n", p->portNumber);
		P(recievePort->mutex);
			//printf("Reading from port buffer: %d\n", p->portNumber);
			TCB_t* nextMsgQItem = DelQueue(recievePort->msgQ);
			*recieveMsgItem = nextMsgQItem->m;
		V(recievePort->mutex);
	V(recievePort->empty);
}

void print_port(port *p) {
	printf("port_num %d\n", p->portNumber);
	for(int i=0; i<10;i++) {
		for(int j=0; j<10;j++) {
			printf("%d-", p->portNumber);
		}
		printf("\n");
	}
}