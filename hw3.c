#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Notes:
// 1. Free memory when needed + errors + create a destroy function
// 2. Make sure I used locks for the minimum time + minimum lock/unlock operations
// 3. Make ALL functions besides init and destroy threadsafe!
// 4. DONE >> The func of GC needs to have 2 arguments - one is the list, the second is MAX -- should bw global!
// 5. remove last k - if k<0 then exit without printf or errors
// 6. We can lock the mutex twice - just make sure thar same num of unlock was done too.
// 7. How to exit the threads if not from routine?

// data structure including list and mutex
struct intlist {
	int value; // assume positive integer
	struct intlist * prev; // pointer to previous node 
	struct intlist * next; // pointer to next node;
	int size; // number of elements in list
	pthread_mutex_t lock;
	pthread_cond_t notEmpty; // cond variable
	pthread_mutexattr_t attr; // for recursive
};

struct intlist myGlobalList; // global list
pthread_cond_t GC; // garbage collector
long MAX; // global variable for the capacity of the list
int exitFlag = 0; // if (exitFlag ==1) then exit from all threads

// headers:
void intlist_init(struct intlist * list);
void intlist_destroy(struct intlist * list);
void intlist_push_head(struct intlist * list, int val);
int intlist_pop_tail(struct intlist * list);
void intlist_remove_last_k(struct intlist * list, int k);
int intlist_size(struct intlist * list);
pthread_mutex_t * intlist_get_mutex(struct intlist * list);

void printList(struct intlist * list); // to remove

void intlist_init(struct intlist * list){
	printf("Entered init...\n");
	int returnVal; // return value from creating the mutex
	int i; // iteration index

	// create a new list
	/*list = (struct intlist *) malloc(sizeof(struct intlist));
	if (list == NULL){
		printf("Error allocation memory for new node: %s\n", strerror(errno));
		exit(errno);
	}*/

	// init size to zero at first
	list->size = 0;

	// create attr for recursive mutex
	returnVal = pthread_mutexattr_init(&list->attr);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutexattr_init()\n");
		free(list);
		exit(-1); 
	}
	returnVal = pthread_mutexattr_settype(&list->attr,PTHREAD_MUTEX_RECURSIVE);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutexattr_settype()\n");
		//free(list);
		exit(-1); 
	}

	// create new mutex for current list and check for errors
	returnVal = pthread_mutex_init(&list->lock, &list->attr);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_init(): %s\n", strerror(returnVal));
		//free(list);
		exit(errno); 
	}

	// create new conditional variable for current list and check for errors
	returnVal = pthread_cond_init(&list->notEmpty, NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init(): %s\n", strerror(returnVal));
	//	pthread_mutex_destroy(&(list->lock));
	//	free(list);
		exit(errno); 
	}
	printf("INIT: myGlob address is:%p, list adress is:%p,\n", (void *) &myGlobalList, (void *) list);
	printf("Exit init...\n");

}



void intlist_push_head(struct intlist * list, int val){
	printf("Entered push head...\n");
	if (list == NULL){
		printf("Cannot push to head - list is NULL!\n");
		//TODO terminate program
		exit(-1);
	}

	struct intlist * newNode = (struct intlist *) malloc(sizeof(struct intlist));
	if (newNode == NULL){
			printf("Error allocation memory for new node: %s\n", strerror(errno));
			//TODO terminate program
			exit(errno);
	}


	// insert as head:
	if (intlist_size(list) == 0 ){

		int returnVal = pthread_mutex_lock(&list->lock); // lock
		if (returnVal != 0) {
			printf("ERROR in pthread_mutex_lock()\n");
		//	free(list);
			exit(-1); 
		}
		newNode->prev = NULL;
		newNode->next = NULL;
		newNode->size = 1;
		newNode->value = val;
		newNode->lock = list->lock;
		newNode->notEmpty = list->notEmpty; // cond variable
		newNode->attr = list->attr;

		list = newNode; // global list gets the address of newNode

		printf("FROM PUSH - list->size is %d and newNode->size is %d with value of list->value%d\n",list->size, newNode->size, list->value);
		printf("myGlob address is:%p, list adress is:%p, newNode address is%p\n", (void *) &myGlobalList, (void *) list, (void *) newNode);
		printf("myGlobalList->value = %d, myGlobalList->size = %d\n", (&(myGlobalList))->value, (&(myGlobalList))->size);
		returnVal = pthread_cond_signal(&list->notEmpty); // signal that list is not empty
		if (returnVal != 0) {
			printf("ERROR in pthread_cond_signal()\n");
			//TODO terminate program
			exit(-1); 
		}

		returnVal = pthread_cond_signal(&list->notEmpty); // signal that list is not empty
		if (returnVal != 0) {
			printf("ERROR in pthread_cond_signal()\n");
			//TODO terminate program
			exit(-1); 
		}

		returnVal = pthread_mutex_unlock(&list->lock); // unlock
		if (returnVal != 0) {
			printf("ERROR in pthread_mutex_unloxk()\n");
			//TODO terminate program
			exit(-1); 
		}

		printf("Exit push head - from FIRST INSERTION...\n");
		printf("\n");
		return;
	}

	

	//struct intlist * oldHead = list; // save old head
	
	newNode->prev = NULL; // this is the head now
	newNode->value = val; // update value

	int returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()\n");
	//	free(list);
		exit(-1); 
	}
	newNode->next = list; // points to old head
	list->prev = newNode; // points to new node
	newNode->size = intlist_size(list) + 1; // update size
	newNode->lock = list->lock; // both nodes share the same mutex
	newNode->attr = list->attr;
	newNode->notEmpty = list->notEmpty;

	list = newNode; // list points to the new head

	returnVal = pthread_cond_signal(&list->notEmpty); // signal that list is not empty
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_signal()\n");
		//TODO terminate program
		exit(-1); 
	}
	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock()\n");
		//TODO terminate program
		exit(-1); 
	}
	printf("Exit push head...\n");
}



pthread_mutex_t * intlist_get_mutex(struct intlist * list){ // Do we need to lock??
		printf("Entered get mutex...\n");
	if (list == NULL){
		printf("Lock not found - list is NULL!\n");
		//TODO terminate program
		exit(-1);
	}

	pthread_mutex_t * returnLock = &list->lock;

	printf("Exit get mutex...\n");
	return returnLock; // check if its true at all....
}



int intlist_size(struct intlist * list){
	printf("Entered to size func...\n");
	if (list == NULL){
		printf("Size not found - list is NULL!\n");
		exit(-1);
		//TODO need to exit program??
	}

	int returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()\n");
		//TODO need to exit program??
		exit(-1); 
	}

	int size = list->size;
	printf("SIZE FUNC- size is %d\n", size);
	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk()\n");
		//TODO terminate program
		exit(-1); 
	}
	printf("Exit from size func...\n");
	return (size); 
}


int intlist_pop_tail(struct intlist * list){
	printf("Entered pop tail...\n");
	if (list == NULL){
		printf("Cant pop tail - list is NULL!\n");
		//TODO terminate program
		exit(-1);
	}

	int popValue; // value to be returned
	int returnVal; // return value to be check from various functions

	returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): \n");
		//TODO terminate program
		exit(-1); 
	}

	while (list->size == 0){ // list is empty
			pthread_cond_wait(&list->notEmpty, &list->lock); // wait here until list has at least one item
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

	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): \n");
		//TODO terminate program
		exit(-1); 
	}
	printf("Exit pop tail...\n");
	return popValue;
	 
}



void intlist_remove_last_k(struct intlist * list, int k){
	printf("Entered remove last k...\n");
	if ((list == NULL) || (k<0)){ // check for invalid arguments
		printf("Cant pop tail - list is NULL or invalid number was entered!\n");
		//TODO terminate program

		exit(-1);
	}

	if ((k == 0 )){ // check for invalid arguments
		printf("Cant pop tail - k is zero!\n");
		//TODO terminate program

		return;
	}


	int i, returnVal;
	int size = intlist_size(list);

	returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): \n");
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

	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): \n");
		//TODO terminate program
		exit(-1); 
	}
	printf("Exit remove last k...\n");

}

void intlist_destroy(struct intlist * list){
	printf("Entered destroy...\n");
	if (list == NULL){
		printf("nothing to destroy - list is NULL!\n");
		//TODO terminate program
		exit(-1);
	}		

	int size = intlist_size(list);
	int i;
	int returnVal;

	returnVal = pthread_mutex_destroy(intlist_get_mutex(list)); // destroy mutex field
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy(): \n");
		//TODO terminate program
		exit(-1); 
	}

	returnVal = pthread_cond_destroy(&list->notEmpty); // destroy cond var field
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy(): \n");
		//TODO terminate program
		exit(-1); 
	}

	intlist_remove_last_k(list,size); // pop and destroy all the items
	
	free(list);
	printf("Exit destroy...\n");
}

//////////////////////////////////////
void * writer_routine(){
	int num; // number to push in the list
	int returnVal;
	// infinite loop
	while(1){
		printf("Entered WRITER ROUTINE\n");
		num = rand();

		intlist_push_head(&myGlobalList, num); 
		printf("Now check size head from writer routine... \n");
		if ( intlist_size(&myGlobalList) > MAX ){ // signal to GC
			pthread_cond_signal(&GC);
			printf("a signal was sent to GC");
		}

		if (exitFlag == 1)
			pthread_exit(NULL);

		printList(&myGlobalList);
		
	}
}


void * reader_routine(){
	int num; // number to push in the list
	// infinite loop
	while(1){
		printf("Entered READER ROUTINE\n");
		num = rand();
		intlist_pop_tail(&myGlobalList); 

		if (exitFlag == 2)
			pthread_exit(NULL);
	}
}


void * GC_routine(){

	int num; // number of items to delete
	
	while(1){
		
		int returnVal = pthread_mutex_lock(intlist_get_mutex(&myGlobalList)); // lock
			if (returnVal != 0) {
				printf("ERROR in pthread_mutex_lock(): \n");
				//TODO terminate program
				exit(-1); 
			}

 			pthread_cond_wait(&GC, (intlist_get_mutex(&myGlobalList)));
 			// now we finished waiting

 			num = (intlist_size(&myGlobalList) + 2 - 1) / 2; // half of the size, rounded up
 			intlist_remove_last_k(&myGlobalList,num);
 			printf("GC - %d items removed from list\n", num);

 			returnVal = pthread_mutex_unlock(intlist_get_mutex(&myGlobalList)); // unlock
			if (returnVal != 0) {
				printf("ERROR in pthread_mutex_unlock(): \n");
				//TODO terminate program
				exit(-1); 
			}

			if (exitFlag == 2)
				pthread_exit(NULL);
		
	}

	
	

}



void printList(struct intlist * list){ // needs to be thread safe?
	// we need to print the size and then all the elements in list
	if (!list){
		printf("List in NULL, nothing to print...\n");
		return;
	}
	printf("Now check size head from printList... \n");
	int size = intlist_size(list); // this is the size of the list
	int i;
	struct intlist * curr = list; // ptr to list
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

	printf("Main starts...\n");
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

	//printf("Arguments are: WNUM = %ld, RNUM = %ld, MAX = %ld, TIME = %ld\n", WNUM , RNUM , MAX, TIME);
	intlist_init(&myGlobalList); // init the list
	pthread_t GC_thread; // thread for garbage collector

	// define attr - for join
	pthread_attr_t attr1; 

	returnVal = pthread_attr_init(&attr1);
	if(returnVal!= 0){
		printf("ERROR in pthread_attr_init()\n");
		//intlist_destroy(list);
		//free(writers);
		exit(-1); 
	}
	returnVal = pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_JOINABLE);
	if(returnVal!= 0){
			printf("ERROR in pthread_attr_setdetachstate()\n");
			//intlist_destroy(list);
			//free(writers);
			//pthread_attr_destroy(&attr);
			exit(-1); 
	}

	// create garbage collector
	returnVal = pthread_cond_init(&GC, NULL); // defining a cond var for garbage collector
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init()\n");
		//intlist_destroy(list);
		// is it enough?
		exit(-1); 
	}

	returnVal = pthread_create(&GC_thread, &attr1, GC_routine, NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_create11()\n");
		//intlist_destroy(list);
		// is it enough?
		exit(-1); 
	}	

	// create WNUM threads for writers:
	pthread_t * writers = (pthread_t *) malloc(sizeof(pthread_t)*WNUM);
	if (writers == NULL){
		printf("Error allocating the writers threads: %s\n", strerror(errno));
		//intlist_destroy(list);
		// is it enough?
		exit(errno);
	}

	
	int i; // iteration index - iterate and create threads
	int k = 0; // if we need to destroy some of the threads due to error
	srand(time(NULL)); // in order to generate random numbers

	

	for (i=0; i<WNUM; i++){
		returnVal = pthread_create(&writers[i], &attr1, writer_routine, NULL);
		if(returnVal!= 0){
			printf("ERROR in pthread_create()\n");
			//intlist_destroy(&myGlobalList);
			//free(writers);
			//pthread_attr_destroy(&attr);
			exit(-1); 
		}
	}


	// create RNUM threads for readers:
	pthread_t * readers = (pthread_t *) malloc(sizeof(pthread_t)*RNUM);
	if (readers == NULL){
		printf("Error allocating the readers threads: %s\n", strerror(errno));
		// terminate all writer threads created till now
		//intlist_destroy(&myGlobalList);
		//free(writers);
		//pthread_attr_destroy(&attr);
		exit(errno);
	}

	for (i=0; i<RNUM; i++){
		returnVal = pthread_create(&readers[i], &attr1, reader_routine, NULL);
		if(returnVal!= 0){
			// terminate all writer + reader threads created till now
			printf("ERROR in pthread_create()\n");
			//intlist_destroy(&myGlobalList);
			//free(writers);
			//free(readers);
			//pthread_attr_destroy(&attr);
			//destroy pthead
			exit(-1);
		} 
	}


	sleep(TIME);

	// stop all running threads - 

	exitFlag = 1; // now all Readers threads suppose to exit from their routines, then - join
	printf("Updated flag to 1\n");
	//Wait for all threads to be complete
	for (i=0; i< RNUM; i++) {
 		pthread_join(readers[i], NULL); // maybe change from NULL to actual return value to be checked?
	}
	printf("Exited from all READERS\n");

	exitFlag = 2; // now all Writers threads suppose to exit from their routines, then - join


	for (i=0; i<WNUM; i++) {
		pthread_join(writers[i], NULL);  // maybe change from NULL to actual return value to be checked?
	}
	pthread_join(GC_thread, NULL);
	printf("Exited from all WRITERS + GC \n");

	// prints size + list elements

	int size = intlist_size(&myGlobalList);
	printf("The size of the list is:%d \n",size);
	printf("The elements from tail to head are:\n");
	for (i=0; i<size; i++){
		printf("%d ," ,intlist_pop_tail(&myGlobalList));

	}
	printf("\n");

	// cleanup + exit gracefully - make function for exiting the program
	free(writers);
	free(readers);
	intlist_destroy(&myGlobalList);
	pthread_attr_destroy(&attr1);
	pthread_cond_destroy(&GC);
	pthread_exit(NULL);
}

