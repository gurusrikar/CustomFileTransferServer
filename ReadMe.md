# Custom FIle Transfer Server

This file server was implemented as part of coursework for Distributed and Multiprocessor Operating Systems:

 * Implemented user level thread scheduling using swapcontext and yield.
 * Implemented semaphores (queue implementation).
 * Implemented port messaging infrastructure for user level threads to communicate with each other using the semaphores (as mentioned above).
 * Implemented file transfer protocol using the port messaging infrastructure (as mentioned above).


### System Design and Implementation

Each message is a represented as a set of 10 integers. The first integer is being used as metadata to identify the type of message. Following are the types of messages.

 * MSG_TYPE_TRANSFER_BEGIN = 0
 * MSG_TYPE_FILE_NAME_LEN = 1
 * MSG_TYPE_FILE_NAME = 2
 * MSG_TYPE_FILE_DATA = 3
 * MSG_TYPE_TRANSFER_DONE = 4
 * MSG_TYPE_TRANSFER_END = 5
 * MSG_TYPE_TRANSFER_ERROR = 6
 * MSG_TYPE_MAX_LIMIT_REACHED = 7
 * MSG_TYPE_SERVER_OK = 8
 * MSG_TYPE_SERVER_NOT_OK = 9

#### The handshakes is as follows:

###### Client 1,2,3..n:
 * Begin transfer message to server.
 * Ok msg from server.
 * File name length msg to server.
 * Ok msg from server.
 * File name msg to server
 * Ok msg from server.
 * Repeatedly send File data msg to server, 36 characters packed in 4 integers
 * Repeatedly recieve ok msg from server.
 * If transfer limit is exceeded, a TLE message is recieved and the client stops sending more data.
 * Message the server transfer end message.
 * Recieve ack from server and go to hibernation.

###### Server:
 * Repeatedly recieve a message from any of the clients and do the following.
 * For transfer Begin message, check number of running transfers and deny more requests.
 * For filename length message, deny if length is more than 15 characters.
 * For file data message, keep a running count of number of bytes recieved and deny if limit is exceeded.
 * After a file transfer done message is received, close the file and reduce the number of running transfers count.

### Run instructions

In a terminal, cd into the repository and run the follwing commands.

```sh
$ gcc file_test.c
$ ./a.out n file1 file2 file2... filem
```

n: number of clients; m: number of files to transfer (m can be >> n). Files named as file1_server, file2_server... would have been created.
<br />
Note that the number of concurrent transfers is set to 3 and can be changed by updating the macro MAX_CONCURRENT_TRANSFERS in file_test.c


### Copyright (in plain English):

 * Please **do not** use this for any of your academic projects/research. This repository is made public only for reference and to demonstrate my learnings out of it.
 * You are welcome to use this code for your personal projects, if you want to tweak/make improvements etc.
 * Respect other developer's in the community and their work.