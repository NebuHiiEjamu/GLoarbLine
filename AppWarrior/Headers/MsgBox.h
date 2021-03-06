/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CWindow.h"

enum {
	icon_Stop		= 1000,
	icon_Note		= 1001,
	icon_Caution	= 1002,
	icon_Question	= 1003
};

#pragma options align=packed
struct SMsgBox {
	Int16 icon;
	Int16 picture;
	Int16 sound;
	Int16 textStyle;
	Int16 btnTextStyle;
	Int16 script;
	const Uint8 *title;
	const void *messageData;
	Uint16 messageSize;
	Uint32 fontSize;
	bool showTextView;
	const Uint8 *button1;
	const Uint8 *button2;
	const Uint8 *button3;
};
#pragma options align=reset

Uint16 MsgBox(Int32 inID, ...);
Uint16 MsgBox(const SMsgBox& inInfo);

CWindow *MakeMsgBox(const SMsgBox& inInfo);
CWindow *MakeMsgBox(const Uint8 *inTitle, const Uint8 *inMsg, Int16 inIcon = 0, Int16 inSound = 0, bool inShowTextView = false);
CWindow *MakeMsgBox(const Uint8 *inTitle, const void *inMsg, Uint16 inMsgSize, Uint32 inFontSize, Int16 inIcon = 0, Int16 inSound = 0, bool inShowTextView = false);

CWindow *MakeMsgBoxWin(Int32 inID, Int32 *outSound = nil, void *inArgs = nil);
CWindow *MakeMsgBoxWin(const SMsgBox& inInfo);
Uint16 TrackMsgBoxWin(CWindow *inWin);

void DisplayMessage(const Uint8 *inTitle, const Uint8 *inMsg, Int16 inIcon = 0, Int16 inSound = 0, bool inShowTextView = false);
void DisplayMessage(const Uint8 *inTitle, const void *inMsg, Uint16 inMsgSize, Uint32 inFontSize, Int16 inIcon = 0, Int16 inSound = 0, bool inShowTextView = false);
