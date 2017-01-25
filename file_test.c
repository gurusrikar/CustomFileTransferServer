#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "msgs.h"
#include <unistd.h>

#define MSG_METADATA_INDEX 0
#define MAX_CONCURRENT_TRANSFERS 3
#define ONE_MB 1048576 //1 MB = 1,048,576 Bytes


void printMessage(int serverPort, int clientPort, message *replyMessage);
void printServerMessage(int serverId, int serverPort, int clientPort, message *incomingMessage);
message * allocateNewMessage(int replyPortNumber, int messgeType);
int packFourCharsInInt(char *a);
void unPackIntToFourChars(int n, char *res);
void serverSendMessage(message *msg, int msgType);
void clientSendMessage(message *msg, int msgType);
void goToLimbo(int clientId, char *msg);
int serverId = 1;

typedef enum MessageType {
	MSG_TYPE_TRANSFER_BEGIN = 0,
    MSG_TYPE_FILE_NAME_LEN  = 1,
    MSG_TYPE_FILE_NAME      = 2,
    MSG_TYPE_FILE_DATA      = 3,
    MSG_TYPE_TRANSFER_DONE  = 4,
    MSG_TYPE_TRANSFER_END   = 5,
    MSG_TYPE_TRANSFER_ERROR = 6,
    MSG_TYPE_MAX_LIMIT_REACHED = 7,
    MSG_TYPE_SERVER_OK      = 8,
    MSG_TYPE_SERVER_NOT_OK  = 9
} MessageType;

void server (int *arg) {
    message *incomingMessage, *outgoingMessage;
    int myServerId = serverId++;
    int recievedBytes = 0, runningTransfers = 0, numOfBytes[100];
    char file[100][20];
    for (int i=0; i< 100; i++) {
    	strcpy(file[i], "\0");
    }

    printf("Server %d started on port %d\n", myServerId, *arg);

    while (1) {
    	// Recieve msgs from clients on server port 
    	recieve(SERVER_PORT, &incomingMessage);
    	printServerMessage(myServerId, SERVER_PORT, incomingMessage->replyPortNumber, incomingMessage);

    	switch (incomingMessage->content[MSG_METADATA_INDEX]) {
    		case MSG_TYPE_TRANSFER_BEGIN:
    			printf("\t\t\t\trecieved transfer begin msg\n");
    			if (runningTransfers == MAX_CONCURRENT_TRANSFERS) {
    				// Send not ok message
    				serverSendMessage(incomingMessage, MSG_TYPE_SERVER_NOT_OK);
    			} else {
    				runningTransfers++;
    				// Send ok message
    				serverSendMessage(incomingMessage, MSG_TYPE_SERVER_OK);
    			}
    			break;
    		
    		case MSG_TYPE_FILE_NAME_LEN:
    			if (incomingMessage->content[1] >= 15) {
    				serverSendMessage(incomingMessage, MSG_TYPE_SERVER_NOT_OK);
    			} else {
    				serverSendMessage(incomingMessage, MSG_TYPE_SERVER_OK);
    			}
    			break;
    		case MSG_TYPE_FILE_NAME:
    			printf("\t\t\t\trecieved file name msg: ");
    			char fname[17] = "\0";
    			
    			for(int i=1; i< 5; i++) {
    				char str[4] = "\0";
    				unPackIntToFourChars(incomingMessage->content[i], str);
    				strcat(fname, str);
    				strcat(fname, "\0");
    			}
    			//fname[9] = '\0';
    			strcat(fname, ".server");
    			// Create a file with this name
    			FILE *serverFp1;
    			serverFp1 = fopen(fname, "a");
    			fclose(serverFp1);

    			strcpy(file[incomingMessage->replyPortNumber], fname);
    			serverSendMessage(incomingMessage, MSG_TYPE_SERVER_OK);
    			break;
    		
    		case MSG_TYPE_FILE_DATA:
    			recievedBytes += 36;
    			if (numOfBytes[incomingMessage->replyPortNumber] >= ONE_MB) {
    				serverSendMessage(incomingMessage, MSG_TYPE_MAX_LIMIT_REACHED);
    			} else {
	    			FILE *serverFp;
	    			char rfname[17] = "\0";
					strcpy(rfname, file[incomingMessage->replyPortNumber]);
	    			serverFp = fopen(rfname, "a");
	    			for(int i=1; i< MSG_SIZE; i++) {
	    				char str[4] = "\0";
	    				unPackIntToFourChars(incomingMessage->content[i], str);
	    				numOfBytes[incomingMessage->replyPortNumber] += strlen(str);
	    				fwrite(str, 1, strlen(str), serverFp);
	    			}
	    			fclose(serverFp);
	    			serverSendMessage(incomingMessage, MSG_TYPE_SERVER_OK);
	    			printf("\t\t\t\tBytes recieved from client %d: %d \n", incomingMessage->replyPortNumber, numOfBytes[incomingMessage->replyPortNumber]);
    			}
    			break;
    		
    		case MSG_TYPE_TRANSFER_DONE:
    			printf("\t\t\t\trecieved file transfer done msg: \n");
    			runningTransfers--;
				serverSendMessage(incomingMessage, MSG_TYPE_TRANSFER_END);
    			break;
    		
    		default:
    			printf("Dont know what to do server\n");
    	}
    }
}

void client (int *arg, char * filename) {
	printf("%s\n", filename);
	int count = 5 + 100 * *arg, replyPortNumber = *arg, connEstablished = 0;
	message *newMessage, *replyMessage;
	printf("Client started on port %d\n", *arg);
	
	newMessage = (message *) malloc(sizeof(message));
	newMessage->replyPortNumber = replyPortNumber;
	
	// Send start transfer message and retry until you recieve begin ok message
	while (!connEstablished) {
		clientSendMessage(newMessage, MSG_TYPE_TRANSFER_BEGIN);
		recieve(*arg, &replyMessage);
		switch (replyMessage->content[MSG_METADATA_INDEX]) {
			case MSG_TYPE_SERVER_OK:
				printf("recieved transfer begin ok\n");
				connEstablished = 1;
				break;
				
			case MSG_TYPE_SERVER_NOT_OK:
				printf("Client %d: Server is busy retry after sometime\n", replyPortNumber);
				yield();
				// sleep(2);
				break;
			default:
				printf("Recieved inappropriate message from server\n");
    	}
	}

	// Confirm with the server about the filename length
	int nameLength = strlen(filename);
	
	replyMessage->content[MSG_METADATA_INDEX] = MSG_TYPE_FILE_NAME_LEN;
	replyMessage->content[1] = nameLength;
	
	clientSendMessage(replyMessage, MSG_TYPE_FILE_NAME_LEN);
	recieve(*arg, &replyMessage);
	
	// If server is not ok with filename length - do not proceed and goto limbo
	if (replyMessage->content[MSG_METADATA_INDEX] == MSG_TYPE_SERVER_NOT_OK)
		goToLimbo(replyPortNumber,"filename too long");

	// Pack and send filename string to the server 
	replyMessage->content[MSG_METADATA_INDEX] = MSG_TYPE_FILE_NAME;
	
	int msgIndex = 1, charIndex = 0;
	for (int i=0; i<nameLength; i+=4) {
		char packet[4];
		packet[0] = charIndex < nameLength ? filename[charIndex++] : '\0';
		packet[1] = charIndex < nameLength ? filename[charIndex++] : '\0';
		packet[2] = charIndex < nameLength ? filename[charIndex++] : '\0';
		packet[3] = charIndex < nameLength ? filename[charIndex++] : '\0';
		replyMessage->content[msgIndex++] = packFourCharsInInt(packet);
	}

	for(msgIndex; msgIndex<MSG_SIZE; msgIndex++) {
		// Fill remaining items of the message as \0 s
		replyMessage->content[msgIndex] = '\0';
	}

	replyMessage->replyPortNumber = replyPortNumber;
	clientSendMessage(replyMessage, MSG_TYPE_FILE_NAME);

    // Send over data if server is Ready.
    recieve(*arg, &replyMessage);
	switch (replyMessage->content[MSG_METADATA_INDEX]) {
		case MSG_TYPE_SERVER_OK:
			printf("recieved file name ok\n");
			
			// A message carries one metadata integer and 9 integers (each of them packing 4 characters)
			FILE *fp;
			char buff[37];
			fp = fopen(filename, "r");
			message *newMessage1;
			if (fp) {
				/* File was opened successfully. */
				int bytesRead = fread(buff, 1, 36, fp);
				buff[36] = '\0';
				while (bytesRead == 36) {
					char packet[4];
					int charIndex = 0, msgIndex = 1;
					newMessage1 = allocateNewMessage(replyPortNumber, MSG_TYPE_FILE_DATA);
					while(charIndex <= 36) {
						packet[0] = buff[charIndex++];
						packet[1] = buff[charIndex++];
						packet[2] = buff[charIndex++];
						packet[3] = buff[charIndex++];
						newMessage1->content[msgIndex++] = packFourCharsInInt(packet);
					}
					newMessage1->replyPortNumber = replyPortNumber;
					clientSendMessage(newMessage1, MSG_TYPE_FILE_DATA);
					recieve(*arg, &replyMessage);
					if (replyMessage->content[MSG_METADATA_INDEX] == MSG_TYPE_MAX_LIMIT_REACHED)
						goToLimbo(replyPortNumber, "transfer limit(1 MB) reached");
					for (int i=0; i<37; i++)
						buff[i] = '\0';
					bytesRead = fread(buff, 1, 36, fp);
				}
				if(bytesRead) {
					// Remaining bytes to send
					char packet[4];
					int charIndex = 0, msgIndex = 1;
					newMessage1 = allocateNewMessage(replyPortNumber, MSG_TYPE_FILE_DATA);
					while(charIndex <= bytesRead) {
						packet[0] = buff[charIndex++];
						packet[1] = charIndex <= bytesRead ? buff[charIndex++] : '\0';
						packet[2] = charIndex <= bytesRead ? buff[charIndex++] : '\0';
						packet[3] = charIndex <= bytesRead ? buff[charIndex++] : '\0';
						newMessage1->content[msgIndex++] = packFourCharsInInt(packet);
					}
					newMessage1->replyPortNumber = replyPortNumber;
					clientSendMessage(newMessage1, MSG_TYPE_FILE_DATA);
					recieve(*arg, &replyMessage);
					if (replyMessage->content[MSG_METADATA_INDEX] == MSG_TYPE_MAX_LIMIT_REACHED)
						goToLimbo(replyPortNumber,"transfer limit reached");
				}

				fclose(fp);
			}
			// Send a transfer done message to server
			replyMessage->replyPortNumber = replyPortNumber;
			clientSendMessage(replyMessage, MSG_TYPE_TRANSFER_DONE);
			break;
		default:
			printf("Recieved inappropriate message from server\n");
    }

	// Wait for transfer end acknowledgement from server
	recieve(*arg, &replyMessage);
	switch (replyMessage->content[MSG_METADATA_INDEX]) {
		case MSG_TYPE_TRANSFER_END:
			printf("Recieved file transfer end msg\n");
			break;
		default:
			printf("Dont know what to do end\n");
    }

    goToLimbo(replyPortNumber, "finished");

}


int main(int argc, char * argv []) {
	init_all_ports();
	ReadyQ = newQueue();
	int i =0, a[100], nClients = 0;
	
	for(i=0;i<100;i++) {
		a[i] = i;
	}
	
	nClients = atoi(argv[1]);

	for (int i=1; i<=nClients; i++) {
		start_thread(client, &a[i], argv[i+1]);
	}

	printf("starting thread");
	start_thread(server, &a[0], NULL);
    
    run();
}


void printMessage(int serverPort, int clientPort, message *replyMessage) {
	printf("Client (Port %d) recieved reply: %d\n", clientPort, replyMessage->content[0]);
	return;
}

void printServerMessage(int serverId, int serverPort, int clientPort, message *incomingMessage) {
	printf("\t\t\t\tServer Id %d (port: %d) recieved message from %d\n", serverId, serverPort, clientPort);
	return;
}

message * allocateNewMessage(int replyPortNumber, int messgeType) {
	message* newMessage = (message *)malloc(sizeof(message));
	newMessage->replyPortNumber = replyPortNumber;
	newMessage->content[MSG_METADATA_INDEX] = messgeType;
	return newMessage;
}

void serverSendMessage(message *msg, int msgType) {
	msg->content[MSG_METADATA_INDEX] = msgType;
    send(msg->replyPortNumber, msg);
}

void clientSendMessage(message *msg, int msgType) {
	msg->content[MSG_METADATA_INDEX] = msgType;
    send(SERVER_PORT, msg);
}

int packFourCharsInInt(char *a) {
	int b = 0;
	int i = 0;
	b = a[i++];
	while(i <= 3) {
		b = b << 8;
		b = b | a[i++];
	}
	return b;
}

void unPackIntToFourChars(int n, char *res) {
	char d,c,b,a;
	a = (n & (255 << 24)) >> 24;
	b = (n & (255 << 16)) >> 16;
	c = (n & (255 << 8)) >> 8;
	d = n & 255;
	res[0] = a;
	res[1] = b;
	res[2] = c;
	res[3] = d;
}

void goToLimbo(int clientId, char *msg) {
	while (1) {
		printf("Client %d: %s\n", clientId, msg);
		// sleep(1);
		yield();
	}
}
