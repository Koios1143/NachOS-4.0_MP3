// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "copyright.h"
#include "list.h"
#include "thread.h"

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

class Scheduler {
  public:
    Scheduler();		// Initialize list of ready threads 
    ~Scheduler();		// De-allocate ready list

    void ReadyToRun(Thread* thread);	
    				// Thread can be dispatched.
    Thread* FindNextToRun();	// Dequeue first thread on the ready 
				// list, if any, and return thread.
    void Run(Thread* nextThread, bool finishing);
    				// Cause nextThread to start running
    void CheckToBeDestroyed();// Check if thread that had been
    				// running needs to be deleted
    void Print();		// Print contents of ready list
    
    // SelfTest for scheduler is implemented in class Thread

    // [ADD MP3]
    static int Level1Comp(Thread *x, Thread *y);
    static int Level2Comp(Thread *x, Thread *y);
    int Level1Empty() { return (level1List->IsEmpty()); }
    int Level2Empty() { return (level2List->IsEmpty()); }
    int Level3Empty() { return (level3List->IsEmpty()); }
    Thread* GetLevel1Front() { return (level1List->Front()); }
    Thread* GetLevel2Front() { return (level2List->Front()); }
    Thread* GetLevel3Front() { return (level3List->Front()); }
    void UpdateAllWaitTicks();
    void Aging();
    void WaitAppend(Thread *x);
    // [END-of-ADD]
    
  private:
    // List<Thread *> *readyList;  // [ADD MP3] we no longer need readyList. ; queue of threads that are ready to run,
				// but not running

    // [ADD MP3]
    SortedList<Thread *> *level1List;   // level 1 ready list, sort by remaining CPU burst ticks
    SortedList<Thread *> *level2List;   // level 2 ready list, sort by priority
    List<Thread *> *level3List;         // level 3 ready list
    List<Thread *> *WaitingList;        // Waiting List. but we only record which thread are in waiting queue
    List<Thread *> *tmp;                // tmp buffer for aging
    void InsertToReadyList(Thread *thread);
    // [END-of-ADD]

    Thread *toBeDestroyed;	// finishing thread to be destroyed
    				// by the next thread that runs
};

#endif // SCHEDULER_H
