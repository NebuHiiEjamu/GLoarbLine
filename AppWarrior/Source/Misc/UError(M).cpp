#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UError.h"
#include "UText.h"
#include "UDebug.h"
#include "UMemory.h"

#include <LowMem.h>
#include <Dialogs.h>
#include <SegLoad.h>
#include <Sound.h>

bool _APIsInForeground();
extern Uint8 _WNCheckWinStatesFlag;
extern Uint8 _APInBkgndFlag;

/*
 * Compile-time Options
 */

#define USE_EMSG	1

/*
 * Global Variables
 */

static DialogPtr gErrorDialog = nil;

/* -------------------------------------------------------------------------- */

// it is optional as to whether you call this
void UError::Init()
{
	if (gErrorDialog == nil)
	{
	/*
		const short kErrorDialogItemListData[] = {
			0x0003,0x0000,0x0000,0x006E,0x010E,0x0082,0x014E,0x0402,0x4F4B,0x0000,0x0000,
			0x000A,0x0038,0x005C,0x014E,0x8802,0x5E30,0x0000,0x0000,0x000A,0x000A,0x002A,
			0x002A,0xA002,0x0002,0x0000,0x0000,0x0082,0x0002,0x0092,0x0161,0x8802,0x5E31
		};
		const Rect kErrorDialogBounds = { 92, 78, 239, 429 };
	*/

		gErrorDialog = GetNewDialog(9437, nil, (WindowPtr)-1);
		if (gErrorDialog == nil)
		{
			DebugBreak("UError - couldn't create error dialog (fatal)");
			SysBeep(20);
			SysBeep(20);
			ExitToShell();
		}
	}
}

Uint32 UError::GetMessage(const SError& inError, void *outText, Uint32 inMaxSize)
{
	const Uint8 *kUnknownErrorMsg = "\pAn unknown error has occured.";
	const Uint8 *kNotEnoughMemMsg = "\pNot enough memory.  Try closing windows.";
	
	Uint32 s = 0;
	Handle h;

	Uint8 saveResLoad = LMGetResLoad();
	::SetResLoad(true);
	h = ::GetResource('EMSG', inError.type);
	LMSetResLoad(saveResLoad);
	
	if (h)
	{
		char saveState = ::HGetState(h);
		::HLock(h);
		try {
			s = UIDVarArray::GetItem(*h, ::GetHandleSize(h), inError.id, outText, inMaxSize);
		} catch(...) {}
		::HSetState(h, saveState);
	}
	
	if (s == 0)
	{
		const Uint8 *tp;
		
		if (inError.type == errorType_Memory && inError.id == memError_NotEnough)
			tp = kNotEnoughMemMsg;
		else
			tp = kUnknownErrorMsg;

		s = tp[0];
		if (s > inMaxSize) s = inMaxSize;
		::BlockMoveData(tp+1, outText, s);
	}
	
	return s;
}

void UError::Alert(const SError& inError)
{
	if (inError.silent) return;
	
	try
	{
		Uint8 errStr[256];
		Uint8 whereStr[256];
		Int16 iType;
		Handle iHandle;
		Rect iRect;
		
		Init();
				
		errStr[0] = GetMessage(inError, errStr+1, sizeof(errStr)-1);
		whereStr[0] = GetDetailMessage(inError, whereStr+2, sizeof(whereStr)-3) + 2;
		whereStr[1] = '(';
		whereStr[whereStr[0]] = ')';
		
		/*
		 * Because parts of the toolbox don't error check very well (%$#@*&!)
		 * the following dialog manager calls will most likely crash if we
		 * don't have enough memory (at least in MacOS 7.x).  Thus, we're
		 * going to make sure we have enough memory.  If not, just beep
		 * instead.
		 */
		if (!UMemory::IsAvailable(1024))
		{
			DebugBreak("UError - not enough memory to display error alert");
			SysBeep(20);
			return;
		}
		
		GetDialogItem(gErrorDialog, 2, &iType, &iHandle, &iRect);
		SetDialogItemText(iHandle, errStr);
		
		GetDialogItem(gErrorDialog, 4, &iType, &iHandle, &iRect);
		SetDialogItemText(iHandle, whereStr);
		
	#if TARGET_API_MAC_CARBON	
		SelectWindow(GetDialogWindow(gErrorDialog));
		ShowWindow(GetDialogWindow(gErrorDialog));
	#else
		SelectWindow(gErrorDialog);
		ShowWindow(gErrorDialog);
	#endif
	
		ModalDialog(nil, &iType);
		
	#if TARGET_API_MAC_CARBON
		HideWindow(GetDialogWindow(gErrorDialog));
	#else
		HideWindow(gErrorDialog);
	#endif
		
		// app can come from back to front while modal dialog is up
		_APInBkgndFlag = !_APIsInForeground();
		_WNCheckWinStatesFlag = true;
	}
	catch(...)
	{
		DebugBreak("UError - an error occured while attempting to display an error alert (!)");
		// don't throw to avert possible infinite loop
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

#include "UMemory.h"
#include "UFileSys.h"

enum {
	errorType_Resource					= 5,
	resError_Unknown					= 100,
	resError_ResFileCorrupt				= resError_Unknown + 1,
	resError_NoSuchResource				= resError_Unknown + 2,
	resError_AddFailed					= resError_Unknown + 3,
	resError_RemoveFailed				= resError_Unknown + 4
};

typedef struct {
	Int16 mac, type, id;
} SErrorEntry;

static const SErrorEntry gErrorTable[] = {
	{ -5039,	errorType_FileSys,		fsError_Unknown					},
	{ -5038,	errorType_FileSys,		fsError_SameItem				},
	{ -5037,	errorType_FileSys,		fsError_Unknown					},
	{ -5036,	errorType_FileSys,		fsError_Unknown					},
	{ -5035,	errorType_FileSys,		fsError_Unknown					},
	{ -5034,	errorType_FileSys,		fsError_Unknown					},
	{ -5033,	errorType_FileSys,		fsError_Unknown					},
	{ -5032,	errorType_FileSys,		fsError_ItemLocked				},
	{ -5031,	errorType_FileSys,		fsError_DiskLocked				},
	{ -5030,	errorType_FileSys,		fsError_Unknown					},
	{ -5029,	errorType_FileSys,		fsError_NoSuchFolder			},
	{ -5028,	errorType_FileSys,		fsError_Unknown					},
	{ -5027,	errorType_FileSys,		fsError_DiskDisappeared			},
	{ -5026,	errorType_FileSys,		fsError_Unknown					},
	{ -5025,	errorType_FileSys,		fsError_Unknown					},
	{ -5024,	errorType_FileSys,		fsError_OperationNotSupported	},
	{ -5023,	errorType_FileSys,		fsError_UserNotAuth				},
	{ -5022,	errorType_FileSys,		fsError_Unknown					},
	{ -5021,	errorType_FileSys,		fsError_Unknown					},
	{ -5020,	errorType_FileSys,		fsError_Unknown					},
	{ -5019,	errorType_Misc,			error_Param						},
	{ -5018,	errorType_FileSys,		fsError_NoSuchItem				},
	{ -5017,	errorType_FileSys,		fsError_ItemAlreadyExists		},
	{ -5016,	errorType_FileSys,		fsError_Unknown					},
	{ -5015,	errorType_FileSys,		fsError_Unknown					},
	{ -5014,	errorType_FileSys,		fsError_Unknown					},
	{ -5013,	errorType_FileSys,		fsError_Unknown					},
	{ -5012,	errorType_FileSys,		fsError_NoSuchItem				},
	{ -5011,	errorType_FileSys,		fsError_OperationNotSupported	},
	{ -5010,	errorType_FileSys,		fsError_FileInUse				},
	{ -5009,	errorType_FileSys,		fsError_EndOfFile				},
	{ -5008,	errorType_FileSys,		fsError_NotEnoughSpace			},
	{ -5007,	errorType_FileSys,		fsError_Unknown					},
	{ -5006,	errorType_FileSys,		fsError_Unknown					},
	{ -5005,	errorType_FileSys,		fsError_Unknown					},
	{ -5004,	errorType_FileSys,		fsError_Unknown					},
	{ -5003,	errorType_FileSys,		fsError_Unknown					},
	{ -5002,	errorType_FileSys,		fsError_Unknown					},
	{ -5001,	errorType_FileSys,		fsError_Unknown					},
	{ -5000,	errorType_FileSys,		fsError_AccessDenied			},
	{ -1310,	errorType_FileSys,		fsError_Unknown					},
	{ -1309,	errorType_FileSys,		fsError_Unknown					},
	{ -1308,	errorType_FileSys,		fsError_Unknown					},
	{ -1307,	errorType_FileSys,		fsError_Unknown					},
	{ -1306,	errorType_FileSys,		fsError_SameItem				},
	{ -1305,	errorType_FileSys,		fsError_DiskSoftwareDamaged		},
	{ -1304,	errorType_FileSys,		fsError_Unknown					},
	{ -1303,	errorType_FileSys,		fsError_Unknown					},
	{ -1302,	errorType_FileSys,		fsError_FileExpected			},
	{ -1301,	errorType_FileSys,		fsError_Unknown					},
	{ -1300,	errorType_FileSys,		fsError_Unknown					},
	{ -619,		errorType_Misc,			error_Protocol					},
	{ -199,		errorType_Resource,		resError_ResFileCorrupt			},
	{ -198,		errorType_Misc,			error_Protocol					},
	{ -197,		errorType_Resource,		resError_RemoveFailed			},
	{ -196,		errorType_Resource,		resError_RemoveFailed			},
	{ -195,		errorType_Resource,		resError_AddFailed				},
	{ -194,		errorType_Resource,		resError_AddFailed				},
	{ -193,		errorType_FileSys,		fsError_NoSuchFile				},
	{ -192,		errorType_Resource,		resError_NoSuchResource			},
	{ -186,		errorType_Resource,		resError_Unknown				},
	{ -185,		errorType_Resource,		resError_Unknown				},
	{ -151,		errorType_Memory,		memError_NotEnough				},
	{ -124,		errorType_FileSys,		fsError_DiskDisappeared			},
	{ -123,		errorType_FileSys,		fsError_OperationNotSupported	},
	{ -122,		errorType_FileSys,		fsError_BadMove					},
	{ -121,		errorType_FileSys,		fsError_Unknown					},
	{ -120,		errorType_FileSys,		fsError_NoSuchFolder			},
	{ -117,		errorType_Memory,		memError_Unknown				},
	{ -116,		errorType_Memory,		memError_BlockInvalid			},
	{ -115,		errorType_Memory,		memError_BlockInvalid			},
	{ -114,		errorType_Memory,		memError_BlockInvalid			},
	{ -113,		errorType_Memory,		memError_BlockInvalid			},
	{ -112,		errorType_Misc,			error_Param						},
	{ -111,		errorType_Memory,		memError_BlockInvalid			},
	{ -110,		errorType_Memory,		memError_BlockInvalid			},
	{ -109,		errorType_Memory,		memError_BlockInvalid			},
	{ -108,		errorType_Memory,		memError_NotEnough				},
	{ -99,		errorType_Memory,		memError_Unknown				},
	{ -61,		errorType_FileSys,		fsError_ChangeDenied			},
	{ -60,		errorType_FileSys,		fsError_DiskSoftwareDamaged		},
	{ -59,		errorType_FileSys,		fsError_Unknown					},
	{ -58,		errorType_FileSys,		fsError_Unknown					},
	{ -57,		errorType_FileSys,		fsError_DiskFormatUnknown		},
	{ -56,		errorType_FileSys,		fsError_Unknown					},
	{ -55,		errorType_FileSys,		fsError_Unknown					},
	{ -54,		errorType_FileSys,		fsError_FileInUse				},
	{ -53,		errorType_FileSys,		fsError_DiskOffline				},
	{ -52,		errorType_FileSys,		fsError_Unknown					},
	{ -51,		errorType_FileSys,		fsError_Unknown					},
	{ -50,		errorType_Misc,			error_Param						},
	{ -49,		errorType_FileSys,		fsError_FileInUse				},		
	{ -48,		errorType_FileSys,		fsError_ItemAlreadyExists		},
	{ -47,		errorType_FileSys,		fsError_FileInUse				},
	{ -46,		errorType_FileSys,		fsError_DiskLocked				},
	{ -45,		errorType_FileSys,		fsError_FileLocked				},
	{ -44,		errorType_FileSys,		fsError_DiskLocked				},
	{ -43,		errorType_FileSys,		fsError_NoSuchFile				},
	{ -42,		errorType_FileSys,		fsError_Unknown					},
	{ -41,		errorType_Memory,		memError_NotEnough				},
	{ -40,		errorType_Misc,			error_Param						},
	{ -39, 		errorType_FileSys,		fsError_EndOfFile				},
	{ -38,		errorType_FileSys,		fsError_Unknown					},
	{ -37,		errorType_FileSys, 		fsError_BadName					},
	{ -36,		errorType_FileSys,		fsError_Unknown					},
	{ -35,		errorType_FileSys, 		fsError_NoSuchDisk				},
	{ -34,		errorType_FileSys,		fsError_NotEnoughSpace			},
	{ -33,		errorType_FileSys,		fsError_Unknown					}
};

/*
 * Given a macintosh error code, _ConvertMacError() looks up the error in a
 * sorted table using a fast binary search, and sets <outType> and <outID>
 * to the appropriate cross-platform error type and ID.  If <inMacError>
 * is not found in the table, <outType> gets set to "errorType_Misc" and
 * <outID> gets set to "error_Unknown".
 */
void _ConvertMacError(Int16 inMacError, Int16& outType, Int16& outID)
{
	Int32 lowBound	= 1;
	Int32 highBound	= sizeof(gErrorTable) / sizeof(SErrorEntry);
	Int32 i;
	const SErrorEntry *table = gErrorTable;
	const SErrorEntry *entry;
	
	do
	{
		i = (lowBound + highBound) >> 1;		// same as:  (lowBound + highBound) / 2
		entry = table + i - 1;
		
		if (inMacError == entry->mac)
		{
			outType = entry->type;
			outID = entry->id;
			return;
		}
		else if (inMacError < entry->mac)
			highBound = i - 1;
		else
			lowBound = i + 1;
	
	} while (lowBound <= highBound);
	
	outType = errorType_Misc;
	outID = error_Unknown;
}

Int16 _ConvertMacError(const SError& inErr)
{
	Int16 type = inErr.type;
	Int16 id = inErr.id;
	const SErrorEntry *entry, *endEntry;
	
	entry = gErrorTable;
	endEntry = entry + (sizeof(gErrorTable) / sizeof(SErrorEntry));
	
	while (entry < endEntry)
	{
		if (entry->id == id && entry->type == type)
			return entry->mac;
		
		entry++;
	}
	
	return paramErr;
}

void _FailMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine)
{
	SError err;
	
	err.file = inFile;
	err.line = inLine;
	err.fatal = err.silent = false;
	err.special = inMacError;
	
	_ConvertMacError(inMacError, err.type, err.id);
	
	throw err;
}






#endif
