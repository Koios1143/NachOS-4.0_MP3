// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//
//  [ADD MP3]
//  Note that we should preempt in the following situations
//  1. current thread is L3, but L2 ready list is not empty
//  2. current thread is L2, but L1 ready list is not empty
//  3. current thread is L1, but L1 ready list's front has lower remaining burst
//  To preempt, call YieldOnReturn()
//  [END-of-ADD]
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    
    // [ADD MP3]
    Thread *currentThread = kernel->currentThread;

    // Update running ticks or waiting ticks
    // And since we calculate running ticks by setting start running tick
    // So we also need to reset start tick after update
    if(currentThread->getStatus() == BLOCKED){
        // if current thread is waiting
        currentThread->UpdateWaitingTicks();
        currentThread->ResetStartWaitingTick();
    }
    else{
        // or current thread is running
        currentThread->UpdateRunningTicks();
        currentThread->ResetStartRunningTick();
    }

    // Then we also need to update all waiting ticks for threads in ready list
    kernel->scheduler->UpdateAllWaitTicks();

    // After update ticks, we can perform again now
    kernel->scheduler->Aging();

    // Finally, check whether there is preempt to be handle
    int currentLevel = currentThread->GetQueueLevel();
    if(currentLevel == 3){
        // each thread has 100 ticks quantum, once running ticks greater than 100, context switch needed
        // NOTE: Although this is not preempt, but we should always handle context switch when CallBack()
        if(currentThread->GetRunningTicks() >= 100){
            interrupt->YieldOnReturn();
        }
        // There are threads in L2
        else if(!kernel->scheduler->Level2Empty()){
            interrupt->YieldOnReturn();
        }
    }
    else if(currentLevel == 2){
        // There are threads in higher level
        if(!kernel->scheduler->Level1Empty()){
            interrupt->YieldOnReturn();
        }
    }
    else{
        // There's another thread also in L1, but with lower remaining CPU burst
        if(!kernel->scheduler->Level1Empty() && kernel->scheduler->Level1Comp(kernel->scheduler->GetLevel1Front(), currentThread) == -1){
            interrupt->YieldOnReturn();
        }
    }
    // [END-of-ADD]
}
