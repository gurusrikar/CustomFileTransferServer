// #include <stdio.h>
#include <ucontext.h>
#define DUMMY -1
#define NUM_MSGS 10

typedef struct message
{
	int content[NUM_MSGS];
	int replyPortNumber;
} message;

struct element
{
	struct element* previous;
	struct element* next;
	int thread_id;
	message *m;
	ucontext_t context;
};

typedef struct element TCB_t;

#include "TCB.h"

TCB_t* NewItem() {
	TCB_t* item = (TCB_t*) malloc(sizeof(TCB_t));
	item->previous = NULL;
	item->next = NULL;
	return item;
}

TCB_t* newQueue() {
	TCB_t* head;
	head = NewItem();//(TCB_t*) malloc(sizeof(TCB_t));
	head->thread_id = DUMMY;
	head->previous = head;
	head->next = head;

	return head;
}

void AddQueue(TCB_t* head, TCB_t* item) {
	if (head == NULL || item == NULL) {
		printf("invalid head or item pointer\n");
		return;
	}

	TCB_t* temp = head;
	while(temp->next != NULL && temp->next != head)
		temp = temp->next;

	temp->next = item;
	item->previous = temp;
	item->next = head;
}

TCB_t* DelQueue(TCB_t* head) {

	if (head == NULL) {
		printf("invalid head pointer\n");
		return NULL;
	}

	if(head->next != NULL && head->next != head) {
		TCB_t* temp = head->next;
		head->next = temp->next;
		if(temp->next != NULL)
			temp->next->previous = head;
		
		temp->next = NULL;
		temp->previous = NULL;
		return temp;
	} else {
		return NULL;
	}

}

void printQueue(TCB_t* head) {
	TCB_t* temp = head;
	if (head == NULL || head->next == head) return;
	while(temp->next != NULL && temp->next != head) {
		printf("%d\t", temp->next->thread_id);
		temp = temp->next;
	}
	printf("\n");
	return;
}