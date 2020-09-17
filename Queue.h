#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Thread.h"

void InsertThreadToReadyQueue(Thread* pThread);
BOOL DeleteThreadReadyQueue(thread_t tid);
BOOL DeleteThread(Thread* pThread);
void InsertThreadIntoWaitingList(Thread* pThread);
void ContextSwitching(Thread* pThread);
void DeleteThreadWaitingList(Thread* pThread);

# endif
