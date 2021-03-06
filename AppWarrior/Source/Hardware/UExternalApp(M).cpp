
#if MACINTOSH

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include <InternetConfig.h>
#include "UExternalApp.h"


struct SExternalAppInfo{
	FSSpec fsSpec;
	OSType osTypeSig;
};


#define APP		((SExternalAppInfo *)inApp)

bool _IsFrontProcess(OSType ostypeSig);
void _MakeFrontProcess(OSType ostypeSig);
void _HideProcess(OSType ostypeSig);
void _ShowProcess(OSType ostypeSig);
void _KillProcess(OSType ostypeSig);

OSErr _FindApplication(ConstStr63Param 	str63AppName, OSType ostypeCreator, FSSpecPtr ptrfsspecA);
bool _OpenItemInFinder(const FSSpec& fsspecITEM, OSType& ostypeSig, bool boolToFront = true);
bool _OpenDocInApp(OSType ostypeSig, const FSSpec& fsspecDoc, Boolean boolToFront = true);


TExternalApp UExternalApp::New(const void *inAppPath, Uint32 inPathSize, const void *inParam, Uint32 inParamSize)
{
	if (!inAppPath || inPathSize != sizeof(FSSpec))
		return nil;
	 		
	OSType osTypeSig;
	if (!_OpenItemInFinder(*(FSSpecPtr)inAppPath, osTypeSig, false))
		return nil;
	
	if (inParam)
	{
	 	if (inParamSize == sizeof(FSSpec))
			_OpenDocInApp(osTypeSig, *(FSSpecPtr)inParam, false);
//		else
//			_OpenParamInApp(osTypeSig, inParam, inParamSize, false);
	}

	SExternalAppInfo *pExternalAppInfo = (SExternalAppInfo *)UMemory::NewClear(sizeof(SExternalAppInfo));
	
	pExternalAppInfo->fsSpec = *(FSSpecPtr)inAppPath;
	pExternalAppInfo->osTypeSig = osTypeSig;

	return (TExternalApp)pExternalAppInfo;
}

void UExternalApp::Dispose(TExternalApp inApp)
{
	if (inApp)
	{
		_KillProcess(APP->osTypeSig);
		UMemory::Dispose((TPtr)inApp);
	}
}

bool UExternalApp::TryToClose(TExternalApp inApp)
{	
	if (inApp)
	{
		_KillProcess(APP->osTypeSig);
		return true;
	}
	
	return false; 
}

bool UExternalApp::TryToActivate(TExternalApp inApp)
{
	if (inApp)
	{
		if (!_IsFrontProcess(APP->osTypeSig))
			_MakeFrontProcess(APP->osTypeSig);
		
		return true;
	}
	
	return false;
}

bool UExternalApp::Suspend(TExternalApp inApp)
{
	if (inApp)
	{
		_HideProcess(APP->osTypeSig);
		return true;
	}
	
	return false; 
}

bool UExternalApp::Resume(TExternalApp inApp)
{
	if (inApp)
	{
		_ShowProcess(APP->osTypeSig);
		return true;
	}
	
	return false; 
}

bool UExternalApp::IsRegisteredAssociation(const Uint8 *inAssociation)
{
	ProcessInfoRec info;
	ProcessSerialNumber psn;
	FourCharCode signature = '\?\?\?\?';	

	// find out the signature of the current process
	psn.highLongOfPSN = psn.lowLongOfPSN = 0;
	if (GetCurrentProcess(&psn) == noErr)
	{
		info.processInfoLength = sizeof(info);
		info.processName = nil;
		info.processAppSpec = nil;
		if (GetProcessInformation(&psn, &info) == noErr)
			signature = info.processSignature;
	}
	
	Uint8 psFileName[16];
	psFileName[0] = UText::Format(psFileName + 1, sizeof(psFileName) - 1, "filename.%#s", inAssociation);
	
	ICInstance inst;
	if (ICStart(&inst, signature) == noErr)
	{
	#if !TARGET_API_MAC_CARBON
		if (ICFindConfigFile(inst, 0, nil) == noErr)
	#endif
		{
			if (ICBegin(inst, icReadOnlyPerm) == noErr)
			{
				ICMapEntry stMapEntry;
				if (ICMapFilename(inst, psFileName, &stMapEntry) != icPrefNotFoundErr)
				{
					ICEnd(inst);
					ICStop(inst);
					
					return true;
				}
				ICEnd(inst);
			}
		}
		ICStop(inst);
	}
	
	return false;
}

bool UExternalApp::RegisterAssociation(const Uint8 */*inAssociation*/, const Uint8 */*inTitle*/)
{
	// ICAddMapEntry
	
	return false;
}

Uint32 UExternalApp::SearchAssociation(const Uint8 *inFileName, Uint32 /*inTypeCode*/, void *outAppPath, Uint32 inMaxPathSize)
{
	ProcessInfoRec info;
	ProcessSerialNumber psn;
	FourCharCode signature = '\?\?\?\?';	

	// find out the signature of the current process
	psn.highLongOfPSN = psn.lowLongOfPSN = 0;
	if (GetCurrentProcess(&psn) == noErr)
	{
		info.processInfoLength = sizeof(info);
		info.processName = nil;
		info.processAppSpec = nil;
		if (GetProcessInformation(&psn, &info) == noErr)
			signature = info.processSignature;
	}
	
	ICInstance inst;
	if (ICStart(&inst, signature) == noErr)
	{
	#if !TARGET_API_MAC_CARBON
		if (ICFindConfigFile(inst, 0, nil) == noErr)
	#endif
		{
			if (ICBegin(inst, icReadOnlyPerm) == noErr)
			{
				ICMapEntry stMapEntry;
				if (ICMapFilename(inst, inFileName, &stMapEntry) != icPrefNotFoundErr)
				{
					ICEnd(inst);
					ICStop(inst);
					
					FSSpec spec;
					if (_FindApplication(stMapEntry.creatorAppName, stMapEntry.fileCreator, &spec) == noErr)
					{
						Uint32 nPathSize = sizeof(FSSpec);
						if (nPathSize > inMaxPathSize) nPathSize = inMaxPathSize;
						UMemory::Copy(outAppPath, &spec, nPathSize);
						
						return nPathSize;
					}
					
					return 0;
				}
				ICEnd(inst);
			}
		}
		ICStop(inst);
	}
	
	return 0;
}

TExternalApp UExternalApp::LaunchAssociation(const Uint8 *inFileName, Uint32 inTypeCode, const void *inParam, Uint32 inParamSize)
{
	FSSpec spec;
	Uint32 nSpecSize = SearchAssociation(inFileName, inTypeCode, &spec, sizeof(FSSpec));
	
	if (nSpecSize != sizeof(FSSpec))
		return nil;
		
	return UExternalApp::New(&spec, nSpecSize, inParam, inParamSize);
}

TExternalApp UExternalApp::LaunchApplication(TFSRefObj* inApp, const void *inParam, Uint32 inParamSize)
{
	return UExternalApp::New(inApp, sizeof(FSSpec), inParam, inParamSize);
}

bool UExternalApp::SearchRunningApp(const Int8 */*inAppPath*/, Uint32 /*inMsg*/, const Int8 */*inMessage*/)
{
	return false;
}

bool UExternalApp::ReadSystemRegistry(const char* /*key*/, const char* /*value*/, 
									  char* /*buffer*/, Uint32& bufferSize)
{
	bufferSize = 0;
	return false;
}

#pragma mark -

const OSType	kostypeApp 			= 'APPL',
				kostypeCdev 		= 'cdev',
				kostypeCreatorMac 	= 'MACS',
				kostypeFinder 		= 'FNDR';

enum EMyProcess {	kenumError,
					kenumLaunchable,
					kenumRunning
				};
			
inline Boolean	_bTST(long lgIn, short shBit ) { return( (lgIn >> shBit) & 1 ); }

Boolean	_DocOpenable(const FSSpec& fsspecFile )
//	Check to see if a document is not already open.
//	Do this by trying to open data fork of file 
//	with write permission.  
//	Return TRUE if attempt is successful.
{
	short		shRefNum = 0;
	OSErr		myErr = noErr;
	Boolean		boolDatOpenable = FALSE;
//														now try to open the data fork
	myErr = ::FSpOpenDF((FSSpecPtr)&fsspecFile, fsWrPerm, &shRefNum);
//														can open file, close data fork
	if ( myErr == noErr )
	{
		::FSClose( shRefNum );
		boolDatOpenable = TRUE;
	}
//														check if user supplied dumb FSSpec
//														let calling function handle it!
	else if ( myErr == fnfErr )
		return( FALSE );

	return( boolDatOpenable );
}

Boolean	_FindProcess(OSType ostypeCreator, OSType ostypeProcType, ProcessSerialNumber* ptrPSN, FSSpecPtr ptrfsspecApp)
//	Step thru all active processes until one is found which matches the creator & 
//	file type OSType's given.  Load PSN & FSSpec of process into ptr's provided.
//	Return TRUE if success.
{
	ProcessInfoRec 	procirRec;
	long 			lgFeature = 0L;
	Boolean 			boolFoundIt = FALSE;
	OSErr			myErr = noErr;

	if (ostypeCreator <= 0L || ostypeProcType <= 0L || ptrPSN == NULL || ptrfsspecApp == NULL)
		return false;
	
	ClearStruct(procirRec);
	
	// check gestalt to see if we can launch
	myErr = ::Gestalt( gestaltOSAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltLaunchControl ) )
		NULL;
	else
		return( FALSE );
	
	ptrPSN->highLongOfPSN = 0;
	ptrPSN->lowLongOfPSN = kNoProcess;
	procirRec.processInfoLength = sizeof( ProcessInfoRec );
	procirRec.processName = nil;
	procirRec.processAppSpec = ptrfsspecApp;
	
	// step thru all active processes until match
	while ( ::GetNextProcess( ptrPSN ) == noErr ) 
	{
		if ( ::GetProcessInformation( ptrPSN, &procirRec ) == noErr ) 
		{
			if ( ( procirRec.processType == long( ostypeProcType ) ) &&
			 	( procirRec.processSignature == ostypeCreator ) ) 
			 {
				boolFoundIt = TRUE;
				break;
			}
		}
	}
	
	return( boolFoundIt );
}

EMyProcess _ScanProcesses(const FSSpec& fsspecITEM, OSType	*ptrostypeSig, ProcessSerialNumber *ptrPSN )
//	Given the FSSpec of an item about to be launched, check
//	current processes to see if is/ needs to be launched.
//	If a process of the same kind is found,
//	then dumps its process signature & PSN
//	into ptrs provided
//
//	Returns (EMyProcess) :
//	kenumError		something bad happened�
//	kenumLaunchable	no equivalent process running
//					OR�
//					same creator running, but fsspec provided
//					is not open
//	kenumRunning		creator & filetype of FSSpec provided
//					is identical to a process already running
//					OR�
//					FSSpec provided is open and its 'owner'
//					application is running
//					(hence no need to do anything)
{
	FInfo			finfoRec;
	ProcessInfoRec 	procirRec;
	FSSpec			fsspecProc;
	long 			lgFeature = 0L;
	OSErr			myErr = noErr;
	EMyProcess		eResult = kenumLaunchable;

	if (ptrostypeSig == NULL || ptrPSN == NULL)
		return( kenumError );
	

	ClearStruct(finfoRec);
	ClearStruct(procirRec);

	// check gestalt to see if we can launch
	myErr = ::Gestalt( gestaltOSAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltLaunchControl ) )
		NULL;
	else
		return( kenumError );
	
	// get OSTypes of fsspecITEM item
	myErr = ::FSpGetFInfo( (FSSpecPtr)&fsspecITEM, &finfoRec );
	if ( myErr != noErr )
		return( kenumError );

	// prepare to examine running processes
	ptrPSN->highLongOfPSN = 0;
	ptrPSN->lowLongOfPSN = kNoProcess;
	procirRec.processInfoLength = sizeof( ProcessInfoRec );
	procirRec.processName = nil;
	procirRec.processAppSpec = &fsspecProc;
	
	// step thru all active processes until match
	while ( ::GetNextProcess( ptrPSN ) == noErr ) 
	{
		if ( ::GetProcessInformation( ptrPSN, &procirRec ) == noErr ) 
		{
			 if ( procirRec.processSignature == finfoRec.fdCreator )
			 {
				if ( procirRec.processType == long( finfoRec.fdType ) )
					eResult = kenumRunning;

				else if (!_DocOpenable( fsspecITEM ) )
					eResult = kenumRunning;
				//	Some notes on this.  If a process of the
				//	same creator is running and the FSSpec
				//	provided by the user is already open,
				//	then I assume that the FSSpec is open
				//	in the running application!
				//	assign data of matching process
				*ptrostypeSig = procirRec.processSignature;
				//	ptrPSN has already been filled!
				break;
			}
		}
	}
	
	return( eResult );
}

void _StallUntilFrontProcess(const ProcessSerialNumber& psnProcess)
//	Call 'WaitNextEvent()' until required process is in front.  Will automatically cut out
//	after a maximum of 'kshLimit' (5) calls (if it hasn't come to front in that time then it never will!)
{
	const short			kshLimit = 5;
	const long			klgGNEInterval = GetCaretTime();

	ProcessSerialNumber	psnFront;
	EventRecord			evRec;
	short				i = 0;
	Boolean				boolInFront = FALSE;

	while ( FALSE == boolInFront && i++ < kshLimit )
	{
		::WaitNextEvent( everyEvent, &evRec, klgGNEInterval, NULL );
		::GetFrontProcess( &psnFront );
		::SameProcess( &psnFront, &psnProcess, &boolInFront );
	}		
}

//	Walk process queue to see if required process is running in front
bool _IsFrontProcess(OSType ostypeSig)
{
	ProcessSerialNumber psnTarget, psnFront;
	FSSpec 				fsspecApp;
	Boolean				boolResult = FALSE;
	OSErr 				myErr = noErr;
	
	if (ostypeSig <= 0L )
		return false;
	
	// find app
	if (_FindProcess(ostypeSig, kostypeApp, &psnTarget, &fsspecApp)) 
	{
		myErr = ::GetFrontProcess( &psnFront );
		
		if ( myErr == noErr )
			::SameProcess( &psnFront, &psnTarget, &boolResult );
	}

	return( boolResult );
}

void _MakeFrontProcess(OSType ostypeSig)
{
	ProcessSerialNumber psnTarget;
	FSSpec 				fsspecApp;

	if (ostypeSig <= 0L)
		return;
	
	// find app
	if (_FindProcess(ostypeSig, kostypeApp, &psnTarget, &fsspecApp)) 
		::SetFrontProcess(&psnTarget);
}

//	Use process manager tricks to force current process to the front
void _MakeCurrentProcessFront( void )
{
	ProcessSerialNumber		psnCurrent;

	::GetCurrentProcess(&psnCurrent);
	::SetFrontProcess(&psnCurrent);

	_StallUntilFrontProcess( psnCurrent );
}

//	Build an object specifier for the visibility property of a process called 'str63Name' running in the finder
void _BuildVisibilityOSPEC(Str63 str63Name, AEDesc *ptraedescOSPEC)
{
	const Boolean	kboolDisposeInputs = TRUE;

	AEDesc 			nullContainer 	= { typeNull, NULL },
					ospecProc 		= { typeNull, NULL },
					aedescName		= { typeNull, NULL },
					aedescVisi 		= { typeNull, NULL };
	long			lgVisiProperty 	= pVisible;
	OSErr			myErr			= noErr;

	
	if (str63Name[0] == 0 || ptraedescOSPEC == NULL)
		return;

	// create AEDesc for process name
	myErr = ::AECreateDesc( typeChar, &str63Name[1], str63Name[0], &aedescName );
	if (myErr != noErr)
		return;

	//	create an ospec to indicate that we are playing with
	//	a process called 'str63Name' of class 'cProcess' running in the finder
	myErr = ::CreateObjSpecifier( cProcess, &nullContainer, 
							formName, &aedescName, 
							kboolDisposeInputs, &ospecProc );
	
	if (myErr != noErr)
		return;
	
	// create AEDesc for visibility property
	myErr = ::AECreateDesc( typeType, &lgVisiProperty, 
						sizeof( long ), &aedescVisi );
	
if (myErr != noErr)
		return;
		
	//	now build a specifier representing the 'pVisible' property
	//	of the target process, using 'ospecProc' as its container...
	myErr = ::CreateObjSpecifier( cProperty, &ospecProc,
						formPropertyID, &aedescVisi, 
						kboolDisposeInputs, ptraedescOSPEC );
}

//	Send a boolean kAESetData AE to the finder to show or hide a running process of signature 'ostypeSig'
Boolean	_ShowHideProcess(OSType ostypeSig, Boolean boolShow)
{
	AEDesc 				aedescOSPEC		= { typeNull, NULL };
	AEAddressDesc			aedescFinderAddr 	= { typeNull, NULL };
	AppleEvent			aeEvent 			= { typeNull, NULL },
						aeReply 			= { typeNull, NULL };
	ProcessSerialNumber		psnProc;
	FSSpec				fsspecProc;
	long					lgFeature			= 0L;
	OSErr				myErr 			= noErr;

	if (ostypeSig <= 0L )
		return false;

	// use Gestalt to check for OSL compliance
	myErr = ::Gestalt( gestaltFinderAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltOSLCompliantFinder ) )
		NULL;
	else
		return( FALSE );
	
	// find name of process
	if ( ! _FindProcess( ostypeSig, kostypeApp, &psnProc, &fsspecProc ) )
		return( FALSE );

	//	create object specifier
	_BuildVisibilityOSPEC( fsspecProc.name, &aedescOSPEC );
	
	// get PSN of finder
	_FindProcess( kostypeCreatorMac, kostypeFinder, &psnProc, &fsspecProc );
	
	// create AE targeted at finder
	myErr = ::AECreateDesc( typeProcessSerialNumber, &psnProc,
						sizeof( ProcessSerialNumber ), &aedescFinderAddr );
		
	myErr = ::AECreateAppleEvent( kAECoreSuite, kAESetData,
							&aedescFinderAddr, kAutoGenerateReturnID,
							kAnyTransactionID, &aeEvent );

	if (myErr != noErr)
		return false;

	// place OSPEC in AE	
	myErr = ::AEPutParamDesc( &aeEvent, keyDirectObject, &aedescOSPEC );
	if (myErr != noErr)
		return false;

	// place boolean in AE
	myErr = ::AEPutParamPtr( &aeEvent, keyAEData, typeBoolean,
						(Ptr)&boolShow, sizeof( Boolean ) );

	if (myErr != noErr)
		return false;
		
	// send AE on its merry way
	myErr = ::AESend( &aeEvent, &aeReply, 
					kAEWaitReply + kAENeverInteract,
					kAEHighPriority, 
					kAEDefaultTimeout, 
					nil, nil );
		
	//	N.B  Use 'kAEWaitReply' instead of 'kAENoReply' to allow
	//	OS time to cope with AppleEvent!

	if (myErr != noErr)
		return false;
		
	// tidy up
	::AEDisposeDesc( &aedescOSPEC );
	::AEDisposeDesc( &aedescFinderAddr );
	::AEDisposeDesc( &aeEvent );
	::AEDisposeDesc( &aeReply );

	return( TRUE );	
}

//	Call showHideProcess() such that required process is hidden
void _HideProcess(OSType ostypeSig)
{
	Boolean	boolResult = FALSE;

	boolResult = _ShowHideProcess( ostypeSig, FALSE );
}

void _ShowProcess(OSType ostypeSig)
//	Call showHideProcess() such that required process is shown
//	N.B.  will NOT bring process into foreground, it will only show it!  
//	Use 'TAELaunch::openItemInFinder( FSSpec, TRUE ); if you want foreground
{
	Boolean	boolResult = FALSE;

	boolResult = _ShowHideProcess( ostypeSig, TRUE );
}

//	send quit AE to a running application
void _KillProcess(OSType ostypeSig)
{
	const long			klgTimeOut = 5L * 60L;

	AEAddressDesc 			aedescTarget 		= { typeNull, NULL };
	AppleEvent 			aeEvent 			= { typeNull, NULL },
			 			aeReply 			= { typeNull, NULL };
	ProcessSerialNumber 	psnProcess;
	FSSpec 				fsspecApp;
	OSErr 				myErr = noErr;
	
	if (ostypeSig <= 0L )
		return;

	// find app and shut it down
	if (_FindProcess( ostypeSig, kostypeApp, &psnProcess, &fsspecApp)) 
	{
		myErr = ::AECreateDesc(	typeProcessSerialNumber,
								&psnProcess,
								sizeof( psnProcess ),
								&aedescTarget );
		myErr = ::AECreateAppleEvent(	kCoreEventClass,
								kAEQuitApplication,
								&aedescTarget,
								kAutoGenerateReturnID,
								kAnyTransactionID,
								&aeEvent );
								
		// send 'quit' AE
		myErr = ::AESend(	&aeEvent,
						&aeReply,
						kAENoReply,
						kAEHighPriority,
						klgTimeOut,
						nil, nil );
		
		if (myErr != noErr)
			return;
		
		// tidy up
		myErr = ::AEDisposeDesc(&aedescTarget);
		myErr = ::AEDisposeDesc( &aeEvent );
		myErr = ::AEDisposeDesc( &aeReply );
	}
}


#pragma mark -

//	Given a FSSpec, add it to a AEDescList at row 'shRow'
void _AddFSSToAEList(AEDescList *ptrDescList, short shRow, const FSSpec& fsspecFile)
{
	AliasHandle	haliasFile;
	OSErr		myErr = noErr;
	
	if (ptrDescList == NULL || shRow < 0 )
		return;
		
	// create alias handle & insert into AEDescList
	myErr = ::NewAlias( nil, &fsspecFile, &haliasFile );
	if (myErr == noErr) 
	{
		::HLock((Handle) haliasFile);
		myErr = ::AEPutPtr( ptrDescList, shRow, typeAlias,
						(Ptr)*haliasFile, (*haliasFile)->aliasSize );
		::DisposeHandle( (Handle) haliasFile );
	}
}

OSErr _SearchPBCatByName(ConstStr63Param str63Name, short shVRefNum, Boolean boolForFile, Boolean boolExactName,
						 OSType	ostypeFileCreator, OSType ostypeFileType, FSSpecArrayPtr ptrfsspecArray, short *ptrshHits)
//	Search a volume using PBCatSearch() to 
//	turn up an array of FSSpecs (if neccessary)
//	*ptrshHits on input sets limit of matches, on output shows actual number of 'hits' found.
{
//														use a 16K search cache for speed
	const long		klgBuffSize = 16384L;
//														number to find in ONE SWEEP
	const short		kshSweepMatches = 20;
//														parameter block for PBCatSearch
	CSParam			csparamRec;
//														search criteria, part 1 & 2
	CInfoPBRec		cipbrRec_1;
	CInfoPBRec		cipbrRec_2;
	Str63			str63FindName = { "\p" };
//														each sweep's results storage
	FSSpec			fsspecHitsArray[ kshSweepMatches ];
	OSErr			myErr = noErr;
	Boolean			boolDone = FALSE, boolTooMany = FALSE;
	short			i, shNumFound = 0;
	Ptr				ptrSearchBuff = nil;

	if (str63Name[0] == 0 || ptrfsspecArray == NULL || ptrshHits == NULL || *ptrshHits <= 0)
		return( memFullErr );
	
	// set up search buffer memory
	ptrSearchBuff = (Ptr)UMemory::New(klgBuffSize * sizeof( unsigned char ));

	if ( ptrSearchBuff == nil )	
		return( memFullErr );

	// make local copy of const string
	BlockMoveData( (Str63*)str63Name, str63FindName, str63Name[0]+1 );

	//	GENERAL SEARCH SETTINGS�
	//
	csparamRec.ioCompletion = nil;						//	no completion routine
	csparamRec.ioNamePtr = nil;							//	no volume name;  use vRefNum
	csparamRec.ioVRefNum = shVRefNum;						//	volume to search
	csparamRec.ioMatchPtr = &fsspecHitsArray[0];				//	points to FSSpec array
	csparamRec.ioReqMatchCount = (long)kshSweepMatches;		//	max no. of matches per sweep
	csparamRec.ioSearchInfo1 = &cipbrRec_1;					//	points to first criteria set
	csparamRec.ioSearchInfo2 = &cipbrRec_2;					//	points to second criteria set
	csparamRec.ioSearchTime = 500L;						//	time out on v.long searches

	csparamRec.ioOptBuffer = ptrSearchBuff;					//	point to search cache
	csparamRec.ioOptBufSize = klgBuffSize;					//	size of search cache

 	csparamRec.ioActMatchCount = 0L;						//	init. number of files found
	csparamRec.ioCatPosition.initialize = 0L;				//	search from beginning of catalogue
	csparamRec.ioSearchBits = 0L;							//	init. search criteria

	//	SEARCH CRITERIA
	//
	csparamRec.ioSearchBits += fsSBFlAttrib;				//	search for file/folder
	csparamRec.ioSearchBits += ( boolExactName ) ? 
						fsSBFullName : fsSBPartialName;	//	search using EXACT : PARTIAL name
		
	cipbrRec_1.hFileInfo.ioNamePtr = str63FindName;			//	name to search for
	cipbrRec_2.hFileInfo.ioNamePtr = nil;					//	set to nil
	cipbrRec_1.hFileInfo.ioFlAttrib = ( boolForFile ) ? 0L : ioDirMask;	//	set bit 4 for FILES : DIRs
	cipbrRec_2.hFileInfo.ioFlAttrib = ioDirMask;				//	set mask for bit 4

	//	search using FILE TYPE
	//
	if ( boolForFile && ostypeFileCreator && ostypeFileType )
		{
		//
		//	Reset 'ioSearchBits' to search for name &
		//	finder info only (finder type/creator is only
		//	for files anyway!)
		//
		//
		csparamRec.ioSearchBits = fsSBFlFndrInfo;
//														search using EXACT : PARTIAL name
		csparamRec.ioSearchBits += ( boolExactName ) ? 
							fsSBFullName : fsSBPartialName;
//														criteria & masks for type searching		
		cipbrRec_1.hFileInfo.ioFlFndrInfo.fdCreator = ostypeFileCreator;
		cipbrRec_2.hFileInfo.ioFlFndrInfo.fdCreator = 0xFFFFFFFF;
		cipbrRec_1.hFileInfo.ioFlFndrInfo.fdType = ostypeFileType;
		cipbrRec_2.hFileInfo.ioFlFndrInfo.fdType = 0xFFFFFFFF;

		cipbrRec_1.hFileInfo.ioFlFndrInfo.fdFlags = 0;
		cipbrRec_2.hFileInfo.ioFlFndrInfo.fdFlags = 0x4000;
		*(long *)&cipbrRec_1.hFileInfo.ioFlFndrInfo.fdLocation = 0L;
		*(long *)&cipbrRec_2.hFileInfo.ioFlFndrInfo.fdLocation = 0L;
		cipbrRec_1.hFileInfo.ioFlFndrInfo.fdFldr = 0;
		cipbrRec_2.hFileInfo.ioFlFndrInfo.fdFldr = 0;
		}

	//	DO SEARCH
	//
	do {
		myErr = PBCatSearch( &csparamRec, FALSE );
//														eofErr returned when all boolDone
		boolDone = ( myErr == eofErr );

		if ( ( ( myErr == noErr ) || boolDone ) && csparamRec.ioActMatchCount > 0 )
			{
//														reset count flag
			boolTooMany = FALSE;
			for ( i = 0; i < csparamRec.ioActMatchCount && ! boolTooMany; i++ )
				{				
//														copy FSSpec(s) to user's array
				BlockMoveData( &csparamRec.ioMatchPtr[i],	
							ptrfsspecArray, 
							sizeof( FSSpec ) );
//														make sure always < max
				if ( ++shNumFound < *ptrshHits )
					ptrfsspecArray++;
//														break out
				else
					boolDone = boolTooMany = TRUE;
				}
			}
		} while ( myErr == noErr && !boolDone );
//														store number found
	*ptrshHits = shNumFound;

//														tidy up memory
	UMemory::Dispose((TPtr)ptrSearchBuff);

//														set OSErr return val
	if ( shNumFound > 0 && boolDone )
		myErr = noErr;
//														file or folder not found
	else
		myErr = boolForFile ? fnfErr : dirNFErr;
		
	return( myErr );
}

//	add the application to the desktop so next search of desktop will find it!
OSErr _AddAppToDesk(FSSpecPtr ptrfsspecApp, OSType ostypeSig)
{
	DTPBRec		dtpRec;
	Str255		str255Dummy = { "\p" };
	OSErr		myErr;

	if (ptrfsspecApp == NULL || ostypeSig <= 0L)
		return( memFullErr );

	// grab vol's desktop ref
	dtpRec.ioNamePtr = str255Dummy;
	dtpRec.ioVRefNum = ptrfsspecApp->vRefNum;

	myErr = ::PBDTGetPath( &dtpRec );
	
	// now insert app into desktop
	if ( myErr == noErr ) 
	{
		dtpRec.ioNamePtr = ptrfsspecApp->name;
		dtpRec.ioDirID = ptrfsspecApp->parID;
		dtpRec.ioFileCreator = ostypeSig;
		
		myErr = ::PBDTAddAPPLSync( &dtpRec );
	}

	return( myErr );
}

OSErr _FindFile(ConstStr63Param	str63Name, OSType ostypeCreator, OSType ostypeFileType, FSSpecPtr ptrfsspecFile)
//	Scan all volumes to find a file called 'str63Name' with a given creator & type,
//	then load hit into FSSpecPtr provided.
//	Return OSErr as to success
{
	short 		i = 0, 
				shtHits = 1;
	VolumeParam	vparamRec;
	Str63		str63VolName = { "\p" };
	Boolean		boolFoundIt = FALSE;
	OSErr		myErr = noErr,
				volErr = noErr;

	if (ostypeCreator <= 0L || ostypeFileType <= 0L || ptrfsspecFile == NULL )
		return( memFullErr );

	vparamRec.ioCompletion = NULL;
	vparamRec.ioNamePtr = str63VolName;

	// search a vol
	do {
		vparamRec.ioVRefNum = 0;
		vparamRec.ioVolIndex = ++i;

	#if TARGET_API_MAC_CARBON
		volErr = ::PBHGetVInfoSync( (HParmBlkPtr)&vparamRec );
	#else
		volErr = ::PBGetVInfoSync( (ParmBlkPtr)&vparamRec );
	#endif
	
		if ( volErr == noErr )
			{
			myErr = _SearchPBCatByName( str63Name,
								vparamRec.ioVRefNum,
								TRUE,					/* Search for file */
								FALSE,					/* Exact name match? */
								ostypeCreator,
								ostypeFileType,
								ptrfsspecFile,
								&shtHits );
//														set for next sweep
			shtHits = 1;
			boolFoundIt = ( myErr == noErr );
			}
		} while ( ! boolFoundIt && ( volErr == noErr ) );
//														equate result to success	
	myErr = boolFoundIt ? noErr : fnfErr;
	
	return( myErr );
}

OSErr _FindApplication(ConstStr63Param 	str63AppName, OSType ostypeCreator, FSSpecPtr ptrfsspecA)
//	Given an application's name & creator OSType, search all mounted volumes (by looking in 
//	desktop database) until a matching app on disc is found.  Then load file's FSSpec into
//	FSSpecPtr provided.  Return OSErr as to success or otherwise.
//
//	This alogrithm searches the desktop!  So if there is something wrong with it, then will have
//	to search desk catalogue (which is much slower).
{
	short 		i = 0;
	VolumeParam	vparamRec;
	Str63		str63Name = { "\p" };
	DTPBRec 		pbdt;
	long 		lgBytesFree = 0L;
	OSErr 		myErr = noErr;
	Boolean 		boolFoundIt = FALSE;
	long 		lgFeature = 0L;
	
	if (ptrfsspecA == NULL || ostypeCreator <= 0L)
		return( memFullErr );
	
	ClearStruct(vparamRec);
	ClearStruct(pbdt);

	// check for system 7
	myErr = ::Gestalt( gestaltSystemVersion, &lgFeature );
	if ( myErr == noErr && ( lgFeature >= 0x0700 ) ) 
	{
		vparamRec.ioCompletion = NULL;
		vparamRec.ioNamePtr = ptrfsspecA->name;
		
		// cycle thru all mounted vols
		do {
			vparamRec.ioVRefNum = 0;
			vparamRec.ioVolIndex = ++i;
	
		#if TARGET_API_MAC_CARBON
			myErr = ::PBHGetVInfoSync( (HParmBlkPtr)&vparamRec );
		#else
			myErr = ::PBGetVInfoSync( (ParmBlkPtr)&vparamRec );
		#endif
			
			// grab desktop database
			if ( myErr == noErr ) 
				{
				pbdt.ioNamePtr = str63Name;
				pbdt.ioVRefNum = vparamRec.ioVRefNum;
				myErr = ::PBDTGetPath( &pbdt );

				// search desktop for app
				if ( myErr == noErr ) 
					{
					pbdt.ioIndex = 0;
					pbdt.ioFileCreator = ostypeCreator;
					myErr = ::PBDTGetAPPLSync( &pbdt );
					if ( myErr == noErr )
						boolFoundIt = TRUE;
					}
				myErr = noErr;
				}
			} while ( ! boolFoundIt && ( myErr == noErr ) );
		}

	// store data in fsspec
	if ( boolFoundIt ) 
	{
		myErr = noErr;
		ptrfsspecA->vRefNum = pbdt.ioVRefNum;
		ptrfsspecA->parID = pbdt.ioAPPLParID;
		::BlockMoveData( str63Name, ptrfsspecA->name, str63Name[0]+1 );
	}
	// try to find app by cat. search!
	else if ( str63AppName[0] > 0 )
	{
		myErr = _FindFile( str63AppName, ostypeCreator, kostypeApp, ptrfsspecA);
		// if found, then insert into desktop
		if ( myErr == noErr )
			myErr = _AddAppToDesk( ptrfsspecA, ostypeCreator );
	}

	return( myErr );
}
						
//	Send in an FSSpec of an item you wish the finder to launch.  Set 'boolToFront' to bring it to the front
//	Returns TRUE and find OSType and ProcessSerialNumber of process just launched if everything went okay
bool _OpenItemInFinder(const FSSpec& fsspecITEM, OSType& ostypeSig, bool boolToFront)
{
	AEDesc				aedescTarget 		= { typeNull, NULL };
	AEDescList			aedesclistFiles 	= { typeNull, NULL };
	AppleEvent			aeEvent 			= { typeNull, NULL }, 
						aeReply 			= { typeNull, NULL };
	AESendMode			aesendMode;
	long				lgFeature 		= 0L;
	FSSpec				fsspecFinder;
	ProcessSerialNumber	psnOriginal, psnProcess, psnFndr;
	OSErr				myErr 			= noErr;
	
	ostypeSig 		= 0L;

//														use Gestalt to check for OSL compliance
	myErr = ::Gestalt( gestaltFinderAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltOSLCompliantFinder ) )
		NULL;
	else
		return( FALSE );
//														check existing processes first�
	switch (_ScanProcesses( fsspecITEM, &ostypeSig, &psnProcess ) )
		{
		case kenumError:
			return( FALSE );
			break;
//														fsspecITEM is openable
		case kenumLaunchable:
			myErr = ::GetFrontProcess( &psnOriginal );
			
			if ( myErr != noErr )
				return( FALSE );
			
			break;
//														item is already open & running
		case kenumRunning:
			if ( boolToFront )
				::SetFrontProcess( &psnProcess );
			
			return( TRUE );
			break;
		}
//														grab process for finder
	if (_FindProcess( kostypeCreatorMac, kostypeFinder, &psnFndr, &fsspecFinder ) )
	{
//														prepare 'odoc' AE for finder
		myErr = ::AECreateDesc( typeProcessSerialNumber,
							&psnFndr, sizeof( ProcessSerialNumber ),
							&aedescTarget );
		if (myErr == noErr)
			myErr = ::AECreateAppleEvent(	kCoreEventClass,
									kAEOpenDocuments,
									&aedescTarget,
									kAutoGenerateReturnID,
									kAnyTransactionID,
									&aeEvent );
//														prepare FSSpec's to send to finder
		if ( myErr == noErr )
			myErr = ::AECreateList( nil, 0, FALSE, &aedesclistFiles );
		
		if ( myErr == noErr )
			_AddFSSToAEList( &aedesclistFiles, 1, fsspecITEM );
		
		if ( myErr == noErr )
			myErr = ::AEPutParamDesc( &aeEvent, keyDirectObject, &aedesclistFiles );
//														send AE to finder
		if (myErr == noErr) 
		{
//														set AE switches
			if ( boolToFront )
				aesendMode = kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract;
			else
				aesendMode = kAEWaitReply + kAECanSwitchLayer + kAENeverInteract;
				//	N.B  Use 'kAEWaitReply' instead of 'kAENoReply' to allow
				//	OS time to cope with AppleEvent!

			myErr = ::AESend(&aeEvent, &aeReply, aesendMode, kAEHighPriority, kAEDefaultTimeout, nil, nil);		
			if (myErr != noErr)
				return false;
		}

		::AEDisposeDesc( &aeEvent );
		::AEDisposeDesc( &aeReply );
		::AEDisposeDesc( &aedesclistFiles );
		::AEDisposeDesc( &aedescTarget );			

		if (myErr != noErr)
			return false;		
		
		// find PSN of process just launched
		if ( kenumError == _ScanProcesses(fsspecITEM, &ostypeSig, &psnProcess))
			return false;
			
		if (!boolToFront)
		{
			//	Finder always puts new processes in foreground,
			//	so only need to pull tricks if want to put it into the BACKGROUND

			// stall until launched app is indeed in front!
			_StallUntilFrontProcess(psnProcess);
			
			// NOW make current app front!			
			_MakeCurrentProcessFront();
		}
	}

	return (myErr == noErr);
}

bool _OpenDocInApp(OSType ostypeSig, const FSSpec& fsspecDoc, Boolean boolToFront)
//	ASSUMES THAT APP IS ALREADY RUNNING!
//	Send in the OSType of an app. and the FSSpec of a document you wish to open in it
//  Return TRUE if success
{
	const long			klgHeapNeeded = 4096L;

	AEDesc 				aedescTarget 		= { typeNull, NULL };
	AppleEvent 			aeEvent 			= { typeNull, NULL },
						aeReply 			= { typeNull, NULL };
	AEDescList 			aedesclistFiles 	= { typeNull, NULL };
	AESendMode 			aesendMode;
	ProcessSerialNumber psnProcess;
	FSSpec 				fsspecApp;
	long 				lgFeature = 0L,
						lgTotHeapSize = 0L,
						lgContigHeapSize = 0L;
	OSErr 				myErr = noErr;

	if (ostypeSig <= 0L )
		return false;
		
	//	make sure we've got enough heap to play with!
	::PurgeSpace( &lgTotHeapSize, &lgContigHeapSize );
	if ( lgContigHeapSize <= klgHeapNeeded )
		return( FALSE );
		
	// check gestalt to see if we can launch
	myErr = ::Gestalt( gestaltOSAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltLaunchControl ) )
		NULL;
	else
		return( FALSE );
	
	// find app. process and launch with doc
	if ( _FindProcess( ostypeSig, kostypeApp, &psnProcess, &fsspecApp ) ) 
		{
//														check to see if item isn't already open
		if ( _DocOpenable( fsspecDoc ) )
			{
//														create AE for target application
			myErr = ::AECreateDesc( typeProcessSerialNumber, &psnProcess, 
								sizeof(psnProcess), &aedescTarget );
	
			myErr = ::AECreateAppleEvent(	kCoreEventClass,
									kAEOpenDocuments,
									&aedescTarget,
									kAutoGenerateReturnID,
									kAnyTransactionID,
									&aeEvent );
//														prepare AEDesc(s) for doc to be launched
			myErr = ::AECreateList( nil, 0, false, &aedesclistFiles );
			_AddFSSToAEList( &aedesclistFiles, 1, fsspecDoc );
			myErr = ::AEPutParamDesc( &aeEvent, keyDirectObject, &aedesclistFiles );
//														set AE switches
			if ( boolToFront )
				aesendMode = kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract;
			else
				aesendMode = kAEWaitReply + kAECanSwitchLayer + kAENeverInteract;
				//	N.B  Use 'kAEWaitReply' instead of 'kAENoReply' to allow
				//	OS time to cope with AppleEvent!
//														send 'odoc' AE to application
			myErr = ::AESend(	&aeEvent,
							&aeReply,
							aesendMode,
							kAEHighPriority,
							kNoTimeOut,
							nil, nil);
			
			if (myErr != noErr)
				return false;			
			
//														tidy up
			myErr = ::AEDisposeDesc( &aedescTarget );
			myErr = ::AEDisposeDesc( &aeEvent );
			myErr = ::AEDisposeDesc( &aeReply );
			myErr = ::AEDisposeDesc( &aedesclistFiles );
			}
//														if required, bring app to front
		if ( boolToFront )
			myErr = ::SetFrontProcess( &psnProcess );
		}
//														application is not running!
	else
		myErr = afpItemNotFound;

	return( myErr == noErr );
}
/*
bool _OpenParamInApp(OSType ostypeSig, const void *inParam, Uint32 inParamSize, Boolean boolToFront)
//	ASSUMES THAT APP IS ALREADY RUNNING!
//  Return TRUE if success
{
	const long			klgHeapNeeded = 4096L;

	AEDesc 				aedescTarget 		= { typeNull, NULL };
	AppleEvent 			aeEvent 			= { typeNull, NULL },
						aeReply 			= { typeNull, NULL };
	AEDescList 			aedesclistFiles 	= { typeNull, NULL };
	AESendMode 			aesendMode;
	ProcessSerialNumber psnProcess;
	FSSpec 				fsspecApp;
	long 				lgFeature = 0L,
						lgTotHeapSize = 0L,
						lgContigHeapSize = 0L;
	OSErr 				myErr = noErr;

	if (ostypeSig <= 0L )
		return false;
		
	//	make sure we've got enough heap to play with!
	::PurgeSpace( &lgTotHeapSize, &lgContigHeapSize );
	if ( lgContigHeapSize <= klgHeapNeeded )
		return( FALSE );
		
	// check gestalt to see if we can launch
	myErr = ::Gestalt( gestaltOSAttr, &lgFeature );
	if ( ( myErr == noErr ) && _bTST( lgFeature, gestaltLaunchControl ) )
		NULL;
	else
		return( FALSE );
	
	// find app. process and launch with doc
	if ( _FindProcess( ostypeSig, kostypeApp, &psnProcess, &fsspecApp ) ) 
		{

//														create AE for target application
			myErr = ::AECreateDesc( typeProcessSerialNumber, &psnProcess, 
								sizeof(psnProcess), &aedescTarget );
	
			myErr = ::AECreateAppleEvent(	kCoreEventClass,
									kAEOpenDocuments,
									&aedescTarget,
									kAutoGenerateReturnID,
									kAnyTransactionID,
									&aeEvent );
//														prepare AEDesc(s) for doc to be launched
			myErr = ::AECreateList( nil, 0, false, &aedesclistFiles );
			_AddFSSToAEList( &aedesclistFiles, 1, fsspecDoc );
			myErr = ::AEPutParamDesc( &aeEvent, keyDirectObject, &aedesclistFiles );
//														set AE switches
			if ( boolToFront )
				aesendMode = kAENoReply + kAECanSwitchLayer + kAEAlwaysInteract;
			else
				aesendMode = kAEWaitReply + kAECanSwitchLayer + kAENeverInteract;
				//	N.B  Use 'kAEWaitReply' instead of 'kAENoReply' to allow
				//	OS time to cope with AppleEvent!
//														send 'odoc' AE to application
			myErr = ::AESend(	&aeEvent,
							&aeReply,
							aesendMode,
							kAEHighPriority,
							kNoTimeOut,
							nil, nil);
			
			if (myErr != noErr)
				return false;			
			
//														tidy up
			myErr = ::AEDisposeDesc( &aedescTarget );
			myErr = ::AEDisposeDesc( &aeEvent );
			myErr = ::AEDisposeDesc( &aeReply );
			myErr = ::AEDisposeDesc( &aedesclistFiles );
		
//														if required, bring app to front
		if ( boolToFront )
			myErr = ::SetFrontProcess( &psnProcess );
		}
//														application is not running!
	else
		myErr = afpItemNotFound;

	return( myErr == noErr );
}
*/
#endif /* MACINTOSH */
