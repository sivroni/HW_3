#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 10

//pthread_mutex_t lock; // our global mutex
typdedef struct myListsDatabase{
	struct node * head; // doubly linked list
	pthread_mutex_t lock; // mutex for "head" list
}myListsDatabase;

myListsDatabase databaseList [MAX]; // global var

struct node {
	int value; // assume positive integer
	node * prev; // pointer to previous node
	node * next; // pointer to next node;
	int size; // number of elements in list
};

// headers:
void intlist_init(node * head);
void intlist_destroy(node * head);
void intlist_push_head(node * head, int val);
int intlist_pop_tail(node * head);
void intlist_remove_last_k(node * head, int k);
int intlist_size(node * head);
pthread_mutex_t * intlist_get_mutex(node * head);

void intlist_init(node * head){
	//TODO check ptr?
	//what is the argument?
	int returnVal; // return value from creating the mutex
	struct node * newNode = (struct node *) malloc(sizeof(struct node));
	if (newNode == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		exit(errno);
	}
	newNode->size = 0;
	newNode->prev = NULL;
	newNode->next = NULL;

	// create a new object including list and mutex
	myListsDatabase data = (myListsDatabase) malloc(sizeof(myListsDatabase));
	if (data ==NULL){
		printf("Error allocation memory for new data struct: %s\n", strerror(errno));
		exit(errno);
	}

	// create new mutex for current list and check for errors
	returnVal = pthread_mutex_init(&(data->lock), NULL);
	if (returnVal) {
		printf("ERROR in pthread_mutex_init(): %s\n", strerror(rc));
		exit(-1); //TODO chanfe to errno?
	}

	data->head = newNode;

	// allocate the bug database list



}



void intlist_destroy(node * head){

}

void intlist_push_head(node * head, int val){
	if (head == NULL)
		return;

	node * oldHead = head; // save old head
	struct node * newNode = (struct node *) malloc(sizeof(struct node));
	if (newNode == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		exit(errno);
	}
	newNode->prev =NULL; // this is the head now
	newNode->next = oldHead;
	oldHead->prev = newNode;
	newNode->value = val;
	newNode->size = oldHead->size + 1;
}





