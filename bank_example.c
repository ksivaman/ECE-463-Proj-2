/* compile with:
gcc -o bank bank_example.c -lpthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

void * withdrawalFn(void *);
void * depositFn(void *);
void * withdrawalFnLock(void *);
void * depositFnLock(void *);


int balance;
int withdrawal_amount;
int deposit_amount;
pthread_mutex_t lock;


int main(int argc, char ** argv){
	/* ----- GET COMMAND LINE ARGUMENTS ----- */
	if(argc != 5){
		printf("Correct usage: ./bank <start_balance> <withdrawal_amount> <deposit_amount> <0/1 for unlock/locked>\n");
		return EXIT_FAILURE;
	}

	/* ----- ASSIGN VALUES ----- */
	balance = atoi(argv[1]);
	withdrawal_amount = atoi(argv[2]);
	deposit_amount = atoi(argv[3]);

	/* ----- INIT THREADS ----- */
	pthread_t withdrawal_thread_id;
	pthread_t deposit_thread_id;
	if(!(atoi(argv[4]))){
		//No lock, no mutex!
		if(pthread_create(&withdrawal_thread_id, NULL, withdrawalFn, NULL)){
			perror("Error creating thread for withdrawing money!");
			return EXIT_FAILURE;
		}

		if(pthread_create(&deposit_thread_id, NULL, depositFn, NULL)){
			perror("Error creating thread for depositing money!");
			return EXIT_FAILURE;
		}
	}
	else{
		//Lock, use mutex!
		//init lock
		pthread_mutex_init(&lock, NULL);

		if(pthread_create(&withdrawal_thread_id, NULL, withdrawalFnLock, NULL)){
			perror("Error creating thread for withdrawing money!");
			return EXIT_FAILURE;
		}

		if(pthread_create(&deposit_thread_id, NULL, depositFnLock, NULL)){
			perror("Error creating thread for depositing money!");
			return EXIT_FAILURE;
		}
	}

	/* ----- WAIT FOR THREADS TO FINISH ----- */
	pthread_join(withdrawal_thread_id, NULL);
	pthread_join(deposit_thread_id, NULL);

	printf("Expected Balance: %d\nActual Balance: %d\n", (atoi(argv[1]) + deposit_amount - withdrawal_amount), balance);

	return EXIT_SUCCESS;
}


void * withdrawalFn(void * arg){
	int curr_balance = balance;
	curr_balance = curr_balance - withdrawal_amount;
	sleep(1);
	balance = curr_balance;

	return NULL;
}


void * depositFn(void * arg){
	int curr_balance = balance;
	curr_balance = curr_balance + deposit_amount;
	sleep(1);
	balance = curr_balance;

	return NULL;
}


void * withdrawalFnLock(void * arg){
	pthread_mutex_lock(&lock);
	int curr_balance = balance;
	curr_balance = curr_balance - withdrawal_amount;
	sleep(1);
	balance = curr_balance;
	pthread_mutex_unlock(&lock);

	return NULL;
}


void * depositFnLock(void * arg){
	pthread_mutex_lock(&lock);
	int curr_balance = balance;
	curr_balance = curr_balance + deposit_amount;
	sleep(1);
	balance = curr_balance;
	pthread_mutex_unlock(&lock);

	return NULL;
}
