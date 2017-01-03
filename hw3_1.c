#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

// Notes:
// 1. DONE >> Free memory when needed + errors + create a destroy function
// 2. DONE >> Make sure I used locks for the minimum time + minimum lock/unlock operations
// 3. DONE >> Make ALL functions besides init and destroy threadsafe!
// 4. DONE >> The func of GC needs to have 2 arguments - one is the list, the second is MAX -- should bw global!
// 5. DONE >> remove last k - if k<0 then exit without printf or errors
// 6. DONE >> We can lock the mutex twice - just make sure thar same num of unlock was done too.
// 7. DONE >> How to exit the threads if not from routine?
// 8. DONE >> maintain tail pointer for the pop method + check edge cases in pop tail
// 9. DONE >> Check return values - print this string when possible 
// 10.Change name of file to hw3
// 11. DONE >> What if gc cleans half and then right away we have num > MAX ? Call GC right away? 
// 12. DONE >> init all vars in the start
// 13. DONE >> change destroy func
// 14. remove all printings + comments
// 15. DONE >> valgrind
// 16. check 3 lists

// struct for list node
struct _intlist_item{
  int value;
  struct _intlist_item *prev;
  struct _intlist_item *next;
} ;

typedef struct _intlist_item intlist_item; 

// struct for the "outside" list, including nodes and more features
struct intlist {
	intlist_item * head; // linked list of the nodes
	intlist_item * tail;
	int size; // number of elements in list
	pthread_mutex_t lock; // mutex that locks the list
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
	srand(time(NULL)); // in order to generate random numbers
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
		exit(errno); 
	}

	printf("Exit init...\n");

}



void intlist_push_head(struct intlist * list, int val){
	printf("Entered push head...\n");
	if (list == NULL){
		printf("Cannot push to head - list is NULL!\n");
		exit(-1);
	}
	int returnVal;
	intlist_item * newNode = (intlist_item *) malloc(sizeof(intlist_item));
	if (newNode == NULL){
			printf("Error allocation memory for new node: %s\n", strerror(errno));
			exit(errno);
	}

	returnVal = pthread_mutex_lock(&list->lock); // lock
		
		if (returnVal != 0) {
			printf("ERROR in pthread_mutex_lock()%s\n", strerror(returnVal));
			exit(-1); 
		}

	//printf("** before the if size == 0 **\n");
	// insert as head:
	if (intlist_size(list) == 0){

		
		newNode->prev = NULL;
		newNode->next = NULL;
		
		list->size = 1;
		newNode->value = val;
		
		(list->head) = newNode; // global list gets the address of newNode
		list->tail = newNode;

		printf("Exit push head - from FIRST INSERTION...\n");
		
		//return;
	}

	//printf("** after the if size == 0 **\n");
else{

	intlist_item * oldHead = list->head;

		newNode->prev = NULL;
		newNode->value = val;
		newNode->next = oldHead; // points to old head
		oldHead->prev = newNode; // points to new node
		list->size = intlist_size(list) + 1; // update size
		list->head = newNode; // global list gets the address of newNode
	}
	
	

	returnVal = pthread_cond_signal(&list->notEmpty); // signal that list is not empty
		if (returnVal != 0) {
			printf("ERROR in pthread_cond_signal()\n");
			exit(-1); 
		}

	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock()%s\n", strerror(returnVal));
		exit(-1); 
	}

	printf("Exit push head...\n");
}



pthread_mutex_t * intlist_get_mutex(struct intlist * list){ 
	
	if (list == NULL){
		printf("Lock not found - list is NULL!\n");
		exit(-1);
	}

	pthread_mutex_t * returnLock = &list->lock;

	return returnLock;
}



int intlist_size(struct intlist * list){
	if (list == NULL){
		printf("Size not found - list is NULL!\n");
		exit(-1);
	
	}
	int size;
	int returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock()%s\n", strerror(returnVal));
		exit(-1); 
	}

	size = list->size;
	printf("SIZE FUNC- size is %d\n", size);

	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unloxk()%s\n", strerror(returnVal));
		exit(-1); 
	}

	return (size); 
}


int intlist_pop_tail(struct intlist * list){
	printf("Entered pop tail...\n");
	if (list == NULL){
		printf("Cant pop tail - list is NULL!\n");
		exit(-1);
	}

	int popValue; // value to be returned
	int returnVal; // return value to be check from various functions

	returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n", strerror(returnVal));
		exit(-1); 
	}

	while (intlist_size(list) == 0){ // list is empty
		printf("waiting...\n"); 
		pthread_cond_wait(&list->notEmpty, &list->lock); // wait here until list has at least one item
	}

	intlist_item * current = list->tail;
	intlist_item * previous = list->tail->prev;

	popValue = current->value;
	list->size = intlist_size(list) -1;

	printf("We aree freeing: %d\n", popValue);
	free(current); // check if it sets to (current->prev)->next to NULL
	list->tail = previous;
	if (previous != NULL)
		previous->next = NULL;
	
	//printf("** tail after deletion is: %d, head after deletion is:%d\n", list->tail->value, list->head->value);
	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n", strerror(returnVal));
		exit(-1); 
	}
	
	printf("Exit pop tail...\n");
	return popValue;
	 
}



void intlist_remove_last_k(struct intlist * list, int k){
	printf("Entered remove last k...\n");
	if ((list == NULL) ){ // check for invalid arguments
		printf("Cant pop tail - list is NULL\n");
		exit(-1);
	}

	if ((k<0)){ // check for invalid arguments
		return;
	}

	if ((k == 0 )){ // check for invalid arguments
		printf("Cant pop tail - k is zero!\n");
		return;
	}


	int i, returnVal, size;
	

	returnVal = pthread_mutex_lock(&list->lock); // lock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_lock(): %s\n", strerror(returnVal));
		exit(-1); 
	}

	size = intlist_size(list);
	if (k <= size){
		for (i=0; i<k; i++){
			intlist_pop_tail(list);
			printList(&myGlobalList);
		}
	}

	else{
		for (i=0; i<size; i++){
			intlist_pop_tail(list);
			printList(&myGlobalList);
		}
	}

	returnVal = pthread_mutex_unlock(&list->lock); // unlock
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_unlock(): %s\n", strerror(returnVal));
		exit(-1); 
	}

	printf("Exit remove last k...\n");

}

void intlist_destroy(struct intlist * list){
	printf("Entered destroy...\n");
	if (list == NULL){
		printf("nothing to destroy - list is NULL!\n");
		exit(-1);
	}		
	int i;
	int returnVal;
	int size;

	size = intlist_size(list);
	
	intlist_remove_last_k(list, size);
	//TODO add removal of elements!

	returnVal = pthread_mutex_destroy(&list->lock);
	if (returnVal != 0) {
		printf("ERROR in pthread_mutex_destroy: \n");
		exit(-1); 
	}

	returnVal = pthread_cond_destroy(&list->notEmpty); // destroy cond var field
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_destroy(): \n");
		exit(-1); 
	}
	printf("Exit destroy...\n");
}

//////////////////////////////////////
void * writer_routine(void *t){
	int num; // number to push in the list
	int returnVal;
	// infinite loop
	while(1){

		printf("Entered WRITER ROUTINE\n");
		num = rand();

		intlist_push_head(&myGlobalList, num); 
		//printf("Now check size head from writer routine... \n");
		if ( intlist_size(&myGlobalList) > MAX ){ // signal to GC
			printf("sending signal to GC\n");
			pthread_cond_signal(&GC);
			printf("\n");
			printf("a signal was sent to GC\n");
			printf("\n");
		}

		if (exitFlag == 2){
			printf("ENTERED FLAG = 2 IN WRITER ROUTINE\n");
			pthread_exit(NULL);
		}

		printList(&myGlobalList);
		printf("\n");
		printf("\n");
		
	}
}


void * reader_routine(void *t){
	int returnVal;
	// infinite loop
	while(1){
		printf("Entered READER ROUTINE\n");
		intlist_pop_tail(&myGlobalList); 
		printList(&myGlobalList);
		printf("\n");
		printf("\n");

		if (exitFlag == 1){
			printf("ENTERED FLAG = 1 IN READER ROUTINE\n");
			pthread_exit(NULL);
		}
		
		
	}
}


void * GC_routine(void *t){

	int num; // number of items to delete
	int returnVal;
	while(1){
				
			returnVal = pthread_mutex_lock(intlist_get_mutex(&myGlobalList)); // lock
					if (returnVal != 0) {
						printf("ERROR in pthread_mutex_lock(): %s\n", strerror(returnVal));
						//TODO terminate program
				exit(-1); 
			}

 			pthread_cond_wait(&GC, (intlist_get_mutex(&myGlobalList)));
 			// now we finished waiting
 			printf("\n");
 			printf("GC finished waiting!\n");
 			printf("\n");
 			num = (intlist_size(&myGlobalList) + 2 - 1) / 2; // half of the size, rounded up
 			intlist_remove_last_k(&myGlobalList,num);
 			printf("GC - %d items removed from list\n", num);

 			returnVal = pthread_mutex_unlock(intlist_get_mutex(&myGlobalList)); // unlock
			if (returnVal != 0) {
				printf("ERROR in pthread_mutex_unlock():%s\n", strerror(returnVal));
				exit(-1); 
			}

			if (exitFlag == 3){
				printf("ENTERED FLAG = 3 IN GC ROUTINE\n");
				pthread_exit(NULL);
			}
		
	}
}



void printList(struct intlist * list){ // DELETE FUNC
	// we need to print the size and then all the elements in list
	if (!list){
		printf("List in NULL, nothing to print...\n");
		return;
	}
	int returnVal = pthread_mutex_lock(intlist_get_mutex(&myGlobalList)); // lock
			if (returnVal != 0) {
				printf("ERROR in pthread_mutex_lock(): %s\n", strerror(returnVal));
				//TODO terminate program
				exit(-1); 
			}

	int size = intlist_size(list); // this is the size of the list
	int i;
	intlist_item * curr = list->head; // ptr to list
	printf("The size of the list is:%d \n", size);

	for (i=0; i<size; i++){
		printf("The %d-th element is %d ,", i, curr->value);
		curr = curr->next;
	}

	returnVal = pthread_mutex_unlock(intlist_get_mutex(&myGlobalList)); // unlock
			if (returnVal != 0) {
				printf("ERROR in pthread_mutex_unlock(): %s\n", strerror(returnVal));
				//TODO terminate program
				exit(-1); 
			}

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

	intlist_init(&myGlobalList); // init the list
	pthread_t GC_thread; // thread for garbage collector
	// define attr - for join
	pthread_attr_t attr1; 
	int i; // iteration index - iterate and create threads
	int k = 0; // if we need to destroy some of the threads due to error
	//srand(time(NULL)); // in order to generate random numbers
	pthread_t * writers;
	pthread_t * readers;
	int size;

	returnVal = pthread_attr_init(&attr1);
	if(returnVal!= 0){
		printf("ERROR in pthread_attr_init()\n");
		exit(-1); 
	}
	returnVal = pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_JOINABLE);
	if(returnVal!= 0){
			printf("ERROR in pthread_attr_setdetachstate()\n");
			exit(-1); 
	}

	// create garbage collector
	returnVal = pthread_cond_init(&GC, NULL); // defining a cond var for garbage collector
	if (returnVal != 0) {
		printf("ERROR in pthread_cond_init()\n");
		exit(-1); 
	}

	returnVal = pthread_create(&GC_thread, &attr1, GC_routine, NULL);
	if (returnVal != 0) {
		printf("ERROR in pthread_create11()%s\n", strerror(returnVal));
		exit(-1); 
	}	

	// create WNUM threads for writers:
	writers = (pthread_t *) malloc(sizeof(pthread_t)*WNUM);
	if (writers == NULL){
		printf("Error allocating the writers threads: %s\n", strerror(errno));
		exit(errno);
	}
	

	for (i=0; i<WNUM; i++){
		returnVal = pthread_create(&writers[i], &attr1, writer_routine, NULL);
		if(returnVal!= 0){
			printf("ERROR in pthread_create()%s\n", strerror(returnVal));
			exit(-1); 
		}
	}


	// create RNUM threads for readers:
	readers = (pthread_t *) malloc(sizeof(pthread_t)*RNUM);
	if (readers == NULL){
		printf("Error allocating the readers threads: %s\n", strerror(errno));
		exit(errno);
	}

	for (i=0; i<RNUM; i++){
		returnVal = pthread_create(&readers[i], &attr1, reader_routine, NULL);
		if(returnVal!= 0){
			printf("ERROR in pthread_create()%s\n", strerror(returnVal));
			exit(-1);
		} 
	}


	sleep(TIME);

	// stop all running threads - 

	exitFlag = 3; // now GC thread should exit, then - join
	printf("flag changed to 3 in main \n");

	returnVal = pthread_join(GC_thread, NULL);
	if(returnVal!= 0){
			printf("ERROR in pthread_create()%s\n", strerror(returnVal));
			exit(-1);
	} 

	printf("Exited from GC \n");
	printf("\n");

	exitFlag = 1; // now all Readers threads suppose to exit from their routines, then - join
	printf("Updated flag to 1\n");
	//Wait for all threads to be complete
	for (i=0; i< RNUM; i++) {
 		returnVal = pthread_join(readers[i], NULL); // maybe change from NULL to actual return value to be checked?
 		if(returnVal!= 0){
			printf("ERROR in pthread_create()%s\n", strerror(returnVal));
			exit(-1);
		} 
	}
	printf("Exited from all READERS\n");
	printf("\n");

	exitFlag = 2; // now all Writers threads suppose to exit from their routines, then - join
	for (i=0; i<WNUM; i++) {
		returnVal = pthread_join(writers[i], NULL);  // maybe change from NULL to actual return value to be checked?
		if(returnVal!= 0){
			printf("ERROR in pthread_create()%s\n", strerror(returnVal));
			exit(-1);
		} 
	}
	printf("Exited from all WRITERS \n");
	printf("\n");

	

	// prints size + list elements

	size = intlist_size(&myGlobalList);
	printf("\n");
	printf("Printing final results:\n");
	printf("The size of the list is:%d \n",size);
	printf("The elements from tail to head are:\n");
	printf("\n");
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

