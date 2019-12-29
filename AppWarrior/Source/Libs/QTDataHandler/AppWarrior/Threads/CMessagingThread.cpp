/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CMessagingThread.h"


HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  CMessagingThread                                            [public]
// ---------------------------------------------------------------------
// Constructor

CMessagingThread::CMessagingThread( EPriority inPriority )
	: CThread( inPriority )
{
}


// ---------------------------------------------------------------------
//  Execute                                         [protected][virtual]
// ---------------------------------------------------------------------
// Execute the thread. Handle the looping here

UInt32
CMessagingThread::Execute()
{
	UInt32 retValue = 0;
	while( IsRunning() && !IsTerminated() ){
		HandleAllMessages();
		retValue = Run();
	}
	return retValue;
}

HL_End_Namespace_BigRedH
