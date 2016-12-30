#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


// Notes:
// 1. Free memory when needed + errors
// 2. Make sure I used locks for the minimum time + minimum lock/unlock operations
// 3. Make ALL functions besides init and destroy threadsafe!
// 4. The func of GC needs to have 2 arguments - one is the list, the second is MAX -- should bw global!
// 5. remove last k - if k<0 then exit without printf or errors
// 6. We can lock the mutex twice - just make sure thar same num of unlock was done too.
// 7. How to exit the threads if not from routine?

// data structure including list and mutex
struct intlist {
	int value; // assume positive integer
	intlist * prev; // pointer to previous node 
	intlist * next; // pointer to next node;
	int size; // number of elements in list
	pthread_mutex_t lock;
	pthread_cond_t notEmpty; // cond variable
	pthread_mutexattr_t attr; // for recursive
};

intlist myGlobalList; // global list
pthread_cond_t GC; // garbage collector
long MAX; // global variable for the capacity of the list

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

	// create attr for recursive mutex
	returnVal = pthread_mutexattr_init(&(list->attr));
	if (returnVal != 0) {
		printf("ERROR in pthread_mutexattr_init()\n");
		free(list);
		exit(-1); 
	}
	returnVal = pthread_mutexattr_settype(&(list->attr),PTHREAD_MUTEX_RECURSIVE);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutexattr_settype()\n");
		free(list);
		exit(-1); 
	}

	// create new mutex for current list and check for errors
	returnVal = pthread_mutex_init(&(list->lock), &(list->attr));
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_init(): %s\n", strerror(returnVal));
		free(list);
		exit(errno); 
	}

	// create new conditional variable for current list and check for errors
	returnVal = pthread_cond_init(&(list->notEmpty), NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init(): %s\n", strerror(returnVal));
		pthread_mutex_destroy(&(list->lock));
		free(list);
		exit(errno); 
	}
	

}



void intlist_push_head(intlist * list, int val){
	if (list == NULL)
		return;

	intlist * oldHead = list; // save old head
	struct intlist * newNode = (struct intlist *) malloc(sizeof(struct intlist));
	if (newNode == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		//TODO terminate program
		exit(errno);
	}
	newNode->prev = NULL; // this is the head now
	newNode->value = val; // update value

	int returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()\n");
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
		printf("ERROR in pthread_cond_signal()\n");
		//TODO terminate program
		exit(-1); 
	}
	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk()\n");
		//TODO terminate program
		exit(-1); 
	}
}



pthread_mutex_t * intlist_get_mutex(intlist * list){ // Do we need to lock??
	if (list == NULL){
		printf("Lock not found - list is NULL!\n");
		//TODO terminate program
		exit(-1);
	}
	int returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()\n");
		free(list);
		exit(-1); 
	}

	pthread_mutex_t * returnLock = &(list->lock);

	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk()\n");
		//TODO terminate program
		exit(-1); 
	}

	return returnLock; // check if its true at all....
}



int intlist_size(intlist * list){
	if (list == NULL){
		printf("Size not found - list is NULL!\n");
		return NULL;
		//TODO need to exit program??
	}

	int returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()\n");
		//TODO need to exit program??
		exit(-1); 
	}

	int size = list->size;

	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk()\n");
		//TODO terminate program
		exit(-1); 
	}

	return (size); 
}


int intlist_pop_tail(intlist * list){
	if (list == NULL){
		printf("Cant pop tail - list is NULL!\n");
		//TODO terminate program
		return NULL;
	}

	int popValue; // value to be returned
	int returnVal; // return value to be check from varoits functions

	returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n");
		//TODO terminate program
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
	list->size = list->size -1;

	free(current); // check if it sets to (current->prev)->next to NULL

	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n");
		//TODO terminate program
		exit(-1); 
	}

	return popValue;
	 
}



void intlist_remove_last_k(intlist * list, int k){
	if ((list == NULL) || (k<1)){ // check for invalid arguments
		printf("Cant pop tail - list is NULL!\n");
		//TODO terminate program

		exit(-1);
	}
	int i;
	int size = intlist_size(list);

	returnVal = pthread_mutex_lock(&(list->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n");
		//TODO terminate program
		exit(-1); 
	}

	if (k <= size){
		for (i=0; i<k; i++){
			intlist_pop_tail(list);
		}
	}

	else{
		for (i=0; i<size; i++){
			intlist_pop_tail(list);
		}
	}

	returnVal = pthread_mutex_unlock(&(list->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n");
		//TODO terminate program
		exit(-1); 
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
		//TODO terminate program
		exit(-1); 
	}

	returnVal = pthread_cond_destroy(&(list->notEmpty)); // destroy cond var field
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy(): %s\n");
		//TODO terminate program
		exit(-1); 
	}

	intlist_remove_last_k(list,size); // pop and destroy all the items
	
}

//////////////////////////////////////
void * writer_routine(){
	int num; // number to push in the list
	// infinite loop
	while(1){
		num = rand();
		intlist_push_head(myGlobalList, num); 
		if (myGlobalList->size > MAX){ // signal to GC
			pthread_cond_signal(&GC);
		}
	}
}


void * reader_routine(){
	int num; // number to push in the list
	// infinite loop
	while(1){
		num = rand();
		intlist_pop_tail(myGlobalList); 
	}
}


void * GC_routine(){

	int num; // number of items to delete
	int returnVal = pthread_mutex_lock(&(myGlobalList->lock)); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n");
		//TODO terminate program
		exit(-1); 
	}

	while(1){
		//while(intlist_size(myGlobalList) < MAX){
 			pthread_cond_wait(&GC, &(myGlobalList->lock));
 			// now we finished waiting
 			num = (intlist_size(myGlobalList) + 2 - 1) / 2; // half of the size, rounded up
 			intlist_remove_last_k(myGlobalList,num);
 			printf("GC - %d items removed from list\n", num);
		//}
	}

	returnVal = pthread_mutex_unlock(&(myGlobalList->lock)); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n");
		//TODO terminate program
		exit(-1); 
	}
	

}



void printList(intlist * list){ // needs to be thread safe?
	// we need to print the size and then all the elements in list
	if (!list){
		printf("List in NULL, nothing to print...\n");
		return;
	}
	int size = intlist_size(list); // this is the size of the list
	int i;
	intlist * curr = list; // ptr to list
	printf("The size of the list is:%d \n", size);

	for (i=0; i<size; i++){
		printf("The %d-th element is %d ,", i, curr->value);
		curr = curr->next;
	}
	printf("\n");

}
//////////////////////////////////////

void main(int argc, char *argv[]){
	if (argc != 5){
		printf("Invalid number of arguments to main function\n");
		exit(-1);
	}

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
	MAX = strtol(argv[3], &ptr, 10);
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
		printf("Invalid argument was entered\n");
		exit(-1);
	}

	intlist_init(&myGlobalList); // init the list
	pthread_t GC_thread; // thread for garbage collector

	// create garbage collector
	returnVal = pthread_cond_init(&GC, NULL); // defining a cond var for garbage collector
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init()\n");
		intlist_destroy(list);
		// is it enough?
		exit(-1); 
	}

	returnVal = pthread_create(&GC_thread, &attr, GC_routine, NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_create()\n");
		intlist_destroy(list);
		// is it enough?
		exit(-1); 
	}	

	// create WNUM threads for writers:
	pthread_t * writers = (pthread_t *) malloc(sizeof(pthread_t)*WNUM);
	if (writers == NULL){
		printf("Error allocating the writers threads: %s\n", strerror(errno));
		intlist_destroy(list);
		// is it enough?
		exit(errno);
	}

	pthread_attr_t attr; 
	int i; // iteration index - iterate and create threads
	int k = 0; // if we need to destroy some of the threads due to error
	srand(time(NULL)); // in order to generate random numbers

	returnVal = pthread_attr_init(&attr);
	if(returnVal!= 0){
		printf("ERROR in pthread_attr_init()\n");
		intlist_destroy(list);
		free(writers);
		exit(-1); 
	}
	returnVal = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	if(returnVal!= 0){
			printf("ERROR in pthread_attr_setdetachstate()\n");
			intlist_destroy(list);
			free(writers);
			pthread_attr_destroy(&attr);
			exit(-1); 
	}


	for (i=0; i<WNUM; i++){
		returnVal = pthread_create(&writers[i], &attr, writer_routine, NULL);
		if(returnVal!= 0){
			for (k=0; k<i;k++){
				// terminate all threads created till now
			}
			printf("ERROR in pthread_create()\n");
			intlist_destroy(myGlobalList);
			free(writers);
			pthread_attr_destroy(&attr);
			exit(-1); 
		}
	}


	// create RNUM threads for readers:
	pthread_t * readers = (pthread_t *) malloc(sizeof(pthread_t)*RNUM);
	if (readers == NULL){
		printf("Error allocating the readers threads: %s\n", strerror(errno));
		// terminate all writer threads created till now
		intlist_destroy(myGlobalList);
		free(writers);
		pthread_attr_destroy(&attr);
		exit(errno);
	}

	for (i=0; i<RNUM; i++){
		returnVal = pthread_create(&readers[i], &attr, reader_routine, NULL);
		if(returnVal!= 0){
			// terminate all writer + reader threads created till now
			printf("ERROR in pthread_create()\n");
			intlist_destroy(myGlobalList);
			free(writers);
			free(readers);
			pthread_attr_destroy(&attr);
			//destroy pthead
			exit(-1);
		} 
	}


	sleep(TIME);

	// stop all running threads - maybe exit first and then join?  

	//Wait for all threads to complete
	for (i=0; i< RNUM; i++) {
 		pthread_join(readers[i], NULL); // maybe change from NULL to actual return value to be checked?
	}
	for (i=0; i<WNUM; i++) {
		pthread_join(writers[i], NULL);  // maybe change from NULL to actual return value to be checked?
	}


	// prints size + list elements - make a function for that

	printList(myGlobalList);

	// cleanup + exit gracefully - make function for exiting the program
	free(writers);
	free(readers);
	// destroy GC thread
	pthread_attr_destroy(&attr);
	pthread_cond_destroy(&GC);
	pthread_exit(NULL);
}

