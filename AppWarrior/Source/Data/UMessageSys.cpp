/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*

********** IMPROVEMENTS ***********

Have a fixed size chunk of memory in SMessageSys used for small messages.
Eg space for up to 8 msgs 64bytes or less?   16 msgs 32 bytes or less?
4 msgs 64bytes and 8 msgs 32bytes?

This would be used for extremely fast allocation of msgs (rather than allocating
on the heap).  It would also minimize the risk of the app having all sorts of
problems because a message couldn't be posted due to not enough memory.

Support for msgs allocated on the heap is still necessary (eg large msgs or
more msgs than will fit in the fixed size area), so store with SMessage a
flag that says where it was allocated.
*/

#include "UMessageSys.h"
#include "UMemory.h"

/*
 * Structures
 */

struct SMessage {
	SMessage *next;
	Uint32 type;
	Int32 priority;
	TMessageProc proc;
	void *context;
	void *object;
	Uint32 dataSize;
	Uint8 data[];
};

struct SMessageSys {
	SMessage *list;
	TMessageProc defaultProc;
	void *defaultContext;
};

/*
 * Macros
 */

#define REF		((SMessageSys *)inSys)

/* -------------------------------------------------------------------------- */

TMessageSys UMessageSys::New()
{
	return (TMessageSys)UMemory::NewClear(sizeof(SMessageSys));
}

void UMessageSys::Dispose(TMessageSys inSys)
{
	if (inSys)
	{
		SMessage *msg, *next;
		
		// kill each message
		msg = REF->list;
		while (msg)
		{
			next = msg->next;
			UMemory::Dispose((TPtr)msg);
			msg = next;
		}
		
		// kill system data
		UMemory::Dispose((TPtr)inSys);
	}
}

void UMessageSys::SetDefaultProc(TMessageSys inSys, TMessageProc inProc, void *inContext)
{
	Require(inSys);
	
	REF->defaultProc = inProc;
	REF->defaultContext = inContext;
}

void UMessageSys::GetDefaultProc(TMessageSys inSys, TMessageProc& outProc, void*& outContext)
{
	Require(inSys);
	
	outProc = REF->defaultProc;
	outContext = REF->defaultContext;
}

/*
 * Post() posts the specified message with the specified options to the message queue of <inSys>.
 * The data at <inData> of <inDataSize> bytes is copied, so you can destroy it as soon as this
 * function returns.  When the message is detected, the specified message handler proc is called
 * (nil means the default message handler, if any).
 */
void UMessageSys::Post(TMessageSys inSys, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority, TMessageProc inProc, void *inContext, void *inObject)
{
	Require(inSys);
	SMessage *msg = (SMessage *)UMemory::New(sizeof(SMessage) + inDataSize);
	
	try
	{
		// set properties as specified
		msg->next = nil;
		msg->type = inMsg;
		msg->priority = inPriority;
		msg->proc = inProc;
		msg->context = inContext;
		msg->object = inObject;
		msg->dataSize = inDataSize;
		if (inData) UMemory::Copy(msg->data, inData, inDataSize);
	}
	catch(...)
	{
		UMemory::Dispose((TPtr)msg);
		throw;
	}
	
	// link to end of queue
	SMessage *p = REF->list;
	if (p)
	{
		for(;;)
		{
			if (p->next == nil) break;
			p = p->next;
		}
		
		p->next = msg;
	}
	else
	{
		REF->list = msg;
	}
}

/*
 * If a message of the specified type, message proc, context, and object exists in the message
 * queue of <inSys>, its data and priority are set to the specified values.  Otherwise, a new
 * message is posted to the queue.  If <inObject> is nil, any object will match.
 */
void UMessageSys::Replace(TMessageSys inSys, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority, TMessageProc inProc, void *inContext, void *inObject)
{
	Require(inSys);
	SMessage *msg, *match, *prev;
	
	msg = REF->list;
	match = prev = nil;
	
	// find matching message
	if (inObject == nil)
	{
		// any object will match
		while (msg)
		{
			if (msg->type == inMsg && msg->proc == inProc && msg->context == inContext)
			{
				match = msg;
				break;
			}
			prev = msg;
			msg = msg->next;
		}
	}
	else
	{
		// match only specified object
		while (msg)
		{
			if (msg->type == inMsg && msg->proc == inProc && msg->context == inContext && msg->object == inObject)
			{
				match = msg;
				break;
			}
			prev = msg;
			msg = msg->next;
		}
	}
	
	// replace or post message
	if (match)
	{
		// replace existing message
		if (match->dataSize == inDataSize)
		{
			// old and new data are same size, so just overwrite
			match->priority = inPriority;
			match->object = inObject;
			if (inData) UMemory::Copy(match->data, inData, inDataSize);
		}
		else
		{
			// allocate new message
			msg = (SMessage *)UMemory::New(sizeof(SMessage) + inDataSize);
			msg->type = inMsg;
			msg->priority = inPriority;
			msg->proc = inProc;
			msg->context = inContext;
			msg->object = inObject;
			msg->dataSize = inDataSize;
			if (inData) UMemory::Copy(msg->data, inData, inDataSize);

			// replace old link with new
			if (prev)
				prev->next = msg;
			else
				REF->list = msg;
			msg->next = match->next;

			// kill original message data
			UMemory::Dispose((TPtr)match);
		}
	}
	else
	{
		// no existing message, so post new
		Post(inSys, inMsg, inData, inDataSize, inPriority, inProc, inContext, inObject);
	}
}

/*
 * Flush() removes all message from the queue of <inSys> that are of the specified message
 * proc, context, and object.  If <inObject> is nil, any object will match.
 */
void UMessageSys::Flush(TMessageSys inSys, TMessageProc inProc, void *inContext, void *inObject)
{
	Require(inSys);
	SMessage *msg, *prev, *next;
	
	msg = REF->list;
	next = prev = nil;
	
	if (inObject == nil)
	{
		// any object will match
		while (msg)
		{
			next = msg->next;
			if (msg->proc == inProc && msg->context == inContext)
			{
				if (prev)
					prev->next = next;
				else
					REF->list = next;
				UMemory::Dispose((TPtr)msg);
			}
			else
				prev = msg;
			msg = next;
		}
	}
	else
	{
		// match only specified object
		while (msg)
		{
			next = msg->next;
			if (msg->proc == inProc && msg->context == inContext && msg->object == inObject)
			{
				if (prev)
					prev->next = next;
				else
					REF->list = next;
				UMemory::Dispose((TPtr)msg);
			}
			else
				prev = msg;
			msg = next;
		}
	}
}

bool UMessageSys::Peek(TMessageSys inSys, Uint32 inMsg, void *outData, Uint32& ioDataSize, TMessageProc inProc, void *inContext, void *inObject)
{
	Require(inSys);
	SMessage *msg, *match;
	Uint32 s;
	
	msg = REF->list;
	match = nil;
	
	// find matching message
	if (inObject == nil)
	{
		// any object will match
		while (msg)
		{
			if (msg->type == inMsg && msg->proc == inProc && msg->context == inContext)
			{
				match = msg;
				break;
			}
			msg = msg->next;
		}
	}
	else
	{
		// match only specified object
		while (msg)
		{
			if (msg->type == inMsg && msg->proc == inProc && msg->context == inContext && msg->object == inObject)
			{
				match = msg;
				break;
			}
			msg = msg->next;
		}
	}
	
	if (match)
	{
		s = match->dataSize;
		if (s > ioDataSize) s = ioDataSize;
		UMemory::Copy(outData, match->data, s);
		ioDataSize = s;
		return true;
	}
	
	return false;
}

// returns true if a message was processed
#pragma warn_possunwant off
bool UMessageSys::Execute(TMessageSys inSys, const Uint32 inTypeList[])
{
	Require(inSys);
	SMessage *msg, *topMsg, *topMsgPrev, *prev;
	Int32 priority;
	
	msg = topMsg = REF->list;
	prev = topMsgPrev = nil;
	priority = topMsg == nil ? min_Int32 : topMsg->priority;
	
	if (inTypeList == nil || inTypeList[0] == 0)
	{
		while (msg)
		{
			if (msg->priority > priority)
			{
				topMsgPrev = prev;
				topMsg = msg;
				priority = topMsg->priority;
			}
			prev = msg;
			msg = msg->next;
		}
	}
	else
	{
		const Uint32 *p;
		Uint32 n, t;
		
		topMsg = nil;
		priority = min_Int32;
		
		while (msg)
		{
			// skip this message if not in type-list
			p = inTypeList;
			t = msg->type;
			while (n = *p++)
			{
				if (t == n)		// if this message is of a valid type
				{
					// then we'll consider it in our search for the highest priority message
					if (msg->priority > priority)
					{
						topMsgPrev = prev;
						topMsg = msg;
						priority = topMsg->priority;
					}
					break;
				}
			}
			
			prev = msg;
			msg = msg->next;
		}
	}
	
	if (topMsg)
	{
		// unlink from queue
		if (topMsgPrev)
			topMsgPrev->next = topMsg->next;
		else
			REF->list = topMsg->next;
		
		// determine proc and context
		TMessageProc proc = topMsg->proc;
		void *context = topMsg->context;
		if (proc == nil)
		{
			proc = REF->defaultProc;
			context = REF->defaultContext;
		}
		
		// call message proc
		if (proc)
		{
			try
			{
				proc(context, topMsg->object, topMsg->type, topMsg->data, topMsg->dataSize);
			}
			catch(SError& err)
			{	// avoid infinite loop of never-ending error messages
				if (topMsg->type != msg_Error)	
				{
					try 
					{ 
						Post(inSys, msg_Error, 
							 &err, sizeof(err), 
							 priority_Draw-1); 
					} 
					catch(...) 
					{}
				}
				// don't throw
			}
			catch(...)
			{
				UDebug::LogToDebugFile("Unknown exception caught");
				UDebug::Break();
			}
		}
		
		UMemory::Dispose((TPtr)topMsg);
		return true;
	}
	
	return false;
}
#pragma warn_possunwant reset






