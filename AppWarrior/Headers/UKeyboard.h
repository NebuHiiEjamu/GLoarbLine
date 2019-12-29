/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "MoreTypes.h"

class UKeyboard
{
	public:
		// down status
		static bool IsKeyDown(Uint16 inKeyCode);
		static bool IsCommandKeyDown();
		static bool IsOptionKeyDown();
		static bool IsShiftKeyDown();
		static bool IsControlKeyDown();
		static Uint16 GetModifiers();
		
		// key types
		static bool IsFunctionKey(Uint16 inKeyCode);
		static bool IsModifierKey(Uint16 inKeyCode);
		
		// misc
		static Uint16 KeyToChar(Uint16 inKeyCode, Uint16 inMods, Uint32 inEncoding = 0);
		static Uint32 FindKeyCommand(const SKeyMsgData& inInfo, const SKeyCmd *inCmds, Uint32 inCount);
};

// key codes (as opposed to character codes)
enum {
	// numeric keypad
	key_nZero			= 0x10,
	key_nOne			= 0x11,
	key_nTwo			= 0x12,
	key_nThree			= 0x13,
	key_nFour			= 0x14,
	key_nFive			= 0x15,
	key_nSix			= 0x16,
	key_nSeven			= 0x17,
	key_nEight			= 0x18,
	key_nNine			= 0x19,
	key_nClear			= 0x1A,
	key_nEquals			= 0x1B,
	key_nDivide			= 0x1C,
	key_nMultiply		= 0x1D,
	key_nTimes			= 0x1D,
	key_nSubtract		= 0x1E,
	key_nMinus			= 0x1E,
	key_nAdd			= 0x1F,
	key_nPlus			= 0x1F,
	key_nEnter			= 0x20,
	key_nPoint			= 0x21,

	// number keys
	key_Zero			= 0x30,		// '0'
	key_One				= 0x31,		// '1'
	key_Two				= 0x32,		// '2'
	key_Three			= 0x33,		// '3'
	key_Four			= 0x34,		// '4'
	key_Five			= 0x35,		// '5'
	key_Six				= 0x36,		// '6'
	key_Seven			= 0x37,		// '7'
	key_Eight			= 0x38,		// '8'
	key_Nine			= 0x39,		// '9'
	
	// alphabetic keys
	key_A				= 0x41,		// 'A'
	key_B				= 0x42,		// 'B'
	key_C				= 0x43,		// 'C'
	key_D				= 0x44,		// 'D'
	key_E				= 0x45,		// 'E'
	key_F				= 0x46,		// 'F'
	key_G				= 0x47,		// 'G'
	key_H				= 0x48,		// 'H'
	key_I				= 0x49,		// 'I'
	key_J				= 0x4A,		// 'J'
	key_K				= 0x4B,		// 'K'
	key_L				= 0x4C,		// 'L'
	key_M				= 0x4D,		// 'M'
	key_N				= 0x4E,		// 'N'
	key_O				= 0x4F,		// 'O'
	key_P				= 0x50,		// 'P'
	key_Q				= 0x51,		// 'Q'
	key_R				= 0x52,		// 'R'
	key_S				= 0x53,		// 'S'
	key_T				= 0x54,		// 'T'
	key_U				= 0x55,		// 'U'
	key_V				= 0x56,		// 'V'
	key_W				= 0x57,		// 'W'
	key_X				= 0x58,		// 'X'
	key_Y				= 0x59,		// 'Y'
	key_Z				= 0x5A,		// 'Z'
	
	// punctuation keys
	key_Hyphen			= 0x60,
	key_Equals			= 0x61,
	key_OpenBracket		= 0x62,
	key_CloseBracket	= 0x63,
	key_Backslash		= 0x64,
	key_Semicolon		= 0x65,
	key_Quote			= 0x66,
	key_Comma			= 0x67,
	key_Period			= 0x68,
	key_Slash			= 0x69,
	key_Tilde			= 0x6A,

	// space keys
	key_Tab				= 0x70,
	key_Return			= 0x71,
	key_Enter			= 0x71,
	key_Space			= 0x72,
	key_SpaceBar		= 0x72,

	// navigation keys
	key_Home			= 0x80,
	key_End				= 0x81,
	key_PageUp			= 0x82,
	key_PageDown		= 0x83,
	key_Up				= 0x84,
	key_Down			= 0x85,
	key_Left			= 0x86,
	key_Right			= 0x87,
	
	// control keys
	key_Escape			= 0x90,
	key_Delete			= 0x91,
	key_Backspace		= 0x91,
	key_Help			= 0x92,
	key_Insert			= 0x92,
	key_Del				= 0x93,
	key_ForwardDelete	= 0x93,
	
	// modifier keys
	key_CapsLock		= 0xA0,
	key_LeftShift		= 0xA1,
	key_RightShift		= 0xA2,
	key_LeftControl		= 0xA3,
	key_RightControl	= 0xA4,
	key_LeftOption		= 0xA5,
	key_LeftAlt			= 0xA5,
	key_RightOption		= 0xA6,
	key_RightAlt		= 0xA6,
	key_LeftCommand		= 0xA7,
	key_RightCommand	= 0xA8,
	
	// function keys
	key_F1				= 0xB0,
	key_F2				= 0xB1,
	key_F3				= 0xB2,
	key_F4				= 0xB3,
	key_F5				= 0xB4,
	key_F6				= 0xB5,
	key_F7				= 0xB6,
	key_F8				= 0xB7,
	key_F9				= 0xB8,
	key_F10				= 0xB9,
	key_F11				= 0xBA,
	key_F12				= 0xBB,
	key_F13				= 0xBC,		// Print Screen
	key_F14				= 0xBD,		// Scroll Lock
	key_F15				= 0xBE,		// Pause
	key_F16				= 0xBF,
	key_F17				= 0xC0,
	key_F18				= 0xC1,
	key_F19				= 0xC2,
	key_F20				= 0xC3,
	key_F21				= 0xC4,
	key_F22				= 0xC5,
	key_F23				= 0xC6,
	key_F24				= 0xC7
};

