/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

class TFSRefObj; // *TFSRefObj*;

// error struct
struct SError {
	Int16 type, id;
	const Int8 *file;
	Uint32 line;
	Int32 special;
	Uint16 fatal	: 1;
	Uint16 silent	: 1;

	// construction
	SError() {}
	SError(Int16 inID);
	SError(Int16 inType, Int16 inID);
	SError(Int16 inID, const Int8 *inFile, Uint32 inLine);
	SError(Int16 inType, Int16 inID, const Int8 *inFile, Uint32 inLine);
	SError(Int16 inID, const Int8 *inFile, Uint32 inLine, bool inFatal, bool inSilent);
	SError(Int16 inType, Int16 inID, const Int8 *inFile, Uint32 inLine, bool inFatal, bool inSilent);

	// operator overloads
	bool operator==(const SError& err) const	{ return type == err.type && id == err.id;	}
	bool operator!=(const SError& err) const	{ return type != err.type || id != err.id;	}
	bool operator==(Int16 inID) const			{ return id == inID; 	}
	bool operator!=(Int16 inID) const			{ return id != inID;	}
	operator Int16()							{ return id;			}
};

// utility class
class UError
{
	public:
		static void Init();
		
		static Uint32 GetMessage(const SError& inError, void *outText, Uint32 inMaxSize);
		static Uint32 GetDetailMessage(const SError& inError, void *outText, Uint32 inMaxSize);
		
		static void Alert(const SError& inError);
		static void Log(TFSRefObj* inLogFile, const SError& inError);
};

// error types
enum {
	errorType_Misc						= 1,
	errorType_Requirement				= 2,
	errorType_Program					= 100
};

// misc errors
enum {
	error_Unknown						= 1,
	error_Param							= error_Unknown + 1,
	error_Protocol						= error_Unknown + 2,
	error_Unimplemented					= error_Unknown + 3,
	error_TimedOut						= error_Unknown + 4,
	error_Terminated					= error_Unknown + 5,
	error_VersionUnknown				= error_Unknown + 6,
	error_FormatUnknown					= error_Unknown + 7,
	error_Corrupt						= error_Unknown + 8,
	error_LimitReached					= error_Unknown + 9,
	error_OutOfRange					= error_Unknown + 10,
	error_NoSuchItem					= error_Unknown + 11,
	error_ItemAlreadyExists				= error_Unknown + 12,
	error_Locked						= error_Unknown + 13
};

// requirement errors
enum {
	reqError_Unknown					= 100,
	reqError_Threads					= reqError_Unknown + 1,
	reqError_Sound						= reqError_Unknown + 2,
	reqError_GFW						= reqError_Unknown + 3
};

void __Fail(Int16 inType, Int16 inID);
void __Fail(Int16 inType, Int16 inID, const Int8 *inFile, Uint32 inLine);
void __Fail(Int16 inType, Int16 inID, const Int8 *inFile, Uint32 inLine, bool inFatal, bool inSilent);

inline void __CheckError(Int16 type, Int16 id) { if (id) __Fail(type, id); }
inline void __CheckError(Int16 type, Int16 id, const Int8 *file, Uint32 line) { if (id) __Fail(type, id, file, line); }

#if DEBUG
	#define Fail(type, id)			__Fail(type, id, __FILE__, __LINE__)
	#define SilentFail(type, id)	__Fail(type, id, __FILE__, __LINE__, false, true)
	#define FatalFail(type, id)		__Fail(type, id, __FILE__, __LINE__, true, false)
	#define CheckError(type, id)	__CheckError(type, id, __FILE__, __LINE__)
	#define Require(test)			if (!(test)) __Fail(errorType_Misc, error_Param, __FILE__, __LINE__)
	#define ASSERT(test)			if (!(test)) DebugBreak("Assertion failed: %s #%d", __FILE__, __LINE__)
#else
	#define Fail(type, id)			__Fail(type, id)
	#define SilentFail(type, id)	__Fail(type, id, nil, 0, false, true)
	#define FatalFail(type, id)		__Fail(type, id, nil, 0, true, false)
	#define CheckError(type, id)	__CheckError(type, id)
	#define Require(test)			if (!(test)) __Fail(errorType_Misc, error_Param)
	#define ASSERT(test)
#endif

#define REQUIRE(test)		Require(test)
#define FAIL(type, id)		Fail(type, id)




