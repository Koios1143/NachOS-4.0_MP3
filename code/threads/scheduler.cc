// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    // [ADD MP3] Now initialize L1~L3 ready list
    // readyList = new List<Thread *>; 
    level1List = new SortedList<Thread *>(Level1Comp);
    level2List = new SortedList<Thread *>(Level2Comp);
    level3List = new List<Thread *>;
    WaitingList = new List<Thread *>;
    tmp = new List<Thread *>;
    // [END-of-ADD]
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    // [ADD MP3]
    // delete readyList; 
    delete level1List;
    delete level2List;
    delete level3List;
    // [END-of-ADD]
} 

//----------------------------------------------------------------------
// [ADD MP3]
// Scheduler::level1Comp
// 	sort priority according to estimate remaining CPU burst
//  lower value, higher priority
// 
//	returns -1 if x < y
//	returns 0 if x == y
//	returns 1 if x > y
//----------------------------------------------------------------------

int
Scheduler::Level1Comp(Thread *x, Thread *y){
    if(x->GetRemainBurst() < y->GetRemainBurst()) return -1;
    else if(x->GetRemainBurst() == y->GetRemainBurst()) return 0;
    else return 1;
}

//----------------------------------------------------------------------
// [ADD MP3]
// Scheduler::level2Comp
// 	sort priority according to real priority
//  higher value, higher priority
//
//		returns -1 if x > y
//		returns 0 if x == y
//		returns 1 if x < y
//----------------------------------------------------------------------

int
Scheduler::Level2Comp(Thread *x, Thread *y){
    if(x->GetPriority() > y->GetPriority()) return -1;
    else if(x->GetPriority() == y->GetPriority()) return 0;
    else return 1;
}

//----------------------------------------------------------------------
//  [ADD MP3]
//  Scheduler::InsertToReadyList
//	Update queue level according to priority.
//  Assume the priority must within 0~149.
//  0~49 for L1; 50~99 for L2; 100~149 for L3.
//----------------------------------------------------------------------

void
Scheduler::InsertToReadyList (Thread *thread) {
    int priority = thread->GetPriority();

    if(priority <= 49) {
        thread->UpdateQueueLevel(3);
        level3List->Append(thread);
    }
    else if(priority <= 99) {
        thread->UpdateQueueLevel(2);
        level2List->Insert(thread);
    }
    else {
        thread->UpdateQueueLevel(1);
        level1List->Insert(thread);
    }
    DEBUG(dbgQLevel, "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getID() << "] is inserted into queue L[" << thread->GetQueueLevel() << "]");
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//
//  Running -> Ready
//  Waiting -> Ready
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);

    // [ADD MP3]
    // readyList->Append(thread);
    if(!WaitingList->IsEmpty() && WaitingList->IsInList(thread)){
        WaitingList->Remove(thread);
    }
    InsertToReadyList(thread); // Insert thread to ready list
    // [END-of-ADD]
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//
//  Running -> Ready
//  Running -> Waiting
//  Running -> Terminated
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    // [ADD MP3]
    Thread *nextThread = NULL;
    if (level1List->IsEmpty()){
        if(level2List->IsEmpty()){
            if(level3List->IsEmpty()){
                nextThread = NULL;
            }
            else{
                nextThread = level3List->RemoveFront();
            }
        }
        else{
            nextThread = level2List->RemoveFront();
        }
    }
    else{
        nextThread = level1List->RemoveFront();
    }

    if(nextThread != NULL) {
        DEBUG(dbgQLevel, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is removed from queue L[" << nextThread->GetQueueLevel() << "]");
    }

    return nextThread;
    // [END-of-ADD]

    // if (readyList->IsEmpty()) {
	// 	return NULL;
    // } else {
    // 	return readyList->RemoveFront();
    // }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    // [ADD MP3]
    if(nextThread != oldThread) {
        oldThread->UpdateRunningTicks();
        DEBUG(dbgQLevel, "[E] Tick [" << kernel->stats->totalTicks << "]: Thread [" << nextThread->getID() << "] is now selected for execution, thread [" << oldThread->getID() << "] is replaced, and it has executed [" << oldThread->GetRunningTicks() << "] ticks");
    }
    if(oldThread->getStatus() == BLOCKED) {
        // come from sleep, which means running -> waiting
        // so we need to reset the running tick here
        oldThread->ResetRunningTicks();
    }
    // [END-of-ADD]
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    // [ADD MP3] before running, reset waiting time, reset start tick
    oldThread->ResetWaitingTicks();
    nextThread->ResetStartRunningTick();
    // [END-of-ADD]

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    // [ADD MP3]
    // readyList->Apply(ThreadPrint);
    level1List->Apply(ThreadPrint);
    level2List->Apply(ThreadPrint);
    level3List->Apply(ThreadPrint);
    // [END-of-ADD]
}

//----------------------------------------------------------------------
//  [ADD MP3]
//  Scheduler::UpdateAllWaitTicks
//	Update all lists' thread waiting tick
//----------------------------------------------------------------------
void
Scheduler::UpdateAllWaitTicks() {
    ListIterator<Thread *> *iter = new ListIterator<Thread *>(level1List);
	for (; !iter->IsDone(); iter->Next()) {
	    iter->Item()->UpdateWaitingTicks();
        iter->Item()->ResetStartWaitingTick();
    }
    delete iter;

    iter = new ListIterator<Thread *>(level2List);
    for (; !iter->IsDone(); iter->Next()) {
	    iter->Item()->UpdateWaitingTicks();
        iter->Item()->ResetStartWaitingTick();
    }
    delete iter;

    iter = new ListIterator<Thread *>(level3List);
    for (; !iter->IsDone(); iter->Next()) {
	    iter->Item()->UpdateWaitingTicks();
        iter->Item()->ResetStartWaitingTick();
    }
    delete iter;

    iter = new ListIterator<Thread *>(WaitingList);
    for (; !iter->IsDone(); iter->Next()) {
	    iter->Item()->UpdateWaitingTicks();
        iter->Item()->ResetStartWaitingTick();
    }
    delete iter;
}

//----------------------------------------------------------------------
//  [ADD MP3]
//  Scheduler::Aging
//	Update queue all thread's waiting time in ready queue
//  Perform aging once it's current waiting time is over 1500 again
//----------------------------------------------------------------------
void
Scheduler::Aging() {
    // First update level3
    // we always remove all elements and append to the list again.
    // The reasome why we update like this is because level1 and level2 will sorted
    // DEBUG(dbgQLevel, "[X] Tick [" << kernel->stats->totalTicks << "]: start aging");
    for(int i=0, end=level3List->NumInList() ; i<end ; i++){
        Thread *cur = level3List->RemoveFront();
        DEBUG(dbgQLevel, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << cur->getID() << "] is removed from queue L[" << cur->GetQueueLevel() << "]");
        if(cur->GetWaitingTicks() >= 1500) {
            cur->UpdatePriority();
        }
        tmp->Append(cur);
    }
    // Then update level2
    for(int i=0, end=level2List->NumInList() ; i<end ; i++){
        Thread *cur = level2List->RemoveFront();
        DEBUG(dbgQLevel, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << cur->getID() << "] is removed from queue L[" << cur->GetQueueLevel() << "]");
        if(cur->GetWaitingTicks() >= 1500) {
            cur->UpdatePriority();
        }
        tmp->Append(cur);
    }
    // Finally update level 1
    for(int i=0, end=level1List->NumInList() ; i<end ; i++){
        Thread *cur = level1List->RemoveFront();
        DEBUG(dbgQLevel, "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << cur->getID() << "] is removed from queue L[" << cur->GetQueueLevel() << "]");
        if(cur->GetWaitingTicks() >= 1500) {
            cur->UpdatePriority();
        }
        tmp->Append(cur);
    }
    // Insert all threads back to the list it should belongs to
    for(int i=0, end=tmp->NumInList() ; i<end ; i++){
        Thread *cur = tmp->RemoveFront();
        InsertToReadyList(cur);
    }
    // DEBUG(dbgQLevel, "[X] Tick [" << kernel->stats->totalTicks << "]: end aging");
}

//----------------------------------------------------------------------
//  [ADD MP3]
//  Scheduler::WaitAppend
//	Append a thread into waiting list
//----------------------------------------------------------------------
void
Scheduler::WaitAppend(Thread *x){
    WaitingList->Append(x);
}