/*
 	File:		TSMTE.h
 
 	Contains:	Text Services Managerfor TextEdit Interfaces.
 
 	Version:	Technology:	System 7.5
 				Release:	QuickTime 4.0
 
 	Copyright:	(c)1991-1998 Apple Technology, Inc. All rights reserved.
 
 	Bugs?:		For bug reports, consult the following page on
 				the World Wide Web:
 
 					http://developer.apple.com/bugreporter/
 
*/
#ifndef __TSMTE__
#define __TSMTE__

#ifndef __TEXTEDIT__
#include <TextEdit.h>
#endif
#ifndef __DIALOGS__
#include <Dialogs.h>
#endif
#ifndef __APPLEEVENTS__
#include <AppleEvents.h>
#endif
#ifndef __TEXTSERVICES__
#include <TextServices.h>
#endif
/* #include <Gestalt.i>*/




#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

/* signature, interface types*/

enum {
	kTSMTESignature				= FOUR_CHAR_CODE('tmTE'),
	kTSMTEInterfaceType			= FOUR_CHAR_CODE('tmTE'),
	kTSMTEDialog				= FOUR_CHAR_CODE('tmDI')
};


/* update flag for TSMTERec*/

enum {
	kTSMTEAutoScroll			= 1
};


/* callback procedure definitions*/

typedef CALLBACK_API( void , TSMTEPreUpdateProcPtr )(TEHandle textH, long refCon);
typedef CALLBACK_API( void , TSMTEPostUpdateProcPtr )(TEHandle textH, long fixLen, long inputAreaStart, long inputAreaEnd, long pinStart, long pinEnd, long refCon);
typedef STACK_UPP_TYPE(TSMTEPreUpdateProcPtr) 					TSMTEPreUpdateUPP;
typedef STACK_UPP_TYPE(TSMTEPostUpdateProcPtr) 					TSMTEPostUpdateUPP;


/* data types*/

struct TSMTERec {
	TEHandle 						textH;
	TSMTEPreUpdateUPP 				preUpdateProc;
	TSMTEPostUpdateUPP 				postUpdateProc;
	long 							updateFlag;
	long 							refCon;
};
typedef struct TSMTERec					TSMTERec;
typedef TSMTERec *						TSMTERecPtr;
typedef TSMTERecPtr *					TSMTERecHandle;
#if OPAQUE_TOOLBOX_STRUCTS
typedef struct OpaqueTSMDialogPtr* 		TSMDialogPtr;
#else

struct TSMDialogRecord {
	DialogRecord 					fDialog;
	TSMDocumentID 					fDocID;
	TSMTERecHandle 					fTSMTERecH;
	long 							fTSMTERsvd[3];				/* reserved*/
};
typedef struct TSMDialogRecord			TSMDialogRecord;

typedef TSMDialogRecord *				TSMDialogPtr;
typedef TSMDialogPtr 					TSMDialogPeek;
#endif  /* OPAQUE_TOOLBOX_STRUCTS */

enum { uppTSMTEPreUpdateProcInfo = 0x000003C0 }; 				/* pascal no_return_value Func(4_bytes, 4_bytes) */
enum { uppTSMTEPostUpdateProcInfo = 0x000FFFC0 }; 				/* pascal no_return_value Func(4_bytes, 4_bytes, 4_bytes, 4_bytes, 4_bytes, 4_bytes, 4_bytes) */
#if MIXEDMODE_CALLS_ARE_FUNCTIONS
EXTERN_API(TSMTEPreUpdateUPP)
NewTSMTEPreUpdateProc		   (TSMTEPreUpdateProcPtr	userRoutine);
EXTERN_API(TSMTEPostUpdateUPP)
NewTSMTEPostUpdateProc		   (TSMTEPostUpdateProcPtr	userRoutine);
EXTERN_API(void)
CallTSMTEPreUpdateProc		   (TSMTEPreUpdateUPP		userRoutine,
								TEHandle				textH,
								long					refCon);
EXTERN_API(void)
CallTSMTEPostUpdateProc		   (TSMTEPostUpdateUPP		userRoutine,
								TEHandle				textH,
								long					fixLen,
								long					inputAreaStart,
								long					inputAreaEnd,
								long					pinStart,
								long					pinEnd,
								long					refCon);
#else
#define NewTSMTEPreUpdateProc(userRoutine) 						(TSMTEPreUpdateUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppTSMTEPreUpdateProcInfo, GetCurrentArchitecture())
#define NewTSMTEPostUpdateProc(userRoutine) 					(TSMTEPostUpdateUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), uppTSMTEPostUpdateProcInfo, GetCurrentArchitecture())
#define CallTSMTEPreUpdateProc(userRoutine, textH, refCon) 		CALL_TWO_PARAMETER_UPP((userRoutine), uppTSMTEPreUpdateProcInfo, (textH), (refCon))
#define CallTSMTEPostUpdateProc(userRoutine, textH, fixLen, inputAreaStart, inputAreaEnd, pinStart, pinEnd, refCon)  CALL_SEVEN_PARAMETER_UPP((userRoutine), uppTSMTEPostUpdateProcInfo, (textH), (fixLen), (inputAreaStart), (inputAreaEnd), (pinStart), (pinEnd), (refCon))
#endif
#if ACCESSOR_CALLS_ARE_FUNCTIONS
/* Getters */
EXTERN_API( DialogPtr )
GetTSMDialogPtr					(TSMDialogPtr 			dialog);

EXTERN_API( TSMDocumentID )
GetTSMDialogDocumentID			(TSMDialogPtr 			dialog);

EXTERN_API( TSMTERecHandle )
GetTSMDialogTextEditHandle		(TSMDialogPtr 			dialog);

/* Setters */
EXTERN_API( void )
SetTSMDialogDocumentID			(TSMDialogPtr 			dialog,
								 TSMDocumentID 			documentID);

EXTERN_API( void )
SetTSMDialogTextEditHandle		(TSMDialogPtr 			dialog,
								 TSMTERecHandle 		teHandle);

#endif  /* ACCESSOR_CALLS_ARE_FUNCTIONS */


#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TSMTE__ */

