// thread.h 
//	Data structures for managing threads.  A thread represents
//	sequential execution of code within a program.
//	So the state of a thread includes the program counter,
//	the processor registers, and the execution stack.
//	
// 	Note that because we allocate a fixed size stack for each
//	thread, it is possible to overflow the stack -- for instance,
//	by recursing to too deep a level.  The most common reason
//	for this occuring is allocating large data structures
//	on the stack.  For instance, this will cause problems:
//
//		void foo() { int buf[1000]; ...}
//
//	Instead, you should allocate all data structures dynamically:
//
//		void foo() { int *buf = new int[1000]; ...}
//
//
// 	Bad things happen if you overflow the stack, and in the worst 
//	case, the problem may not be caught explicitly.  Instead,
//	the only symptom may be bizarre segmentation faults.  (Of course,
//	other problems can cause seg faults, so that isn't a sure sign
//	that your thread stacks are too small.)
//	
//	One thing to try if you find yourself with seg faults is to
//	increase the size of thread stack -- ThreadStackSize.
//
//  	In this interface, forking a thread takes two steps.
//	We must first allocate a data structure for it: "t = new Thread".
//	Only then can we do the fork: "t->fork(f, arg)".
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"
#include "sysdep.h"
#include "machine.h"
#include "addrspace.h"

// CPU register state to be saved on context switch.  
// The x86 needs to save only a few registers, 
// SPARC and MIPS needs to save 10 registers, 
// the Snake needs 18,
// and the RS6000 needs to save 75 (!)
// For simplicity, I just take the maximum over all architectures.

#define MachineStateSize 75 


// Size of the thread's private execution stack.
// WATCH OUT IF THIS ISN'T BIG ENOUGH!!!!!
const int StackSize = (8 * 1024);	// in words


// Thread state
enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED, ZOMBIE };


// The following class defines a "thread control block" -- which
// represents a single thread of execution.
//
//  Every thread has:
//     an execution stack for activation records ("stackTop" and "stack")
//     space to save CPU registers while not running ("machineState")
//     a "status" (running/ready/blocked)
//    
//  Some threads also belong to a user address space; threads
//  that only run in the kernel have a NULL address space.

class Thread {
  private:
    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for SWITCH to work.
    int *stackTop;			 // the current stack pointer
    void *machineState[MachineStateSize];  // all registers except for stackTop

  public:
    Thread(char* debugName, int threadID, int initP);		// initialize a Thread 
    ~Thread(); 				// deallocate a Thread
					// NOTE -- thread being deleted
					// must not be running when delete 
					// is called

    // basic thread operations

    void Fork(VoidFunctionPtr func, void *arg); 
    				// Make thread run (*func)(arg)
    void Yield();  		// Relinquish the CPU if any 
				// other thread is runnable
    void Sleep(bool finishing); // Put the thread to sleep and 
				// relinquish the processor
    void Begin();		// Startup code for the thread	
    void Finish();  		// The thread is done executing
    
    void CheckOverflow();   	// Check if thread stack has overflowed
    void setStatus(ThreadStatus st) { status = st; }
    ThreadStatus getStatus() { return (status); }
	  char* getName() { return (name); }
    
	  int getID() { return (ID); }
    void Print() { cout << name; }
    void SelfTest();		// test whether thread impl is working

    // [ADD MP3]
    int GetPriority() { return (priority); }
    int GetQueueLevel() { return (queueLevel); }
    int GetRunningTicks() { return (T); }
    int GetWaitingTicks() { return (W); }
    double GetRemainBurst() { return (burstTime); }
    void UpdateRunningTicks(); // Update running tick according to total ticks and start running tick
    void UpdateWaitingTicks(); // Update waiting tick according to total ticks and start waiting tick
    void ResetRunningTicks() { T = 0; }   // Reset running ticks to 0
    void ResetWaitingTicks() { W = 0; }   // Reset waiting ticks to 0
    void ResetStartRunningTick(); // Reset start running ticks to current tick
    void ResetStartWaitingTick(); // Reset start waiting ticks to current tick
    void UpdateQueueLevel(int newLevel) { queueLevel = newLevel; } // This function for scheduler easily update queue level
    void UpdateRemainBurst(); // Update remaining burst time. Run it only when state from running state to waiting state
    void UpdatePriority() { priority = min(priority + 10, 149); }  // Update priority by 10. Called when in againg, waiting time is more than 1500 ticks
    // [END-of-ADD]

  private:
    // some of the private data for this class is listed above
    
    int *stack; 	 	// Bottom of the stack 
				// NULL if this is the main thread
				// (If NULL, don't deallocate stack)
    ThreadStatus status;	// ready, running or blocked
    char* name;
	  int   ID;
    // [ADD MP3]
    int priority;     // priority (0~149)
    int queueLevel;   // queue level (1~3)
    int startRunningTick;
    int startWaitingTick;
    int T;            // total run ticks, accumulate only when in running state, reset when go to running state
    int W;            // total wait ticks, accumulate only when in ready queue, reset when go to running state
    double burstTime; // approximate remaining burst time, t_i = 0.5*T + 0.5*t_{i-1}, t_0 = 0, update only when from running state to waiting state
    // [END-of-ADD]
    void StackAllocate(VoidFunctionPtr func, void *arg);
    				// Allocate a stack for thread.
				// Used internally by Fork()

// A thread running a user program actually has *two* sets of CPU registers -- 
// one for its state while executing user code, one for its state 
// while executing kernel code.

    int userRegisters[NumTotalRegs];	// user-level CPU register state

  public:
    void SaveUserState();		// save user-level register state
    void RestoreUserState();		// restore user-level register state

    AddrSpace *space;			// User code this thread is running.
};

// external function, dummy routine whose sole job is to call Thread::Print
extern void ThreadPrint(Thread *thread);	 

// Magical machine-dependent routines, defined in switch.s

extern "C" {
// First frame on thread execution stack; 
//   	call ThreadBegin
//	call "func"
//	(when func returns, if ever) call ThreadFinish()
void ThreadRoot();

// Stop running oldThread and start running newThread
void SWITCH(Thread *oldThread, Thread *newThread);
}

#endif // THREAD_H
