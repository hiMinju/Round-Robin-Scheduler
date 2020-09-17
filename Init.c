#include "Init.h"
#include "Thread.h"
#include "Scheduler.h"
#include <stdio.h>
#include <unistd.h>
#include "Queue.h"

extern BOOL join;
extern int reap;

// need to make sigaction with flags

void alarmHandler(int sig) {
    // how about decide to context switch in this handler?
    RunScheduler();
}

void childHandler(int sig) {
    int pid = getpid();

    if(join == 0) {
           // when child exist properly
        
        Thread* pThread; // the thread will be removed
        // remove thread from thread table
        for(int i=0; i<MAX_THREAD_NUM; i++) {
            if(pThreadTblEnt[i].bUsed==1 
            && pThreadTblEnt[i].pThread->pid == pid) {

                pThread = pThreadTblEnt[i].pThread;

                // whether chlid is zombie or not -> join or not


                pThreadTblEnt[i].bUsed = 0;
                pThreadTblEnt[i].pThread = NULL;

                break;
            }
        } 
    }


    for(int i=0; i<MAX_READYQUEUE_NUM; i++) { //1
        if(pReadyQueueEnt[i].pHead != NULL) { //2
        // if head is not, to use cpu change the current thread
            Thread* head = pReadyQueueEnt[i].pHead;
            // __ContextSwitch(pCurrentThread->pid, pThread->pid);
            
            //----------repeat--------------
            int index = head -> priority;
            if(head->phNext!=NULL) {
                head->phPrev = NULL;
                pReadyQueueEnt[i].pHead = head->phNext;
                
                pCurrentThread = head;
            }
            else
                pReadyQueueEnt[i].pHead = NULL;

            kill(pCurrentThread->pid, SIGCONT);
            break;
        }
    }
}

void Init(void)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    struct sigaction act;
    act.sa_handler = childHandler;
    act.sa_flags = SA_NOCLDSTOP;
    act.sa_mask = set;
    sigaction (SIGCHLD, &act, NULL);

    pCurrentThread = NULL;

    pWaitingQueueHead = NULL;
    pWaitingQueueTail = NULL;

    if (signal(SIGALRM, alarmHandler) == SIG_ERR) {
        perror("signal() error!");
    }

    for(int i=0; i<MAX_READYQUEUE_NUM; i++) {
        //init
        pReadyQueueEnt[i].pHead = NULL;
        pReadyQueueEnt[i].pTail = NULL;
        pReadyQueueEnt[i].queueCount = 0;
    }
}
void deallocateThread(Thread* pThread)
{    
    for(int i=0; i<MAX_THREAD_NUM; i++) {
        if(pThreadTblEnt[i].pThread == pThread) {
            pThreadTblEnt[i].bUsed = 0;
            free(pThread);
            break;
        }
    }
}

BOOL DeleteThreadReadyQueue(thread_t tid) {
    Thread* temp;
    for(int i=0; i<MAX_READYQUEUE_NUM; i++) {
        temp = pReadyQueueEnt[i].pHead;
        for(int j=0; j<pReadyQueueEnt[i].queueCount; j++) {
            if(temp->pid==tid) {
                Thread* prev = temp->phPrev;
                Thread* next = temp->phNext;

                prev->phNext = next;
                next->phPrev = prev;

                temp->phNext = NULL;
                temp->phPrev = NULL;
                deallocateThread(temp);

                --pReadyQueueEnt[i].queueCount;
                
                if(pReadyQueueEnt[i].queueCount==0) {
                    pReadyQueueEnt[i].pHead = NULL;
                    pReadyQueueEnt[i].pTail = NULL;
                }
                return 0;
            }
            else {
                temp = temp->phNext;
            }
        }
    }
    return 1;
}

BOOL DeleteThread(Thread* pThread) {

    int index = pThread -> priority;
    Thread* temp = pReadyQueueEnt[index].pHead;

    if(temp->phNext==NULL){ // only one in ready queue
        pReadyQueueEnt[index].pHead = NULL;
    }
    else if(temp->phNext!=NULL&&temp->pid==pThread->pid) { // target is head
        pReadyQueueEnt[index].pHead = temp->phNext;
        temp->phPrev = NULL;
        temp->phNext = NULL;

        --pReadyQueueEnt[index].queueCount;
        return 0;
    }
    else if(pReadyQueueEnt[index].pTail->pid == pThread->pid) {
        // target is tail

        if(pReadyQueueEnt[index].pTail->phPrev!=NULL)
            pReadyQueueEnt[index].pTail = pReadyQueueEnt[index].pTail->phPrev;
        else
            pReadyQueueEnt[index].pTail = NULL;

        pReadyQueueEnt[index].pTail->phPrev = NULL;

        //pReadyQueueEnt[index].pTail->phNext = NULL;

        --pReadyQueueEnt[index].queueCount;
        return 0;
    }

    while(temp->phNext!=NULL) {
        if(temp->pid==pThread->pid) {
            Thread* prev = temp->phPrev;
            Thread* next = temp->phNext;

            prev->phNext = next;
            next->phPrev = prev;

            temp->phNext = NULL;
            temp->phPrev = NULL;
            --pReadyQueueEnt[index].queueCount;
            if(pReadyQueueEnt[index].queueCount==0) {
                pReadyQueueEnt[index].pHead = NULL;
                pReadyQueueEnt[index].pTail = NULL;
            }
            else if(pReadyQueueEnt[index].pHead == temp){
                pReadyQueueEnt[index].pHead = pThread->phNext;
            }

            
            return 0;
        }
        else {
            temp = temp->phNext;
        }
    }
    return 1;
}