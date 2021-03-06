/***********************************************************************************

The inOptions and outType on New() should probably go.

inOptions is bad because you shouldn't be checking at that stage.  For example,
say you want to create a file.  So you use fsOption_FailIfExists, and then
call CreateFile().  What if the file becomes existant between the option
check and the CreateFile() call!!  And this _can_ happen on a pre-emptively
multitasked system.

So, it would be better if New() doesn't do any checks, and you simply catch()
any errors on CreateFile(), as you should be doing anyway.

Go thru Hotline client and server  etc and remove as much usage of the options
as possible - to ensure there won't be problems removing the options from
New().

Maybe the only option needed is fsOption_ResolveAliases.

Also >>> need an encoding parameter for New().

************************************************************************************

Get rid of the type and creator codes!!

They should be STRINGS!  MIME style strings, and for compatibility you
just support

"mactypecode/TTTTCCCC"


************************************************************************************/



#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UFileSys.h"

// include <Folders.h> and work around conflict
//#define kRootFolder GETLOST3453342
#include <Folders.h>
//#undef kRootFolder

extern "C" {
#include "FullPath.h"
#include "FileCopy.h"
#include "MoreFilesExtras.h"
#include "MoreFiles.h"
#include "MoreDesktopMgr.h"
#include "Navigation.h"
}

Int16 _GetSystemVersion();
bool _APIsInForeground();
extern Uint8 _WNCheckWinStatesFlag;
extern Uint8 _APInBkgndFlag;

//#define REF		((SFileSysRef *)inRef)


struct SFileSysRef {
	FSSpec spec;		// must be first item
	TPtr flatData, unflatData;
	Int16 refNum, resRefNum;
};

struct SFileRecordHook {
	StandardFileReply *pFileReplay;
	FSSpec *pFolderSpec;
};

#if TARGET_API_MAC_CARBON
static pascal void _FileEventProc(NavEventCallbackMessage inCallBackSelector, NavCBRecPtr inCallBackParams, NavCallBackUserData inCallBackUD);
#else
static pascal short _FileDialogHook(short inItem, DialogPtr inDialog, void *inUserData);
#endif

static void _FSUpdateSpec(SFileSysRef *inRef);
static Int32 _FSGetDirID(Int16 vRefNum, Int32 dirID, const Uint8 *name, bool *outIsDir);
static Int16 _FSGetVRefNum(const Uint8 *pathname, Int16 vRefNum);
static Uint32 _FSRead(Int16 inRefNum, Uint32 inOffset, void *outData, Uint32 inMaxSize);
static Uint32 _FSWrite(Int16 inRefNum, Uint32 inOffset, const void *inData, Uint32 inDataSize);
static bool _FSMakeSpec(const FSSpec *inStartFolder, const Uint8 *inPathString, const void *inPathData, Uint32 inPathDataSize, const Uint8 *inName, FSSpec& outSpec);
static void _FSStripName(Uint8 *ioName);
static void _FSStripName(void *ioName, Uint32 inSize);
static OSErr _FSResolveAliasFile(FSSpec *ioSpec, Boolean *outIsFolder, Boolean *outWasAliased);
static bool _FSResolveAliasAndGetType(const FSSpec *inSpec, FSSpec *outResolvedSpec, Uint16 *outOrigType);
static OSErr _FSIsAliasFile(const FSSpec *inSpec, Boolean *outIsAlias, Boolean *outIsFolder);
static bool _FSDoNewOptions(FSSpec *ioSpec, Uint32 inOptions, Uint16 *outType);
TFSRefObj* _PathToRef(const void *inPath, Uint32 inPathSize);
void _MacSecondsToStamp(Uint32 inSecs, SDateTimeStamp& outStamp);
Uint32 _StampToMacSeconds(const SDateTimeStamp& inStamp);
static void _FSCreateAliasFile(const FSSpec *inFile, const FSSpec *inOriginal);
static void _FSForceFinderUpdate(const FSSpec *inFolder);
static bool _FSHasNavServices();


// set this to false to stop user interface being shown in the process of resolving an alias
bool _FSAliasUI = true;

// mac error handling
void _FailMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine);
inline void _CheckMacError(Int16 inMacError, const Int8 *inFile, Uint32 inLine) { if (inMacError) _FailMacError(inMacError, inFile, inLine); }
#if DEBUG
	#define FailMacError(id)		_FailMacError(id, __FILE__, __LINE__)
	#define CheckMacError(id)		_CheckMacError(id, __FILE__, __LINE__)
#else
	#define FailMacError(id)		_FailMacError(id, nil, 0)
	#define CheckMacError(id)		_CheckMacError(id, nil, 0)
#endif

/* -------------------------------------------------------------------------- */

/*
UFileSys::New() is a Turbo-Studly(TM) function for creating a reference to a file
system object.  It follows the specified path from the specified starting folder,
and creates a reference for an item with the specified name.

The starting folder must always be supplied, but can be one of the special constants.
If the name and path are nil or empty, the new ref is of the starting folder.
If the path is nil or empty, the item is inside the starting folder.
If the name is nil or empty, the item is the last item in the path.

By default (if inOptions is zero), New() creates a reference for a file or folder or
alias regardless of whether that item actually exists or not, and does not resolve
aliases (except in the path).  If the starting folder or path does not exist, an
exception is thrown.  Behaviour can be modified by using these options:

fsOption_NilIfNonExistantParent - if set, New() will return nil (instead of throwing
an exception) if the starting folder or path does not exist.  If so, the item doesn't
exist either, and the type will be fsItemType_NonExistantParent.

fsOption_NilIfNonExistant - if set, New() will return nil (instead of a valid TFSRefObj*)
if the specified item or starting folder or path does not exist.  Will also return
nil if an alias cannot be resolved.

fsOption_NilIfExists - if set, New() will return nil if the specified item (be it
file, folder or alias) exists.

fsOption_NilIfNotFile - if set, New() will return nil if the item exists and is not a
file.  Aliases do not count as files.

fsOption_NilIfNotFolder - if set, New() will return nil if the item exists and is not
a folder.  Aliases do not count as folders.

fsOption_FailIfNonExistant - if set, New() will throw an exception if the specified
item does not exist.  By default, the exception is fsError_NoSuchItem.  However, if
fsOption_FailIfNotFile/fsOption_NilIfNotFile is on, the exception will be
fsError_NoSuchFile.  Similarly, if fsOption_FailIfNotFolder/fsOption_NilIfNotFolder
is on, the exception will be fsError_NoSuchFolder.

fsOption_FailIfExists - if set, New() will throw a fsError_ItemAlreadyExists
exception if the specified item (be it file, folder or alias) exists.

fsOption_FailIfNotFile - if set, New() will throw a fsError_FileExpected exception
if the item exists and is not a file.  Aliases do not count as files.

fsOption_FailIfNotFolder - if set, New() will throw a fsError_FolderExpected exception
if the item exists and is not a folder.  Aliases do not count as folders.

fsOption_ResolveAliases - if set, New() will resolve the item if it is an alias.
If the alias cannot be resolved, an exception is thrown.

If supplied, outType is always set to is the type of the returned item (if resolving
aliases, type is of the resolved item).  outType is still set if the return value is
nil.
*/
TFSRefObj* UFileSys::New(TFSRefObj* inStartFolder, const Uint8 *inPath, const Uint8 *inName, Uint32 inOptions, Uint16 *outType)
{
	FSSpec spec;
	
	Require(inStartFolder);
	
	try
	{
		_FSMakeSpec((FSSpec *)inStartFolder, inPath, nil, 0, inName, spec);
	}
	catch(...)
	{
		// _FSMakeSpec fails if inStartFolder can't be found or path can't be followed
		if (inOptions & (fsOption_NilIfNonExistant | fsOption_NilIfNonExistantParent))	// if either option is on
		{
			if (outType) *outType = fsItemType_NonExistantParent;
			return nil;
		}
		throw;
	}
	
	if (_FSDoNewOptions(&spec, inOptions, outType))
		return nil;
	
	SFileSysRef *ref = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	ref->spec = spec;

	return (TFSRefObj*)ref;
}

TFSRefObj* UFileSys::New(TFSRefObj* inStartFolder, const void *inPath, Uint32 inPathSize, const Uint8 *inName, Uint32 inOptions, Uint16 *outType)
{
	FSSpec spec;
	
	Require(inStartFolder);
	if (inPathSize < 6) inPath = nil;
	
	try
	{
		_FSMakeSpec((FSSpec *)inStartFolder, nil, inPath, inPathSize, inName, spec);
	}
	catch(...)
	{
		// _FSMakeSpec fails if inStartFolder can't be found or path can't be followed
		if (inOptions & (fsOption_NilIfNonExistant | fsOption_NilIfNonExistantParent))	// if either option is on
		{
			if (outType) *outType = fsItemType_NonExistantParent;
			return nil;
		}
		throw;
	}
	
	if (_FSDoNewOptions(&spec, inOptions, outType))
		return nil;
	
	SFileSysRef *ref = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	ref->spec = spec;

	return (TFSRefObj*)ref;
}

// creates a TFSRefObj* pointing to a non-existant item in the temporary folder
TFSRefObj* UFileSys::NewTemp(TFSRefObj* /* inRef */)
{
	/**************************************
	
	There's a problem when you use NewTemp(), which creates a temp
	file on the system volume, and then you use SwapData() - if the
	main file is not on the same volume, you can't swap!
	
	So IF inRef != nil, the temp file should be created on the same
	volume as inRef.   It could be in the same dir and named something
	like "samename.tmp001".
	
	If inRef == nil, the code is as it is currently.
	
	**********************************************/

	static Uint16 tempFileCount = 0;
	CInfoPBRec pb;
	Uint8 appName[64];
	Uint8 nameStr[64];
	Int32 dirID;
	Int16 vRefNum;
	Uint8 *p;

	// get name of current application
	p = LMGetCurApName();
	::BlockMoveData(p, appName, p[0]+1);
	if (appName[0] > 10) appName[0] = 10;

	// find temp folder
	FindFolder(kOnSystemDisk, kTemporaryFolderType, true, &vRefNum, &dirID);
	
	pb.hFileInfo.ioFDirIndex = 0;			// name will never be zero-length
	pb.hFileInfo.ioVRefNum = vRefNum;
	pb.hFileInfo.ioNamePtr = (StringPtr)nameStr;
		
	for (;;)
	{
		tempFileCount++;
		nameStr[0] = UText::Format(nameStr+1, 31, "%#s-temp-%hu", appName, tempFileCount);
				
		pb.hFileInfo.ioDirID = dirID;		// gotta set this every time
		if (PBGetCatInfoSync(&pb)) break;
	}
	
	SFileSysRef *ref = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));

	ref->spec.vRefNum = vRefNum;
	ref->spec.parID = dirID;
	::BlockMoveData(nameStr, ref->spec.name, nameStr[0]+1);

	return (TFSRefObj*)ref;
}

// if the ref is open, the clone ref is not
TFSRefObj* UFileSys::Clone(TFSRefObj* inRef)
{
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	SFileSysRef *newRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	newRef->spec = ((SFileSysRef *)inRef)->spec;
	return (TFSRefObj*)newRef;
}

// closes file if open
void UFileSys::Dispose(TFSRefObj* inRef)
{
	if (inRef)
	{
		if (((SFileSysRef *)inRef)->refNum) FSClose(((SFileSysRef *)inRef)->refNum);
		if (((SFileSysRef *)inRef)->resRefNum) FSClose(((SFileSysRef *)inRef)->resRefNum);
		UMemory::Dispose(((SFileSysRef *)inRef)->flatData);
		UMemory::Dispose(((SFileSysRef *)inRef)->unflatData);
		UMemory::Dispose((TPtr)inRef);
	}
}

void UFileSys::SetRef(TFSRefObj* inDst, TFSRefObj* inSrc)
{
	Require(inDst && inSrc);
	
	if (((SFileSysRef *)inDst)->refNum == 0)	// if not open
		((SFileSysRef *)inDst)->spec = ((SFileSysRef *)inSrc)->spec;
}
SInt16 UFileSys::OpenResourceFork(TFSRefObj* inStartFolder,
								  const UInt8 *inPath, const Uint8 *inName,
							      SInt8 rw)
{
	FSSpec spec;
	_FSMakeSpec((FSSpec *)inStartFolder, inPath, nil,0, inName, spec);
	SInt16 cres = ::CurResFile();
	SInt16 h = ::FSpOpenResFile(&spec, rw); // permission 0
	::UseResFile(cres); // thank you stupid MAC programmers 
    return h;
}
							  


/* -------------------------------------------------------------------------- */
#pragma mark -

// fails if file already exists
void UFileSys::CreateFile(TFSRefObj* inRef, Uint32 inTypeCode, Uint32 inCreatorCode)
{
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, fsError_ItemAlreadyExists);
	CheckMacError(FSpCreate(&((SFileSysRef *)inRef)->spec, inCreatorCode, inTypeCode, 0));
}

void UFileSys::CreateFileAndOpen(TFSRefObj* inRef, Uint32 inTypeCode, Uint32 inCreatorCode, Uint32 inOptions, Uint32 inPerm)
{
	FSSpec *spec;
	Int16 refNum;
	OSErr err;
	
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, error_Protocol);
	spec = &((SFileSysRef *)inRef)->spec;
	
	if (inOptions == kAlwaysCreateFile)
	{
		err = ::FSpDelete(spec);
		if (err != noErr && err != fnfErr && err != dirNFErr && err != nsvErr) FailMacError(err);
		CheckMacError(::FSpCreate(spec, inCreatorCode, inTypeCode, 0));
		CheckMacError(::FSpOpenAware(spec, inPerm, &refNum));
	}
	else if (inOptions == kOpenExistingFile)
	{
		CheckMacError(::FSpOpenAware(spec, inPerm, &refNum));
	}
	else if (inOptions == kAlwaysOpenFile)
	{
		err = ::FSpOpenAware(spec, inPerm, &refNum);
		if (err)
		{
			if (err == fnfErr)
			{
				CheckMacError(::FSpCreate(spec, inCreatorCode, inTypeCode, 0));
				CheckMacError(::FSpOpenAware(spec, inPerm, &refNum));
			}
			else
				FailMacError(err);
		}
	}
	else	// kCreateNewFile
	{
		CheckMacError(::FSpCreate(spec, inCreatorCode, inTypeCode, 0));
		CheckMacError(::FSpOpenAware(spec, inPerm, &refNum));
	}
	
	((SFileSysRef *)inRef)->refNum = refNum;	// refNum might have been set even if error, so we're using a temp variable
}

// fails if folder already exists
void UFileSys::CreateFolder(TFSRefObj* inRef)
{
	Require(inRef);
	
	Int32 dirID;
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	CheckMacError(::FSpDirCreate(spec, 0, &dirID));
	
	spec->parID = dirID;
	spec->name[0] = 0;
}

void UFileSys::CreateAlias(TFSRefObj* inRef, TFSRefObj* inOriginal)
{
	Require(inRef && inOriginal);
	
	_FSCreateAliasFile((FSSpec *)inRef, (FSSpec *)inOriginal);
}

bool UFileSys::Exists(TFSRefObj* inRef, bool *outIsFolder)
{
	Require(inRef);
	CInfoPBRec pb;
	Str63 tempName;
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	OSErr err;

	if (((SFileSysRef *)inRef)->refNum)
	{
		if (outIsFolder) *outIsFolder = false;
		return true;
	}

	::BlockMoveData(spec->name, tempName, spec->name[0]+1);

	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = spec->vRefNum;
	pb.hFileInfo.ioDirID = spec->parID;
	
	err = ::PBGetCatInfoSync(&pb);
	if (err)
	{
		if (err == fnfErr || err == bdNamErr || err == dirNFErr)
		{
			if (outIsFolder) *outIsFolder = false;
			return false;
		}
		else
			FailMacError(err);
	}
	
	if (outIsFolder)
		*outIsFolder = (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0;
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// does nothing if the file doesn't exist
void UFileSys::DeleteFile(TFSRefObj* inRef)
{
	Require(inRef);
	
	// can't delete open file
	if (((SFileSysRef *)inRef)->refNum)
		Fail(errorType_FileSys, fsError_FileInUse);
	
	OSErr err = FSpDelete(&((SFileSysRef *)inRef)->spec);		// also deletes empty folders, but probably won't matter
	
	if (err != noErr && err != fnfErr && err != dirNFErr && err != nsvErr)
		FailMacError(err);
}

// fails if folder contains other items
void UFileSys::DeleteFolder(TFSRefObj* inRef)
{
	Require(inRef);
	
	OSErr err = FSpDelete(&((SFileSysRef *)inRef)->spec);		// also deletes files, but probably won't matter
	
	if (err != noErr && err != fnfErr && err != dirNFErr && err != nsvErr)
		FailMacError(err);
}

void UFileSys::DeleteFolderContents(TFSRefObj* inRef)
{
	Require(inRef);
	
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	CheckMacError(DeleteDirectoryContents(spec->vRefNum, spec->parID, spec->name));
}

void UFileSys::MoveToTrash(TFSRefObj* inRef)
{
	Require(inRef);
	
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	Int16 foundVRefNum;
	Int32 foundDirID;
	FSSpec dstSpec;
	OSErr err;
	Uint8 str[64];
	
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	CheckMacError(::FindFolder(spec->vRefNum, kTrashFolderType, kCreateFolder, &foundVRefNum, &foundDirID));	
	CheckMacError(::FSMakeFSSpec(foundVRefNum, foundDirID, nil, &dstSpec));
	err = ::FSpCatMove(&((SFileSysRef *)inRef)->spec, &dstSpec);
	
	if (err)
	{
		if (err == dupFNErr)
		{
			if (spec->name[0] == 0)
				CheckMacError(GetDirName(spec->vRefNum, spec->parID, str));
			else
				::BlockMoveData(spec->name, str, spec->name[0]+1);
			
			GetNumberName((TFSRefObj*)&dstSpec, 1, str);
			err = FSpMoveRenameCompat(spec, &dstSpec, str);
			
			if (err == notAFileErr)
			{
				foundDirID = _FSGetDirID(spec->vRefNum, spec->parID, spec->name, nil);
				CheckMacError(::FSpRename(spec, str));
				CheckMacError(::CatMove(spec->vRefNum, foundDirID, "\p", dstSpec.parID, dstSpec.name));
			}
			else
				CheckMacError(err);
		}
		else
			FailMacError(err);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// if rename successful, ref is updated to point to same item (OR, ref still points to item after rename)
void UFileSys::SetName(TFSRefObj* inRef, const Uint8 *inName, Uint32 /* inEncoding */)
{
	Require(inRef && inName && inName[0]);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	
	_FSUpdateSpec(((SFileSysRef *)inRef));
	CheckMacError(::FSpRename(spec, inName));
	
	if (spec->name[0])	// if no name, using parID, which will be the same after rename
		::BlockMoveData(inName, spec->name, inName[0]+1);
}

// sets the name of the target file/folder in the TFSRefObj* without actually changing anything on disk
void UFileSys::SetRefName(TFSRefObj* inRef, const Uint8 *inName, Uint32 /* inEncoding */)
{
	Require(inRef && inName && inName[0]);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	
	// don't change the name if the file is open
	if (((SFileSysRef *)inRef)->refNum) 
		return;
	
	if (spec->name[0] == 0)
	{
		// it's a directory, so find the parent directory, then use that with inName
		long parID;
		CheckMacError(::GetParentID(spec->vRefNum, spec->parID, spec->name, &parID));
		spec->parID = parID;
	}
	
	spec->name[0] = inName[0];
#if TARGET_API_MAC_CARBON
	if (spec->name[0] > sizeof(spec->name) - 1) 
		spec->name[0] = sizeof(spec->name) - 1;
#else
	if (spec->name[0] > 31) 
		spec->name[0] = 31;
#endif
	
	::BlockMoveData(inName+1, spec->name+1, spec->name[0]);
}

// outName must point to 256 bytes
void UFileSys::GetName(TFSRefObj* inRef, Uint8 *outName, Uint32 *outEncoding)
{
	Require(inRef);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	if (outName)
	{
		if (spec->name[0])
			::BlockMoveData(spec->name, outName, spec->name[0]+1);
		else
		{
			CInfoPBRec pb;

			outName[0] = 0;

			pb.hFileInfo.ioFDirIndex = -1;		// MUST be -1 because name is zero-length
			pb.hFileInfo.ioNamePtr = outName;
			pb.hFileInfo.ioVRefNum = spec->vRefNum;
			pb.hFileInfo.ioDirID = spec->parID;
			
			CheckMacError(::PBGetCatInfoSync(&pb));
		}
	}
	
	if (outEncoding) *outEncoding = 0;
}

Uint32 UFileSys::GetName(TFSRefObj* inRef, void *outData, Uint32 inMaxSize, Uint32 *outEncoding)
{
	Require(inRef && outData);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	Uint32 s;
	
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	s = spec->name[0];
	if (s)
	{
		if (s > inMaxSize) s = inMaxSize;
		::BlockMoveData(spec->name+1, outData, s);
	}
	else
	{
		CInfoPBRec pb;
		Str63 tempName;

		tempName[0] = 0;

		pb.hFileInfo.ioFDirIndex = -1;		// MUST be -1 because name is zero-length
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioVRefNum = spec->vRefNum;
		pb.hFileInfo.ioDirID = spec->parID;
		
		CheckMacError(::PBGetCatInfoSync(&pb));
		
		s = tempName[0];
		if (s > inMaxSize) s = inMaxSize;
		::BlockMoveData(tempName+1, outData, s);
	}
	
	if (outEncoding) *outEncoding = 0;

	return s;
}

// return true if the name was changed
bool UFileSys::EnsureUniqueName(TFSRefObj* inRef)
{
	Require(inRef);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;

	if (!Exists(inRef) || ((SFileSysRef *)inRef)->refNum)	// don't change the name if the file exists or is open
		return false;
		
	if (spec->name[0] == 0)
	{
		// it's a directory, so find the parent directory, then use that with inName
		long parID;
		CheckMacError(::GetParentID(spec->vRefNum, spec->parID, spec->name, &parID));
		spec->parID = parID;
	}
	
	Uint8 bufName[64];
	UMemory::Copy(bufName, spec->name, spec->name[0] + 1);
		
	Uint32 nNameCount = 0;
	Uint8 bufNameCount[16];
	
	do
	{
		UMemory::Copy(spec->name, bufName, bufName[0] + 1);

		nNameCount++;
		if (nNameCount >= 10000)
			return false;
		
		bufNameCount[0] = UText::IntegerToText(bufNameCount + 1, sizeof(bufNameCount) - 1, nNameCount);
	#if TARGET_API_MAC_CARBON
		if (spec->name[0] + bufNameCount[0] > sizeof(spec->name) - 1)
			spec->name[0] = sizeof(spec->name) - 1 - bufNameCount[0];
	#else
		if (spec->name[0] + bufNameCount[0] > 31)
			spec->name[0] = 31 - bufNameCount[0];
	#endif
		
		spec->name[0] += UMemory::Copy(spec->name + spec->name[0] + 1, bufNameCount + 1, bufNameCount[0]);
		
	} while(Exists(inRef));

	return true;
}

Uint32 UFileSys::GetPath(TFSRefObj* inRef, void *outPath, Uint32 inMaxSize, Uint32 *outEncoding)
{
	Require(inRef && outPath);
	
	Uint32 nPathSize = sizeof(FSSpec);
	if (nPathSize > inMaxSize) nPathSize = inMaxSize;
	UMemory::Copy(outPath, &((SFileSysRef *)inRef)->spec, nPathSize);

	if (outEncoding) *outEncoding = 0;
	
	return nPathSize;
}

Uint32 UFileSys::GetFolder(TFSRefObj* inRef, void *outFolder, Uint32 /*inMaxSize*/, Uint32 */*outEncoding*/)
{
	Require(inRef && outFolder);
	
	Fail(errorType_Misc, error_Unimplemented);
	
	return 0;
}

TFSRefObj* UFileSys::GetFolder(TFSRefObj* inRef)
{
	Require(inRef);
	
	Str63 psParentName;
	psParentName[0] = GetParentName(inRef, psParentName + 1, sizeof(psParentName) - 1);
	if (!psParentName[0])
		return nil;
		
	Int32 nParentID;
	CheckMacError(::GetParentID(((SFileSysRef *)inRef)->spec.vRefNum, ((SFileSysRef *)inRef)->spec.parID, nil, &nParentID));

	SFileSysRef *ref = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	ref->spec.vRefNum = ((SFileSysRef *)inRef)->spec.vRefNum;
	ref->spec.parID = nParentID;
	::BlockMoveData(psParentName, ref->spec.name, psParentName[0] + 1);

	return (TFSRefObj*)ref;
}

// outName must point to 256 bytes
void UFileSys::GetParentName(TFSRefObj* inRef, Uint8 *outName, Uint32 *outEncoding)
{
	if (outName)
		outName[0] = GetParentName(inRef, outName+1, 255, outEncoding);
	else if (outEncoding)
		*outEncoding = 0;
}

Uint32 UFileSys::GetParentName(TFSRefObj* inRef, void *outData, Uint32 inMaxSize, Uint32 *outEncoding)
{
	CInfoPBRec pb;
	Str63 name;
	
	Require(inRef && outData);
	
	if (outEncoding) *outEncoding = 0;

	name[0] = 0;
	pb.hFileInfo.ioVRefNum = ((SFileSysRef *)inRef)->spec.vRefNum;
	
	if (((SFileSysRef *)inRef)->spec.name[0] == 0)
		CheckMacError(::GetParentID(pb.hFileInfo.ioVRefNum, ((SFileSysRef *)inRef)->spec.parID, name, &pb.hFileInfo.ioDirID));
	else
		pb.hFileInfo.ioDirID = ((SFileSysRef *)inRef)->spec.parID;
	
	pb.hFileInfo.ioNamePtr = name;
	pb.hFileInfo.ioFVersNum = 0;
	pb.hFileInfo.ioFDirIndex = -1;	// MUST be -1 because name is zero-length

	CheckMacError(PBGetCatInfoSync(&pb));
	
	if (name[0] > inMaxSize) name[0] = inMaxSize;
	::BlockMoveData(name+1, outData, name[0]);
	return name[0];
}

/*
 * GetNumberName() takes a file or folder name and starting number,
 * appends a number to the name to produce a unique name for the
 * specified folder, and returns the number that was used.
 */
Uint16 UFileSys::GetNumberName(TFSRefObj* inRef, Uint16 inNum, Uint8 *ioName, Uint32 /* inEncoding */)
{
	CInfoPBRec pb;
	Uint8 nameStr[64];
	Int32 dirID;
	bool isDir;
	
	// must NOT access fields in inRef beyond the spec so can typecast FSSpec to TFSRefObj*

	Require(inRef);
	dirID = _FSGetDirID(((SFileSysRef *)inRef)->spec.vRefNum, ((SFileSysRef *)inRef)->spec.parID, ((SFileSysRef *)inRef)->spec.name, &isDir);
	if (!isDir) Fail(errorType_FileSys, fsError_FolderExpected);

	pb.hFileInfo.ioFDirIndex = 0;			// we won't call it with a zero-length name
	pb.hFileInfo.ioVRefNum = ((SFileSysRef *)inRef)->spec.vRefNum;
	pb.hFileInfo.ioNamePtr = (StringPtr)nameStr;
	
	if (ioName[0] > 28) ioName[0] = 28;
	
	for (;;)
	{
		nameStr[0] = UText::Format(nameStr+1, 31, "%#s%hu", ioName, inNum);
		inNum++;
		
		pb.hFileInfo.ioDirID = dirID;		// gotta set this every time
		if (PBGetCatInfoSync(&pb)) break;
	}
	
	::BlockMoveData(nameStr, ioName, nameStr[0]+1);
	return inNum;
}

// returns new size of name, which will not be greater than the input size
Uint32 UFileSys::ValidateFileName(void *ioName, Uint32 inSize, Uint32 /* inEncoding */)
{ 
	// check if we are running under MacOS X
	if (_GetSystemVersion() >= 0x0A00)
	{
		if (inSize > 255) 
			inSize = 255;
	}
	else
	{
		// max 31 chars
		if (inSize > 31) 
		{
	 		// keep the extension
 			Uint8 *pExtension = UMemory::SearchByte('.', ioName, inSize);
			if (pExtension)
			{
				Uint32 nExtensionSize = (Uint8 *)ioName + inSize - pExtension;
				if (nExtensionSize > 30)
					nExtensionSize = 30;
			
				UMemory::Copy((Uint8 *)ioName + 31 - nExtensionSize, pExtension, nExtensionSize);
			}

			inSize = 31;
		}
	}

	_FSStripName(ioName, inSize);
	
	return inSize;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// if move successful, source ref is updated to point to item in new location
void UFileSys::Move(TFSRefObj* inRef, TFSRefObj* inDestFolder)
{
	Require(inRef && inDestFolder);
	FSSpec *srcSpec, *dstSpec;
	Int32 dstDirID;
	bool isDir;
	
	_FSUpdateSpec(((SFileSysRef *)inRef));

	srcSpec = &((SFileSysRef *)inRef)->spec;
	dstSpec = &((SFileSysRef *)inDestFolder)->spec;
	
	if (srcSpec->vRefNum != dstSpec->vRefNum)
		Fail(errorType_FileSys, fsError_DifferentDisks);
	
	dstDirID = _FSGetDirID(dstSpec->vRefNum, dstSpec->parID, dstSpec->name, &isDir);
	if (!isDir) Fail(errorType_FileSys, fsError_FolderExpected);

	CheckMacError(::FSpCatMove(srcSpec, dstSpec));
	
	if (srcSpec->name[0])	// if no name, using parID, which will be the same after move
		srcSpec->parID = dstDirID;
}

// if move successful, inRef is updated to point to item in new location
// if inNewName is not nil, inDest is the destination folder
// if inNewName is nil, the name from inDest is used (inDest should point to a non-existant file)
void UFileSys::MoveAndRename(TFSRefObj* inRef, TFSRefObj* inDest, const Uint8 *inNewName, Uint32 /* inEncoding */)
{
	Require(inRef && inDest);
	FSSpec *srcSpec, *dstSpec;
	Int32 dstDirID;
	bool isDir;

	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	srcSpec = &((SFileSysRef *)inRef)->spec;
	dstSpec = &((SFileSysRef *)inDest)->spec;

	if (srcSpec->vRefNum != dstSpec->vRefNum)
		Fail(errorType_FileSys, fsError_DifferentDisks);
	
	if (inNewName)
	{
		dstDirID = _FSGetDirID(dstSpec->vRefNum, dstSpec->parID, dstSpec->name, &isDir);
		if (!isDir) Fail(errorType_FileSys, fsError_FolderExpected);
		
		CheckMacError(::FSpMoveRenameCompat(srcSpec, dstSpec, inNewName));

		if (srcSpec->name[0])	// if no name, using parID, which will be the same after move
			srcSpec->parID = dstDirID;
	}
	else	// inDest points to a non-existant file
	{
		FSSpec spec;
		
		Uint8 *newName = dstSpec->name;
		if (newName[0] == 0) Fail(errorType_FileSys, fsError_FileExpected);
				
		spec.vRefNum = dstSpec->vRefNum;
		spec.parID = dstSpec->parID;
		spec.name[0] = 0;
		
		CheckMacError(::FSpMoveRenameCompat(srcSpec, &spec, newName));
		
		srcSpec->parID = spec.parID;
		::BlockMoveData(newName, srcSpec->name, newName[0]+1);
	}
}

// swaps the data for each file (but name and location stay the same)
void UFileSys::SwapData(TFSRefObj* inRefA, TFSRefObj* inRefB)
{
	CheckMacError(::FSpExchangeFiles((FSSpec *)inRefA, (FSSpec *)inRefB));
	
	Int16 saveA = ((SFileSysRef *)inRefA)->refNum;
	((SFileSysRef *)inRefA)->refNum = ((SFileSysRef *)inRefB)->refNum;
	((SFileSysRef *)inRefB)->refNum = saveA;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// fails if already open.  Once open, the ref will always point to the opened file (even if moved or renamed)
void UFileSys::Open(TFSRefObj* inRef, Uint32 inPerm)
{
	Require(inRef);
	Int16 refNum;
	
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, error_Protocol);
	
	CheckMacError(::FSpOpenAware(&((SFileSysRef *)inRef)->spec, inPerm, &refNum));
	
	((SFileSysRef *)inRef)->refNum = refNum;	// refNum might have been set even if error, so we're using a temp variable
}

// does nothing if already closed
// also stops any flatten/unflatten
void UFileSys::Close(TFSRefObj* inRef)
{
	// note that should not throw exceptions in a cleanup routine such as this
	if (inRef)
	{
		if (((SFileSysRef *)inRef)->refNum)
		{
			FCBPBRec pb;
			
			// last chance to grab the actual name/location of the file
			pb.ioNamePtr = ((SFileSysRef *)inRef)->spec.name;
			pb.ioVRefNum = 0;
			pb.ioRefNum = ((SFileSysRef *)inRef)->refNum;
			pb.ioFCBIndx = 0;
			if (PBGetFCBInfoSync(&pb) == noErr)
			{
				((SFileSysRef *)inRef)->spec.vRefNum = pb.ioFCBVRefNum;
				((SFileSysRef *)inRef)->spec.parID = pb.ioFCBParID;
			}
			
			// close the file
			FSClose(((SFileSysRef *)inRef)->refNum);
			((SFileSysRef *)inRef)->refNum = 0;
		}
		
		if (((SFileSysRef *)inRef)->resRefNum)
		{
			FSClose(((SFileSysRef *)inRef)->resRefNum);
			((SFileSysRef *)inRef)->resRefNum = 0;
		}
		
		if (((SFileSysRef *)inRef)->flatData)
		{
			UMemory::Dispose(((SFileSysRef *)inRef)->flatData);
			((SFileSysRef *)inRef)->flatData = nil;
		}
		
		if (((SFileSysRef *)inRef)->unflatData)
		{
			UMemory::Dispose(((SFileSysRef *)inRef)->unflatData);
			((SFileSysRef *)inRef)->unflatData = nil;
		}
	}
}

Uint32 UFileSys::Read(TFSRefObj* inRef, Uint32 inOffset, void *outData, Uint32 inMaxSize, Uint32 /* inOptions */)
{
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum == 0) Fail(errorType_Misc, error_Protocol);
	return _FSRead(((SFileSysRef *)inRef)->refNum, inOffset, outData, inMaxSize);
}

Uint32 UFileSys::Write(TFSRefObj* inRef, Uint32 inOffset, const void *inData, Uint32 inDataSize, Uint32 /* inOptions */)
{
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum == 0) Fail(errorType_Misc, error_Protocol);
	return _FSWrite(((SFileSysRef *)inRef)->refNum, inOffset, inData, inDataSize);
}

Uint32 UFileSys::GetSize(TFSRefObj* inRef, Uint32 inOptions)
{
	Require(inRef);
	long dataSize, rsrcSize;

	if (((SFileSysRef *)inRef)->refNum == 0)	// if not open
	{
		CheckMacError(::FSpGetFileSize(&((SFileSysRef *)inRef)->spec, &dataSize, &rsrcSize));
		return (inOptions & 1) ? (Uint32)dataSize : (Uint32)dataSize + (Uint32)rsrcSize;
	}
	else
	{
		CheckMacError(::GetEOF(((SFileSysRef *)inRef)->refNum, &dataSize));
		return dataSize;
	}
}

void UFileSys::SetSize(TFSRefObj* inRef, Uint32 inSize)
{
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum == 0) Fail(errorType_Misc, error_Protocol);
	CheckMacError(::SetEOF(((SFileSysRef *)inRef)->refNum, inSize));
}

void UFileSys::Flush(TFSRefObj* inRef)
{
	if (inRef && ((SFileSysRef *)inRef)->refNum)
		::FlushFile(((SFileSysRef *)inRef)->refNum);
	else
		::FlushVol(nil, 0);
}

// returns nil if no data
THdl UFileSys::ReadToHdl(TFSRefObj* inRef, Uint32 inOffset, Uint32& ioSize, Uint32 /* inOptions */)
{
	Require(inRef);

	long dataSize;
	Uint32 s = ioSize;
	Uint32 ns;
	Int16 refNum = ((SFileSysRef *)inRef)->refNum;
	
	if (refNum == 0) Fail(errorType_Misc, error_Protocol);
	
	CheckMacError(::GetEOF(refNum, &dataSize));
	
	if (inOffset >= dataSize)
	{
		ioSize = 0;
		return nil;
	}
	
	if (inOffset + s > dataSize)
		s = dataSize - inOffset;
	
	if (s == 0)
	{
		ioSize = 0;
		return nil;
	}
	
	THdl h = UMemory::NewHandle(s);
	
	try
	{
		{
			void *p;
			StHandleLocker lock(h, p);
			ns = _FSRead(refNum, inOffset, p, s);
		}
		
		if (ns < s)
		{
			if (ns == 0)
			{
				UMemory::Dispose(h);
				ioSize = 0;
				return nil;
			}
			UMemory::SetSize(h, ns);
		}
	}
	catch(...)
	{
		UMemory::Dispose(h);
		throw;
	}
	
	ioSize = ns;
	return h;
}


bool UFileSys::Copy(TFSRefObj* inSource, TFSRefObj* ioDest, Uint32 inDestTypeCode, Uint32 inDestCreatorCode)
{
	FInfo fi;

	Require(inSource && ioDest);
	_FSUpdateSpec((SFileSysRef *)inSource);
	
	FSSpec *srcSpec = &((SFileSysRef *)inSource)->spec;
	FSSpec *destSpec = &((SFileSysRef *)ioDest)->spec;

	THdl h = UMemory::NewHandle(200 * 1024, 1);
	scopekill(THdlObj, h);
	{
		void *p;
		StHandleLocker lock(h, p);
		
//		CheckMacError(::FSpFileCopy(srcSpec, destSpec, destSpec->name, p, h->GetSize(), true));
		CheckMacError(::FileCopy(srcSpec->vRefNum, srcSpec->parID, srcSpec->name, destSpec->vRefNum, destSpec->parID, nil, destSpec->name, p, h->GetSize(), true));
	}
	
	CheckMacError(::FSpGetFInfo(destSpec, &fi));
	fi.fdType = inDestTypeCode;
	fi.fdCreator = inDestCreatorCode;
	
	CheckMacError(::FSpSetFInfo(destSpec, &fi));
	
	return true;
}


/* -------------------------------------------------------------------------- */
#pragma mark -

Uint16 UFileSys::GetType(TFSRefObj* inRef)
{
	FSSpec resolvedSpec;
	Uint16 type;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	_FSResolveAliasAndGetType(&((SFileSysRef *)inRef)->spec, &resolvedSpec, &type);
	
	return type;
}

void UFileSys::SetComment(TFSRefObj* inRef, const void *inText, Uint32 inTextSize, Uint32 /* inEncoding */)
{
	Str255 str;

	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	if (inTextSize > 255) inTextSize = 255;
	str[0] = inTextSize;
	::BlockMoveData(inText, str+1, inTextSize);
	
	CheckMacError(::FSpDTSetComment(&((SFileSysRef *)inRef)->spec, str)); //?? seems that FSpDTSetComment doesn't work under the first release of Mac OS X
}

Uint32 UFileSys::GetComment(TFSRefObj* inRef, void *outText, Uint32 inMaxSize, Uint32 *outEncoding)
{
	Str255 str;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	if (outEncoding) *outEncoding = 0;
	
	if (::FSpDTGetComment(&((SFileSysRef *)inRef)->spec, str))
		return 0;		// FSpDTGetComment returns errors when there is no comment
	
	if (str[0] > inMaxSize) str[0] = inMaxSize;
	::BlockMoveData(str+1, outText, str[0]);
	return str[0];
}

void UFileSys::SetTypeAndCreatorCode(TFSRefObj* inRef, Uint32 inTypeCode, Uint32 inCreatorCode)
{
	FInfo fi;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	CheckMacError(::FSpGetFInfo(&((SFileSysRef *)inRef)->spec, &fi));
	fi.fdType = inTypeCode;
	fi.fdCreator = inCreatorCode;
	
	CheckMacError(::FSpSetFInfo(&((SFileSysRef *)inRef)->spec, &fi));
}

void UFileSys::GetTypeAndCreatorCode(TFSRefObj* inRef, Uint32& outTypeCode, Uint32& outCreatorCode)
{
	FInfo fi;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	CheckMacError(::FSpGetFInfo(&((SFileSysRef *)inRef)->spec, &fi));
	
	outTypeCode = fi.fdType;
	outCreatorCode = fi.fdCreator;
}

Uint32 UFileSys::GetTypeCode(TFSRefObj* inRef)
{
	FInfo fi;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	CheckMacError(::FSpGetFInfo(&((SFileSysRef *)inRef)->spec, &fi));
	
	return fi.fdType;
}

Uint32 UFileSys::GetCreatorCode(TFSRefObj* inRef)
{
	FInfo fi;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	
	CheckMacError(::FSpGetFInfo(&((SFileSysRef *)inRef)->spec, &fi));
	
	return fi.fdCreator;
}

void UFileSys::GetDateStamp(TFSRefObj* inRef, SDateTimeStamp *outModified, SDateTimeStamp *outCreated)
{
	CInfoPBRec pb;
	Str63 tempName;
	
	Require(inRef);
	_FSUpdateSpec(((SFileSysRef *)inRef));
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	
	::BlockMoveData(spec->name, tempName, spec->name[0]+1);

	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = spec->vRefNum;
	pb.hFileInfo.ioDirID = spec->parID;
	
	CheckMacError(::PBGetCatInfoSync(&pb));
	
	if (outModified) _MacSecondsToStamp(pb.hFileInfo.ioFlMdDat, *outModified);
	if (outCreated) _MacSecondsToStamp(pb.hFileInfo.ioFlCrDat, *outCreated);
}

bool UFileSys::IsHidden(TFSRefObj* inRef)
{
	CInfoPBRec pb;
	Str63 tempName;
	
	// can't just use FSpGetFInfo for this func because it doesn't work with directories
	Require(inRef);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;

	::BlockMoveData(spec->name, tempName, spec->name[0]+1);

	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = spec->vRefNum;
	pb.hFileInfo.ioDirID = spec->parID;
	
	CheckMacError(::PBGetCatInfoSync(&pb));

	return (pb.hFileInfo.ioFlFndrInfo.fdFlags & fInvisible) != 0;
}


bool UFileSys::Equals(TFSRefObj* inRef, TFSRefObj* inCompare)
{
	if (((SFileSysRef *)inRef)->spec.parID != ((SFileSysRef *)inCompare)->spec.parID || ((SFileSysRef *)inRef)->spec.vRefNum != ((SFileSysRef *)inCompare)->spec.vRefNum)
		return false;
	
	if (pstrcmp(((SFileSysRef *)inRef)->spec.name, ((SFileSysRef *)inCompare)->spec.name) == 0)
		return true;
		
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// only files of the specified types (terminate list with 0) are shown in the listing
// returns nil if user cancelled
TFSRefObj* UFileSys::UserSelectFile(TFSRefObj* inInitianFolder, const Uint32 *inTypeList, Uint32 /* inOptions */)
{
#if TARGET_API_MAC_CARBON

	SFileSysRef *pRef = nil;
	
	// init options structure
	NavDialogOptions options;
	::NavGetDefaultDialogOptions(&options);

	// create initial folder descriptor
	AEDesc initianFolder = { typeNull, nil };
	::AECreateDesc(typeFSS, &((SFileSysRef *)inInitianFolder)->spec, sizeof(FSSpec), &initianFolder);
	
	// create UPP to the event-handling function
	NavEventUPP eventUPP = ::NewNavEventUPP(_FileEventProc);

	// create file type list
	Handle hTypeList = nil;
	if (inTypeList && inTypeList[0])
	{
		Int16 nNumTypes = 0;
		Uint32 *pTypeList = (Uint32 *)inTypeList;
		while (*pTypeList++) ++nNumTypes;
		if (nNumTypes > 4) nNumTypes = 4;
		
		hTypeList = ::NewHandle(sizeof(NavTypeList) + nNumTypes * sizeof(Uint32));
		if (hTypeList)
		{
			(*(NavTypeListHandle)hTypeList)->componentSignature = kNavGenericSignature;
			(*(NavTypeListHandle)hTypeList)->osTypeCount = nNumTypes;
			::BlockMoveData(inTypeList, (*(NavTypeListHandle)hTypeList)->osType, nNumTypes * sizeof(Uint32));
		
			HLock(hTypeList);
		}
	}
	
	// create the dialog
	NavReplyRecord reply;
	OSErr err = ::NavGetFile(&initianFolder, &reply, &options, eventUPP, NULL, NULL, (NavTypeListHandle)hTypeList, NULL);
	
	// dispose initial folder descriptor and UPP
	::AEDisposeDesc(&initianFolder);
	::DisposeNavEventUPP(eventUPP);
	
	// dispose file type list
	if (hTypeList)
	{
		HUnlock(hTypeList);
		::DisposeHandle(hTypeList);
	}
	
	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;
	
	// create the file system specification record
	if (err == noErr && reply.validRecord)
	{
		AEKeyword unusedKeyword;
		AEDesc fileSpecDesc = { typeNull, nil };
		err = ::AEGetNthDesc(&reply.selection, 1, typeFSS, &unusedKeyword, &fileSpecDesc);

		if (err == noErr)
		{
			pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
			::AEGetDescData(&fileSpecDesc, &pRef->spec, sizeof(FSSpec));
		}

		// dispose file descriptor and reply structure
		::AEDisposeDesc(&fileSpecDesc);
		::NavDisposeReply(&reply);
	}

	return (TFSRefObj*)pRef;
	
#else

	SFTypeList stTypeList;
	StandardFileReply stFileReply;
	Point stWhere = {-1, -1};					// center

	// init record structure
	SFileRecordHook stRecordHook;
	stRecordHook.pFileReplay = &stFileReply;
	if (inInitianFolder)
		stRecordHook.pFolderSpec = &((SFileSysRef *)inInitianFolder)->spec;
	else
		stRecordHook.pFolderSpec = nil;
		
	// create file type list
	Int16 nNumTypes;
	if (inTypeList == nil || inTypeList[0] == 0)
		nNumTypes = -1;
	else
	{
		nNumTypes = 0;
		Uint32 *pTypeList = (Uint32 *)inTypeList;
		while (*pTypeList++) ++nNumTypes;	
		if (nNumTypes > 4) nNumTypes = 4;
		
		::BlockMoveData(inTypeList, stTypeList, nNumTypes * sizeof(Uint32));
	}	

	// create the dialog
	DlgHookYDUPP pDialogHook = NewDlgHookYDProc(_FileDialogHook);
	::CustomGetFile(nil, nNumTypes, stTypeList, &stFileReply, 0, stWhere, pDialogHook, nil, nil, nil, &stRecordHook);
	::DisposeRoutineDescriptor(pDialogHook);

	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;

	if (!stFileReply.sfGood) 
		return nil;
	
	// create the file system specification record
	SFileSysRef *pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	pRef->spec = stFileReply.sfFile;
	
	return (TFSRefObj*)pRef;
	
#endif
}

TFSRefObj* UFileSys::UserSelectFolder(TFSRefObj* inInitianFolder, Uint32 /* inOptions */)
{
#if TARGET_API_MAC_CARBON

	SFileSysRef *pRef = nil;
	
	// init options structure
	NavDialogOptions options;
	::NavGetDefaultDialogOptions(&options);
	
	// create initial folder descriptor
	AEDesc initianFolder = { typeNull, nil };
	::AECreateDesc(typeFSS, &((SFileSysRef *)inInitianFolder)->spec, sizeof(FSSpec), &initianFolder);

	// create UPP to the event-handling function
	NavEventUPP eventUPP = ::NewNavEventUPP(_FileEventProc);

	// create the dialog
	NavReplyRecord reply;
	OSErr err = ::NavChooseFolder(&initianFolder, &reply, &options, eventUPP, NULL, NULL);
	
	// dispose initial folder descriptor and UPP
	::AEDisposeDesc(&initianFolder);
	::DisposeNavEventUPP(eventUPP);

	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;
	
	// create the file system specification record
	if (err == noErr && reply.validRecord)
	{
		AEKeyword unusedKeyword;
		AEDesc fileSpecDesc = { typeNull, nil };
		err = ::AEGetNthDesc(&reply.selection, 1, typeFSS, &unusedKeyword, &fileSpecDesc);

		if (err == noErr)
		{
			pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
			::AEGetDescData(&fileSpecDesc, &pRef->spec, sizeof(FSSpec));
		}

		// dispose file descriptor and reply structure
		::AEDisposeDesc(&fileSpecDesc);
		::NavDisposeReply(&reply);
	}
	
	return (TFSRefObj*)pRef;

#else

	Str255 pDefaultName = "\p��������������";	// we don't want a file with this name to exist
	StandardFileReply stFileReply;
	Point stWhere = {-1, -1};					// center
	Int16 bufActiveList[2] = {1, 7};			// only accept keydowns on file list

	// init record structure
	SFileRecordHook stRecordHook;
	stRecordHook.pFileReplay = &stFileReply;
	if (inInitianFolder)
		stRecordHook.pFolderSpec = &((SFileSysRef *)inInitianFolder)->spec;
	else
		stRecordHook.pFolderSpec = nil;

	// create the dialog
	DlgHookYDUPP pDialogHook = NewDlgHookYDProc(_FileDialogHook);
	::CustomPutFile(nil, pDefaultName, &stFileReply, 9438, stWhere, pDialogHook, nil, bufActiveList, nil, &stRecordHook);
	::DisposeRoutineDescriptor(pDialogHook);

	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;

	if (!stFileReply.sfGood) 
		return nil;
	
	// create the file system specification record
	CheckMacError(::FSMakeFSSpec(stFileReply.sfFile.vRefNum, stFileReply.sfFile.parID, nil, &stFileReply.sfFile));

	SFileSysRef *pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	pRef->spec = stFileReply.sfFile;
	
	return (TFSRefObj*)pRef;
	
#endif
}

TFSRefObj* UFileSys::UserSaveFile(TFSRefObj* inInitianFolder, const Uint8 *inDefaultName, const Uint8 *inPrompt, Uint32 /* inNameEncoding */, Uint32 /* inPromptEncoding */, Uint32 /* inOptions */)
{
	// set the default name
	if (inDefaultName == nil)
		inDefaultName = "\pUntitled";
	
#if TARGET_API_MAC_CARBON

	SFileSysRef *pRef = nil;
	
	// init options structure
	NavDialogOptions options;
	::NavGetDefaultDialogOptions(&options);
	options.dialogOptionFlags -= kNavAllowStationery;
	::BlockMoveData(inDefaultName, options.savedFileName, inDefaultName[0] + 1);
	
	if (inPrompt && inPrompt[0]) 
		::BlockMoveData(inPrompt, options.windowTitle, inPrompt[0] + 1);

	// create initial folder descriptor
	AEDesc initianFolder = { typeNull, nil };
	::AECreateDesc(typeFSS, &((SFileSysRef *)inInitianFolder)->spec, sizeof(FSSpec), &initianFolder);
	
	// create UPP to the event-handling function
	NavEventUPP eventUPP = ::NewNavEventUPP(_FileEventProc);
	
	// create the dialog
	NavReplyRecord reply;
	OSErr err = ::NavPutFile(&initianFolder, &reply, &options, eventUPP, NULL, kNavGenericSignature, NULL);
	
	// dispose initial folder descriptor and UPP
	::AEDisposeDesc(&initianFolder);
	::DisposeNavEventUPP(eventUPP);
	
	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;
	
	// create the file system specification record
	if (err == noErr && reply.validRecord)
	{
		AEKeyword unusedKeyword;
		AEDesc fileSpecDesc = { typeNull, nil };
		err = ::AEGetNthDesc(&reply.selection, 1, typeFSS, &unusedKeyword, &fileSpecDesc);

		if (err == noErr)
		{
			pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
			::AEGetDescData(&fileSpecDesc, &pRef->spec, sizeof(FSSpec));
		}

		// dispose file descriptor and reply structure
		::AEDisposeDesc(&fileSpecDesc);
		::NavDisposeReply(&reply);
	}

	return (TFSRefObj*)pRef;
	
#else

	if (inPrompt == nil)
		inPrompt = "\pSave As:";

	StandardFileReply stFileReply;
	Point stWhere = {-1, -1};					// center

	// init record structure
	SFileRecordHook stRecordHook;
	stRecordHook.pFileReplay = &stFileReply;
	if (inInitianFolder)
		stRecordHook.pFolderSpec = &((SFileSysRef *)inInitianFolder)->spec;
	else
		stRecordHook.pFolderSpec = nil;
	
	// create the dialog
	DlgHookYDUPP pDialogHook = NewDlgHookYDProc(_FileDialogHook);
	::CustomPutFile(inPrompt, inDefaultName, &stFileReply, 0, stWhere, pDialogHook, nil, nil, nil, &stRecordHook);
	::DisposeRoutineDescriptor(pDialogHook);

	// app can come from back to front while modal dialog is up
	_APInBkgndFlag = !_APIsInForeground();
	_WNCheckWinStatesFlag = true;
	
	if (!stFileReply.sfGood) 
		return nil;

	// create the file system specification record
	SFileSysRef *pRef = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	pRef->spec = stFileReply.sfFile;
	
	return (TFSRefObj*)pRef;
	
#endif
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * Returns nil if no items in folder.  Format of handle is:
 *
 *		Uint32 count;
 *		SFSListItem item[count];	// each item begins on four-byte boundary
 */
THdl UFileSys::GetListing(TFSRefObj* inRef, Uint32 inOptions)
{
	Require(inRef);
	CInfoPBRec pb;
	FSSpec aliasSpec;
	Uint8 buf[256];
	SFSListItem& item = *(SFSListItem *)buf;
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	Int32 dirID;
	Uint32 s;
	OSErr err;
	bool isDir;
	Uint16 itemIndex = 1;
	Int16 volNum;
	THdl h;
	Boolean isFolder, wasAliased;
	
	// get the real location
	dirID = _FSGetDirID(spec->vRefNum, spec->parID, spec->name, &isDir);
	if (!isDir) Fail(errorType_FileSys, fsError_FolderExpected);
	volNum = _FSGetVRefNum(spec->name, spec->vRefNum);
	
	// allocate handle to store item info
	h = UMemory::NewHandle(sizeof(Uint32));
	
	// set unchanging fields in item info
	item.createdDate.year = item.modifiedDate.year = 1904;
	item.createdDate.msecs = item.modifiedDate.msecs = 0;
	item.rsvd = 0;

	// index through the directory, getting info on each item
	for(;;)
	{
		// get info on the specified index
		pb.hFileInfo.ioNamePtr = item.name;
		pb.hFileInfo.ioDirID = dirID;
		pb.hFileInfo.ioVRefNum = volNum;
		pb.hFileInfo.ioFDirIndex = itemIndex;
		err = PBGetCatInfoSync(&pb);
		if (err)
		{
			if (err == fnfErr) break;
			else goto error;
		}
		
		// store info for this item
		if (pb.hFileInfo.ioFlAttrib & ioDirMask)	// if directory
		{
			// store info for this folder
storeDirInfo:
			item.typeCode = 'fldr';
			item.creatorCode = 0;
			item.size = pb.dirInfo.ioDrNmFls;
			item.flags = (pb.dirInfo.ioDrUsrWds.frFlags & 0x4000) ? 1 : 0;		// invisibility
		}
		else
		{
			item.flags = (pb.hFileInfo.ioFlFndrInfo.fdFlags & 0x4000) ? 1 : 0;	// invisibility

			// if the item is an alias, we want the info to be from the original item, not the alias
			if (pb.hFileInfo.ioFlFndrInfo.fdFlags & 0x8000)		// if alias
			{
				item.flags |= 2;
				
				// make spec of alias file
				aliasSpec.vRefNum = pb.hFileInfo.ioVRefNum;
				aliasSpec.parID = pb.hFileInfo.ioFlParID;
				::BlockMoveData(item.name, aliasSpec.name, item.name[0]+1);
				
				// attempt to resolve alias
				if (!(inOptions & kDontResolveAliases) && _FSResolveAliasFile(&aliasSpec, &isFolder, &wasAliased) == noErr)
				{
					// resolved successfully, get info on target
					pb.hFileInfo.ioNamePtr = aliasSpec.name;
					pb.hFileInfo.ioDirID = aliasSpec.parID;
					pb.hFileInfo.ioVRefNum = aliasSpec.vRefNum;
					pb.hFileInfo.ioFDirIndex = (aliasSpec.name[0] == 0) ? -1 : 0;
					if (PBGetCatInfoSync(&pb) != noErr) goto unattachedAlias;
					if (pb.hFileInfo.ioFlAttrib & ioDirMask) goto storeDirInfo;
				}
				else
				{
					// list as unattached
				unattachedAlias:
					pb.hFileInfo.ioFlFndrInfo.fdType = 'alis';
					pb.hFileInfo.ioFlFndrInfo.fdCreator = 0;
					pb.hFileInfo.ioFlLgLen = pb.hFileInfo.ioFlRLgLen = 0;
				}
			}
			
			// store type, creator and size for this file
			item.typeCode = pb.hFileInfo.ioFlFndrInfo.fdType;
			item.creatorCode = pb.hFileInfo.ioFlFndrInfo.fdCreator;
			item.size = pb.hFileInfo.ioFlLgLen + pb.hFileInfo.ioFlRLgLen;
		}
		
		// store created and modified dates for this item
		item.createdDate.seconds = pb.hFileInfo.ioFlCrDat;
		item.modifiedDate.seconds = pb.hFileInfo.ioFlMdDat;

		// append data to handle, aligning to four byte boundary
		s = (sizeof(SFSListItem) + item.name[0] + 4) & 0xFFFFFFFC;
		err = ::PtrAndHand(&item, (Handle)h, s);
		if (err) goto error;
		
		// next item
		itemIndex++;
	}
	
	// if no items, return nil
	if (itemIndex == 1)
	{
		UMemory::Dispose(h);
		return nil;
	}
		
	// store count and return handle
	UMemory::WriteLong(h, 0, itemIndex - 1);
	return h;
	
	// error, kill handle and fail
	error:
	UMemory::Dispose(h);
	FailMacError(err);
	return nil;
}

// outName must point to 256 bytes (or be nil)
bool UFileSys::GetListNext(THdl inListData, Uint32& ioOffset, Uint8 *outName, Uint32 *outTypeCode, Uint32 *outCreatorCode, Uint32 *outSize, SDateTimeStamp *outCreatedDate, SDateTimeStamp *outModifiedDate, Uint32 *outFlags)
{
	Uint8 *p;
	Uint32 maxSize;
	SFSListItem *item;
	
	if (inListData == nil) return false;
	StHandleLocker lock(inListData, (void*&)p);
	maxSize = UMemory::GetSize(inListData);
	
	if (ioOffset == 0) ioOffset = 4;		// skip count
	if ((ioOffset + sizeof(SFSListItem)) > maxSize) return false;
	
	item = (SFSListItem *)(p + ioOffset);
	
	if (outName) ::BlockMoveData(item->name, outName, item->name[0]+1);
	
	if (outTypeCode)		*outTypeCode = item->typeCode;
	if (outCreatorCode)		*outCreatorCode = item->creatorCode;
	if (outSize)			*outSize = item->size;
	if (outCreatedDate)		*outCreatedDate = item->createdDate;
	if (outModifiedDate)	*outModifiedDate = item->modifiedDate;
	if (outFlags)			*outFlags = item->flags;
	
	ioOffset += (sizeof(SFSListItem) + item->name[0] + 4) & 0xFFFFFFFC;
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// returns true if now points to a valid file that is not an alias
// outOrigType is set to what type of FS item the spec was BEFORE it was resolved
bool UFileSys::ResolveAlias(TFSRefObj* inRef, Uint16 *outOrigType)
{
	Require(inRef);
	FSSpec *spec = &((SFileSysRef *)inRef)->spec;
	return _FSResolveAliasAndGetType(spec, spec, outOrigType);
}

Uint32 UFileSys::GetApplicationPath(void *outAppPath, Uint32 /*inMaxPathSize*/)
{
	Require(outAppPath);
	
	Fail(errorType_Misc, error_Unimplemented);
	
	return 0;
}

Uint32 UFileSys::GetApplicationURL(UInt8 *url, Uint32 urlZ)
{
	Require(url);
	
	UInt32 size = 8;
	{   // prepend file:///
		if (urlZ < size)
			return 0;
		for( char *b = "file:///"; *b!=0; ++b)
			*url++ = *b;
	}
	
	ProcessSerialNumber psn = {0};
	int err = ::GetCurrentProcess(&psn);
	if (err != 0)
		return 0;
	
	ProcessInfoRec info = {0};
	FSSpec spec = {0};
	info.processInfoLength = sizeof(info);
	info.processAppSpec = &spec;
	err = ::GetProcessInformation(&psn, &info);
	if (err != 0)
		return 0;
	
	short z = 0;
	Handle handle = 0;
	err = ::GetFullPath(spec.vRefNum, spec.parID, "\p", &z, &handle);
	if (err != 0)
		return 0;
	if (size+z > urlZ)
		z = urlZ-size;
	{ // copy to url buffer	
		::HLock(handle);
		char *b = *handle, *e = b+z;
		while(b!=e)	 // skip the disk name
		{
			--z;
			if (*b++ == ':')
				break; 
		}
		for( ; b!=e; ++b) // include the path on disk
			*url++ = (*b==':') ? '/' : *b;
		size += z;
		::HUnlock(handle);
	}
	::DisposeHandle(handle);
	
	return size;
}

Uint32 UFileSys::GetTempPath(void *outTempPath, Uint32 /*inMaxPathSize*/)
{
	Require(outTempPath);
	
	Fail(errorType_Misc, error_Unimplemented);
	
	return 0;
}

bool UFileSys::IsDiskLocked(TFSRefObj* inRef)
{
	Require(inRef);
	return CheckVolLock(nil, ((SFileSysRef *)inRef)->spec.vRefNum) != 0;
}

Uint64 UFileSys::GetDiskFreeSpace(TFSRefObj* inRef)
{
	Require(inRef);

	// in order to call XGetVInfo we need Mac OS 8.5 or later
	if (_GetSystemVersion() < 0x0850)
	{
		HParamBlockRec info;
		ClearStruct(info);
	
		CheckMacError(::GetVolumeInfoNoName(nil, ((SFileSysRef *)inRef)->spec.vRefNum, &info));
		return (Uint64)info.volumeParam.ioVFrBlk * (Uint64)info.volumeParam.ioVAlBlkSiz;
	}

	short nRefNum;
	Uint64 nFreeBytes, nTotalBytes;
	CheckMacError(::XGetVInfo(((SFileSysRef *)inRef)->spec.vRefNum, nil, &nRefNum, &nFreeBytes, &nTotalBytes));

	return nFreeBytes;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

SFlattenRef *UFileSys::FlattenRef(TFSRefObj* inRef)
{
	Require(inRef);

	SFlattenRef *pFlattenRef = (SFlattenRef *)UMemory::New(sizeof(SFlattenRef) + sizeof(FSSpec));

	pFlattenRef->sig = 'FSrf';
	pFlattenRef->vers = 1;
	pFlattenRef->OS = 'MACH';
	
	pFlattenRef->dataSize = sizeof(FSSpec);
	UMemory::Copy(pFlattenRef->data, &((SFileSysRef *)inRef)->spec, pFlattenRef->dataSize);

	return pFlattenRef;
}

TFSRefObj* UFileSys::UnflattenRef(SFlattenRef *inFlattenRef)
{
	Require(inFlattenRef);

	if (inFlattenRef->sig != 'FSrf') Fail(errorType_Misc, error_FormatUnknown);
	if (inFlattenRef->vers != 1) Fail(errorType_Misc, error_VersionUnknown);
	if (inFlattenRef->OS != 'MACH') Fail(errorType_Misc, error_FormatUnknown);
		
	return _PathToRef(inFlattenRef->data, inFlattenRef->dataSize);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
The file package format is a archive style, COMPACT format for transfering files
between platforms, over network connections and for archiving.

It looks like this:

	Uint32 format;				// always 'FILP'
	Uint16 version;				// currently 1
	Uint8 rsvd[16];				// should all be 0
	Uint16 forkCount;			// number of forks following
	struct {
		Uint32 forkType;		// type of fork, eg, 'DATA', 'INFO', 'MACR'
		Uint32 compType;		// compression type (0=none)
		Uint32 rsvd;			// should be 0
		Uint32 dataSize;		// number of bytes in following string
		Uint8 data[dataSize];	// data for this fork
	} forks[forkCount];
*/

#pragma options align=packed
struct SFlatFileHeader {
	Uint32 format;
	Uint16 version;
	Uint8 rsvd[16];
	Uint16 forkCount;
};

struct SFlatFileForkHeader {
	Uint32 forkType;
	Uint32 compType;
	Uint32 rsvd;
	Uint32 dataSize;
};

struct SFlatFileInfoFork {
	Uint32 platform;			// operating system, eg, 'AMAC', 'MWIN'
	Uint32 typeSig;				// file type signature
	Uint32 creatorSig;			// file creator signature
	Uint32 flags;
	Uint32 platFlags;			// platform specific flags
	Uint8 rsvd[32];
	SDateTimeStamp createDate;
	SDateTimeStamp modifyDate;
	Uint16 nameScript;
	Uint16 nameSize;
	Uint8 nameData[];
};

struct SFlatFileResumeEntry {
	Uint32 fork;
	Uint32 dataSize;
	Uint32 rsvdA, rsvdB;
};

struct SFlatFileResumeData {
	Uint32 format;				// always 'RFLT'
	Uint16 version;				// currently 1
	Uint8 rsvd[34];
	Uint16 count;
	SFlatFileResumeEntry forkInfo[];
};
#pragma options align=reset

// returns size of outInfo
static Uint32 _FSCreateFlatFileInfo(const FSSpec *inSpec, SFlatFileInfoFork& outInfo, Uint32 *outDataSize = nil, Uint32 *outResSize = nil)
{
	CInfoPBRec pb;
	Str255 str;
	Uint8 *p;
	
	::BlockMoveData(inSpec->name, str, inSpec->name[0]+1);

	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioNamePtr = str;
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioDirID = inSpec->parID;
	
	CheckMacError(::PBGetCatInfoSync(&pb));

	UMemory::Clear(&outInfo, sizeof(SFlatFileInfoFork));
	
	outInfo.platform = 'AMAC';
	outInfo.typeSig = pb.hFileInfo.ioFlFndrInfo.fdType;
	outInfo.creatorSig = pb.hFileInfo.ioFlFndrInfo.fdCreator;
	outInfo.platFlags = pb.hFileInfo.ioFlFndrInfo.fdFlags;
	
	_MacSecondsToStamp(pb.hFileInfo.ioFlCrDat, outInfo.createDate);
	_MacSecondsToStamp(pb.hFileInfo.ioFlMdDat, outInfo.modifyDate);
	
	outInfo.nameSize = str[0];
	::BlockMoveData(str+1, outInfo.nameData, outInfo.nameSize);
	
	if (::FSpDTGetComment(inSpec, str) != noErr) str[0] = 0;
	p = outInfo.nameData + outInfo.nameSize;
	*((Uint16 *)p)++ = str[0];
	::BlockMoveData(str+1, p, str[0]);
	
	if (outDataSize) *outDataSize = pb.hFileInfo.ioFlLgLen;
	if (outResSize) *outResSize = pb.hFileInfo.ioFlRLgLen;
	
	return sizeof(SFlatFileInfoFork) + outInfo.nameSize + sizeof(Uint16) + str[0];
}

static void _FSSetFlatFileInfo(const FSSpec *inSpec, const SFlatFileInfoFork& inInfo, Uint32 inSize)
{
	Uint32 s, secs;
	Uint8 *p, *ep;
	
	Str255 str;
	::BlockMoveData(inSpec->name, str, inSpec->name[0]+1);

	CInfoPBRec pb;
	ClearStruct(pb);
	
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioNamePtr = str;
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioDirID = inSpec->parID;
	
	CheckMacError(::PBGetCatInfoSync(&pb));
	
	pb.hFileInfo.ioFDirIndex = 0;
	pb.hFileInfo.ioNamePtr = str;			// have to set these again
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioDirID = inSpec->parID;
	pb.hFileInfo.ioFlFndrInfo.fdType = inInfo.typeSig;
	pb.hFileInfo.ioFlFndrInfo.fdCreator = inInfo.creatorSig;
	pb.hFileInfo.ioFlFndrInfo.fdFlags = inInfo.platFlags;
	
	// bump date so finder knows it was modified
	::GetDateTime(&secs);
	pb.hFileInfo.ioFlMdDat = (secs == pb.hFileInfo.ioFlMdDat ? secs + 1 : secs);
	
	// clear initted flag so finder will look for BNDLs, position icon etc
	pb.hFileInfo.ioFlFndrInfo.fdFlags &= ~0x100;
	
	// don't allow aliases (for security)
	pb.hFileInfo.ioFlFndrInfo.fdFlags &= ~0x8000;
	
	CheckMacError(::PBSetCatInfoSync(&pb));
	
	p = BPTR(inInfo.nameData) + inInfo.nameSize;
	ep = BPTR(&inInfo) + inSize;
	if (p < ep)
	{
		s = *((Uint16 *)p)++;
		if (s)
		{
			if (s > sizeof(str)-1) s = sizeof(str)-1;
			str[0] = s;
			::BlockMoveData(p, str+1, s);		
			::FSpDTSetComment(inSpec, str);	//?? seems that FSpDTSetComment doesn't work under the first release of Mac OS X
		}
	}
	
	::FlushVol(nil, inSpec->vRefNum);
}

struct SFileFlatten {
	Uint32 stage;
	Uint32 pos;
	Uint32 dataResumeSize, rsrcResumeSize;
	SFlatFileHeader hdr;
	SFlatFileForkHeader dataHdr;
	SFlatFileForkHeader rsrcHdr;
	SFlatFileForkHeader infoHdr;
	SFlatFileInfoFork info;
	Uint8 infoNameBuf[350];
};

void UFileSys::StartFlatten(TFSRefObj* inRef)
{
	FSSpec *spec;
	SFileFlatten *ref;
	Int16 dataRefNum, rsrcRefNum;
	OSErr err;
	
	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, error_Protocol);

	dataRefNum = rsrcRefNum = 0;
	spec = &((SFileSysRef *)inRef)->spec;
	ref = (SFileFlatten *)UMemory::NewClear(sizeof(SFileFlatten));
	
	try
	{
		ref->infoHdr.dataSize = _FSCreateFlatFileInfo(spec, ref->info, &ref->dataHdr.dataSize, &ref->rsrcHdr.dataSize);
		
		ref->infoHdr.forkType = 'INFO';
		ref->dataHdr.forkType = 'DATA';
		ref->rsrcHdr.forkType = 'MACR';
		
		ref->hdr.format = 'FILP';
		ref->hdr.version = 1;
		ref->hdr.forkCount = 3;
		
		err = ::FSpOpenAware(spec, dmRdDenyWr, &dataRefNum);
		if (err)
		{
			dataRefNum = 0;		// sometimes sets it even if error
			FailMacError(err);
		}
		err = ::FSpOpenRFAware(spec, dmRdDenyWr,  &rsrcRefNum);
		if (err)
		{
			rsrcRefNum = 0;
			FailMacError(err);
		}
	}
	catch(...)
	{
		if (dataRefNum) ::FSClose(dataRefNum);
		if (rsrcRefNum) ::FSClose(rsrcRefNum);
		((SFileSysRef *)inRef)->refNum = ((SFileSysRef *)inRef)->resRefNum = 0;
		UMemory::Dispose((TPtr)ref);
		throw;
	}
	
	((SFileSysRef *)inRef)->refNum = dataRefNum;
	((SFileSysRef *)inRef)->resRefNum = rsrcRefNum;
	((SFileSysRef *)inRef)->flatData = (TPtr)ref;
}

void UFileSys::ResumeFlatten(TFSRefObj* inRef, const void *inResumeData, Uint32 inDataSize)
{
	Require(inRef && inDataSize >= sizeof(SFlatFileResumeData));
	SFlatFileResumeData *rd;
	SFileFlatten *ref;
	Uint16 i, n;
	
	rd = (SFlatFileResumeData *)inResumeData;
	
	if (rd->format != 'RFLT') Fail(errorType_Misc, error_FormatUnknown);
	if (rd->version != 1) Fail(errorType_Misc, error_VersionUnknown);

	StartFlatten(inRef);
	ref = (SFileFlatten *)((SFileSysRef *)inRef)->flatData;
	
	n = rd->count;
	
	for (i=0; i<n; i++)
	{
		switch (rd->forkInfo[i].fork)
		{
			case 'DATA':
				ref->dataResumeSize = rd->forkInfo[i].dataSize;
				if (ref->dataResumeSize >= ref->dataHdr.dataSize)
					ref->dataHdr.dataSize = 0;
				else
					ref->dataHdr.dataSize -= ref->dataResumeSize;
				break;
				
			case 'MACR':
				ref->rsrcResumeSize = rd->forkInfo[i].dataSize;
				if (ref->rsrcResumeSize >= ref->rsrcHdr.dataSize)
					ref->rsrcHdr.dataSize = 0;
				else
					ref->rsrcHdr.dataSize -= ref->rsrcResumeSize;
				break;
		}
	}
}

void UFileSys::StopFlatten(TFSRefObj* inRef)
{
	if (inRef && ((SFileSysRef *)inRef)->flatData)
		Close(inRef);
}

Uint32 UFileSys::GetFlattenSize(TFSRefObj* inRef)
{
	Require(inRef);
	if (((SFileSysRef *)inRef)->flatData == nil) Fail(errorType_Misc, error_Protocol);
	SFileFlatten *ref = (SFileFlatten *)((SFileSysRef *)inRef)->flatData;
	
	return sizeof(SFlatFileHeader) + (sizeof(SFlatFileForkHeader) * 3)
		+ ref->infoHdr.dataSize + ref->dataHdr.dataSize + ref->rsrcHdr.dataSize;
}

Uint32 UFileSys::ProcessFlatten(TFSRefObj* inRef, void *outData, Uint32 inMaxSize)
{
	void *p;
	Uint32 totalSize, s, resumeSize;
	Uint32 bytesWritten = 0;
	Int16 refNum;
	
	Require(inRef);
	if (((SFileSysRef *)inRef)->flatData == nil) Fail(errorType_Misc, error_Protocol);
	SFileFlatten *ref = (SFileFlatten *)((SFileSysRef *)inRef)->flatData;

	/*
	 * Determine what and how we're going to write to the output buffer
	 */
determineAction:
	switch (ref->stage)
	{
		case 0:
			p = &ref->hdr;
			totalSize = sizeof(SFlatFileHeader);
			goto writeBuffer;
		
		case 1:
			p = &ref->infoHdr;
			totalSize = sizeof(SFlatFileForkHeader);
			goto writeBuffer;
		
		case 2:
			p = &ref->info;
			totalSize = ref->infoHdr.dataSize;
			goto writeBuffer;
		
		case 3:
			p = &ref->dataHdr;
			totalSize = sizeof(SFlatFileForkHeader);
			goto writeBuffer;
		
		case 4:
			totalSize = ref->dataHdr.dataSize;
			refNum = ((SFileSysRef *)inRef)->refNum;		// data fork
			resumeSize = ref->dataResumeSize;
			goto writeFile;
		
		case 5:
			p = &ref->rsrcHdr;
			totalSize = sizeof(SFlatFileForkHeader);
			goto writeBuffer;
		
		case 6:
			totalSize = ref->rsrcHdr.dataSize;
			refNum = ((SFileSysRef *)inRef)->resRefNum;	// resource fork
			resumeSize = ref->rsrcResumeSize;
			goto writeFile;
		
		default:
			return bytesWritten;
	}
	
	/*
	 * Write the data from file <refNum> of <totalSize> bytes
	 */
writeFile:
	s = totalSize - ref->pos;
	
	if (s > inMaxSize) s = inMaxSize;
	
	_FSRead(refNum, resumeSize + ref->pos, outData, s);
	
	ref->pos += s;
	
	if (ref->pos >= totalSize)
	{
		ref->stage++;
		ref->pos = 0;
	}

	goto continueOutput;

	/*
	 * Write the data at <p> of <totalSize> bytes
	 */
writeBuffer:
	s = totalSize - ref->pos;
	
	if (s > inMaxSize) s = inMaxSize;
	
	::BlockMoveData(BPTR(p) + ref->pos, outData, s);
	
	ref->pos += s;
	
	if (ref->pos >= totalSize)
	{
		ref->stage++;
		ref->pos = 0;
	}

	goto continueOutput;
	
	/*
	 * If there's still space in the buffer, keep writing
	 */
continueOutput:
	bytesWritten += s;
	
	if (s < inMaxSize)
	{
		outData = BPTR(outData) + s;
		inMaxSize -= s;
		goto determineAction;
	}
	
	return bytesWritten;
}

bool UFileSys::IsFlattening(TFSRefObj* inRef)
{
	if (inRef) return ((SFileSysRef *)inRef)->flatData != nil;
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

struct SFileUnflatten {
	Uint32 stage;
	Uint32 pos;
	Uint32 dataResumeSize, rsrcResumeSize;
	Int16 dataRefNum;
	Int16 rsrcRefNum;
	Uint32 infoSize;
	SFlatFileHeader hdr;
	SFlatFileForkHeader forkHdr;
	SFlatFileInfoFork info;
	Uint8 infoNameBuf[520];
	bool gotInfoData;
};

void UFileSys::StartUnflatten(TFSRefObj* inRef)
{
	Int16 dataRefNum, rsrcRefNum;
	OSErr err;

	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, error_Protocol);

	dataRefNum = rsrcRefNum = 0;
	SFileUnflatten *ref = (SFileUnflatten *)UMemory::NewClear(sizeof(SFileUnflatten));
	
	try
	{
		err = ::FSpOpenAware(&((SFileSysRef *)inRef)->spec, dmWrDenyRdWr, &dataRefNum);
		if (err)
		{
			dataRefNum = 0;		// sometimes sets it even if error
			FailMacError(err);
		}
		
		err = ::FSpOpenRFAware(&((SFileSysRef *)inRef)->spec, dmWrDenyRdWr, &rsrcRefNum);
		if (err)
		{
			rsrcRefNum = 0;
			FailMacError(err);
		}
	}
	catch(...)
	{
		if (dataRefNum) ::FSClose(dataRefNum);
		if (rsrcRefNum) ::FSClose(rsrcRefNum);
		((SFileSysRef *)inRef)->refNum = ((SFileSysRef *)inRef)->resRefNum = 0;
		UMemory::Dispose((TPtr)ref);
		throw;
	}
	
	((SFileSysRef *)inRef)->refNum = dataRefNum;
	((SFileSysRef *)inRef)->resRefNum = rsrcRefNum;
	((SFileSysRef *)inRef)->unflatData = (TPtr)ref;
}

THdl UFileSys::ResumeUnflatten(TFSRefObj* inRef)
{
	Int16 dataRefNum, rsrcRefNum;
	OSErr err;
	Int32 eof;
	THdl h = nil;

	Require(inRef);
	if (((SFileSysRef *)inRef)->refNum) Fail(errorType_Misc, error_Protocol);

	dataRefNum = rsrcRefNum = 0;
	SFileUnflatten *ref = (SFileUnflatten *)UMemory::NewClear(sizeof(SFileUnflatten));
	
	try
	{
		err = ::FSpOpenAware(&((SFileSysRef *)inRef)->spec, dmWrDenyRdWr, &dataRefNum);
		if (err)
		{
			dataRefNum = 0;
			FailMacError(err);
		}
		err = ::FSpOpenRFAware(&((SFileSysRef *)inRef)->spec, dmWrDenyRdWr, &rsrcRefNum);
		if (err)
		{
			rsrcRefNum = 0;
			FailMacError(err);
		}
		
		h = UMemory::NewHandleClear(sizeof(SFlatFileResumeData) + (sizeof(SFlatFileResumeEntry)*2));
		SFlatFileResumeData *rd;
		StHandleLocker lock(h, (void*&)rd);
		
		rd->format = 'RFLT';
		rd->version = 1;
		rd->count = 2;
		
		CheckMacError(::GetEOF(dataRefNum, &eof));
		rd->forkInfo[0].fork = 'DATA';
		rd->forkInfo[0].dataSize = eof;
		ref->dataResumeSize = eof;
		
		CheckMacError(::GetEOF(rsrcRefNum, &eof));
		rd->forkInfo[1].fork = 'MACR';
		rd->forkInfo[1].dataSize = eof;
		ref->rsrcResumeSize = eof;
	}
	catch(...)
	{
		if (dataRefNum) ::FSClose(dataRefNum);
		if (rsrcRefNum) ::FSClose(rsrcRefNum);
		((SFileSysRef *)inRef)->refNum = ((SFileSysRef *)inRef)->resRefNum = 0;
		UMemory::Dispose(h);
		UMemory::Dispose((TPtr)ref);
		throw;
	}
	
	((SFileSysRef *)inRef)->refNum = dataRefNum;
	((SFileSysRef *)inRef)->resRefNum = rsrcRefNum;
	((SFileSysRef *)inRef)->unflatData = (TPtr)ref;
	
	return h;
}

void UFileSys::StopUnflatten(TFSRefObj* inRef)
{
	if (inRef && ((SFileSysRef *)inRef)->unflatData)
		Close(inRef);
}

bool UFileSys::ProcessUnflatten(TFSRefObj* inRef, const void *inData, Uint32 inDataSize)
{
	Uint32 totalSize, s, resumeSize;
	void *p;
	Int16 refNum;
	
	Require(inRef);
	if (((SFileSysRef *)inRef)->unflatData == nil) Fail(errorType_Misc, error_Protocol);
	SFileUnflatten *ref = (SFileUnflatten *)((SFileSysRef *)inRef)->unflatData;

	// this is not very good - we should really have a better way of determining when we're done
	if (inDataSize == 0)
	{
		if (ref->gotInfoData)
		{
			_FSUpdateSpec(((SFileSysRef *)inRef));
			_FSSetFlatFileInfo(&((SFileSysRef *)inRef)->spec, ref->info, ref->infoSize);
		}
		return false;
	}
	
	/*
	 * Determine how we're going to process the supplied data
	 */
determineAction:
	switch (ref->stage)
	{
		case 0:
			p = &ref->hdr;
			totalSize = sizeof(SFlatFileHeader);
			goto readBuffer;
		
		case 1:
			if (ref->hdr.format != 'FILP') Fail(errorType_Misc, error_FormatUnknown);
			if (ref->hdr.version != 1) Fail(errorType_Misc, error_VersionUnknown);
			goto nextStage;
		
		case 2:
			p = &ref->forkHdr;
			totalSize = sizeof(SFlatFileForkHeader);
			goto readBuffer;

		case 3:
			if (ref->forkHdr.compType != 0)
			{
				DebugBreak("UFileSys::ProcessUnflatten - unknown compression type");
				Fail(errorType_Misc, error_FormatUnknown);
			}
			goto nextStage;

		case 4:
			totalSize = ref->forkHdr.dataSize;
			switch (ref->forkHdr.forkType)
			{
				case 'INFO':
					if (totalSize > sizeof(SFlatFileInfoFork) + sizeof(ref->infoNameBuf))
						totalSize = sizeof(SFlatFileInfoFork) + sizeof(ref->infoNameBuf);
					ref->gotInfoData = true;
					ref->infoSize = totalSize;
					p = &ref->info;
					goto readBuffer;
				
				case 'DATA':
					refNum = ((SFileSysRef *)inRef)->refNum;		// data fork
					resumeSize = ref->dataResumeSize;
					goto readFile;
				
				case 'MACR':
					refNum = ((SFileSysRef *)inRef)->resRefNum;	// resource fork
					resumeSize = ref->rsrcResumeSize;
					goto readFile;
				
				default:
					goto skipData;
			}

		case 5:
			if (inDataSize != 0)
			{
				ref->stage = 2;			// process another fork
				goto determineAction;
			}
			
		default:
			if (ref->gotInfoData)
			{
				_FSUpdateSpec(((SFileSysRef *)inRef));
				_FSSetFlatFileInfo(&((SFileSysRef *)inRef)->spec, ref->info, ref->infoSize);
			}
			return false;
	}

	
	/*
	 * Read <totalSize> bytes and write to buffer <p>
	 */
readBuffer:
	s = totalSize - ref->pos;
	
	if (s > inDataSize) s = inDataSize;
	
	::BlockMoveData(inData, BPTR(p) + ref->pos, s);
	
	ref->pos += s;
	
	if (ref->pos >= totalSize)
	{
		ref->stage++;
		ref->pos = 0;
	}
	
	goto continueProcess;
	
	
	/*
	 * Read <totalSize> bytes and write to file <refNum>
	 */
readFile:
	s = totalSize - ref->pos;
	
	if (s > inDataSize) s = inDataSize;
	
	// ******* use Allocate() ?
	_FSWrite(refNum, resumeSize + ref->pos, inData, s);
	
	ref->pos += s;
	
	if (ref->pos >= totalSize)
	{
		ref->stage++;
		ref->pos = 0;
	}

	goto continueProcess;
	
	
	/*
	 * Ignore and move past <totalSize> bytes from the input stream
	 */
skipData:
	s = totalSize - ref->pos;
	
	if (s > inDataSize) s = inDataSize;
	
	ref->pos += s;
	
	if (ref->pos >= totalSize)
	{
		ref->stage++;
		ref->pos = 0;
	}
	
	goto continueProcess;
	
	
	/*
	 * Jump to the next stage
	 */
nextStage:
	s = 0;
	ref->stage++;
	ref->pos = 0;
	goto continueProcess;
	
	
	/*
	 * If there's still unread data, keep reading
	 */
continueProcess:
	if (s < inDataSize)
	{
		inData = BPTR(inData) + s;
		inDataSize -= s;
		goto determineAction;
	}
	
	return true;
}

bool UFileSys::IsUnflattening(TFSRefObj* inRef)
{
	if (inRef) return ((SFileSysRef *)inRef)->unflatData != nil;
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * When the file is closed, the spec points to something that may or
 * may not exist.  When open, we want to track the file even if it
 * moves, so we call this function to update the spec with the
 * current location.
 */
static void _FSUpdateSpec(SFileSysRef *inRef)
{
	if (inRef->refNum)
	{
		FCBPBRec pb;
		
		pb.ioNamePtr = inRef->spec.name;
		pb.ioVRefNum = 0;
		pb.ioRefNum = inRef->refNum;
		pb.ioFCBIndx = 0;
		
		CheckMacError(PBGetFCBInfoSync(&pb));
		
		inRef->spec.vRefNum = pb.ioFCBVRefNum;
		inRef->spec.parID = pb.ioFCBParID;
	}
}

static Int32 _FSGetDirID(Int16 vRefNum, Int32 dirID, const Uint8 *name, bool *outIsDir)
{
	CInfoPBRec pb;
	Str63 tempName;
	bool isDir;

	if (name == nil || name[0] == 0)
	{
		tempName[0] = 0;
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioFDirIndex = -1;		// use ioDirID
	}
	else
	{
		pb.hFileInfo.ioNamePtr = (StringPtr)name;
		pb.hFileInfo.ioFDirIndex = 0;		// use ioNamePtr and ioDirID
	}
	
	pb.hFileInfo.ioVRefNum = vRefNum;
	pb.hFileInfo.ioDirID = dirID;
	
	CheckMacError(PBGetCatInfoSync(&pb));
	
	isDir = (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0;
	if (outIsDir) *outIsDir = isDir;
	
	return isDir ? pb.dirInfo.ioDrDirID : pb.hFileInfo.ioFlParID;
}

static Int16 _FSGetVRefNum(const Uint8 *pathname, Int16 vRefNum)
{
	HParamBlockRec pb;
	Str255 tempPathname;

	pb.volumeParam.ioVRefNum = vRefNum;
	if (pathname == nil)
	{
		pb.volumeParam.ioNamePtr = nil;
		pb.volumeParam.ioVolIndex = 0;		// use ioVRefNum only
	}
	else
	{
		::BlockMoveData(pathname, tempPathname, pathname[0] + 1);	// make a copy of the string and
		pb.volumeParam.ioNamePtr = (StringPtr)tempPathname;		// use the copy so original isn't trashed
		pb.volumeParam.ioVolIndex = -1;							// use ioNamePtr/ioVRefNum combination
	}
	
	CheckMacError(::PBHGetVInfoSync(&pb));
	
	return pb.volumeParam.ioVRefNum;
}

static bool _FSSearchVolForApp(Int16 vol, Uint32 sig, FSSpec& outSpec)
{
	DTPBRec pb;
	Int16 err;
	
	// get the real vol ref num
	vol = _FSGetVRefNum(nil, vol);
	
	// get ref num of desktop database
	pb.ioVRefNum = vol;
	pb.ioNamePtr = nil;
	CheckMacError(PBDTGetPath(&pb));
	
	// search desktop database for application
	pb.ioNamePtr = outSpec.name;
	pb.ioIndex = 1;
	pb.ioFileCreator = sig;
	err = PBDTGetAPPLSync(&pb);
	
	// check if not found
	if (err == afpItemNotFound || err == fnfErr) return false;
	CheckMacError(err);
	
	// found, make SFileSpec
	outSpec.vRefNum = vol;
	outSpec.parID = pb.ioAPPLParID;
	return true;
}

bool _FSGetProgramSpec(Uint32 inSig, FSSpec& outSpec)
{
	// should really search other volumes too
	return _FSSearchVolForApp(0, inSig, outSpec);
}

void _FSLaunchApp(const FSSpec *inSpec, bool inBringToFront)
{
	LaunchParamBlockRec pb;
		
	pb.launchBlockID = extendedBlock;
	pb.launchEPBLength = extendedBlockLen;
	pb.launchFileFlags = 0;
	pb.launchControlFlags = launchContinue + launchNoFileFlags;
	pb.launchAppSpec = (FSSpec *)inSpec;
	pb.launchAppParameters = nil;
	
	if (!inBringToFront) pb.launchControlFlags += launchDontSwitch;
	
	CheckMacError(::LaunchApplication(&pb));
}

static Uint32 _FSRead(Int16 inRefNum, Uint32 inOffset, void *outData, Uint32 inMaxSize)
{
	ParamBlockRec pb;
	OSErr err;
	
	pb.ioParam.ioRefNum = inRefNum;
	pb.ioParam.ioBuffer = (Ptr)outData;
	pb.ioParam.ioReqCount = (Int32)inMaxSize;
	pb.ioParam.ioActCount = 0;
	pb.ioParam.ioPosMode = fsFromStart;		// add 0x0020 to disable caching
	pb.ioParam.ioPosOffset = inOffset;
	
	err = PBReadSync(&pb);

	if (err && err != eofErr) FailMacError(err);

	return pb.ioParam.ioActCount;
}

static Uint32 _FSWrite(Int16 inRefNum, Uint32 inOffset, const void *inData, Uint32 inDataSize)
{
	ParamBlockRec pb;

	pb.ioParam.ioRefNum = inRefNum;
	pb.ioParam.ioBuffer = (Ptr)inData;
	pb.ioParam.ioReqCount = (Int32)inDataSize;
	pb.ioParam.ioPosMode = fsFromStart;		// add 0x0020 to disable caching
	pb.ioParam.ioPosOffset = inOffset;
	
	CheckMacError(PBWriteSync(&pb));

	return pb.ioParam.ioActCount;
}

/*
Returns false if not found.
If no names in path, outFolder is start folder (inRoot).
The path is formatted like this:

	Uint16 count;
	struct {
		Uint16 script;
		Uint8 len;
		Uint8 name[len];	// name of folder/directory
	} dir[count];

If count is zero, target is root/starting folder.
*/
static bool _FSPathDataToFolder(const FSSpec& inRoot, const void *inPath, Uint32 inPathSize, FSSpec& outFolder)
{
	CInfoPBRec pb;
	Str255 tempName;
	Uint8 *datap = (Uint8 *)inPath;
	Uint8 *ep = datap + inPathSize;
	Uint16 len, cnt;
	Boolean isFolder, wasAliased;
	FSSpec spec;
	
	// get info for root directory
	::BlockMoveData(inRoot.name, tempName, inRoot.name[0]+1);
	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = inRoot.vRefNum;
	pb.hFileInfo.ioDirID = inRoot.parID;
	if ((PBGetCatInfoSync(&pb) != noErr) || !(pb.hFileInfo.ioFlAttrib & ioDirMask))
		return false;
	
	// get number of folders
	cnt = (inPathSize < 6) ? 0 : *(Uint16 *)datap;
	datap += sizeof(Uint16);

	// get info for each directory along path
	while (cnt--)
	{
		// move to name of directory
		datap += sizeof(Uint16);
		if (datap >= ep) break;				// corruption check
		len = *(Uint8 *)datap;
		if (len == 0 || (datap + 1 + len) > ep) break;	// corruption check
		
		// store name that we're going to get info on
		::BlockMoveData(datap, tempName, datap[0]+1);
		_FSStripName(tempName);
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioFDirIndex = 0;
		if (tempName[0] == 0) return false;
		
		// get info for this directory
		if (PBGetCatInfoSync(&pb) != noErr)
			return false;
		
		// if not a directory, check if alias
		if (!(pb.hFileInfo.ioFlAttrib & ioDirMask))
		{
			// make spec of this object
			spec.vRefNum = pb.hFileInfo.ioVRefNum;
			spec.parID = pb.hFileInfo.ioFlParID;
			::BlockMoveData(pb.hFileInfo.ioNamePtr, spec.name, pb.hFileInfo.ioNamePtr[0]+1);
			
			// check for alias
			if (_FSResolveAliasFile(&spec, &isFolder, &wasAliased) != noErr || !isFolder)
				return false;
			
			// get directory ID
			pb.hFileInfo.ioNamePtr = spec.name;
			pb.hFileInfo.ioVRefNum = spec.vRefNum;
			pb.hFileInfo.ioDirID = spec.parID;
			pb.hFileInfo.ioFDirIndex = (spec.name[0] == 0) ? -1 : 0;
			if ((PBGetCatInfoSync(&pb) != noErr) || !(pb.hFileInfo.ioFlAttrib & ioDirMask))
				return false;
		}
		
		// next directory
		datap += sizeof(Uint8) + len;
	}
	
	// output target directory
	if (::FSMakeFSSpec(pb.hFileInfo.ioVRefNum, pb.hFileInfo.ioDirID, "\p", &outFolder) != noErr)
		return false;
	
	// successfully located target directory
	return true;
}

// names in path are separated by \r
static bool _FSPathStringToFolder(const FSSpec& inRoot, const Uint8 *inPath, FSSpec& outFolder)
{
	CInfoPBRec pb;
	Str255 tempName;
	Uint8 *p, *ep, *tp;
	Uint16 s;
	Boolean isFolder, wasAliased;
	FSSpec spec;
		
	// get info for root directory
	::BlockMoveData(inRoot.name, tempName, inRoot.name[0]+1);
	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = inRoot.vRefNum;
	pb.hFileInfo.ioDirID = inRoot.parID;
	if ((PBGetCatInfoSync(&pb) != noErr) || !(pb.hFileInfo.ioFlAttrib & ioDirMask))
		return false;
	
	// get ptrs to data
	s = inPath[0];
	if (s && inPath[inPath[0]] == '\r') s--;	// ignore \r at end
	p = BPTR(inPath)+1;
	ep = p + s;
	if (s && *p == '\r') p++;					// ignore \r at start

	// get info for each directory along path
	while (p < ep)
	{
		// determine size of name
		tp = UMemory::SearchByte('\r', p, ep - p);
		if (tp == nil) tp = ep;
		s = tp - p;
		if (s == 0) return false;
		
		// store name that we're going to get info on
		::BlockMoveData(p, tempName+1, s);
		tempName[0] = s;
		_FSStripName(tempName);
		pb.hFileInfo.ioNamePtr = tempName;
		pb.hFileInfo.ioFDirIndex = 0;
		if (tempName[0] == 0) return false;
		
		// get info for this directory
		if (PBGetCatInfoSync(&pb) != noErr)
			return false;
		
		// if not a directory, check if alias
		if (!(pb.hFileInfo.ioFlAttrib & ioDirMask))
		{
			// make spec of this object
			spec.vRefNum = pb.hFileInfo.ioVRefNum;
			spec.parID = pb.hFileInfo.ioFlParID;
			::BlockMoveData(pb.hFileInfo.ioNamePtr, spec.name, pb.hFileInfo.ioNamePtr[0]+1);
			
			// check for alias
			if (_FSResolveAliasFile(&spec, &isFolder, &wasAliased) != noErr || !isFolder)
				return false;
			
			// get directory ID
			pb.hFileInfo.ioNamePtr = spec.name;
			pb.hFileInfo.ioVRefNum = spec.vRefNum;
			pb.hFileInfo.ioDirID = spec.parID;
			pb.hFileInfo.ioFDirIndex = (spec.name[0] == 0) ? -1 : 0;
			if ((PBGetCatInfoSync(&pb) != noErr) || !(pb.hFileInfo.ioFlAttrib & ioDirMask))
				return false;
		}
		
		// next directory
		p += s;				// move past name
		p++;				// move past \r
	}
	
	// output target directory
	if (::FSMakeFSSpec(pb.hFileInfo.ioVRefNum, pb.hFileInfo.ioDirID, "\p", &outFolder) != noErr)
		return false;
	
	// successfully located target directory
	return true;
}

// returns whether or not exists (spec is still valid either way).  
// fails if can't locate inStartFolder or path.  Does NOT resolve aliases 
// for target (but does for path).
static bool _FSMakeSpec(const FSSpec *inStartFolder, const Uint8 *inPathString, const void *inPathData, Uint32 inPathDataSize, const Uint8 *inName, FSSpec& outSpec)
{
	FSSpec startDir;
	Int32 dirID;
	Int16 vRefNum;
	bool isDir;
	OSErr err;
	Str255 name, path;
	Uint8 *p;

	// get name
	if (inName == nil)
		name[0] = 0;
	else
	{
		::BlockMoveData(inName, name, inName[0]+1);
		_FSStripName(name);
	}
	
	// get path string
	if (inPathString && inPathString[0])
	{
		::BlockMoveData(inPathString, path, inPathString[0]+1);
		if (path[path[0]] == '\r')	// remove any \r from the end of the path string
			path[0]--;
	}
	else
		path[0] = 0;
	
	// determine starting directory
	if (inStartFolder == (FSSpec *)kProgramFolder)
	{
		// special case optimization
		if (path[0] == 0 && inPathData == nil && name[0])
		{
			err = ::FSMakeFSSpec(0, 0, name, &outSpec);
			
			if (err == noErr)
				return true;
			else if (err == fnfErr)
				return false;
			else
				FailMacError(err);
		}
		
		CheckMacError(::FSMakeFSSpec(0, 0, nil, &startDir));
	}
	else if (inStartFolder == (FSSpec *)kRootFolderHL)
	{
		CheckMacError(::FSMakeFSSpec(-1, 0, nil, &startDir));
	}
	else if (inStartFolder == (FSSpec *)kDesktopFolder)
	{
		CheckMacError(::FindFolder(kOnSystemDisk, kDesktopFolderType, kCreateFolder, &vRefNum, &dirID));
		CheckMacError(::FSMakeFSSpec(vRefNum, dirID, nil, &startDir));
	}
	else if (inStartFolder == (FSSpec *)kTempFolder)
	{
		CheckMacError(::FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &vRefNum, &dirID));
		CheckMacError(::FSMakeFSSpec(vRefNum, dirID, nil, &startDir));
	}
	else if (inStartFolder == (FSSpec *)kPrefsFolder)
	{
		CheckMacError(::FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder, &vRefNum, &dirID));
		CheckMacError(::FSMakeFSSpec(vRefNum, dirID, nil, &startDir));
	}
	else
		startDir = *inStartFolder;
	
	// follow path (if any)
	if (path[0])			// if have a path string
	{
		if (name[0] == 0)	// if no name, then target is last item in path
		{
			p = UMemory::SearchByteBackwards('\r', path+1, path[0]);
			
			if (p == nil)
			{
				::BlockMoveData(path, name, path[0]+1);
			}
			else
			{
				p++;
				name[0] = (path + 1 + path[0]) - p;
				::BlockMoveData(p, name+1, name[0]);
				
				path[0] -= name[0];
				path[0]--;
				
				if (!_FSPathStringToFolder(startDir, path, startDir))
					Fail(errorType_FileSys, fsError_NoSuchFolder);
			}
			
			_FSStripName(name);
		}
		else
		{
			if (!_FSPathStringToFolder(startDir, path, startDir))
				Fail(errorType_FileSys, fsError_NoSuchFolder);
		}
	}
	else if (inPathData && *(Uint16 *)inPathData)
	{
		if (name[0] == 0)	// if no name, then target is last item in path
		{
			name[0] = UFileSys::GetPathTargetName(inPathData, max_Uint16, name+1, sizeof(name)-1);
			_FSStripName(name);
			
			if (*(Uint16 *)inPathData != 1)
			{
				(*(Uint16 *)inPathData)--;
				try
				{
					if (!_FSPathDataToFolder(startDir, inPathData, inPathDataSize, startDir))
						Fail(errorType_FileSys, fsError_NoSuchFolder);
				}
				catch(...)
				{
					(*(Uint16 *)inPathData)++;	// inPathData is const, so restore original value
					throw;
				}
				(*(Uint16 *)inPathData)++;
			}
		}
		else
		{
			if (!_FSPathDataToFolder(startDir, inPathData, inPathDataSize, startDir))
				Fail(errorType_FileSys, fsError_NoSuchFolder);
		}
	}
	
	// output target spec
	if (name[0] == 0)
	{
		// target is starting directory
		outSpec = startDir;
		return true;
	}
	else
	{
		// target is "name" inside starting directory
		dirID = _FSGetDirID(startDir.vRefNum, startDir.parID, startDir.name, &isDir);
		if (!isDir) FailMacError(dirNFErr);
		err = ::FSMakeFSSpec(startDir.vRefNum, dirID, name, &outSpec);
		if (err == noErr)
			return true;
		else if (name[0] > 32)
		{
		    // try to shorten the name by creating a short signature, but preserve extension
		    UInt32 chk = UMemory::Checksum(name+1, name[0], name[0]);
		    Uint8 * ext = UMemory::SearchByteBackwards('.', name+1, name[0]);
		    if (ext != nil)
		    {
		        UInt32 extLen = name+1+name[0] - ext;
		        if (extLen > 16)
		        	extLen = 16;
		    	UMemory::Copy(name+1+32-extLen, ext, extLen);
		    	UText::Format(name+1+27-extLen,5, "#%04X", chk);
		    }
		    else
		    	UText::Format(name+1+27,5, "#%04X", chk);
		    
		    name[0] = 32; //truncate to 32 chars
		    
			// try again with short name
			err = ::FSMakeFSSpec(startDir.vRefNum, dirID, name, &outSpec);
			if (err == noErr)
				return true;
		}
		if (err == fnfErr)	   // file not found
			return false;
		else 
			FailMacError(err);
	}
	
	return false;
}

// remove illegal characters from file name
static void _FSStripName(Uint8 *ioName)
{
	if (!ioName || !ioName[0])
		return;
		
	_FSStripName(ioName + 1, ioName[0]);
}

// remove illegal characters from file name
static void _FSStripName(void *ioName, Uint32 inSize)
{
	if (!ioName || !inSize)
		return;

	Uint8 *p = (Uint8 *)ioName;
	Uint32 n = inSize;

	// check if we are running under MacOS X
	if (_GetSystemVersion() >= 0x0A00)
	{	
		// first char can't be period
		if (*p == '.') 
			*p = '-';
		// if the name is one char then it must be a nice char
		if (inSize == 1) 
		{
			switch (*p)
			{
				case '?':
				case '{':
				case '}':
				case '[':
				case ']':
				case '(':
				case ')':
				case '~':
					*p = '-';
					break;
			};
		}
		// some chars are not allowed at all
		while (n--)
		{
			switch (*p)
			{
				case ':':
				//case ';':
				case '/':
				//case '\'':
			
					*p = '-';
					break;
			};
			p++;
		}
	}
	else
	{
		while (n--)
		{
			if (*p == ':') 
				*p = '-';
			p++;
		}
	}
}

// must return error code rather than failing
static OSErr _FSIsAliasFile(const FSSpec *inSpec, Boolean *outIsAlias, Boolean *outIsFolder)
{
	CInfoPBRec pb;
	OSErr err;
	Str63 tempName;

	if (inSpec == nil || outIsAlias == nil || outIsFolder == nil)
		return paramErr;

	*outIsAlias = *outIsFolder = false;
	
	::BlockMoveData(inSpec->name, tempName, inSpec->name[0]+1);

	pb.hFileInfo.ioCompletion = nil;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioDirID = inSpec->parID;
	pb.hFileInfo.ioFVersNum = 0;			// MFS compatibility, see TN #204
	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;

	err = PBGetCatInfoSync(&pb);

	if (err == noErr)
	{
		if (pb.hFileInfo.ioFlAttrib & ioDirMask)				// if directory
			*outIsFolder = true;
		else if (pb.hFileInfo.ioFlFndrInfo.fdFlags & 0x8000)	// if alias
			*outIsAlias = true;
	}

	return err;
}

static bool _FSParentExists(const FSSpec *inSpec)
{
	CInfoPBRec pb;
	Str63 name;
	
	name[0] = 0;
	pb.hFileInfo.ioNamePtr = name;
	pb.hFileInfo.ioVRefNum = inSpec->vRefNum;
	pb.hFileInfo.ioDirID = inSpec->parID;
	pb.hFileInfo.ioFVersNum = 0;
	pb.hFileInfo.ioFDirIndex = -1;	// MUST be -1 because name is zero-length

	return (PBGetCatInfoSync(&pb) == noErr);
}

// must return error code rather than failing
static OSErr _FSResolveAliasFileMountOption(FSSpec *fileFSSpec, Boolean resolveAliasChains, Boolean *targetIsFolder, Boolean *wasAliased, Boolean mountRemoteVols)
{
	#define MAXCHAINS	10	// maximum number of aliases to resolve before giving up

	short myResRefNum;
	Handle alisHandle;
	FSSpec initFSSpec;
	Boolean updateFlag, foundFlag, wasAliasedTemp, specChangedFlag;
	short chainCount;
	OSErr retCode;

	if (fileFSSpec == nil || targetIsFolder == nil || wasAliased == nil)
		return paramErr;

	initFSSpec = *fileFSSpec;	// so FSSpec can be restored in case of error
	chainCount = MAXCHAINS;		// circular alias chain protection
	myResRefNum = -1;			// resource file not open

	*targetIsFolder = foundFlag = specChangedFlag = false;

	// loop through chain of alias files
	do {
		chainCount--;

		// check if FSSpec is an alias file or a directory
		// note that targetIsFolder => NOT wasAliased
		retCode = _FSIsAliasFile(fileFSSpec, wasAliased, targetIsFolder);
		if (retCode != noErr || !(*wasAliased)) break;

		// get the resource file reference number
		myResRefNum = FSpOpenResFile(fileFSSpec, fsCurPerm);
		retCode = ResError();
		if (myResRefNum == -1) break;

		// the first 'alis' resource in the file is the appropriate alias
		alisHandle = Get1IndResource(rAliasType, 1); retCode = ResError();
		if (alisHandle == nil) break;

		// load the resource explicitly in case SetResLoad(FALSE)
		LoadResource(alisHandle);
		retCode = ResError();
		if (retCode != noErr) break;
		retCode = FollowFinderAlias(fileFSSpec, (AliasHandle)alisHandle, mountRemoteVols, fileFSSpec, &updateFlag);

		// FollowFinderAlias returns nsvErr if volume not mounted
		if (retCode == noErr)
		{
			if (updateFlag)
			{
				// the resource in the alias file needs updating
				ChangedResource(alisHandle);
				WriteResource(alisHandle);
			}

			specChangedFlag = true;		// in case of error, restore file spec

			retCode = _FSIsAliasFile(fileFSSpec, &wasAliasedTemp, targetIsFolder);
			if (retCode == noErr)		// we're done unless it was an alias file and we're following a chain
				foundFlag = !(wasAliasedTemp && resolveAliasChains);
		}
		
		CloseResFile(myResRefNum);
		myResRefNum = -1;
	} while (retCode == noErr && chainCount > 0 && !foundFlag);

	// return file not found error for circular alias chains
	if (chainCount == 0 && !foundFlag)
		retCode = fnfErr;

	// if error occurred, close resource file and restore the original FSSpec
	if (myResRefNum != -1)
		CloseResFile(myResRefNum);
	if (retCode != noErr && specChangedFlag)
		*fileFSSpec = initFSSpec;
	
	return retCode;
}

// must return error code rather than failing
static OSErr _FSResolveAliasFile(FSSpec *ioSpec, Boolean *outIsFolder, Boolean *outWasAliased)
{
	// note if trying to mount a volume, and user clicks cancel, ResolveAliasFile returns userCanceledErr
	if (_FSAliasUI)
		return ::ResolveAliasFile(ioSpec, true, outIsFolder, outWasAliased);
	else
		return _FSResolveAliasFileMountOption(ioSpec, true, outIsFolder, outWasAliased, false);
}

// returns true if outResolvedSpec now points to a valid file or folder that is not an alias
// outOrigType is set to what type of FS item the spec was BEFORE it was resolved
static bool _FSResolveAliasAndGetType(const FSSpec *inSpec, FSSpec *outResolvedSpec, Uint16 *outOrigType)
{
	OSErr err;
	Boolean isFolder, wasAliased, isAlias;
	FSSpec origSpec;
	
	// grab a copy so inSpec and outResolvedSpec can be the same spec
	origSpec = *inSpec;
	*outResolvedSpec = origSpec;
	
	if (_FSAliasUI)
		err = ::ResolveAliasFile(outResolvedSpec, true, &isFolder, &wasAliased);
	else
		err = _FSResolveAliasFileMountOption(outResolvedSpec, true, &isFolder, &wasAliased, false);
		
	if (err)
	{
		// alias file doesn't exist, or alias could not be resolved, or some other error
		if (outOrigType)
		{
			_FSIsAliasFile(&origSpec, &isAlias, &isFolder);
			
			if (isAlias)	// will be false if error (such as origSpec doesn't exist)
				*outOrigType = fsItemType_UnattachedAlias;
			else			// if a non-alias file existed, ResolveAliasFile would have returned noErr
				*outOrigType = _FSParentExists(&origSpec) ? fsItemType_NonExistant : fsItemType_NonExistantParent;
		}
		return false;
	}
	
	if (outOrigType)
	{
		if (isFolder)
			*outOrigType = wasAliased ? fsItemType_FolderAlias : fsItemType_Folder;
		else
			*outOrigType = wasAliased ? fsItemType_FileAlias : fsItemType_File;
	}
	
	return true;
}

// returns true if you should return nil
static bool _FSDoNewOptions(FSSpec *ioSpec, Uint32 inOptions, Uint16 *outType)
{
	FSSpec resolvedSpec;
	Uint16 type;

	if (inOptions || outType)
	{
		// get resolved spec and ORIGINAL type (type for "spec")
		_FSResolveAliasAndGetType(ioSpec, &resolvedSpec, &type);
		
		if (inOptions & fsOption_ResolveAliases)
		{
			*ioSpec = resolvedSpec;
			if (type == fsItemType_FileAlias) type = fsItemType_File;
			else if (type == fsItemType_FolderAlias) type = fsItemType_Folder;
			else if (type == fsItemType_UnattachedAlias)
			{
				if (outType) *outType = fsItemType_UnattachedAlias;
				
				if (inOptions & fsOption_NilIfNonExistant)
					return true;
				else
					Fail(errorType_FileSys, fsError_CannotResolveAlias);
			}
		}

		// set outType even if return nil
		if (outType) *outType = type;

		if (type == fsItemType_NonExistantParent)
		{
			if (inOptions & fsOption_NilIfNonExistantParent)
				return true;
			else
				goto nonExistant;
		}
		else if (type == fsItemType_NonExistant)
		{
nonExistant:
			if (inOptions & fsOption_NilIfNonExistant)
				return true;
			else if (inOptions & fsOption_FailIfNonExistant)
			{
				if (inOptions & (fsOption_FailIfNotFile | fsOption_NilIfNotFile))			// if either option is on
					Fail(errorType_FileSys, fsError_NoSuchFile);
				else if (inOptions & (fsOption_FailIfNotFolder | fsOption_NilIfNotFolder))	// if either option is on
					Fail(errorType_FileSys, fsError_NoSuchFolder);
				else
					Fail(errorType_FileSys, fsError_NoSuchItem);
			}
		}
		else
		{
			if (inOptions & fsOption_NilIfExists)
				return true;
			else if (inOptions & fsOption_FailIfExists)
				Fail(errorType_FileSys, fsError_ItemAlreadyExists);

			if (inOptions & fsOption_NilIfNotFile)
			{
				if (type != fsItemType_File)
					return true;
			}
			else if (inOptions & fsOption_FailIfNotFile)
			{
				if (type != fsItemType_File)
					Fail(errorType_FileSys, fsError_FileExpected);
			}
			else if (inOptions & fsOption_NilIfNotFolder)
			{
				if (type != fsItemType_Folder)
					return true;
			}
			else if (inOptions & fsOption_FailIfNotFolder)
			{
				if (type != fsItemType_Folder)
					Fail(errorType_FileSys, fsError_FolderExpected);
			}
		}
	}
	
	return false;
}

TFSRefObj* _FSSpecToRef(const FSSpec& inSpec)
{
	SFileSysRef *ref = (SFileSysRef *)UMemory::NewClear(sizeof(SFileSysRef));
	ref->spec = inSpec;
	return (TFSRefObj*)ref;
}

TFSRefObj* _PathToRef(const void *inPath, Uint32 inPathSize)
{
	if (!inPath || inPathSize != sizeof(FSSpec))
		return nil;
	
	return _FSSpecToRef(*((FSSpec *)inPath));
}

static void _FSCreateAliasFile(const FSSpec *inFile, const FSSpec *inOriginal)
{
	FInfo fi;
	CInfoPBRec pb;
	Str63 tempName;
	AliasHandle halias = nil;
	OSErr err;
	Int16 refNum = -1;
	Int16 saveResFile = -1;
	
	// get info on the original file or folder
	::BlockMoveData(inOriginal->name, tempName, inOriginal->name[0]+1);
	pb.hFileInfo.ioFDirIndex = (tempName[0] == 0) ? -1 : 0;
	pb.hFileInfo.ioNamePtr = tempName;
	pb.hFileInfo.ioVRefNum = inOriginal->vRefNum;
	pb.hFileInfo.ioDirID = inOriginal->parID;
	CheckMacError(::PBGetCatInfoSync(&pb));

	// determine correct type/creator to use for alias
	if (pb.hFileInfo.ioFlAttrib & ioDirMask)			// if folder
	{
		fi.fdType = 'fdrp';
		fi.fdCreator = 'MACS';
	}
	else
	{
		fi.fdType = pb.hFileInfo.ioFlFndrInfo.fdType;
		fi.fdCreator = pb.hFileInfo.ioFlFndrInfo.fdCreator;
	}

	// create alias record
	CheckMacError(::NewAlias(nil, inOriginal, &halias));
	
	// create resource file to become the alias file
	::FSpCreateResFile(inFile, fi.fdCreator, fi.fdType, 0);
	err = ResError();
	if (err) goto bail;
	
	// save current resource file
	saveResFile = CurResFile();
	
	// open resource file
	refNum = FSpOpenResFile(inFile, fsRdWrPerm);
	if (refNum == -1)
	{
		err = ResError();
		goto bail;
	}
	
	// make resource file current
	UseResFile(refNum);
	
	// add alias resource to resource file
	AddResource((Handle)halias, 'alis', 0, "\p");
	err = ResError();
	if (err) goto bail;
	halias = nil;			// belongs to resource manager now
	
	// close resource file and restore current resource file
	CloseResFile(refNum);
	UseResFile(saveResFile);
	
	// mark the resource file as an alias file
	err = FSpGetFInfo(inFile, &fi);
	if (err) goto bail;
	fi.fdFlags |= 0x8000;	// turn isAlias on
	fi.fdFlags &= ~0x0100;	// turn isInitted off
	err = FSpSetFInfo(inFile, &fi);
	if (err) goto bail;
	
	// error handling
	return;
bail:
	if (halias) DisposeHandle((Handle)halias);
	if (refNum != -1)
	{
		CloseResFile(refNum);
		FSpDelete(inFile);
	}
	if (saveResFile != -1) UseResFile(saveResFile);
	FailMacError(err);
}

static void _FSForceFinderUpdate(const FSSpec *inFolder)
{
	// ***** this doesn't seem to work for files on the desktop, and anyway, it doesn't seem to be necessary under MacOS 8.1
	
	AEDesc ae = { typeNull, nil };
	AEDesc finderTarg = { typeNull, nil };
	AEDesc objSpec = { typeNull, nil };
	AEDesc reply = { typeNull, nil };
	
	ASSERT(inFolder);
	
	// create finder target
	Uint32 finderSig = 'MACS';
	if (::AECreateDesc(typeApplSignature, &finderSig, sizeof(finderSig), &finderTarg)) goto bail;
	
	// create "update" apple event
	if (::AECreateAppleEvent('fndr', 'fupd', &finderTarg, kAutoGenerateReturnID, kAnyTransactionID, &ae)) goto bail;
	if (::AECreateDesc(typeFSS, inFolder, sizeof(FSSpec), &objSpec)) goto bail;
	if (::AEPutParamDesc(&ae, keyDirectObject, &objSpec)) goto bail;

	// send the apple event
	::AESend(&ae, &reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, nil, nil);

	// clean up
	bail:
	if (ae.dataHandle) ::AEDisposeDesc(&ae);
	if (finderTarg.dataHandle) ::AEDisposeDesc(&finderTarg);
	if (objSpec.dataHandle) ::AEDisposeDesc(&objSpec);
	if (reply.dataHandle) ::AEDisposeDesc(&reply);
}

void _MIMEToMacTypeCode(const void *inText, Uint32 inSize, Uint32& outTypeCode, Uint32& outCreatorCode)
{
	// ******** can we use InternetConfig for this? yes!
	// if available, use IC, otherwise use these built-in translations
	
	outTypeCode = outCreatorCode = 0;

	switch (inSize)
	{
		case 9:
			if (UMemory::Equal(inText, "image/gif", 9))
			{	outTypeCode = 'GIFf';	outCreatorCode = 'GKON';	}
			else if (UMemory::Equal(inText, "audio/wav", 9))
			{	outTypeCode = 'WAVE';	outCreatorCode = 'TVOD';	}
			break;
		case 10:
			if (UMemory::Equal(inText, "text/plain", 10))
			{	outTypeCode = 'TEXT';	outCreatorCode = 'ttxt';	}
			else if (UMemory::Equal(inText, "image/pict", 10))
			{	outTypeCode = 'PICT';	outCreatorCode = 'ttxt';	}
			else if (UMemory::Equal(inText, "image/jpeg", 10))
			{	outTypeCode = 'JPEG';	outCreatorCode = 'GKON';	}
			else if (UMemory::Equal(inText, "image/tiff", 10))
			{	outTypeCode = 'TIFF';	outCreatorCode = 'GKON';	}
			else if (UMemory::Equal(inText, "audio/aiff", 10))
			{	outTypeCode = 'AIFF';	outCreatorCode = 'TVOD';	}
			else if (UMemory::Equal(inText, "video/mpeg", 10))
			{	outTypeCode = 'MPEG';	outCreatorCode = 'TVOD';	}
			break;
		case 15:
			if (UMemory::Equal(inText, "video/quicktime", 15))
			{	outTypeCode = 'MooV';	outCreatorCode = 'TVOD';	}
			break;
		case 16:
			if (UMemory::Equal(inText, "mactypecode/", 12))
			{
				outTypeCode = *(Uint32 *)(BPTR(inText)+12);
				outCreatorCode = 0;
			}
			break;
		case 20:
			if (UMemory::Equal(inText, "mactypecode/", 12))
			{
				outTypeCode = *(Uint32 *)(BPTR(inText)+12);
				outCreatorCode = *(Uint32 *)(BPTR(inText)+16);
			}
			break;
		case 21:
			if (UMemory::Equal(inText, "application/x-stuffit", 21))
			{	outTypeCode = 'SITD';	outCreatorCode = 'SIT!';	}
			break;
		case 23:
			if (UMemory::Equal(inText, "application/x-macbinary", 23))
			{	outTypeCode = 'BINA';	outCreatorCode = 'SITx';	}
			break;
		case 24:
			if (UMemory::Equal(inText, "application/mac-binhex40", 24))
			{	outTypeCode = 'BINA';	outCreatorCode = 'SITx';	}
			else if (UMemory::Equal(inText, "application/octet-stream", 24))
			{	outTypeCode = 'BINA';	outCreatorCode = 'hDmp';	}
			break;
	}
}

static void _FSReleaseNavServices()
{
	::NavUnload();
}

static bool _FSHasNavServices()
{
	return false;

#if USING_NAV_SERVICES
	/*
		state is one of:
			not inited 				= 0
			not available			= 1	
			available and inited	= 2
	*/
	
	static Uint16 state = 0;
	
	if(state == 2)
		return true;
	else if(state == 1)
		return false;
	
	if (::NavServicesAvailable())
	{
		state = 2;
		::NavLoad();
		UProgramCleanup::Install(_FSReleaseNavServices);
		return true;
	}
	else
	{
		state = 1;
		return false;
	}
#endif
}

#if TARGET_API_MAC_CARBON
void _SearchAndRedrawWindow(WindowPtr inWindow);

static pascal void _FileEventProc(NavEventCallbackMessage inCallBackSelector, NavCBRecPtr inCallBackParams, NavCallBackUserData inCallBackUD)
{
	#pragma unused(inCallBackUD)
	
	if (inCallBackSelector == kNavCBEvent && inCallBackParams != NULL)
	{
		switch (inCallBackParams->eventData.eventDataParms.event->what)
		{
			case activateEvt:
				break;
			
			case updateEvt:
				_SearchAndRedrawWindow((WindowPtr)inCallBackParams->eventData.eventDataParms.event->message);
				break;
			
			case nullEvent:
				break;
		};
	}
}

#else

static pascal short _FileDialogHook(short inItem, DialogPtr inDialog, void *inUserData)
{
	if (!inUserData)
		return inItem;

	SFileRecordHook *pRecordHook = (SFileRecordHook *)inUserData;
	if (!pRecordHook->pFolderSpec)
		return inItem;

	// make sure we're dealing with the proper dialog
	if (::GetWRefCon(inDialog) == sfMainDialogRefCon)
	{
		// just when opening the dialog...
		if (inItem == sfHookFirstCall)
		{
			bool bIsDir;
			Int32 nDirID = _FSGetDirID(pRecordHook->pFolderSpec->vRefNum, pRecordHook->pFolderSpec->parID, pRecordHook->pFolderSpec->name, &bIsDir);
			
			if (bIsDir)
			{
				pRecordHook->pFileReplay->sfFile.vRefNum = pRecordHook->pFolderSpec->vRefNum;
				pRecordHook->pFileReplay->sfFile.parID = nDirID;
			
				pRecordHook->pFileReplay->sfScript = smSystemScript;
				inItem = sfHookChangeSelection;
			}
		}
	}

	return inItem;
}
#endif

/* -------------------------------------------------------------------------- */
#pragma mark -

void _AddResFileToSearchChain(TFSRefObj* inRef)
{
	Require(inRef);
	FSpOpenResFile(&((SFileSysRef *)inRef)->spec, fsRdPerm);
	CheckMacError(ResError());
}

char **_GetMacRes(Uint32 inType, Int16 inID)
{
	Handle h = ::GetResource(inType, inID);
	
	if (h == nil)
	{
		Int16 err = ::ResError();
		FailMacError(err ? err : resNotFound);
	}
	
	return h;
}

THdl _GetMacResDetached(Uint32 inType, Int16 inID)
{
	Handle mh = ::GetResource(inType, inID);
	
	if (mh == nil)
	{
		Int16 err = ::ResError();
		FailMacError(err ? err : resNotFound);
	}

	Uint32 s = ::GetHandleSize(mh);
	
	char saveState = HGetState(mh);
	HLock(mh);
	
	THdl h;
	try
	{
		h = UMemory::NewHandle(*mh, s);
	}
	catch(...)
	{
		HSetState(mh, saveState);
		throw;
	}
	
	HSetState(mh, saveState);

	return h;
}

// outStateData must point to 4 Uint32s, resource should be purgeable, returns ptr to data
void *_LoadMacRes(Uint32 inType, Int16 inID, Uint32 *outStateData, Uint32 *outSize)
{
#if DEBUG
	Require(outStateData);
#endif

	Handle h = ::GetResource(inType, inID);
	
	if (h == nil)
	{
		Int16 err = ::ResError();
		FailMacError(err ? err : resNotFound);
	}
	
	outStateData[0] = (Uint32)h;
	outStateData[1] = ::HGetState(h);
	
	::HLock(h);

	if (outSize) *outSize = ::GetHandleSize(h);
	
	return *h;
}

void _ReleaseMacRes(Uint32 *inStateData)
{
	if (inStateData) HSetState((Handle)inStateData[0], inStateData[1]);
}

char **_MakeMacHandle(const void *inData, Uint32 inDataSize)
{
	Handle h = ::NewHandle(inDataSize);
	if (h == nil) Fail(errorType_Memory, memError_NotEnough);
	
	::BlockMoveData(inData, *h, inDataSize);
	
	return h;
}

void _GetQuickTimeFSSpec(TFSRefObj* inRef, FSSpec& outSpec)
{
	Require(inRef);
	
	outSpec = ((SFileSysRef *)inRef)->spec;
}

#endif
