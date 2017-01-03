#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <errno.h>
#include <unistd.h>



/************************* List Section ***************************/


/* Instruction:
 * 1) add your list implementation here - includes everything (structs, functions, defines, etc....)
 * 2) go to checkAllFlow function (the last function in this page) - and fixed all the 7 TODOs there (it is very simple changes)
 * 3) compile file: "gcc -pthread -o hw3_test hw3_test.c"
 * 4) run test: ./hw3_test
 *				On success: "SUCCESS!!"
 *				On failure: the test will stop and an error will be printed with the line number in the main test.
 *
 *	 !! Be awere !!
 *		(*) It is a random test so: 1) you can run it several times
 * 								    2) if an error occured it might be a bit of a problem 
 *									(but the test is mostly determinstic[since the list is determinstic]
 *										so I beleive it would be easy to recreate the error)
 *
 *      (*) if you want the test to be fully determenistic just go to "DETER:" comment(in main function) and change the list
 */


/************************** List End *****************************/


/*************************Test section****************************/

intlist list;

/* declarations */
void removeLastK(int k, int lineNumber);
void popExpected(int popExpected, int lineNumber);
void checkSize(int expectedSize, int lineNumber);
void checkAllFlow(int* listBack, int lineNumber);



int main(int argc, char** argv)
{
	const int INSERT_LIST_SIZE = 20;
	const int SPLIT_SIZE = 10; /* should be smaller then INSERT_LIST_SIZE */
	int i = 0;
	int listSize = 0;

	intlist_init(&list);

	int insertList[INSERT_LIST_SIZE];
	for (i=0; i<INSERT_LIST_SIZE; i++)
	{
		/* DETER: you can make your own list */
		insertList[i] = rand();
	}

	checkSize(0, __LINE__);

	removeLastK(1, __LINE__);
	checkSize(0, __LINE__);
	removeLastK(0, __LINE__);
	checkSize(0, __LINE__);
	/* check negative value */
	removeLastK(-5, __LINE__);
	checkSize(0, __LINE__);


	for (i=0; i<SPLIT_SIZE; i++)
	{
		intlist_push_head(&list, insertList[i]);
	}

	checkSize(SPLIT_SIZE, __LINE__);
	checkAllFlow(insertList, __LINE__);

	popExpected(insertList[0], __LINE__);
	popExpected(insertList[1], __LINE__);
	checkSize(SPLIT_SIZE-2, __LINE__);
	checkAllFlow(insertList+2, __LINE__); /* removed the first 2 elements */


	intlist_push_head(&list, insertList[i++]);
	intlist_push_head(&list, insertList[i++]);
	checkSize(SPLIT_SIZE, __LINE__);
	checkAllFlow(insertList+2, __LINE__); /* removed the first 2 elements */


	removeLastK(1, __LINE__);
	removeLastK(2, __LINE__);
	checkSize(SPLIT_SIZE-3, __LINE__);
	checkAllFlow(insertList+5, __LINE__); /* removed the first 5 elements */



	listSize = SPLIT_SIZE-3 + (INSERT_LIST_SIZE-i);

	for (; i<INSERT_LIST_SIZE; i++)
	{
		intlist_push_head(&list, insertList[i]);
	}
	checkSize(listSize, __LINE__);
	checkAllFlow(insertList+5, __LINE__);


	removeLastK(listSize+1, __LINE__);
	checkSize(0, __LINE__);

	intlist_push_head(&list, insertList[1]);
	checkAllFlow(insertList+1 ,__LINE__);

	printf("SUCCESS!!\n");
}


void removeLastK(int k, int lineNumber)
{
	int listSize = intlist_size(&list);

	intlist_remove_last_k(&list, k);
}

void popExpected(int popExpected, int lineNumber)
{
	int listSize = intlist_size(&list);

	int popVal = intlist_pop_tail(&list);
	if (popExpected != popVal)
	{
		printf("%d: Wrong pop value: expected %d, got %d\n", lineNumber, popExpected, popVal);
		exit(EXIT_FAILURE);
	}

	checkSize(listSize-1, lineNumber);
}


void checkSize(int expectedSize, int lineNumber)
{
	int listSize = intlist_size(&list);
	if (expectedSize != listSize)
	{
		printf("%d: Wrong list size: expected %d, got %d\n", lineNumber, expectedSize, listSize);
		exit(EXIT_FAILURE);
	}
}



/****************checkAllFlow start*****************************/
void checkAllFlow(int* listBack, int lineNumber)
{
	int i = 0;
	int nodeVal = 0;
	/* TODO: change intlistNode - to your node struct name */
	intlistNode currentNode = 0;
	int listSize = intlist_size(&list);

	if (0 == listSize)
	{
		printf("%d: Got empty list\n", lineNumber);
		return;
	}

	/* TODO: change list->tailNode - to your tail pointer */
	currentNode = list->tailNode;

	for (i=0; i<listSize; i++)
	{
		/* TODO: change currentNode->value - to your node value */
		nodeVal = currentNode->value;
		if (listBack[i] != nodeVal)
		{
			printf("%d: Wrong list head: expected %d, got %d\n",
					lineNumber, listBack[i], nodeVal);

			exit(EXIT_FAILURE);
		}

		/* TODO: change currentNode->prevNode - to your prev node */
		currentNode = currentNode->prevNode;
	}
	if (currentNode != NULL)
	{
		printf("%d: currentNode should be null\n", lineNumber);
		exit(EXIT_FAILURE);
	}

	/* TODO: change list->headNode - to your list head */
	currentNode = list->headNode;
	for (i=listSize-1; i>=0; i--)
	{
		/* TODO: change currentNode->value - to your node value */
		nodeVal = currentNode->value;
		if (listBack[i] != nodeVal)
		{
			printf("%d: Wrong list head: expected %d, got %d\n",
					lineNumber, listBack[i], nodeVal);

			exit(EXIT_FAILURE);
		}

		/* TODO: change currentNode->nextNode - to your next node */
		currentNode = currentNode->nextNode;
	}
	if (currentNode != NULL)
	{
		printf("%d: currentNode should be null\n", lineNumber);
		exit(EXIT_FAILURE);
	}
}
/************checkAllFlow end********************/