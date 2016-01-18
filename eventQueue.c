/**
Chris DeJarlais
TCSS 422
Operating Systems
eventQueue.c
*/
#include "eventQueue.h"
/*to write the next value*/
void writeValue(node myNode)
{		
	if(!safeWrite[myNode->processes->process_id / 2]) {	
		myNode->processes->state = waiting;
		addNode(blockedProducerConsumer[myNode->processes->process_id / 2], myNode);
	} else {
		pthread_mutex_lock(&mutexes[myNode->processes->process_id / 2]);
		myNum[myNode->processes->process_id / 2] = myNode->processes->myValue;
		safeRead[myNode->processes->process_id / 2] = TRUE;	
		safeWrite[myNode->processes->process_id / 2] = FALSE;
		pthread_mutex_unlock(&mutexes[myNode->processes->process_id / 2]);
        pthread_mutex_lock(&readyQueueAdd);
		addNode(myReadyQueue, myNode);
        pthread_mutex_unlock(&readyQueueAdd); 
	}			
}
/*to read the new value written*/
void readValue(node myNode)
{			
	if(!safeRead[myNode->processes->process_id / 2]) {
		myNode->processes->state = waiting;
		addNode(blockedProducerConsumer[myNode->processes->process_id / 2], myNode);
	} else {
		pthread_mutex_lock(&mutexes[myNode->processes->process_id / 2]);
		myNode->processes->myValue = myNum[myNode->processes->process_id / 2];
		safeWrite[myNode->processes->process_id / 2] = TRUE;
		safeRead[myNode->processes->process_id / 2] = FALSE;
        pthread_mutex_lock(&printMutex);		
		printf("\nnew value of num is %d read by consumer number %d",myNode->processes->myValue, myNode->processes->process_id);		
        pthread_mutex_unlock(&printMutex);
		fprintf(fp,"\nnew value read by consumer: %d ",myNode->processes->myValue);
		pthread_mutex_unlock(&mutexes[myNode->processes->process_id / 2]);
	    usleep(40000);
		pthread_mutex_lock(&readyQueueAdd);
        addNode(myReadyQueue, myNode);
        pthread_mutex_unlock(&readyQueueAdd);
    }
		
}
/*scan for io*/
void checkIO(node *myNode)
{
	if(myTimer == 0) {
		new_process = TRUE;
		(*myNode)->processes->state = waiting;
		addNode(IO[rand() % IO_SIZE], *myNode);
		(*myNode) = returnFirst(&myReadyQueue);
	}
	
}
/*scan for deadlock*/
void* deadlockDetectorThread(void *myDeadlock) {
        while(TRUE) {
            if((double)(clock() - last_time_called) / clock() > 0.8500 && last_process_id >= 0) {
                pthread_mutex_lock(&deadlockCheck);
                deadlockHelper();
                pthread_mutex_unlock(&deadlockCheck);
                last_time_called = clock();
            } 
        }
}
void deadlockHelper() {
	    float waitTime;
	    int current_time;
        current_time = clock();
        waitTime = (double)(current_time - last_time_run) / current_time;
		fprintf(fp2, "\nprocess %d has spent %f percent of \ntotal cpu cycle time spent waiting", last_process_id, waitTime);
		if(waitTime >= 0.850000) {
			fprintf(fp2,"\nprocess %d  last ran at %d and has been waiting %f of processor time"
			, last_process_id, last_time_run, waitTime);
		}
}
/*run the cpu
@param myReadyQueue the queue of processes waiting for CPU access
*/
void runCpu(fifoQueue myReadyQueue, node *myNode) {
	char type;
	(*myNode)->processes->time_stamp = clock();
    pthread_mutex_lock(&deadlockCheck);
    last_time_run =	(*myNode)->processes->time_stamp;
    last_process_id = (*myNode)->processes->process_id;
    pthread_mutex_unlock(&deadlockCheck); 
	++(*myNode)->processes->usedQuanta;
	if((*myNode)->processes->consumer) {
		readValue(*myNode);
		(*myNode) = returnFirst(&myReadyQueue);
		new_process = TRUE;
	} else if((*myNode)->processes->producer) {
		(*myNode)->processes->myValue++;
		writeValue(*myNode);
		(*myNode) = returnFirst(&myReadyQueue);
		new_process = TRUE;
	}else if((*myNode)->processes->aType || (*myNode)->processes->bType) {
		/*if((*myNode)->processes->aType) {
			addNode(aBlocked,*myNode);
		} else {
			addNode(bBlocked,*myNode);
		}*/
        tryLockSingleThread(*myNode);
		(*myNode) = returnFirst(&myReadyQueue);
		new_process = TRUE;
	}  else {
		checkIO(myNode);
	}
	if((*myNode)->processes->usedQuanta == (*myNode)->processes->runLength) {//process is done with run time, make random decision to reenqueue
			reenqueueNode(myReadyQueue, (*myNode));
			(*myNode) = returnFirst(&myReadyQueue);// run next node in readyQueue
			new_process = TRUE;
	}	
}
/*thread to continuously check for io*/
void* ioThread(void *myIo) {
	while(TRUE) {
		runIo((fifoQueue)myIo);
	}
}
/*function continuously called by io thread*/
void runIo(fifoQueue myIo) {
	if(myIo->first != NULL) {
		node temp;
		--myIo->first->processes->ioLength;
		if(myIo->first->processes->ioLength == 0) {
			temp = returnFirst(&myIo);
			temp->processes->ioLength = rand() % IO_LENGTH + OFFSET;
			temp->processes->state = ready;
            pthread_mutex_lock(&printMutex);
            printf("\nprocess id %d done with io", temp->processes->process_id);         
            pthread_mutex_unlock(&printMutex);
            pthread_mutex_lock(&readyQueueAdd);
            addNode(myReadyQueue, temp);
            pthread_mutex_unlock(&readyQueueAdd);
		}		
	}
}
void* lockThread(void *myResUser) {
	while(TRUE) {
		pthread_mutex_lock(&deadlockCheck);
		tryLock((fifoQueue)myResUser);
		pthread_mutex_unlock(&deadlockCheck);
	}
}
/*function continuously called by io thread*/
void tryLock(fifoQueue myBlockedResUser) {
	if(myBlockedResUser->first != NULL) {
		char type;
		node temp;
		if(myBlockedResUser->first->processes->aType) {
			type = 'A';
		} else {
			type = 'B';
		}
		//if(myBlockedResUser->first->processes->aType) {
			pthread_mutex_lock(&R1_mutexes[myBlockedResUser->first->processes->pair_number]);
        	pthread_mutex_lock(&R2_mutexes[myBlockedResUser->first->processes->pair_number]);
			pthread_mutex_unlock(&R1_mutexes[myBlockedResUser->first->processes->pair_number]);
			pthread_mutex_unlock(&R2_mutexes[myBlockedResUser->first->processes->pair_number]);
            pthread_mutex_lock(&printMutex);
			printf("\nmutex R1 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			printf("\nmutex R2 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			printf("\nboth resources in use by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
            pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nmutex R1 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nmutex R2 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nboth resources in use by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
		    pthread_mutex_lock(&printMutex);	
            printf("\nmutex R1 unlocked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			printf("\nmutex R2 unlocked by process process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
		    pthread_mutex_unlock(&printMutex);	
            fprintf(fp3,"\nmutex R1 unlocked by process type %c with process id number %d", type,myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nmutex R2 unlocked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			temp = returnFirst(&myBlockedResUser);
            pthread_mutex_lock(&readyQueueAdd);
			addNode(myReadyQueue, temp);
            pthread_mutex_unlock(&readyQueueAdd);
		/*} else {
			pthread_mutex_lock(&R2_mutexes[myBlockedResUser->first->processes->pair_number]);
			printf("\nmutex R2 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_lock(&R1_mutexes[myBlockedResUser->first->processes->pair_number]);
			printf("\nmutex R1 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_lock(&printMutex);
			fprintf(fp3,"\nmutex R2 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nmutex R1 locked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nboth resources in use by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_unlock(&printMutex);
			pthread_mutex_unlock(&R1_mutexes[myBlockedResUser->first->processes->pair_number]);
			printf("\nmutex R1 unlocked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_unlock(&R2_mutexes[myBlockedResUser->first->processes->pair_number]);
			printf("\nmutex R2 unlocked by process process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_lock(&printMutex);
			fprintf(fp3,"\nmutex R1 unlocked by process type %c with process id number %d", type,myBlockedResUser->first->processes->process_id);
			fprintf(fp3,"\nmutex R2 unlocked by process type %c with process id number %d", type, myBlockedResUser->first->processes->process_id);
			pthread_mutex_unlock(&printMutex);
			temp = returnFirst(&myBlockedResUser);
			pthread_mutex_lock(&readyQueueAdd);
            addNode(myReadyQueue, temp);
			pthread_mutex_unlock(&readyQueueAdd);
        }*/
	}
}
void tryLockSingleThread(node myNode) {
		char type;
		if(myNode->processes->aType) {
			type = 'A';
		} else {
			type = 'B';
		}
            /*myNode->processes->timeStamp = clock();
            pthread_mutex_lock(&deadlockCheck);
            last_time_run =	myNode->processes->time_stamp;
            last_process_id = myNode->processes->process_id;
            pthread_mutex_unlock(&deadlockCheck);*/ 
		//if(myNode->processes->aType) {
			pthread_mutex_lock(&R1_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R1 locked by process type %c with process id number %d", type, myNode->processes->process_id);
            pthread_mutex_unlock(&printMutex); 
            fprintf(fp3,"\nmutex R1 locked by process type %c with process id number %d", type,  myNode->processes->process_id);
        	pthread_mutex_lock(&R2_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R2 locked by process type %c with process id number %d", type, myNode->processes->process_id); 
		    pthread_mutex_unlock(&printMutex);	
            fprintf(fp3,"\nmutex R2 locked by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_lock(&printMutex);
            printf("\nboth resources in use by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nboth resources in use by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&R1_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R1 unlocked by process type %c with process id number %d", type, myNode->processes->process_id); 
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nmutex R1 unlocked by process type %c with process id number %d", type, myNode->processes->process_id);
			pthread_mutex_unlock(&R2_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R2 unlocked by process process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nmutex R2 unlocked by process type %c with process id number %d", type,  myNode->processes->process_id);
            pthread_mutex_lock(&readyQueueAdd);
			addNode(myReadyQueue, myNode);
            pthread_mutex_unlock(&readyQueueAdd);
        /*} else {
			pthread_mutex_lock(&R2_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R2 locked by process type %c with process id number %d", type, myNode->processes->process_id);
            pthread_mutex_unlock(&printMutex); 
            fprintf(fp3,"\nmutex R2 locked by process type %c with process id number %d", type,  myNode->processes->process_id);
        	pthread_mutex_lock(&R1_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R1 locked by process type %c with process id number %d", type, myNode->processes->process_id); 
		    pthread_mutex_unlock(&printMutex);	
            fprintf(fp3,"\nmutex R1 locked by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_lock(&printMutex);
            printf("\nboth resources in use by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nboth resources in use by process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&R1_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R1 unlocked by process type %c with process id number %d", type, myNode->processes->process_id); 
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nmutex R1 unlocked by process type %c with process id number %d", type, myNode->processes->process_id);
			pthread_mutex_unlock(&R2_mutexes[myNode->processes->pair_number]);
			pthread_mutex_lock(&printMutex);
            printf("\nmutex R2 unlocked by process process type %c with process id number %d", type,  myNode->processes->process_id);
			pthread_mutex_unlock(&printMutex);
            fprintf(fp3,"\nmutex R2 unlocked by process type %c with process id number %d", type,  myNode->processes->process_id);
            pthread_mutex_lock(&readyQueueAdd);
			addNode(myReadyQueue, myNode);
            pthread_mutex_unlock(&readyQueueAdd);
    
        }*/
}

/*thread to continuously run the timer*/
void* timerThread(void *myParam) {
	myTimer = rand() % TIMER_MOD + OFFSET;
	while(TRUE) {
		pthread_mutex_lock(&timerAndCpu);
		if(!cpu_safe && timer_safe) {//timer got here first, wait for cpu
			timer_safe = FALSE;//
			cpu_safe = TRUE;
			pthread_cond_wait(&cpu_running, &timerAndCpu);//wait for signal from cpu that it ran
			runTimer();
			pthread_cond_signal(&cpu_running);//signal to cpu that timer ran
		} else {
			runTimer();
		}
		pthread_mutex_unlock(&timerAndCpu);
		
	}
}
/*to run the timer */
void runTimer() {
	--myTimer;
	if(new_process) {
			myTimer = rand() % TIMER_MOD + OFFSET;
			new_process = FALSE;
	}
	//printf("\nthe new timer time is %d", myTimer);
	
}

/*to create a fifoQueue structure
@return the new fifoQueue*/
fifoQueue createQueue() {
    fifoQueue que = (fifoQueue)malloc(sizeof(struct Queue));
    que->first = NULL;
    que->last = NULL;
    que->size = 0;
    return que;
}

/*create a process control block
@return the new pcb*/
pcb createPCB(int *id, int myProducer, int myConsumer, int myAType, int myBType) {
    pcb new_pcb = (pcb)malloc(sizeof(struct PCB));
	new_pcb->myValue = START_INDEX;
	new_pcb->producer = myProducer;
	new_pcb->consumer = myConsumer;
    new_pcb->runLength = PCB_RUN + rand() % PCB_RUN;
    new_pcb->usedQuanta = FALSE;
    new_pcb->ioLength = rand() % IO_LENGTH + OFFSET;
    new_pcb->process_id = *id;
    new_pcb->state = ready;
	new_pcb->aType = myAType;
	new_pcb->bType = myBType;
	new_pcb->time_stamp = clock();
	new_pcb->pair_number = mut_res_pair_num;
	if(myBType){ 
		++mut_res_pair_num;
		printf("\nnew pair number %d", mut_res_pair_num);
	}
	++*id;
    return new_pcb;
}
/*create a node
@param new_pcb the process control block pointer
@return the new node
*/
node createNode(pcb myPcb) {
    node new_node = (node)malloc(sizeof(struct Node));
    new_node->processes = myPcb;
    new_node->next = NULL;
    return new_node;
}
/*to delete the entire list
@param head the front node in the list
*/
void deleteList(node *head){
    node curr = *head;
    node next;
    while(curr != NULL){
        next = curr->next;
        free(curr->processes);
        free(curr);
        curr = next;
    }
    *head = NULL;
}
/*add a node to the queue or mutex
@param myFifo the fifoQueue or Mutex
@param myNode the pcb to include
*/
void addNode(fifoQueue myFifo, node myNode) {
	//printf("\nnode with process id %d added", myNode->processes->process_id);
    if(myFifo->first == NULL) {
        myFifo->first = myNode;
        myFifo->first->next = NULL;
    } else {
        if(myFifo->last == NULL) {
            myFifo->last = myNode;
            myFifo->first->next = myFifo->last;
        } else {
            myFifo->last->next = myNode;
            myFifo->last = myFifo->last->next;
        }
    }
    myFifo->size++;
}


/*reenqueue node at the end of the run
@param fp the file to print to
@param process_id the id of process for adding new node
@param myReadyQueue the processes waiting for cpu
@param myNode the node done with first run
*/
void reenqueueNode(fifoQueue myReadyQueue, node myNode) {
    if((rand() % PERCENT) > REENQUEUE_CHANCE) {//put process back in readyQueue
        myNode->processes->usedQuanta = FALSE;//reinitialize quanta to 0
        myNode->processes->state = ready;
        myNode->processes->ioLength = FALSE;//io length set to 0
        pthread_mutex_lock(&readyQueueAdd);
        addNode(myReadyQueue, myNode);
        pthread_mutex_unlock(&readyQueueAdd);
        pthread_mutex_lock(&printMutex);
        printf("\nprocess %d added back to the queue", myNode->processes->process_id);
        pthread_mutex_unlock(&printMutex);
    } else {
        pthread_mutex_lock(&printMutex);
        printf("\nprocess %d dropped from the queue", myNode->processes->process_id);
        pthread_mutex_unlock(&printMutex);
        free(myNode->processes);
        free(myNode->next);
        free(myNode);
    }
    //printFifo(myReadyQueue);
}
/*return first node in list
@param myFifo the queue from which to remove first element
*/
node returnFirst(fifoQueue *myFifo) {
    node temp = (*myFifo)->first;
    (*myFifo)->first = (*myFifo)->first->next;
    if((*myFifo)->first == NULL)
        (*myFifo)->last = NULL;
    temp->next = NULL;
    (*myFifo)->size--;
    return temp;
}
/*prints the fifo queue
@param myFifo the fifoqueue to print
@param fp the file to output to
*/
void printFifo(fifoQueue myFifo) {
    node temp = myFifo->first;
    while(temp != NULL) {
		printf("\n%d ", temp->processes->process_id);
        temp = temp->next;
    }
    printf("\n");
}
/*checks if empty
@param myFifo the queue to check for empty
@return whether or not empty
*/
int is_empty(fifoQueue myFifo) {
    return myFifo->first == NULL;
}
/*peeks at first element id
@param myFifo the queue to peek at
@return process id of first element
*/
int peek(fifoQueue myFifo) {
    if(myFifo->first != NULL)
        return myFifo->first->processes->time_stamp;
}

