#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define NUM_THREADS	5

/*** MINIMAL ERROR HANDLING FOR EASE OF READING ***/

int main (int argc, char *argv[])
{
	int rc;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;

	rc = pthread_mutexattr_init(&attr);
	rc = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	rc = pthread_mutex_init(&lock, &attr);

	pthread_mutex_lock(&lock);
	pthread_mutex_lock(&lock);

	// working even though lock was grabbed twice,
	// because it was grabbed by the same thread
	
	// important to unlock the same number of times we locked!
	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&lock);
	
	pthread_mutex_destroy(&lock);
	pthread_exit(NULL);
}
