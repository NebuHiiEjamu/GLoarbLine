/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "MoreTypes.h"
#include "UError.h"
#include "UDragAndDrop.h"
#include "CWindow.h"
#include "UApplication.h"
#include "UTimer.h"

class CApplication
{
	public:
		// construction
		CApplication();
		virtual ~CApplication();
		
		// loop
		virtual void Run();
		virtual void UserQuit();
		virtual void Quit();
		
		// events
		virtual void KeyCommand(Uint32 inCmd, const SKeyMsgData& inInfo);
		virtual void Error(const SError& inInfo);
		virtual void Timer(TTimer inTimer);
		virtual void OpenDocuments();
		virtual void OpenDocument(TFSRefObj* inRef);
		virtual void ShowPreferences();
		
		// window hits
		virtual void WindowHit(CWindow *inWindow, const SHitMsgData& inInfo);
		static void WindowHitHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
				
		// inline helpers
		TTimer NewTimer()														{	return UTimer::New(MessageHandler, this);									}
		TTimer StartNewTimer(Uint32 inMillisecs, Uint32 inIsRepeating = false)	{	return UTimer::StartNew(MessageHandler, this, inMillisecs, inIsRepeating);	}
		void InstallKeyCommands(const SKeyCmd *inCmds, Uint32 inCount)			{	UWindow::InstallKeyCommands(inCmds, inCount, MessageHandler, this);			}
		
	protected:
		// messages
		virtual void HandleMessage(void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
		static void MessageHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
};

extern CApplication *gApplication;


