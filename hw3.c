#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SIZE 10 // init database to this size at first

//pthread_mutex_t lock; // our global mutex
/*typdedef struct myListsDatabase{
	struct node * head; // doubly linked list
	pthread_mutex_t lock; // mutex for "head" list
}myListsDatabase;*/

// Notes:
// 1. Free memory when needed + errors
// 2. Make sure I used locks for the minimum time + minimum lock/unlock operations

// data structure including list and mutex
struct intlist {
	int value; // assume positive integer
	intlist * prev; // pointer to previous node 
	intlist * next; // pointer to next node;
	int size; // number of elements in list
	pthread_mutex_t lock;
	pthread_cond_t notEmpty;
};

intlist * databaseList = NULL; // global var - list of ALL doubly linked-lists
int numOfLists = 0; // global var - sum of ALL existing doubly linked-lists
int databaseSize = INITIAL_SIZE; // global var - number of the size of databaseList
pthread_cond_t GC; // garbage collector - currently we have only one

// headers:
void intlist_init(intlist * list);
void intlist_destroy(intlist * list);
void intlist_push_head(intlist * list, int val);
int intlist_pop_tail(intlist * list);
void intlist_remove_last_k(intlist * list, int k);
int intlist_size(intlist * list);
pthread_mutex_t * intlist_get_mutex(intlist * list);


void intlist_init(intlist * list){
	//TODO check ptr?
	int returnVal; // return value from creating the mutex
	int foundOpenSpot = 0; // flag for finding a new opening in database
	int i; // iteration index

	// create a new list
	struct intlist * list = (struct intlist *) malloc(sizeof(struct intlist));
	if (list == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		exit(errno);
	}

	// init fields
	list->size = 0;
	list->prev = NULL;
	list->next = NULL;

	// create new mutex for current list and check for errors
	returnVal = pthread_mutex_init(&(list->lock), NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_init(): %s\n", strerror(returnVal);
		free(list);
		exit(errno); 
	}

	// create new conditional variable for current list and check for errors
	returnVal = pthread_cond_init(&(list->notEmpty), NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init(): %s\n", strerror(returnVal));
		pthread_mutex_destroy(&(list->lock));
		free(list);
		//TODO destroy more
		exit(errno); 
	}


	// allocate new list of doublylinked list if not created yet
	if (databaseList == NULL){
		databaseList = (intlist *) malloc(sizeof(intlist)*databaseSize);
		if (databaseList == NULL){
			printf("Error allocation memory for new data struct: %s\n", strerror(errno));
			free(list);
			pthread_mutex_destroy(&(list->lock));
			pthread_cond_destroy(&(list->notEmpty));
			exit(errno);
		}
	}
	
	// now we can assume a database is already allocate.
	// add the new doubly linked list (called "list") to the databaseList
	// means - find an opening in databaseList. 
	// If none is avaliable - double the size of databaseList and then add the new "list"

	numOfLists++; // update counter of all lists

	for (i=0; i<numOfLists; i++){
		if (databaseList[i] == NULL){ // found an open spot
			databaseList[i] = list; // add to database
			foundOpenSpot = 1; // mark that an open spot was found
			break; // no need for further search
		}
	}

	// if no open spot was found -- database is full -- double database size
	if (!foundOpenSpot){ 
		databaseSize = databaseSize * 2;
		databaseList = (intlist *) realloc(databaseList, sizeof(intlist)*databaseSize);
		if (databaseList == NULL){
			printf("Error allocation memory for new data struct: %s\n", strerror(errno));
			free(list);
			pthread_mutex_destroy(&lock);
			//TODO free database and lists in it
			exit(errno);
		}
		// move new list to an open spot
		databaseList[ (databaseSize / 2) + 1 ] = list;
	}

	

}



void intlist_push_head(intlist * list, int val){
	if (list == NULL)
		return;

	intlist * oldHead = list; // save old head
	struct intlist * newNode = (struct intlist *) malloc(sizeof(struct intlist));
	if (newNode == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		//TODO free more - maybe destroy?
		exit(errno);
	}
	newNode->prev = NULL; // this is the head now
	newNode->value = val; // update value

	int returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n");
		free(list);
		exit(-1); 
	}
	newNode->next = oldHead; // points to old head
	oldHead->prev = newNode; // points to new node
	newNode->size = oldHead->size + 1; // update size
	newNode->lock = oldHead->lock; // both nodes share the same mutex

	list = newNode; // list suppose to point to the new head

	returnVal = pthread_cond_signal(&(list->notEmpty)); // signal that list is not empty
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_signal(): %s\n");
		free(list);
		exit(-1); 
	}
	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk(): %s\n");
		free(list);
		exit(-1); 
	}
}



pthread_mutex_t * intlist_get_mutex(intlist * list){
	if (list == NULL){
		printf("Lock not found - list is NULL!\n");
		return NULL;
		//TODO need to exit program??
	}

	return &(list->lock); // check if its true at all....
}



int intlist_size(intlist * list){
	if (list == NULL){
		printf("Size not found - list is NULL!\n");
		return NULL;
		//TODO need to exit program??
	}

	return (list->size); 
}


int intlist_pop_tail(intlist * list){
	if (list == NULL){
		printf("Cant pop tail - list is NULL!\n");
		return NULL;
		//TODO need to exit program??
	}
	int popValue; // value to be returned
	int returnVal; // return value to be check from varoits functions
	returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n");
		free(list);
		exit(-1); 
	}
	while (list->size == 0){ // list is empty
			pthread_cond_wait(&(list->notEmpty), &(list->lock)); // wait here until list has at least one item
	}
	// pop tail:
	struct intlist * current = list;
	while (current->next != NULL){
				current = current->next;
	}
	// current is now the tail to be removed
	popValue = current->value;
	(list->size)--;
	free(current); // check if it sets to (current->prev)->next to NULL
	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n");
		free(list);
		exit(-1); 
	}

	return popValue;
	 
}



void intlist_remove_last_k(intlist * list, int k){
	if ((list == NULL) || (k<1)){ // check for invalid arguments
		printf("Cant pop tail - list is NULL!\n");
		return NULL;
		//TODO need to exit program??
	}

	int i; // iteration index
	for (i=0; i<k; i++){
		intlist_pop_tail(list);
	}

}

void intlist_destroy(intlist * list){
	if (list == NULL) // nothing to free
		return;

	int size = list->size;
	int i;
	int returnVal;

	returnVal = pthread_mutex_destroy(intlist_get_mutex(list)); // destroy mutex field
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy(): %s\n");
		free(list);
		exit(-1); 
	}

	returnVal = pthread_cond_destroy(&(list->notEmpty)); // destroy cond var field
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy(): %s\n");
		free(list);
		exit(-1); 
	}

	for(i=0; i<size; i++){
		if (intlist_pop_tail(list) == NULL){
			printf("Error while trying to delete the %d-th item from list\n",i);
			exit(-1);//TODO what else?
		}
	}
}

//////////////////////////////////////

void main(int argc, char *argv[]){
	if (argc != 5){
		printf("Invalid number of arguments to main function\n");
		exit(-1);
	}

	struct intlist list; // single doubly linked list
	// init variables
	char * ptr; // for strtol function
	int returnVal; // return value to be check from various functions

	errno = 0;
	long WNUM = strtol(argv[1], &ptr, 10);
	if (errno != 0){
		printf("Error converting WNUM from string: %s\n", strerror(errno));
		exit(errno);
	}

	errno = 0;
	long RNUM = strtol(argv[2], &ptr, 10);
	if (errno != 0){
		printf("Error converting RNUM from string: %s\n", strerror(errno));
		exit(errno);
	}

	errno = 0;
	long MAX = strtol(argv[3], &ptr, 10);
	if (errno != 0){
		printf("Error converting MAX from string: %s\n", strerror(errno));
		exit(errno);
	}

	errno = 0;
	long TIME = strtol(argv[4], &ptr, 10);
	if (errno != 0){
		printf("Error converting TIME from string: %s\n", strerror(errno));
		exit(errno);
	}

	if ((WNUM<0) || (RNUM<0) || (TIME<0) || (MAX<0)){ // check for invalid arguments
		printf("Invalid argument was entered\n: %s\n");
		exit(-1);
	}

	intlist_init(&list); // init the list

	returnVal = pthread_cond_init(&GC, NULL); // defining a cond var for garbage collector
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init(): %s\n");
		free(list);
		exit(-1); 
	}

/*
	pthread_t threads[3];
  pthread_attr_t attr;
 pthread_cond_init (&count_threshold_cv, NULL);

  //For portability, explicitly create threads in a joinable state
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_create(&threads[0], &attr, watch_count, (void *)t1);
  pthread_create(&threads[1], &attr, inc_count, (void *)t2);
pthread_create(&threads[2], &attr, inc_count, (void *)t3); */

}

