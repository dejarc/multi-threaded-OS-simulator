/**
Chris DeJarlais
TCSS 422
Operating Systems
discontinuityTest.c
demoed in office on 12-17-15
*/
#include "eventQueue.h"
int main()
{
	int orderId = START_INDEX;
	time_t t;
	srand((unsigned)time(&t));
	myReadyQueue = createQueue();
	fp = fopen("producersConsumers.txt", "w+");
	fp2 = fopen("deadlockDetector.txt", "w+");
	fp3 = fopen("mutResUsers.txt", "w+");
	int i;
	int index = START_INDEX;
	int aId;
	int bId;
	last_time_called = FIRST_DEADLOCK_CHECK;
	last_process_id = NA;
	last_time_run = NA;
	for(i = START_INDEX; i < READ_WRITE; i++) {
		safeWrite[i] = TRUE;
		blockedProducerConsumer[i] = createQueue();
		printf("\nproducer number %d created with ", orderId);
		addNode(myReadyQueue, createNode(createPCB(&orderId, TRUE, FALSE, FALSE, FALSE)));
		printf(" consumer  number %d", orderId);
		addNode(myReadyQueue, createNode(createPCB(&orderId, FALSE, TRUE, FALSE, FALSE)));
		usleep(100000);
	}
	for(i = START_INDEX; i < MUT_RES_PAIRS; i++) {
		pthread_mutex_init(&R1_mutexes[i], NULL);
		pthread_mutex_init(&R2_mutexes[i], NULL);
	}
	pthread_mutex_init(&deadlockCheck, NULL);
	pthread_mutex_init(&readyQueueAdd, NULL);
	printf("\nprint mutex returned %d", pthread_mutex_init(&printMutex, NULL));
	//aBlocked = createQueue();
	//bBlocked = createQueue();
	for(i = START_INDEX; i < IO_SIZE; i++) {
		IO[i] = createQueue();
		printf("\nthread %d returned value %d",i, pthread_create(&(tid[i]),NULL,&ioThread,IO[i]) );
	}
	//printf("\nthread %d returned value %d",THREADS - 3, pthread_create(&(tid[THREADS - 3]),NULL,&lockThread,aBlocked));
	//printf("\nthread %d returned value %d",THREADS - 2,pthread_create(&(tid[THREADS - 2]),NULL,&lockThread,bBlocked));
	printf("\ndeadlock thread %d returned value %d",THREADS - 2, pthread_create(&(tid[THREADS - 2]),NULL,&deadlockDetectorThread,NULL));	
	printf("\ntimer mutex returned %d", pthread_mutex_init(&timerAndCpu, NULL));
	printf("\ncpu condition variable returned value %d",pthread_cond_init(&cpu_running, NULL));
	printf("\ntimer thread %d returned value %d",THREADS - 1, pthread_create(&(tid[THREADS - 1]),NULL,&timerThread,NULL));
	node temp = returnFirst(&myReadyQueue);
	while(myReadyQueue->first->next != NULL) {
		if(clock() % NEW_INSERT == START_INDEX && orderId < UPPER_LIMIT) {
			pthread_mutex_lock(&printMutex);
			printf("\ndummy process added with id %d", orderId);
			pthread_mutex_unlock(&printMutex);
			pthread_mutex_lock(&readyQueueAdd);
			addNode(myReadyQueue, createNode(createPCB(&orderId, FALSE, FALSE, FALSE, FALSE)));//dummy process 
			pthread_mutex_unlock(&readyQueueAdd);
		    	if(index < MUT_RES_PAIRS) {
				pthread_mutex_lock(&readyQueueAdd);
				aId = orderId;
				addNode(myReadyQueue, createNode(createPCB(&orderId, FALSE, FALSE, TRUE, FALSE)));
				bId = orderId;
				addNode(myReadyQueue, createNode(createPCB(&orderId, FALSE, FALSE, FALSE, TRUE))); 
				pthread_mutex_unlock(&readyQueueAdd);
				printf("\nProcess A Type %d created with ", aId);
				printf("Process B Type %d", bId);
				index++;
		    	}
		}
		if(clock() > 600000000) {
		    pthread_mutex_lock(&printMutex);
		    printf("\ntime limit has been reached");
		    pthread_mutex_unlock(&printMutex);
		    break;
		} 
		pthread_mutex_lock(&timerAndCpu);
		if(!timer_safe && cpu_safe) {
			runCpu(myReadyQueue, &temp);
			timer_safe = TRUE;
			cpu_safe = FALSE;
			pthread_cond_signal(&cpu_running);//signal to timer that the cpu ran
			pthread_cond_wait(&cpu_running, &timerAndCpu);//wait for signal that timer ran
		} else {
			runCpu(myReadyQueue, &temp);
		}
		pthread_mutex_unlock(&timerAndCpu);	
	} 
	fclose(fp);
	fclose(fp2);
	fclose(fp3);
	return 0;
}
