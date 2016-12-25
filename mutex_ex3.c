#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define NUM_THREADS	5

static long TOTAL;

int counter = 0;
pthread_mutex_t lock;

int next_counter(void)
{	
	pthread_mutex_lock(&lock);
	int temp = ++counter;
	pthread_mutex_unlock(&lock);
	return temp;
	
	//return __sync_fetch_and_add(&counter, 1);
}

int is_prime(long x)
{
	long i;
	for (i = 2; i < x/2; ++i)
	{
		if (x % i == 0)
			return 0;
	}
	return 1;
}

void *prime_print(void *t)
{
	int i = 0;
	long tid;
	double result=0.0;
	tid = (long)t;
	printf("Thread %ld starting...\n",tid);
	
	long j = 0;
	while (j < TOTAL) {
		j = next_counter();
		++i;
		if (is_prime(j)) {
	        printf("Prime: %ld\n", j);
		}
	}

	printf("Thread %ld done.\n",tid);
	pthread_exit((void*) i);
}

int main (int argc, char *argv[])
{
	pthread_t thread[NUM_THREADS];
	int rc;
	long t;
	void *status;
	
	TOTAL =  pow(10,5);
	
	rc = pthread_mutex_init(&lock, NULL);
	if (rc) {
		printf("ERROR in pthread_mutex_init(): %s\n", strerror(rc));
		exit(-1);
	}

	for(t=0; t<NUM_THREADS; t++)
	{
		printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], NULL, prime_print, (void *)t); 
		if (rc) {
			printf("ERROR in pthread_create(): %s\n", strerror(rc));
			exit(-1);
		}
	}

	for(t=0; t<NUM_THREADS; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR in pthread_join(): %s\n", strerror(rc));
			exit(-1);
		}
		printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}
 
	printf("Main: program completed. Exiting.  Counter = %d\n",counter);
	
	pthread_mutex_destroy(&lock);
	pthread_exit(NULL);
}
