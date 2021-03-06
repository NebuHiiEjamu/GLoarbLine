/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "GrafTypes.h"

struct SKeyMsgData {
	Uint32 time;				// time key event occured (milliseconds)
	Uint16 keyChar;				// key character
	Uint16 keyCode;				// key code
	Uint16 mods;				// modifier keys
};

struct SMouseMsgData {
	Uint32 time;				// time mouse event occured (milliseconds)
	SPoint loc;					// mouse location
	Uint16 button;				// mouse button
	Uint16 mods;				// modifier keys
	Uint32 misc;
};

struct SKeyCmd {
	Uint32 id;
	Uint16 key;
	Uint16 mods;
};

typedef void (*TMessageProc)(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
typedef Int32 (*TComparePtrsProc)(void *inPtrA, void *inPtrB, void *inRef);


// modifier key bits
enum {
	modKey_Control		= 0x01,	// control modifier key
	modKey_Option		= 0x02,	// option modifier key
	modKey_CapsLock		= 0x04,	// caps lock modifier key
	modKey_Shift		= 0x08,	// shift modifier key
	modKey_Command		= 0x10,	// command modifier key
	modKey_Alt			= modKey_Option,
	kIsDoubleClick		= 0x2000,
	kIsTripleClick		= 0x4000,
	kIsKeyCode			= 0x8000,
#if WIN32
	modKey_Shortcut		= modKey_Control
#else
	modKey_Shortcut		= modKey_Command
#endif
};

// queue priorities
enum {
	priority_Higher		= 20,	// extra high priority
	priority_High		= 10,	// high priority
	priority_Normal		= 0,	// normal priority
	priority_Low		= -10,	// low priority
	priority_Lower		= -20	// extra low priority
};

// messages
enum {
	msg_Error			= 1,	// an error has occured (data is SError)
	msg_Timer			= 2,	// timer has gone off
	msg_Quit			= 3,	// request application quit
	msg_Draw			= 4,	// object needs to be drawn (data is update rect)

	msg_KeyDown			= 5,	// key has been pressed (data is SKeyMsgData)
	msg_KeyUp			= 6,	// key has been released (data is SKeyMsgData)
	msg_KeyRepeat		= 7,	// key is being held down (data is SKeyMsgData)
	msg_KeyCommand		= 8,	// key command/shortcut 

	msg_MouseDown		= 9,	// mouse button pressed (data is SMouseMsgData)
	msg_MouseUp			= 10,	// mouse button released (data is SMouseMsgData)
	msg_MouseEnter		= 11,	// mouse has entered area (data is SMouseMsgData)
	msg_MouseLeave		= 12,	// mouse has left area (data is SMouseMsgData)
	msg_MouseMove		= 13,	// mouse has moved within area (data is SMouseMsgData)
	
	msg_DragBegin		= 14,	// drag operation has begun (data is SDragMsgData)
	msg_DragEnter		= 15,	// drag has entered area (data is SDragMsgData)
	msg_DragMove		= 16,	// drag has moved within area (data is SDragMsgData)
	msg_DragLeave		= 17,	// drag has left area (data is SDragMsgData)
	msg_DragEnd			= 18,	// drag operation has ended (data is SDragMsgData)
	msg_DragDropped		= 19,	// drag has been dropped (data is SDragMsgData)
	
	msg_UserClose		= 20,	// user clicked close box
	msg_UserZoom		= 21,	// user clicked zoom box
	msg_BoundsChanged	= 22,	// bounds changed
	
	msg_Suspend			= 24,	// object is being suspended
	msg_Resume			= 25,	// object is being resumed
	msg_Activate		= 26,	// object is being activated
	msg_Deactivate		= 27,	// object is being deactivated
	msg_Focus			= 28,	// object is gaining keyboard focus
	msg_Unfocus			= 29,	// object is loosing keyboard focus
	
	msg_Hit				= 30,	// object has been hit
	msg_WindowHit		= 31,	// window has been hit
	msg_HidePopupWindow	= 32,
	msg_OpenDocuments	= 33,
	msg_StateChanged	= 34,
	msg_ReleaseBuffer	= 35,
	msg_CaptureReleased = 36,
	msg_ShowPreferences	= 37,
	
	msg_AppInBackground	= 50,
	msg_AppInForeground = 51,

	msg_SendToQuickTime = 997,
	msg_OpenURL			= 998,
	
	msg_Special			= 1000	// your own messages must be 1000-2000
};

// message priorities
enum {
	priority_Timer		= priority_Lower,
	priority_Suspend	= priority_Normal,
	priority_Resume		= priority_Normal,
	priority_Quit		= priority_Normal,
	priority_MouseDown	= priority_High,
	priority_MouseUp	= priority_MouseDown,
	priority_MouseMove	= priority_MouseDown,
	priority_MouseEnter	= priority_MouseDown,
	priority_MouseLeave	= priority_MouseDown,
	priority_KeyDown	= priority_High,
	priority_KeyUp		= priority_KeyDown,
	priority_KeyRepeat	= priority_KeyDown,
	priority_Hit		= priority_Normal,
	priority_WindowHit	= priority_Hit,
	priority_Draw		= priority_MouseDown + 1,	// ensure changes get drawn before undone (eg, button)
	priority_Activate	= priority_Draw + 1,		// always activate before draw to avoid redundant drawing
	priority_Deactivate	= priority_Activate
};

// input/output access permissions
enum {								//	CALLER CAN		OTHERS CAN
	perm_None			= 0x0000,	//	-				read, write
	perm_NoneDenyRd		= 0x0010,	//	-				write
	perm_NoneDenyWr		= 0x0020,	//	-				read
	perm_NoneDenyRdWr	= 0x0030,	//	-				-
	perm_Rd				= 0x0001,	// 	read			read, write
	perm_RdDenyRd		= 0x0011,	//	read			write
	perm_RdDenyWr		= 0x0021,	//	read			read
	perm_RdDenyRdWr		= 0x0031,	//	read			-
	perm_Wr				= 0x0002,	//	write			read, write
	perm_WrDenyRd		= 0x0012,	//	write			write
	perm_WrDenyWr		= 0x0022,	//	write			read
	perm_WrDenyRdWr		= 0x0032,	//	write			-
	perm_RdWr			= 0x0003,	//	read, write		read, write
	perm_RdWrDenyRd		= 0x0013,	//	read, write		write
	perm_RdWrDenyWr		= 0x0023,	//	read, write		read
	perm_RdWrDenyRdWr	= 0x0033,	//	read, write		-
	
	perm_Read			= perm_RdDenyWr,
	perm_ReadWrite		= perm_RdWrDenyWr
};

// input/output callback functions
typedef Uint32 (*TIOReadProc)(void *inRef, Uint32 inOffset, void *outData, Uint32 inMaxSize);
typedef Uint32 (*TIOWriteProc)(void *inRef, Uint32 inOffset, const void *inData, Uint32 inDataSize);
typedef Uint32 (*TIOGetSizeProc)(void *inRef);
typedef void (*TIOSetSizeProc)(void *inRef, Uint32 inSize);
typedef Uint32 (*TIOControlProc)(void *inRef, Uint32 inCmd, void *inParamA, void *inParamB);


