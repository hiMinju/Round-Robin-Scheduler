#include "Init.h"
#include "Thread.h"
#include "Scheduler.h"
#include "Queue.h"

int RunScheduler( void )
{
    // it is for normal state
    // when any thread is in ready queue, it has to switch current thread to readyqueue's head

    // 1. check the exist of each index of ready queue's head
    // 2. if it is, need to change current thread to ready thread then send cont, stop signal
    // 3. if not, do nothing and send alarm signal after 2 sec
    BOOL empty = 0;
    for(int i=0; i<MAX_READYQUEUE_NUM; i++) { //1
        if(pReadyQueueEnt[i].pHead != NULL) { //2
            Thread* pThread = pReadyQueueEnt[i].pHead;

            InsertThreadToReadyQueue(pCurrentThread);

            int index = pThread -> priority;
            Thread* temp = pReadyQueueEnt[index].pHead;
            if(temp->phNext!=NULL) {
                pReadyQueueEnt[index].pHead = temp->phNext;
            }
            else {
                pReadyQueueEnt[index].pHead = NULL;
            }

            pReadyQueueEnt[index].pHead->phPrev = NULL;

            __ContextSwitch(pCurrentThread->pid, pThread->pid);

            pCurrentThread = pThread;

            empty = 1;
            break;
        }
    }
    //3
    if(empty == 0) {
        kill(pCurrentThread, SIGCONT);
    }

    alarm(TIMESLICE);
    return 0;
}

void InsertThreadToReadyQueue(Thread* pThread){
    int index = pThread->priority; // index of pReadyQueueEnt

    if(pReadyQueueEnt[index].pHead==NULL) { // if head is empty
        pReadyQueueEnt[index].pHead = pThread;
        pReadyQueueEnt[index].pTail = pThread;
        pThread->phPrev = NULL;
        pThread->phNext = NULL;
    }
    else { // more than one -> edit only tail part
        Thread* tail = pReadyQueueEnt[index].pTail;
        tail->phNext = pThread;
        pReadyQueueEnt[index].pTail = pThread;
        pThread->phPrev = tail;
        pThread->phNext = NULL;
    }
    ++pReadyQueueEnt[index].queueCount;
}

void ContextSwitching(Thread* pThread) {
    if(pCurrentThread==NULL) {
        pCurrentThread = pThread;
        kill(pCurrentThread->pid, SIGCONT);
    }
    else if (pThread->priority < pCurrentThread->priority) {
        InsertThreadToReadyQueue(pCurrentThread);
        pCurrentThread->status = THREAD_STATUS_READY;

        pThread->status = THREAD_STATUS_RUN;

        __ContextSwitch(pCurrentThread->pid, pThread->pid);
        pCurrentThread = pThread;
    }
    else {
        pThread->status = THREAD_STATUS_READY;
        InsertThreadToReadyQueue(pThread);
    }
}

void __ContextSwitch(int curpid, int newpid)
{    
    if(kill(curpid, SIGSTOP)==-1)
        printf("sigstop error\n");
    if(kill(newpid, SIGCONT)==-1)
        printf("sigcont error\n");
}
