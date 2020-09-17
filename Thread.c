#include "Init.h"
#include "Thread.h"
#include "Scheduler.h"
#include <unistd.h>
#include <stdio.h>
#include "Queue.h"

extern BOOL join = 0;
int reap;

int thread_create(thread_t *thread, thread_attr_t *attr, int priority, void *(*start_routine) (void *), void *arg)
{
    char* stack = malloc(STACK_SIZE);
    int flags = CLONE_VM|CLONE_SIGHAND|CLONE_FS|CLONE_FILES|SIGCHLD;

    if(stack==0)
    {
        perror("malloc: could not allocate stack\n");
        exit(1);
    }

    int pid = clone((void*)start_routine, (char*)stack+STACK_SIZE, SIGCHLD|flags, arg);
    if(pCurrentThread!=NULL)
        kill(pid, SIGSTOP); // stop child thread    

    Thread* pThread;
    pThread = (Thread*)malloc(sizeof(Thread));
    pThread->priority = priority;
    pThread->pid = pid;
    pThread->stackAddr = stack;
    pThread->stackSize = STACK_SIZE;
    pThread->status = THREAD_STATUS_READY;

    for(int i=0; i<MAX_READYQUEUE_NUM; i++) {
        if(pThreadTblEnt[i].bUsed==0) {
            pThreadTblEnt[i].bUsed = 1;
            pThreadTblEnt[i].pThread = pThread;
            *thread = i; // correct
            break;
        }
    }
    //context switching

    if(pCurrentThread==NULL) {
        pCurrentThread = pThread;
        kill(pCurrentThread->pid, SIGCONT);
    }
    else if (pThread->priority < pCurrentThread->priority) {
        InsertThreadToReadyQueue(pCurrentThread);
        pCurrentThread->status = THREAD_STATUS_READY;

        pThread->status = THREAD_STATUS_RUN;

        if(kill(pCurrentThread->pid, SIGSTOP)==-1)
        printf("sigstop error\n");
        if(kill(pThread->pid, SIGCONT)==-1)
        printf("sigcont error\n");

        pCurrentThread = pThread;
    }
    else {
        pThread->status = THREAD_STATUS_READY;
        InsertThreadToReadyQueue(pThread);
    }

    return *thread;
}

int thread_suspend(thread_t tid)
{
    Thread* pThread = pThreadTblEnt[tid].pThread;

    if(pThread->status==THREAD_STATUS_READY) {
        // delete pThread in ready queue
        
        DeleteThread(pThread);
        pThread->status = THREAD_STATUS_WAIT;
        //move to waiting queue
        InsertThreadIntoWaitingList(pThread);

        kill(pThread->pid, SIGSTOP);

        return 0;
    }
    else if(pThread->status==THREAD_STATUS_WAIT || pThread->status==THREAD_STATUS_RUN) {
        return 0;
    }
    else { // zombie
        return -1;
    }
}

int thread_cancel(thread_t tid)
{
    Thread* pThread = pThreadTblEnt[tid].pThread;
    kill(pThread->pid, SIGKILL);

    if(pThread->status==THREAD_STATUS_RUN || pThread->status==THREAD_STATUS_ZOMBIE)
        return -1;

    if(pThread->status == THREAD_STATUS_WAIT) { // in waiting list
        DeleteThreadWaitingList(pThread);
    }
    // else if(pThread->status == THREAD_STATUS_READY) { // in ready queue
    //     Thread *temp = pReadyQueueEnt[0].pHead;

	// 	printf("ready[0] queue : ");
	// 	for(;temp!=NULL;temp=temp->phNext){
	// 		printf(" %d",temp->pid);
	// 	}
	// 	printf("\n");
    //     printf("to delete: %d\n", pThread->pid);
    //     DeleteThread(pThread);
    // }
    // //deallocate tcb
    // pThreadTblEnt[tid].bUsed = 0;
    // pThreadTblEnt[tid].pThread = NULL;
    free(pThread);

    return 0;
}

int thread_resume(thread_t tid)
{
    Thread* pThread = pThreadTblEnt[tid].pThread;
    int priority = pThread->priority;

    if(pThread->status == THREAD_STATUS_WAIT) {
        if(priority < pCurrentThread->priority) {
            InsertThreadToReadyQueue(pCurrentThread);

            Thread *temp = pReadyQueueEnt[0].pHead;

            pCurrentThread->status = THREAD_STATUS_READY;

            DeleteThreadWaitingList(pThread);
            pThread->status = THREAD_STATUS_RUN;

            __ContextSwitch(pCurrentThread->pid, pThread->pid);
            pCurrentThread = pThread;
        }
        else {
            DeleteThreadWaitingList(pThread);
            pThread->status = THREAD_STATUS_READY;
            InsertThreadToReadyQueue(pThread);
        }
        return 0;
    }
    else return -1;
}

void DeleteThreadWaitingList(Thread* pThread) {
    Thread* prev = pThread->phPrev;
    Thread* next = pThread->phNext;
    if(prev==NULL)
        pWaitingQueueHead = NULL;
    else
        prev->phNext = next;

    if(next==NULL)
        pWaitingQueueTail = NULL;
    else
        next->phPrev = prev;
    

    pThread->phNext = NULL;
    pThread->phPrev = NULL;
}


thread_t thread_self()
{
    return getpid();
}

int thread_join(thread_t tid, void **retval)
{
    join = 1;
    // tid is the index of thread table
    Thread* child = pThreadTblEnt[tid].pThread;

    if(child->status != THREAD_STATUS_ZOMBIE) {
        int pid = getpid();

        Thread* parent; // the parent thread
        // find thread in table through pid
        for(int i=0; i<MAX_THREAD_NUM; i++) {
            if(pThreadTblEnt[i].bUsed==1 
            && pThreadTblEnt[i].pThread->pid == pid) {
                parent = pThreadTblEnt[i].pThread;

                break;
            }
        }

        parent->status = THREAD_STATUS_WAIT;
        InsertThreadIntoWaitingList(parent);

        for(int i=0; i<MAX_READYQUEUE_NUM; i++) { //1
            if(pReadyQueueEnt[i].pHead != NULL) { //2
            
            // if head is not, to use cpu change the current thread
                Thread* head = pReadyQueueEnt[i].pHead;
                //----------repeat--------------
                int index = head -> priority;
                if(head->phNext!=NULL) {
                    head->phPrev = NULL;
                    pReadyQueueEnt[i].pHead = head->phNext;
                }
                else
                    pReadyQueueEnt[i].pHead = NULL;

                if(head != NULL)
                    pCurrentThread = head;

                kill(pCurrentThread->pid, SIGCONT);
                break;
            }
        }
         
        pause(); // wake up when get signal from chlid

        // move the parent thread to running or ready state
        ContextSwitching(parent);
        DeleteThreadWaitingList(parent);
    }

    *retval = child->exitCode;
    // if(child->status==THREAD_STATUS_ZOMBIE) {
    //     DeleteThreadWaitingList(child);
    // }
    DeleteThreadWaitingList(child);

    // reaping child thread & stack
    free(child->stackAddr);
    free(child);

    join = 0;
    return 0;
}

void reaping(Thread* child, void **retval) {
  


    kill(getppid(), SIGCONT);
}

int thread_exit(void *retval)
{
    Thread* current = pCurrentThread;
    current->exitCode = (int**)retval;

    //kill(current->pid, SIGKILL);

    // if(current->status==THREAD_STATUS_RUN || pThread->status==THREAD_STATUS_ZOMBIE)
    //     return -1;

    if(current->status == THREAD_STATUS_READY) {
        DeleteThread(current);
    }

    // move the current thread to waiting list
    current->status = THREAD_STATUS_ZOMBIE;
    InsertThreadIntoWaitingList(current);
    
    for(int i=0; i<MAX_READYQUEUE_NUM; i++) { //1
        if(pReadyQueueEnt[i].pHead != NULL) { //2
            Thread* pThread = pReadyQueueEnt[i].pHead;

            int index = pThread -> priority;
            Thread* temp = pReadyQueueEnt[index].pHead;
            if(temp->phNext!=NULL) {
                pReadyQueueEnt[index].pHead = temp->phNext;
            }
            else {
                pReadyQueueEnt[index].pHead = NULL;
            }
            pReadyQueueEnt[index].pHead->phPrev = NULL;

            kill(pThread->pid, SIGCONT);
            pCurrentThread = pThread;

            break;
        }
    }
    
    exit(0);
    return 0;
}

void InsertThreadIntoWaitingList(Thread* pThread){
    if(pWaitingQueueHead==NULL || pWaitingQueueTail==NULL) {
        pWaitingQueueHead = pThread;
        pWaitingQueueTail = pThread;
        pThread->phNext=NULL;
        pThread->phPrev=NULL;
    }
    else { // insert to tail
        Thread* temp = pWaitingQueueTail;
        temp->phNext = pThread; //previous tail
        pWaitingQueueTail = pThread; //change tail
        pWaitingQueueTail->phNext = NULL;
        pWaitingQueueTail->phPrev = temp;
    }
}