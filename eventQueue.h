/**
Chris DeJarlais
TCSS 422
Operating Systems
eventQueue.h
demoed in office on 12-17-15
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include<time.h>
#ifndef EVENTQUEUE_H_INCLUDED
#define EVENTQUEUE_H_INCLUDED
#define PCB_RUN 200
#define TRUE 1
#define FALSE 0
#define IO_SIZE 4
#define THREADS 6 
#define REENQUEUE_CHANCE 60 
#define IO_LENGTH 120 
#define PERCENT 100
#define TIMER_MOD 70
#define OFFSET 5
#define IO_REQUEST 2
#define START_INDEX 0
#define NOT_RUN 0
#define READ_WRITE 20 
#define MUT_RES_PAIRS 25
#define NA -1
#define UPPER_LIMIT 1000
#define NEW_INSERT 29    
#define MUT_RES_INSERT 11
#define FIRST_DEADLOCK_CHECK 60000
enum state_type{new, ready, running, waiting, halted};
typedef struct Queue * fifoQueue;
typedef struct PCB * pcb;
typedef struct Node * node;
struct PCB {
    int process_id;
    int ioLength;//io event length
    int mutexOwned;
    int mutexRun;//mutex hold length
    int runLength;//process run length
    int usedQuanta;
    int aType;
    int bType;
    int timeStamp;
    int producer;
    int consumer;
    int pair_number;
    int myValue;
    int time_stamp; 
    enum state_type state;

};
struct Node {
    node next;
    pcb processes;
};
struct Queue {
    node first;
    node last;
    int size;
};
int safeRead[READ_WRITE];
int safeWrite[READ_WRITE];
int myNum[READ_WRITE];
int myTimer;
int cpu_safe;
int timer_safe;
int new_process;
FILE *fp;
FILE *fp2;
FILE *fp3;
int mut_res_pair_num;
int last_process_id;
int last_time_run;
int last_time_called;
fifoQueue myReadyQueue;
fifoQueue IO[IO_SIZE];//array of Io queues-printer, keyboard, mouse, monitor;
fifoQueue blockedProducerConsumer[READ_WRITE];
fifoQueue aBlocked;
fifoQueue bBlocked;
pthread_t tid[THREADS];
pthread_mutex_t mutexes[READ_WRITE];//producer consumer mutexes
pthread_mutex_t R1_mutexes[MUT_RES_PAIRS];
pthread_mutex_t R2_mutexes[MUT_RES_PAIRS];
pthread_mutex_t deadlockCheck;
pthread_mutex_t timerAndCpu;
pthread_mutex_t printMutex;
pthread_mutex_t readyQueueAdd; 
pthread_cond_t cpu_running;
fifoQueue createQueue();
pcb createPCB(int *orderId, int producer, int consumer, int aType, int bType);
node createNode(pcb myPCB);
void tryLockSingleThread(node myNode); 
void deadlockHelper();
void tryLock(fifoQueue myResUser);
void* lockThread(void *myLock); 
void* deadlockDetectorThread(void *myDeadlock);
void deadlockDetector(float waitTime);
void readValue(node myNode);
void writeValue(node myNode);
void runCpu(fifoQueue myReadyQueue, node *temp);
void* ioThread(void *myIo);
void runIo(fifoQueue myIo);
void* timerThread(void *myTimer);
void runTimer(); 
void printFifo(fifoQueue myFifo);
int is_empty(fifoQueue myReadyQueue);
int peek(fifoQueue myReadyQueue);
void addNode(fifoQueue myFifo, node myNode);
void deleteList(node * head);
void reenqueueNode(fifoQueue myReadyQueue, node myNode);

node returnFirst(fifoQueue*myFifo);
#endif // EVENTQUEUE_H_INCLUDED
