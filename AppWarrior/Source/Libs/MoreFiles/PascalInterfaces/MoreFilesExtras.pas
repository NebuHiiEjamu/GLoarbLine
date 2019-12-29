{
     File:       MoreFilesExtras.p
 
     Contains:   A collection of useful high-level File Manager routines.
 
     Version:    Technology: MoreFiles
                 Release:    1.5.2
 
     Copyright:  � 1992-2001 by Apple Computer, Inc., all rights reserved.
 
     Bugs?:      For bug reports, consult the following page on
                 the World Wide Web:
 
                     http://developer.apple.com/bugreporter/
 
}

{
    You may incorporate this sample code into your applications without
    restriction, though the sample code has been provided "AS IS" and the
    responsibility for its operation is 100% yours.  However, what you are
    not permitted to do is to redistribute the source as "DSC Sample Code"
    after having made changes. If you're going to re-distribute the source,
    we require that you make it clear in the source that the code was
    descended from Apple Sample Code, but that you've made changes.
}

{$IFC UNDEFINED UsingIncludes}
{$SETC UsingIncludes := 0}
{$ENDC}

{$IFC NOT UsingIncludes}
 UNIT MoreFilesExtras;
 INTERFACE
{$ENDC}

{$IFC UNDEFINED __MOREFILESEXTRAS__}
{$SETC __MOREFILESEXTRAS__ := 1}

{$I+}
{$SETC MoreFilesExtrasIncludes := UsingIncludes}
{$SETC UsingIncludes := 1}

{$IFC UNDEFINED __MACTYPES__}
{$I MacTypes.p}
{$ENDC}
{$IFC UNDEFINED __FILES__}
{$I Files.p}
{$ENDC}

{$PUSH}
{$ALIGN MAC68K}
{$LibExport+}

{***************************************************************************}

{
**  Bit masks and macros to get common information out of ioACUser returned
**  by PBGetCatInfo (remember to clear ioACUser before calling PBGetCatInfo
**  since some file systems don't bother to set this field).
**
**  Use the GetDirAccessRestrictions or FSpGetDirAccessRestrictions
**  functions to retrieve the ioACUser access restrictions byte for
**  a folder.
**
**  Note:   The access restriction byte returned by PBGetCatInfo is the
**          2's complement of the user's privileges byte returned in
**          ioACAccess by PBHGetDirAccess.
}


CONST
																{  mask for just the access restriction bits  }
	acUserAccessMask			= $07;							{  common access privilege settings  }
	acUserFull					= $00;							{  no access restiction bits on  }
	acUserNone					= $07;							{  all access restiction bits on  }
	acUserDropBox				= $03;							{  make changes, but not see files or folders  }
	acUserBulletinBoard			= $04;							{  see files and folders, but not make changes  }


	{	***************************************************************************	}

	{	
	**  Deny mode permissions for use with the HOpenAware, HOpenRFAware,
	**  FSpOpenAware, and FSpOpenRFAware functions.
	**  Note: Common settings are the ones with comments.
		}

	dmNone						= $0000;
	dmNoneDenyRd				= $10;
	dmNoneDenyWr				= $20;
	dmNoneDenyRdWr				= $30;
	dmRd						= $01;							{  Single writer, multiple readers; the readers  }
	dmRdDenyRd					= $11;
	dmRdDenyWr					= $21;							{  Browsing - equivalent to fsRdPerm  }
	dmRdDenyRdWr				= $31;
	dmWr						= $02;
	dmWrDenyRd					= $12;
	dmWrDenyWr					= $22;
	dmWrDenyRdWr				= $32;
	dmRdWr						= $03;							{  Shared access - equivalent to fsRdWrShPerm  }
	dmRdWrDenyRd				= $13;
	dmRdWrDenyWr				= $23;							{  Single writer, multiple readers; the writer  }
	dmRdWrDenyRdWr				= $33;							{  Exclusive access - equivalent to fsRdWrPerm  }


	{	***************************************************************************	}

	{	
	**  For those times where you need to use more than one kind of File Manager parameter
	**  block but don't feel like wasting stack space, here's a parameter block you can reuse.
		}


TYPE
	UniversalFMPBPtr = ^UniversalFMPB;
	UniversalFMPB = RECORD
		CASE INTEGER OF
		0: (
			PB:					ParamBlockRec;
			);
		1: (
			ciPB:				CInfoPBRec;
			);
		2: (
			dtPB:				DTPBRec;
			);
		3: (
			hPB:				HParamBlockRec;
			);
		4: (
			cmPB:				CMovePBRec;
			);
		5: (
			wdPB:				WDPBRec;
			);
		6: (
			fcbPB:				FCBPBRec;
			);
		7: (
			xPB:				XVolumeParam;
			);
	END;

	UniversalFMPBHandle					= ^UniversalFMPBPtr;

	{	
	**  Used by GetUGEntries to return user or group lists
		}
	UGEntryPtr = ^UGEntry;
	UGEntry = RECORD
		objType:				INTEGER;								{  object type: -1 = group; 0 = user  }
		objID:					LONGINT;								{  the user or group ID  }
		name:					Str31;									{  the user or group name  }
	END;

	UGEntryHandle						= ^UGEntryPtr;

	{	
	**  I use the following records instead of the AFPVolMountInfo and AFPXVolMountInfo structures in Files.h
		}
	Str8								= STRING[8];
	MyAFPVolMountInfoPtr = ^MyAFPVolMountInfo;
	MyAFPVolMountInfo = RECORD
		length:					INTEGER;								{  length of this record  }
		media:					VolumeType;								{  type of media, always AppleShareMediaType  }
		flags:					INTEGER;								{  0 = normal mount; set bit 0 to inhibit greeting messages  }
		nbpInterval:			SInt8;									{  NBP interval parameter; 7 is a good choice  }
		nbpCount:				SInt8;									{  NBP count parameter; 5 is a good choice  }
		uamType:				INTEGER;								{  User Authentication Method  }
		zoneNameOffset:			INTEGER;								{  offset from start of record to zoneName  }
		serverNameOffset:		INTEGER;								{  offset from start of record to serverName  }
		volNameOffset:			INTEGER;								{  offset from start of record to volName  }
		userNameOffset:			INTEGER;								{  offset from start of record to userName  }
		userPasswordOffset:		INTEGER;								{  offset from start of record to userPassword  }
		volPasswordOffset:		INTEGER;								{  offset from start of record to volPassword  }
		zoneName:				Str32;									{  server's AppleTalk zone name  }
		filler1:				SInt8;									{  to word align volPassword  }
		serverName:				Str32;									{  server name  }
		filler2:				SInt8;									{  to word align volPassword  }
		volName:				Str27;									{  volume name  }
		userName:				Str31;									{  user name (zero length Pascal string for guest)  }
		userPassword:			Str8;									{  user password (zero length Pascal string if no user password)  }
		filler3:				SInt8;									{  to word align volPassword  }
		volPassword:			Str8;									{  volume password (zero length Pascal string if no volume password)  }
		filler4:				SInt8;									{  to end record on word boundry  }
	END;

	MyAFPVolMountInfoHandle				= ^MyAFPVolMountInfoPtr;
	MyAFPXVolMountInfoPtr = ^MyAFPXVolMountInfo;
	MyAFPXVolMountInfo = RECORD
		length:					INTEGER;								{  length of this record  }
		media:					VolumeType;								{  type of media, always AppleShareMediaType  }
		flags:					INTEGER;								{  bits for no messages, no reconnect, etc  }
		nbpInterval:			SInt8;									{  NBP interval parameter; 7 is a good choice  }
		nbpCount:				SInt8;									{  NBP count parameter; 5 is a good choice  }
		uamType:				INTEGER;								{  User Authentication Method  }
		zoneNameOffset:			INTEGER;								{  offset from start of record to zoneName  }
		serverNameOffset:		INTEGER;								{  offset from start of record to serverName  }
		volNameOffset:			INTEGER;								{  offset from start of record to volName  }
		userNameOffset:			INTEGER;								{  offset from start of record to userName  }
		userPasswordOffset:		INTEGER;								{  offset from start of record to userPassword  }
		volPasswordOffset:		INTEGER;								{  offset from start of record to volPassword  }
		extendedFlags:			INTEGER;								{  extended flags word  }
		uamNameOffset:			INTEGER;								{  offset to a pascal UAM name string  }
		alternateAddressOffset:	INTEGER;								{  offset to Alternate Addresses in tagged format  }
		zoneName:				Str32;									{  server's AppleTalk zone name  }
		filler1:				SInt8;									{  to word align volPassword  }
		serverName:				Str32;									{  server name  }
		filler2:				SInt8;									{  to word align volPassword  }
		volName:				Str27;									{  volume name  }
		userName:				Str31;									{  user name (zero length Pascal string for guest)  }
		userPassword:			Str8;									{  user password (zero length Pascal string if no user password)  }
		filler3:				SInt8;									{  to word align volPassword  }
		volPassword:			Str8;									{  volume password (zero length Pascal string if no volume password)  }
		filler4:				SInt8;									{  to word align uamNameOffset  }
		uamName:				Str32;									{  UAM name  }
		filler5:				SInt8;									{  to word align alternateAddress  }
		alternateAddress:		SInt8;									{  AFPAlternateAddress  }
	END;

	MyAFPXVolMountInfoHandle			= ^MyAFPXVolMountInfoPtr;

	{	***************************************************************************	}

	{	 Functions to get information out of GetVolParmsInfoBuffer. 	}

	{	 version 1 field getters 	}

FUNCTION GetVolParmsInfoVersion({CONST}VAR volParms: GetVolParmsInfoBuffer): INTEGER;

FUNCTION GetVolParmsInfoAttrib({CONST}VAR volParms: GetVolParmsInfoBuffer): LONGINT;

FUNCTION GetVolParmsInfoLocalHand({CONST}VAR volParms: GetVolParmsInfoBuffer): Handle;

FUNCTION GetVolParmsInfoServerAdr({CONST}VAR volParms: GetVolParmsInfoBuffer): LONGINT;


{ version 2 field getters (assume zero result if version < 2) }

FUNCTION GetVolParmsInfoVolumeGrade({CONST}VAR volParms: GetVolParmsInfoBuffer): LONGINT;

FUNCTION GetVolParmsInfoForeignPrivID({CONST}VAR volParms: GetVolParmsInfoBuffer): LONGINT;


{ version 3 field getters (assume zero result if version < 3) }

FUNCTION GetVolParmsInfoExtendedAttributes({CONST}VAR volParms: GetVolParmsInfoBuffer): LONGINT;


{ attribute bits supported by all versions of GetVolParmsInfoBuffer }

FUNCTION isNetworkVolume({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasLimitFCBs({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasLocalWList({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoMiniFndr({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoVNEdit({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoLclSync({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasTrshOffLine({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoSwitchTo({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoDeskItems({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoBootBlks({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasAccessCntl({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasNoSysDir({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasExtFSVol({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasOpenDeny({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasCopyFile({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasMoveRename({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasDesktopMgr({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasShortName({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasFolderLock({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasPersonalAccessPrivileges({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasUserGroupList({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasCatSearch({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasFileIDs({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasBTreeMgr({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION hasBlankAccessPrivileges({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION supportsAsyncRequests({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION supportsTrashVolumeCache({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;


{ attribute bits supported by version 3 and greater versions of GetVolParmsInfoBuffer }

FUNCTION volIsEjectable({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsHFSPlusAPIs({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsFSCatalogSearch({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsFSExchangeObjects({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupports2TBFiles({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsLongNames({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsMultiScriptNames({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsNamedForks({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volSupportsSubtreeIterators({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;

FUNCTION volL2PCanMapFileBlocks({CONST}VAR volParms: GetVolParmsInfoBuffer): BOOLEAN;


{***************************************************************************}

{ Functions for testing ioACUser bits. }

FUNCTION userIsOwner(ioACUser: SInt8): BOOLEAN;

FUNCTION userHasFullAccess(ioACUser: SInt8): BOOLEAN;

FUNCTION userHasDropBoxAccess(ioACUser: SInt8): BOOLEAN;

FUNCTION userHasBulletinBoard(ioACUser: SInt8): BOOLEAN;

FUNCTION userHasNoAccess(ioACUser: SInt8): BOOLEAN;


{***************************************************************************}

PROCEDURE TruncPString(destination: StringPtr; source: Str255; maxLength: INTEGER);

{
    The TruncPString function copies up to maxLength characters from
    the source Pascal string to the destination Pascal string. TruncPString
    ensures that the truncated string ends on a single-byte character, or on
    the last byte of a multi-byte character.
    
    destination     output: destination Pascal string.
    source          input:  source Pascal string.
    maxLength       output: The maximum allowable length of the destination
                            string.
}

{***************************************************************************}

FUNCTION GetTempBuffer(buffReqSize: LONGINT; VAR buffActSize: LONGINT): Ptr;

{
    The GetTempBuffer function allocates a temporary buffer for file system
    operations which is at least 1024 bytes (1K) and a multiple of
    1024 bytes.
    
    buffReqSize     input:  Size you'd like the buffer to be.
    buffActSize     output: Size of buffer allocated.
    function result output: Pointer to memory allocated or nil if no memory
                            was available. The caller is responsible for
                            disposing of this buffer with DisposePtr.
}

{***************************************************************************}

FUNCTION GetVolumeInfoNoName(pathname: Str255; vRefNum: INTEGER; pb: HParmBlkPtr): OSErr;

{
    GetVolumeInfoNoName uses pathname and vRefNum to call PBHGetVInfoSync
    in cases where the returned volume name is not needed by the caller.
    The pathname and vRefNum parameters are not touched, and the pb
    parameter is initialized by PBHGetVInfoSync except that ioNamePtr in
    the parameter block is always returned as NULL (since it might point
    to GetVolumeInfoNoName's local variable tempPathname).

    I noticed using this code in several places, so here it is once.
    This reduces the code size of MoreFiles.

    pathName    input:  Pointer to a full pathname or nil.  If you pass in a 
                        partial pathname, it is ignored. A full pathname to a
                        volume must end with a colon character (:).
    vRefNum     input:  Volume specification (volume reference number, working
                        directory number, drive number, or 0).
    pb          input:  A pointer to HParamBlockRec.
                output: The parameter block as filled in by PBHGetVInfoSync
                        except that ioNamePtr will always be NULL.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume, or pb was NULL
}

{***************************************************************************}

FUNCTION XGetVolumeInfoNoName(pathname: Str255; vRefNum: INTEGER; pb: XVolumeParamPtr): OSErr;

{
    XGetVolumeInfoNoName uses pathname and vRefNum to call PBXGetVolInfoSync
    in cases where the returned volume name is not needed by the caller.
    The pathname and vRefNum parameters are not touched, and the pb
    parameter is initialized by PBXGetVolInfoSync except that ioNamePtr in
    the parameter block is always returned as NULL (since it might point
    to XGetVolumeInfoNoName's local variable tempPathname).

    pathName    input:  Pointer to a full pathname or nil.  If you pass in a 
                        partial pathname, it is ignored. A full pathname to a
                        volume must end with a colon character (:).
    vRefNum     input:  Volume specification (volume reference number, working
                        directory number, drive number, or 0).
    pb          input:  A pointer to HParamBlockRec.
                output: The parameter block as filled in by PBXGetVolInfoSync
                        except that ioNamePtr will always be NULL.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume, or pb was NULL
}

{***************************************************************************}

FUNCTION GetCatInfoNoName(vRefNum: INTEGER; dirID: LONGINT; name: Str255; pb: CInfoPBPtr): OSErr;

{
    GetCatInfoNoName uses vRefNum, dirID and name to call PBGetCatInfoSync
    in cases where the returned object is not needed by the caller.
    The vRefNum, dirID and name parameters are not touched, and the pb
    parameter is initialized by PBGetCatInfoSync except that ioNamePtr in
    the parameter block is always returned as NULL (since it might point
    to GetCatInfoNoName's local variable tempName).

    I noticed using this code in several places, so here it is once.
    This reduces the code size of MoreFiles.

    vRefNum         input:  Volume specification.
    dirID           input:  Directory ID.
    name            input:  Pointer to object name, or nil when dirID
                            specifies a directory that's the object.
    pb              input:  A pointer to CInfoPBRec.
                    output: The parameter block as filled in by
                            PBGetCatInfoSync except that ioNamePtr will
                            always be NULL.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
        
}

{***************************************************************************}

FUNCTION DetermineVRefNum(pathname: Str255; vRefNum: INTEGER; VAR realVRefNum: INTEGER): OSErr;

{
    The DetermineVRefNum function determines the volume reference number of
    a volume from a pathname, a volume specification, or a combination
    of the two.
    WARNING: Volume names on the Macintosh are *not* unique -- Multiple
    mounted volumes can have the same name. For this reason, the use of a
    volume name or full pathname to identify a specific volume may not
    produce the results you expect.  If more than one volume has the same
    name and a volume name or full pathname is used, the File Manager
    currently uses the first volume it finds with a matching name in the
    volume queue.

    pathName    input:  Pointer to a full pathname or nil.  If you pass in a 
                        partial pathname, it is ignored. A full pathname to a
                        volume must end with a colon character (:).
    vRefNum     input:  Volume specification (volume reference number, working
                        directory number, drive number, or 0).
    realVRefNum output: The real volume reference number.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume
}

{***************************************************************************}

FUNCTION HGetVInfo(volReference: INTEGER; volName: StringPtr; VAR vRefNum: INTEGER; VAR freeBytes: UInt32; VAR totalBytes: UInt32): OSErr;

{
    The HGetVInfo function returns the name, volume reference number,
    available space (in bytes), and total space (in bytes) for the
    specified volume. You can specify the volume by providing its drive
    number, volume reference number, or 0 for the default volume.
    This routine is compatible with volumes up to 4 gigabytes.
    
    volReference    input:  The drive number, volume reference number,
                            or 0 for the default volume.
    volName         input:  A pointer to a buffer (minimum Str27) where
                            the volume name is to be returned or must
                            be nil.
                    output: The volume name.
    vRefNum         output: The volume reference number.
    freeBytes       output: The number of free bytes on the volume.
                            freeBytes is an unsigned long value.
    totalBytes      output: The total number of bytes on the volume.
                            totalBytes is an unsigned long value.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume
    
    __________
    
    Also see:   XGetVInfo
}

{***************************************************************************}

FUNCTION XGetVInfo(volReference: INTEGER; volName: StringPtr; VAR vRefNum: INTEGER; VAR freeBytes: UInt64; VAR totalBytes: UInt64): OSErr;

{
    The XGetVInfo function returns the name, volume reference number,
    available space (in bytes), and total space (in bytes) for the
    specified volume. You can specify the volume by providing its drive
    number, volume reference number, or 0 for the default volume.
    This routine is compatible with volumes up to 2 terabytes.
    
    volReference    input:  The drive number, volume reference number,
                            or 0 for the default volume.
    volName         input:  A pointer to a buffer (minimum Str27) where
                            the volume name is to be returned or must
                            be nil.
                    output: The volume name.
    vRefNum         output: The volume reference number.
    freeBytes       output: The number of free bytes on the volume.
                            freeBytes is an UnsignedWide value.
    totalBytes      output: The total number of bytes on the volume.
                            totalBytes is an UnsignedWide value.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume
    
    __________
    
    Also see:   HGetVInfo
}

{***************************************************************************}

FUNCTION CheckVolLock(pathname: Str255; vRefNum: INTEGER): OSErr;

{
    The CheckVolLock function determines if a volume is locked - either by
    hardware or by software. If CheckVolLock returns noErr, then the volume
    is not locked.

    pathName    input:  Pointer to a full pathname or nil.  If you pass in a 
                        partial pathname, it is ignored. A full pathname to a
                        volume must end with a colon character (:).
    vRefNum     input:  Volume specification (volume reference number, working
                        directory number, drive number, or 0).
    
    Result Codes
        noErr               0       No error - volume not locked
        nsvErr              -35     No such volume
        wPrErr              -44     Volume locked by hardware
        vLckdErr            -46     Volume locked by software
        paramErr            -50     No default volume
}

{***************************************************************************}
{
**  The following routines call Mac OS routines that are not supported by
**  Carbon:
**  
**      GetDriverName
**      FindDrive
**      GetDiskBlocks
**      GetVolState
}


{***************************************************************************}

FUNCTION GetDriverName(driverRefNum: INTEGER; VAR driverName: Str255): OSErr;

{
    The GetDriverName function returns a device driver's name.

    driverRefNum    input:  The driver reference number.
    driverName      output: The driver's name.
    
    Result Codes
        noErr               0       No error
        badUnitErr          -21     Bad driver reference number
}

{***************************************************************************}

FUNCTION FindDrive(pathname: Str255; vRefNum: INTEGER; VAR driveQElementPtr: DrvQElPtr): OSErr;

{
    The FindDrive function returns a pointer to a mounted volume's
    drive queue element.

    pathName            input:  Pointer to a full pathname or nil. If you
                                pass in a partial pathname, it is ignored.
                                A full pathname to a volume must end with
                                a colon character (:).
    vRefNum             input:  Volume specification (volume reference
                                number, working directory number, drive
                                number, or 0).
    driveQElementPtr    output: Pointer to a volume's drive queue element
                                in the drive queue. DO NOT change the
                                DrvQEl.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume
        nsDrvErr            -56     No such drive
}

{***************************************************************************}

FUNCTION GetDiskBlocks(pathname: Str255; vRefNum: INTEGER; VAR numBlocks: UInt32): OSErr;

{
    The GetDiskBlocks function returns the number of physical disk
    blocks on a disk drive. NOTE: This is not the same as volume
    allocation blocks!

    pathName    input:  Pointer to a full pathname or nil. If you
                        pass in a partial pathname, it is ignored.
                        A full pathname to a volume must end with
                        a colon character (:).
    vRefNum     input:  Volume specification (volume reference
                        number, working directory number, drive
                        number, or 0).
    numBlocks   output: The number of physical disk blocks on the disk drive.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume, driver reference
                                    number is zero, ReturnFormatList
                                    returned zero blocks, DriveStatus
                                    returned an unknown value, or
                                    driveQElementPtr->qType is unknown
        nsDrvErr            -56     No such drive
        statusErr           �18     Driver does not respond to this
                                    status request
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies
                                    a nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
}

{***************************************************************************}

FUNCTION GetVolState(pathname: Str255; vRefNum: INTEGER; VAR volumeOnline: BOOLEAN; VAR volumeEjected: BOOLEAN; VAR driveEjectable: BOOLEAN; VAR driverWantsEject: BOOLEAN): OSErr;

{
    The GetVolState function determines if a volume is online or offline,
    if an offline volume is ejected, and if the volume's driver is
    ejectable or wants eject calls.
    
    pathName            input:  Pointer to a full pathname or nil.
    vRefNum             input:  Volume specification (volume reference number,
                                working directory number, drive number, or 0).
    volumeOnline        output: True if the volume is online;
                                False if the volume is offline.
    volumeEjected       output: True if the volume is ejected (ejected
                                volumes are always offline); False if the
                                volume is not ejected.
    driveEjectable      output: True if the volume's drive is ejectable;
                                False if the volume's drive is not ejectable.
    driverWantsEject    output: True if the volume's driver wants an Eject
                                request after unmount (even if the drive
                                is not ejectable); False if the volume's
                                driver does not need an eject request.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume, or pb was NULL
}

{***************************************************************************}


{***************************************************************************}

FUNCTION GetVolFileSystemID(pathname: Str255; vRefNum: INTEGER; VAR fileSystemID: INTEGER): OSErr;

{
    The GetVolFileSystemID function returned the file system ID of
    a mounted volume. The file system ID identifies the file system
    that handles requests to a particular volume. Here's a partial list
    of file system ID numbers (only Apple's file systems are listed):
        FSID    File System
        -----   -----------------------------------------------------
        $0000   Macintosh HFS or MFS
        $0100   ProDOS File System
        $0101   PowerTalk Mail Enclosures
        $4147   ISO 9660 File Access (through Foreign File Access)
        $4242   High Sierra File Access (through Foreign File Access)
        $464D   QuickTake File System (through Foreign File Access)
        $4953   Macintosh PC Exchange (MS-DOS)
        $4A48   Audio CD Access (through Foreign File Access)
        $4D4B   Apple Photo Access (through Foreign File Access)
    
    See the Technical Note "FL 35 - Determining Which File System
    Is Active" and the "Guide to the File System Manager" for more
    information.
    
    pathName        input:  Pointer to a full pathname or nil.  If you pass
                            in a partial pathname, it is ignored. A full
                            pathname to a volume must contain at least
                            one colon character (:) and must not start with
                            a colon character.
    vRefNum         input:  Volume specification (volume reference number,
                            working directory number, drive number, or 0).
    fileSystemID    output: The volume's file system ID.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        paramErr            -50     No default volume, or pb was NULL
}

{***************************************************************************}

FUNCTION UnmountAndEject(pathname: Str255; vRefNum: INTEGER): OSErr;

{
    The UnmountAndEject function unmounts and ejects a volume. The volume
    is ejected only if it is ejectable and not already ejected.
    
    pathName    input:  Pointer to a full pathname or nil.  If you pass in a 
                        partial pathname, it is ignored. A full pathname to a
                        volume must end with a colon character (:).
    vRefNum     input:  Volume specification (volume reference number, working
                        directory number, drive number, or 0).
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad volume name
        fBsyErr             -47     One or more files are open
        paramErr            -50     No default volume
        nsDrvErr            -56     No such drive
        extFSErr            -58     External file system error - no file
                                    system claimed this call.
}

{***************************************************************************}

FUNCTION OnLine(volumes: FSSpecPtr; reqVolCount: INTEGER; VAR actVolCount: INTEGER; VAR volIndex: INTEGER): OSErr;

{
    The OnLine function returns the list of volumes currently mounted in
    an array of FSSpec records.
    
    A noErr result indicates that the volumes array was filled
    (actVolCount == reqVolCount) and there may be additional volumes
    mounted. A nsvErr result indicates that the end of the volume list
    was found and actVolCount volumes were actually found this time.

    volumes     input:  Pointer to array of FSSpec where the volume list
                        is returned.
    reqVolCount input:  Maximum number of volumes to return (the number of
                        elements in the volumes array).
    actVolCount output: The number of volumes actually returned.
    volIndex    input:  The current volume index position. Set to 1 to
                        start with the first volume.
                output: The volume index position to get the next volume.
                        Pass this value the next time you call OnLine to
                        start where you left off.
    
    Result Codes
        noErr               0       No error, but there are more volumes
                                    to list
        nsvErr              -35     No more volumes to be listed
        paramErr            -50     volIndex was <= 0
}

{***************************************************************************}

FUNCTION SetDefault(newVRefNum: INTEGER; newDirID: LONGINT; VAR oldVRefNum: INTEGER; VAR oldDirID: LONGINT): OSErr;

{
    The SetDefault function sets the default volume and directory to the
    volume specified by newVRefNum and the directory specified by newDirID.
    The current default volume reference number and directory ID are
    returned in oldVRefNum and oldDir and must be used to restore the
    default volume and directory to their previous state *as soon as
    possible* with the RestoreDefault function. These two functions are
    designed to be used as a wrapper around Standard I/O routines where
    the location of the file is implied to be the default volume and
    directory. In other words, this is how you should use these functions:
    
        error = SetDefault(newVRefNum, newDirID, &oldVRefNum, &oldDirID);
        if ( error == noErr )
        (
            // call the Stdio functions like remove, rename, tmpfile,
            // fopen, freopen, etc. or non-ANSI extensions like
            // fdopen,fsetfileinfo, -- create, open, unlink, etc. here!
            
            error = RestoreDefault(oldVRefNum, oldDirID);
        )
    
    By using these functions as a wrapper, you won't need to open a working
    directory (because SetDefault and RestoreDefault use HSetVol) and you
    won't have to worry about the effects of using HSetVol (documented in
    Technical Note "FL 11 - PBHSetVol is Dangerous" and in the
    Inside Macintosh: Files book in the description of the HSetVol and 
    PBHSetVol functions) because the default volume/directory is restored
    before giving up control to code that might be affected by HSetVol.
    
    newVRefNum  input:  Volume specification (volume reference number,
                        working directory number, drive number, or 0) of
                        the new default volume.
    newDirID    input:  Directory ID of the new default directory.
    oldVRefNum  output: The volume specification to save for use with
                        RestoreDefault.
    oldDirID    output: The directory ID to save for use with
                        RestoreDefault.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        bdNamErr            -37     Bad volume name
        fnfErr              -43     Directory not found
        paramErr            -50     No default volume
        afpAccessDenied     -5000   User does not have access to the directory
    
    __________
    
    Also see:   RestoreDefault
}

{***************************************************************************}

FUNCTION RestoreDefault(oldVRefNum: INTEGER; oldDirID: LONGINT): OSErr;

{
    The RestoreDefault function restores the default volume and directory
    to the volume specified by oldVRefNum and the directory specified by 
    oldDirID. The oldVRefNum and oldDirID parameters were previously
    obtained from the SetDefault function. These two functions are designed
    to be used as a wrapper around Standard C I/O routines where the
    location of the file is implied to be the default volume and directory.
    In other words, this is how you should use these functions:
    
        error = SetDefault(newVRefNum, newDirID, &oldVRefNum, &oldDirID);
        if ( error == noErr )
        (
            // call the Stdio functions like remove, rename, tmpfile,
            // fopen, freopen, etc. or non-ANSI extensions like
            // fdopen,fsetfileinfo, -- create, open, unlink, etc. here!
            
            error = RestoreDefault(oldVRefNum, oldDirID);
        )
    
    By using these functions as a wrapper, you won't need to open a working
    directory (because SetDefault and RestoreDefault use HSetVol) and you
    won't have to worry about the effects of using HSetVol (documented in
    Technical Note "FL 11 - PBHSetVol is Dangerous" and in the
    Inside Macintosh: Files book in the description of the HSetVol and 
    PBHSetVol functions) because the default volume/directory is restored
    before giving up control to code that might be affected by HSetVol.
    
    oldVRefNum  input: The volume specification to restore.
    oldDirID    input:  The directory ID to restore.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        bdNamErr            -37     Bad volume name
        fnfErr              -43     Directory not found
        paramErr            -50     No default volume
        rfNumErr            -51     Bad working directory reference number
        afpAccessDenied     -5000   User does not have access to the directory
    
    __________
    
    Also see:   SetDefault
}

{***************************************************************************}

FUNCTION GetDInfo(vRefNum: INTEGER; dirID: LONGINT; name: Str255; VAR fndrInfo: DInfo): OSErr;

{
    The GetDInfo function gets the finder information for a directory.

    vRefNum         input:  Volume specification.
    dirID           input:  Directory ID.
    name            input:  Pointer to object name, or nil when dirID
                            specifies a directory that's the object.
    fndrInfo        output: If the object is a directory, then its DInfo.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
        
    __________
    
    Also see:   FSpGetDInfo, FSpGetFInfoCompat
}

{***************************************************************************}

FUNCTION FSpGetDInfo({CONST}VAR spec: FSSpec; VAR fndrInfo: DInfo): OSErr;

{
    The FSpGetDInfo function gets the finder information for a directory.

    spec        input:  An FSSpec record specifying the directory.
    fndrInfo    output: If the object is a directory, then its DInfo.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
        
    __________
    
    Also see:   FSpGetFInfoCompat, GetDInfo
}

{***************************************************************************}

FUNCTION SetDInfo(vRefNum: INTEGER; dirID: LONGINT; name: Str255; {CONST}VAR fndrInfo: DInfo): OSErr;

{
    The SetDInfo function sets the finder information for a directory.

    vRefNum         input:  Volume specification.
    dirID           input:  Directory ID.
    name            input:  Pointer to object name, or nil when dirID
                            specifies a directory that's the object.
    fndrInfo        input:  The DInfo.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   FSpSetDInfo, FSpSetFInfoCompat
}

{***************************************************************************}

FUNCTION FSpSetDInfo({CONST}VAR spec: FSSpec; {CONST}VAR fndrInfo: DInfo): OSErr;

{
    The FSpSetDInfo function sets the finder information for a directory.

    spec        input:  An FSSpec record specifying the directory.
    fndrInfo    input:  The DInfo.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   FSpSetFInfoCompat, SetDInfo
}

{***************************************************************************}

FUNCTION GetDirectoryID(vRefNum: INTEGER; dirID: LONGINT; name: Str255; VAR theDirID: LONGINT; VAR isDirectory: BOOLEAN): OSErr;

{
    The GetDirectoryID function gets the directory ID number of the
    directory specified.  If a file is specified, then the parent
    directory of the file is returned and isDirectory is false.  If
    a directory is specified, then that directory's ID number is
    returned and isDirectory is true.
    WARNING: Volume names on the Macintosh are *not* unique -- Multiple
    mounted volumes can have the same name. For this reason, the use of a
    volume name or full pathname to identify a specific volume may not
    produce the results you expect.  If more than one volume has the same
    name and a volume name or full pathname is used, the File Manager
    currently uses the first volume it finds with a matching name in the
    volume queue.
    
    vRefNum         input:  Volume specification.
    dirID           input:  Directory ID.
    name            input:  Pointer to object name, or nil when dirID
                            specifies a directory that's the object.
    theDirID        output: If the object is a file, then its parent directory
                            ID. If the object is a directory, then its ID.
    isDirectory     output: True if object is a directory; false if
                            object is a file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION FSpGetDirectoryID({CONST}VAR spec: FSSpec; VAR theDirID: LONGINT; VAR isDirectory: BOOLEAN): OSErr;

{
    The FSpGetDirectoryID function gets the directory ID number of the
    directory specified by spec. If spec is to a file, then the parent
    directory of the file is returned and isDirectory is false.  If
    spec is to a directory, then that directory's ID number is
    returned and isDirectory is true.
    
    spec            input:  An FSSpec record specifying the directory.
    theDirID        output: The directory ID.
    isDirectory     output: True if object is a directory; false if
                            object is a file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION GetDirName(vRefNum: INTEGER; dirID: LONGINT; VAR name: Str31): OSErr;

{
    The GetDirName function gets the name of a directory from its
    directory ID.

    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        output: Points to a Str31 where the directory name is to be
                        returned.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume or
                                    name parameter was NULL
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION GetIOACUser(vRefNum: INTEGER; dirID: LONGINT; name: Str255; VAR ioACUser: SInt8): OSErr;

{
    GetIOACUser returns a directory's access restrictions byte.
    Use the masks and macro defined in MoreFilesExtras to check for
    specific access priviledges.
    
    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        input:  Pointer to object name, or nil when dirID
                        specifies a directory that's the object.
    ioACUser    output: The access restriction byte
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION FSpGetIOACUser({CONST}VAR spec: FSSpec; VAR ioACUser: SInt8): OSErr;

{
    FSpGetIOACUser returns a directory's access restrictions byte.
    Use the masks and macro defined in MoreFilesExtras to check for
    specific access priviledges.
    
    spec        input:  An FSSpec record specifying the directory.
    ioACUser    output: The access restriction byte
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION GetParentID(vRefNum: INTEGER; dirID: LONGINT; name: Str255; VAR parID: LONGINT): OSErr;

{
    The GetParentID function gets the parent directory ID number of the
    specified object.
    
    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        input:  Pointer to object name, or nil when dirID specifies
                        a directory that's the object.
    parID       output: The parent directory ID of the specified object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION GetFilenameFromPathname(pathname: Str255; VAR filename: Str255): OSErr;

{
    The GetFilenameFromPathname function gets the file (or directory) name
    from the end of a full or partial pathname. Returns notAFileErr if the
    pathname is nil, the pathname is empty, or the pathname cannot refer to
    a filename (with a noErr result, the pathname could still refer to a
    directory).
    
    pathname    input:  A full or partial pathname.
    filename    output: The file (or directory) name.
    
    Result Codes
        noErr               0       No error
        notAFileErr         -1302   The pathname is nil, the pathname
                                    is empty, or the pathname cannot refer
                                    to a filename
    
    __________
    
    See also:   GetObjectLocation.
}

{***************************************************************************}

FUNCTION GetObjectLocation(vRefNum: INTEGER; dirID: LONGINT; pathname: Str255; VAR realVRefNum: INTEGER; VAR realParID: LONGINT; VAR realName: Str255; VAR isDirectory: BOOLEAN): OSErr;

{
    The GetObjectLocation function gets a file system object's location -
    that is, its real volume reference number, real parent directory ID,
    and name. While we're at it, determine if the object is a file or directory.
    If GetObjectLocation returns fnfErr, then the location information
    returned is valid, but it describes an object that doesn't exist.
    You can use the location information for another operation, such as
    creating a file or directory.
    
    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    pathname    input:  Pointer to object name, or nil when dirID specifies
                        a directory that's the object.
    realVRefNum output: The real volume reference number.
    realParID   output: The parent directory ID of the specified object.
    realName    output: The name of the specified object (the case of the
                        object name may not be the same as the object's
                        catalog entry on disk - since the Macintosh file
                        system is not case sensitive, it shouldn't matter).
    isDirectory output: True if object is a directory; false if object
                        is a file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        notAFileErr         -1302   The pathname is nil, the pathname
                                    is empty, or the pathname cannot refer
                                    to a filename
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSMakeFSSpecCompat
}

{***************************************************************************}

FUNCTION GetDirItems(vRefNum: INTEGER; dirID: LONGINT; name: Str255; getFiles: BOOLEAN; getDirectories: BOOLEAN; items: FSSpecPtr; reqItemCount: INTEGER; VAR actItemCount: INTEGER; VAR itemIndex: INTEGER): OSErr;

{
    The GetDirItems function returns a list of items in the specified
    directory in an array of FSSpec records. File, subdirectories, or
    both can be returned in the list.
    
    A noErr result indicates that the items array was filled
    (actItemCount == reqItemCount) and there may be additional items
    left in the directory. A fnfErr result indicates that the end of
    the directory list was found and actItemCount items were actually
    found this time.

    vRefNum         input:  Volume specification.
    dirID           input:  Directory ID.
    name            input:  Pointer to object name, or nil when dirID
                            specifies a directory that's the object.
    getFiles        input:  Pass true to have files added to the items list.
    getDirectories  input:  Pass true to have directories added to the
                            items list.
    items           input:  Pointer to array of FSSpec where the item list
                            is returned.
    reqItemCount    input:  Maximum number of items to return (the number
                            of elements in the items array).
    actItemCount    output: The number of items actually returned.
    itemIndex       input:  The current item index position. Set to 1 to
                            start with the first item in the directory.
                    output: The item index position to get the next item.
                            Pass this value the next time you call
                            GetDirItems to start where you left off.
    
    Result Codes
        noErr               0       No error, but there are more items
                                    to list
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found, there are no more items
                                    to be listed.
        paramErr            -50     No default volume or itemIndex was <= 0
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
}

{***************************************************************************}

FUNCTION DeleteDirectoryContents(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The DeleteDirectoryContents function deletes the contents of a directory.
    All files and subdirectories in the specified directory are deleted.
    If a locked file or directory is encountered, it is unlocked and then
    deleted.  If any unexpected errors are encountered,
    DeleteDirectoryContents quits and returns to the caller.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to directory name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        wPrErr              -44     Hardware volume lock    
        fLckdErr            -45     File is locked  
        vLckdErr            -46     Software volume lock    
        fBsyErr             -47     File busy, directory not empty, or working directory control block open 
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   DeleteDirectory
}

{***************************************************************************}

FUNCTION DeleteDirectory(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The DeleteDirectory function deletes a directory and its contents.
    All files and subdirectories in the specified directory are deleted.
    If a locked file or directory is encountered, it is unlocked and then
    deleted.  After deleting the directories contents, the directory is
    deleted. If any unexpected errors are encountered, DeleteDirectory
    quits and returns to the caller.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to directory name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        wPrErr              -44     Hardware volume lock
        fLckdErr            -45     File is locked
        vLckdErr            -46     Software volume lock
        fBsyErr             -47     File busy, directory not empty, or working directory control block open 
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   DeleteDirectoryContents
}

{***************************************************************************}

FUNCTION CheckObjectLock(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The CheckObjectLock function determines if a file or directory is locked.
    If CheckObjectLock returns noErr, then the file or directory
    is not locked. If CheckObjectLock returns fLckdErr, the it is locked.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   FSpCheckObjectLock
}

{***************************************************************************}

FUNCTION FSpCheckObjectLock({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpCheckObjectLock function determines if a file or directory is locked.
    If FSpCheckObjectLock returns noErr, then the file or directory
    is not locked.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    Also see:   CheckObjectLock
}

{***************************************************************************}

FUNCTION GetFileSize(vRefNum: INTEGER; dirID: LONGINT; fileName: Str255; VAR dataSize: LONGINT; VAR rsrcSize: LONGINT): OSErr;

{
    The GetFileSize function returns the logical size of a file's
    data and resource fork.
    
    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        input:  The name of the file.
    dataSize    output: The number of bytes in the file's data fork.
    rsrcSize    output: The number of bytes in the file's resource fork.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErrdirNFErr    -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpGetFileSize
}

{***************************************************************************}

FUNCTION FSpGetFileSize({CONST}VAR spec: FSSpec; VAR dataSize: LONGINT; VAR rsrcSize: LONGINT): OSErr;

{
    The FSpGetFileSize function returns the logical size of a file's
    data and resource fork.
    
    spec        input:  An FSSpec record specifying the file.
    dataSize    output: The number of bytes in the file's data fork.
    rsrcSize    output: The number of bytes in the file's resource fork.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        paramErr            -50     No default volume
        dirNFErrdirNFErr    -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   GetFileSize
}

{***************************************************************************}

FUNCTION BumpDate(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The BumpDate function changes the modification date of a file or
    directory to the current date/time.  If the modification date is already
    equal to the current date/time, then add one second to the
    modification date.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpBumpDate
}

{***************************************************************************}

FUNCTION FSpBumpDate({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpBumpDate function changes the modification date of a file or
    directory to the current date/time.  If the modification date is already
    equal to the current date/time, then add one second to the
    modification date.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   BumpDate
}

{***************************************************************************}

FUNCTION ChangeCreatorType(vRefNum: INTEGER; dirID: LONGINT; name: Str255; creator: OSType; fileType: OSType): OSErr;

{
    The ChangeCreatorType function changes the creator or file type of a file.

    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        input:  The name of the file.
    creator     input:  The new creator type or 0x00000000 to leave
                        the creator type alone.
    fileType    input:  The new file type or 0x00000000 to leave the
                        file type alone.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        notAFileErr         -1302   Name was not a file
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpChangeCreatorType
}

{***************************************************************************}

FUNCTION FSpChangeCreatorType({CONST}VAR spec: FSSpec; creator: OSType; fileType: OSType): OSErr;

{
    The FSpChangeCreatorType function changes the creator or file type of a file.

    spec        input:  An FSSpec record specifying the file.
    creator     input:  The new creator type or 0x00000000 to leave
                        the creator type alone.
    fileType    input:  The new file type or 0x00000000 to leave the
                        file type alone.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        notAFileErr         -1302   Name was not a file
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   ChangeCreatorType
}

{***************************************************************************}

FUNCTION ChangeFDFlags(vRefNum: INTEGER; dirID: LONGINT; name: Str255; setBits: BOOLEAN; flagBits: UInt16): OSErr;

{
    The ChangeFDFlags function sets or clears Finder Flag bits in the
    fdFlags field of a file or directory's FInfo record.
    
    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    name        input:  Pointer to object name, or nil when dirID specifies
                        a directory that's the object.
    setBits     input:  If true, then set the bits specified in flagBits.
                        If false, then clear the bits specified in flagBits.
    flagBits    input:  The flagBits parameter specifies which Finder Flag
                        bits to set or clear. If a bit in flagBits is set,
                        then the same bit in fdFlags is either set or
                        cleared depending on the state of the setBits
                        parameter.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpChangeFDFlags
}

{***************************************************************************}

FUNCTION FSpChangeFDFlags({CONST}VAR spec: FSSpec; setBits: BOOLEAN; flagBits: UInt16): OSErr;

{
    The FSpChangeFDFlags function sets or clears Finder Flag bits in the
    fdFlags field of a file or directory's FInfo record.
    
    spec        input:  An FSSpec record specifying the object.
    setBits     input:  If true, then set the bits specified in flagBits.
                        If false, then clear the bits specified in flagBits.
    flagBits    input:  The flagBits parameter specifies which Finder Flag
                        bits to set or clear. If a bit in flagBits is set,
                        then the same bit in fdFlags is either set or
                        cleared depending on the state of the setBits
                        parameter.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   ChangeFDFlags
}

{***************************************************************************}

FUNCTION SetIsInvisible(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The SetIsInvisible function sets the invisible bit in the fdFlags
    word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpSetIsInvisible, ClearIsInvisible, FSpClearIsInvisible
}

{***************************************************************************}

FUNCTION FSpSetIsInvisible({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpSetIsInvisible function sets the invisible bit in the fdFlags
    word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsInvisible, ClearIsInvisible, FSpClearIsInvisible
}

{***************************************************************************}

FUNCTION ClearIsInvisible(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The ClearIsInvisible function clears the invisible bit in the fdFlags
    word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsInvisible, FSpSetIsInvisible, FSpClearIsInvisible
}

{***************************************************************************}

FUNCTION FSpClearIsInvisible({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpClearIsInvisible function clears the invisible bit in the fdFlags
    word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsInvisible, FSpSetIsInvisible, ClearIsInvisible
}

{***************************************************************************}

FUNCTION SetNameLocked(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The SetNameLocked function sets the nameLocked bit in the fdFlags word
    of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpSetNameLocked, ClearNameLocked, FSpClearNameLocked
}

{***************************************************************************}

FUNCTION FSpSetNameLocked({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpSetNameLocked function sets the nameLocked bit in the fdFlags word
    of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetNameLocked, ClearNameLocked, FSpClearNameLocked
}

{***************************************************************************}

FUNCTION ClearNameLocked(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The ClearNameLocked function clears the nameLocked bit in the fdFlags
    word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetNameLocked, FSpSetNameLocked, FSpClearNameLocked
}

{***************************************************************************}

FUNCTION FSpClearNameLocked({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpClearNameLocked function clears the nameLocked bit in the fdFlags
    word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetNameLocked, FSpSetNameLocked, ClearNameLocked
}

{***************************************************************************}

FUNCTION SetIsStationery(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The SetIsStationery function sets the isStationery bit in the
    fdFlags word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpSetIsStationery, ClearIsStationery, FSpClearIsStationery
}

{***************************************************************************}

FUNCTION FSpSetIsStationery({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpSetIsStationery function sets the isStationery bit in the
    fdFlags word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsStationery, ClearIsStationery, FSpClearIsStationery
}

{***************************************************************************}

FUNCTION ClearIsStationery(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The ClearIsStationery function clears the isStationery bit in the
    fdFlags word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsStationery, FSpSetIsStationery, FSpClearIsStationery
}

{***************************************************************************}

FUNCTION FSpClearIsStationery({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpClearIsStationery function clears the isStationery bit in the
    fdFlags word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetIsStationery, FSpSetIsStationery, ClearIsStationery
}

{***************************************************************************}

FUNCTION SetHasCustomIcon(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The SetHasCustomIcon function sets the hasCustomIcon bit in the
    fdFlags word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpSetHasCustomIcon, ClearHasCustomIcon, FSpClearHasCustomIcon
}

{***************************************************************************}

FUNCTION FSpSetHasCustomIcon({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpSetHasCustomIcon function sets the hasCustomIcon bit in the
    fdFlags word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetHasCustomIcon, ClearHasCustomIcon, FSpClearHasCustomIcon
}

{***************************************************************************}

FUNCTION ClearHasCustomIcon(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The ClearHasCustomIcon function clears the hasCustomIcon bit in the
    fdFlags word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetHasCustomIcon, FSpSetHasCustomIcon, FSpClearHasCustomIcon
}

{***************************************************************************}

FUNCTION FSpClearHasCustomIcon({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpClearHasCustomIcon function clears the hasCustomIcon bit in the
    fdFlags word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   SetHasCustomIcon, FSpSetHasCustomIcon, ClearHasCustomIcon
}

{***************************************************************************}

FUNCTION ClearHasBeenInited(vRefNum: INTEGER; dirID: LONGINT; name: Str255): OSErr;

{
    The ClearHasBeenInited function clears the hasBeenInited bit in the
    fdFlags word of the specified file or directory's finder information.
    
    vRefNum input:  Volume specification.
    dirID   input:  Directory ID.
    name    input:  Pointer to object name, or nil when dirID specifies
                    a directory that's the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpClearHasBeenInited
}

{***************************************************************************}

FUNCTION FSpClearHasBeenInited({CONST}VAR spec: FSSpec): OSErr;

{
    The FSpClearHasBeenInited function clears the hasBeenInited bit in the
    fdFlags word of the specified file or directory's finder information.
    
    spec    input:  An FSSpec record specifying the object.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   ClearHasBeenInited
}

{***************************************************************************}

FUNCTION CopyFileMgrAttributes(srcVRefNum: INTEGER; srcDirID: LONGINT; srcName: Str255; dstVRefNum: INTEGER; dstDirID: LONGINT; dstName: Str255; copyLockBit: BOOLEAN): OSErr;

{
    The CopyFileMgrAttributes function copies all File Manager attributes
    from the source file or directory to the destination file or directory.
    If copyLockBit is true, then set the locked state of the destination
    to match the source.

    srcVRefNum  input:  Source volume specification.
    srcDirID    input:  Source directory ID.
    srcName     input:  Pointer to source object name, or nil when
                        srcDirID specifies a directory that's the object.
    dstVRefNum  input:  Destination volume specification.
    dstDirID    input:  Destination directory ID.
    dstName     input:  Pointer to destination object name, or nil when
                        dstDirID specifies a directory that's the object.
    copyLockBit input:  If true, set the locked state of the destination
                        to match the source.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   FSpCopyFileMgrAttributes
}

{***************************************************************************}

FUNCTION FSpCopyFileMgrAttributes({CONST}VAR srcSpec: FSSpec; {CONST}VAR dstSpec: FSSpec; copyLockBit: BOOLEAN): OSErr;

{
    The FSpCopyFileMgrAttributes function copies all File Manager attributes
    from the source file or directory to the destination file or directory.
    If copyLockBit is true, then set the locked state of the destination
    to match the source.

    srcSpec     input:  An FSSpec record specifying the source object.
    dstSpec     input:  An FSSpec record specifying the destination object.
    copyLockBit input:  If true, set the locked state of the destination
                        to match the source.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        fnfErr              -43     File not found
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     No default volume
        dirNFErr            -120    Directory not found or incomplete pathname
        afpAccessDenied     -5000   User does not have the correct access
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
    
    __________
    
    See also:   CopyFileMgrAttributes
}

{***************************************************************************}

FUNCTION HOpenAware(vRefNum: INTEGER; dirID: LONGINT; fileName: Str255; denyModes: INTEGER; VAR refNum: INTEGER): OSErr;

{
    The HOpenAware function opens the data fork of a file using deny mode
    permissions instead the normal File Manager permissions.  If OpenDeny
    is not available, then HOpenAware translates the deny modes to the
    closest File Manager permissions and tries to open the file with
    OpenDF first, and then Open if OpenDF isn't available. By using
    HOpenAware with deny mode permissions, a program can be "AppleShare
    aware" and fall back on the standard File Manager open calls
    automatically.

    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    fileName    input:  The name of the file.
    denyModes   input:  The deny modes access under which to open the file.
    refNum      output: The file reference number of the opened file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        tmfoErr             -42     Too many files open
        fnfErr              -43     File not found
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        afpAccessDenied     -5000   User does not have the correct access to the file
        afpDenyConflict     -5006   Requested access permission not possible
    
    __________
    
    See also:   FSpOpenAware, HOpenRFAware, FSpOpenRFAware
}

{***************************************************************************}

FUNCTION FSpOpenAware({CONST}VAR spec: FSSpec; denyModes: INTEGER; VAR refNum: INTEGER): OSErr;

{
    The FSpOpenAware function opens the data fork of a file using deny mode
    permissions instead the normal File Manager permissions.  If OpenDeny
    is not available, then FSpOpenAware translates the deny modes to the
    closest File Manager permissions and tries to open the file with
    OpenDF first, and then Open if OpenDF isn't available. By using
    FSpOpenAware with deny mode permissions, a program can be "AppleShare
    aware" and fall back on the standard File Manager open calls
    automatically.

    spec        input:  An FSSpec record specifying the file.
    denyModes   input:  The deny modes access under which to open the file.
    refNum      output: The file reference number of the opened file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        tmfoErr             -42     Too many files open
        fnfErr              -43     File not found
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        afpAccessDenied     -5000   User does not have the correct access to the file
        afpDenyConflict     -5006   Requested access permission not possible
    
    __________
    
    See also:   HOpenAware, HOpenRFAware, FSpOpenRFAware
}

{***************************************************************************}

FUNCTION HOpenRFAware(vRefNum: INTEGER; dirID: LONGINT; fileName: Str255; denyModes: INTEGER; VAR refNum: INTEGER): OSErr;

{
    The HOpenRFAware function opens the resource fork of a file using deny
    mode permissions instead the normal File Manager permissions.  If
    OpenRFDeny is not available, then HOpenRFAware translates the deny
    modes to the closest File Manager permissions and tries to open the
    file with OpenRF. By using HOpenRFAware with deny mode permissions,
    a program can be "AppleShare aware" and fall back on the standard
    File Manager open calls automatically.

    vRefNum     input:  Volume specification.
    dirID       input:  Directory ID.
    fileName    input:  The name of the file.
    denyModes   input:  The deny modes access under which to open the file.
    refNum      output: The file reference number of the opened file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        tmfoErr             -42     Too many files open
        fnfErr              -43     File not found
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        afpAccessDenied     -5000   User does not have the correct access to the file
        afpDenyConflict     -5006   Requested access permission not possible
    
    __________
    
    See also:   HOpenAware, FSpOpenAware, FSpOpenRFAware
}

{***************************************************************************}

FUNCTION FSpOpenRFAware({CONST}VAR spec: FSSpec; denyModes: INTEGER; VAR refNum: INTEGER): OSErr;

{
    The FSpOpenRFAware function opens the resource fork of a file using deny
    mode permissions instead the normal File Manager permissions.  If
    OpenRFDeny is not available, then FSpOpenRFAware translates the deny
    modes to the closest File Manager permissions and tries to open the
    file with OpenRF. By using FSpOpenRFAware with deny mode permissions,
    a program can be "AppleShare aware" and fall back on the standard
    File Manager open calls automatically.

    spec        input:  An FSSpec record specifying the file.
    denyModes   input:  The deny modes access under which to open the file.
    refNum      output: The file reference number of the opened file.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     No such volume
        tmfoErr             -42     Too many files open
        fnfErr              -43     File not found
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Volume is locked or read-only
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        afpAccessDenied     -5000   User does not have the correct access to the file
        afpDenyConflict     -5006   Requested access permission not possible
    
    __________
    
    See also:   HOpenAware, FSpOpenAware, HOpenRFAware
}

{***************************************************************************}

FUNCTION FSReadNoCache(refNum: INTEGER; VAR count: LONGINT; buffPtr: UNIV Ptr): OSErr;

{
    The FSReadNoCache function reads any number of bytes from an open file
    while asking the file system to bypass its cache mechanism.
    
    refNum  input:  The file reference number of an open file.
    count   input:  The number of bytes to read.
            output: The number of bytes actually read.
    buffPtr input:  A pointer to the data buffer into which the bytes are
                    to be read.
    
    Result Codes
        noErr               0       No error
        readErr             �19     Driver does not respond to read requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        ioErr               �36     Data does not match in read-verify mode
        fnOpnErr            -38     File not open
        rfNumErr            -51     Bad reference number
        afpAccessDenied     -5000   User does not have the correct access to
                                    the file

    __________
    
    See also:   FSWriteNoCache
}

{***************************************************************************}

FUNCTION FSWriteNoCache(refNum: INTEGER; VAR count: LONGINT; buffPtr: UNIV Ptr): OSErr;

{
    The FSReadNoCache function writes any number of bytes to an open file
    while asking the file system to bypass its cache mechanism.
    
    refNum  input:  The file reference number of an open file.
    count   input:  The number of bytes to write to the file.
            output: The number of bytes actually written.
    buffPtr input:  A pointer to the data buffer from which the bytes are
                    to be written.
    
    Result Codes
        noErr               0       No error
        writErr             �20     Driver does not respond to write requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        dskFulErr           -34     Disk full   
        ioErr               �36     Data does not match in read-verify mode
        fnOpnErr            -38     File not open
        wPrErr              -44     Hardware volume lock    
        fLckdErr            -45     File is locked  
        vLckdErr            -46     Software volume lock    
        rfNumErr            -51     Bad reference number
        wrPermErr           -61     Read/write permission doesn�t
                                    allow writing   
        afpAccessDenied     -5000   User does not have the correct access to
                                    the file

    __________
    
    See also:   FSReadNoCache
}

{***************************************************************************}

FUNCTION FSWriteVerify(refNum: INTEGER; VAR count: LONGINT; buffPtr: UNIV Ptr): OSErr;

{
    The FSWriteVerify function writes any number of bytes to an open file
    and then verifies that the data was actually written to the device.
    
    refNum  input:  The file reference number of an open file.
    count   input:  The number of bytes to write to the file.
            output: The number of bytes actually written and verified.
    buffPtr input:  A pointer to the data buffer from which the bytes are
                    to be written.
    
    Result Codes
        noErr               0       No error
        readErr             �19     Driver does not respond to read requests
        writErr             �20     Driver does not respond to write requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        dskFulErr           -34     Disk full   
        ioErr               �36     Data does not match in read-verify mode
        fnOpnErr            -38     File not open
        eofErr              -39     Logical end-of-file reached
        posErr              -40     Attempt to position mark before start
                                    of file
        wPrErr              -44     Hardware volume lock    
        fLckdErr            -45     File is locked  
        vLckdErr            -46     Software volume lock    
        rfNumErr            -51     Bad reference number
        gfpErr              -52     Error during GetFPos
        wrPermErr           -61     Read/write permission doesn�t
                                    allow writing   
        memFullErr          -108    Not enough room in heap zone to allocate
                                    verify buffer
        afpAccessDenied     -5000   User does not have the correct access to
                                    the file
}

{***************************************************************************}

FUNCTION CopyFork(srcRefNum: INTEGER; dstRefNum: INTEGER; copyBufferPtr: UNIV Ptr; copyBufferSize: LONGINT): OSErr;

{
    The CopyFork function copies all data from the source fork to the
    destination fork of open file forks and makes sure the destination EOF
    is equal to the source EOF.
    
    srcRefNum       input:  The source file reference number.
    dstRefNum       input:  The destination file reference number.
    copyBufferPtr   input:  Pointer to buffer to use during copy. The
                            buffer should be at least 512-bytes minimum.
                            The larger the buffer, the faster the copy.
    copyBufferSize  input:  The size of the copy buffer.
    
    Result Codes
        noErr               0       No error
        readErr             �19     Driver does not respond to read requests
        writErr             �20     Driver does not respond to write requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        dskFulErr           -34     Disk full   
        ioErr               �36     Data does not match in read-verify mode
        fnOpnErr            -38     File not open
        wPrErr              -44     Hardware volume lock    
        fLckdErr            -45     File is locked  
        vLckdErr            -46     Software volume lock    
        rfNumErr            -51     Bad reference number
        wrPermErr           -61     Read/write permission doesn�t
                                    allow writing   
        afpAccessDenied     -5000   User does not have the correct access to
                                    the file
}

{***************************************************************************}

FUNCTION GetFileLocation(refNum: INTEGER; VAR vRefNum: INTEGER; VAR dirID: LONGINT; fileName: StringPtr): OSErr;

{
    The GetFileLocation function gets the location (volume reference number,
    directory ID, and fileName) of an open file.

    refNum      input:  The file reference number of an open file.
    vRefNum     output: The volume reference number.
    dirID       output: The parent directory ID.
    fileName    input:  Points to a buffer (minimum Str63) where the
                        filename is to be returned or must be nil.
                output: The filename.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     Specified volume doesn�t exist
        fnOpnErr            -38     File not open
        rfNumErr            -51     Reference number specifies nonexistent
                                    access path
    
    __________
    
    See also:   FSpGetFileLocation
}

{***************************************************************************}

FUNCTION FSpGetFileLocation(refNum: INTEGER; VAR spec: FSSpec): OSErr;

{
    The FSpGetFileLocation function gets the location of an open file in
    an FSSpec record.

    refNum      input:  The file reference number of an open file.
    spec        output: FSSpec record containing the file name and location.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     Specified volume doesn�t exist
        fnOpnErr            -38     File not open
        rfNumErr            -51     Reference number specifies nonexistent
                                    access path
    
    __________
    
    See also:   GetFileLocation
}

{***************************************************************************}

FUNCTION CopyDirectoryAccess(srcVRefNum: INTEGER; srcDirID: LONGINT; srcName: Str255; dstVRefNum: INTEGER; dstDirID: LONGINT; dstName: Str255): OSErr;

{
    The CopyDirectoryAccess function copies the AFP directory access
    privileges from one directory to another. Both directories must be on
    the same file server, but not necessarily on the same server volume.
    
    srcVRefNum  input:  Source volume specification.
    srcDirID    input:  Source directory ID.
    srcName     input:  Pointer to source directory name, or nil when
                        srcDirID specifies the directory.
    dstVRefNum  input:  Destination volume specification.
    dstDirID    input:  Destination directory ID.
    dstName     input:  Pointer to destination directory name, or nil when
                        dstDirID specifies the directory.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     Volume not found
        fnfErr              -43     Directory not found
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     Volume doesn't support this function
        afpAccessDenied     -5000   User does not have the correct access
                                    to the directory
        afpObjectTypeErr    -5025   Object is a file, not a directory
    
    __________
    
    See also:   FSpCopyDirectoryAccess
}

{***************************************************************************}

FUNCTION FSpCopyDirectoryAccess({CONST}VAR srcSpec: FSSpec; {CONST}VAR dstSpec: FSSpec): OSErr;

{
    The FSpCopyDirectoryAccess function copies the AFP directory access
    privileges from one directory to another. Both directories must be on
    the same file server, but not necessarily on the same server volume.

    srcSpec     input:  An FSSpec record specifying the source directory.
    dstSpec     input:  An FSSpec record specifying the destination directory.
    
    Result Codes
        noErr               0       No error
        nsvErr              -35     Volume not found
        fnfErr              -43     Directory not found
        vLckdErr            -46     Volume is locked or read-only
        paramErr            -50     Volume doesn't support this function
        afpAccessDenied     -5000   User does not have the correct access
                                    to the directory
        afpObjectTypeErr    -5025   Object is a file, not a directory
    
    __________
    
    See also:   CopyDirectoryAccess
}

{***************************************************************************}

FUNCTION HMoveRenameCompat(vRefNum: INTEGER; srcDirID: LONGINT; srcName: Str255; dstDirID: LONGINT; dstpathName: Str255; copyName: Str255): OSErr;

{
    The HMoveRenameCompat function moves a file or directory and optionally
    renames it.  The source and destination locations must be on the same
    volume. This routine works even if the volume doesn't support MoveRename.
    
    vRefNum     input:  Volume specification.
    srcDirID    input:  Source directory ID.
    srcName     input:  The source object name.
    dstDirID    input:  Destination directory ID.
    dstName     input:  Pointer to destination directory name, or
                        nil when dstDirID specifies a directory.
    copyName    input:  Points to the new name if the object is to be
                        renamed or nil if the object isn't to be renamed.
    
    Result Codes
        noErr               0       No error
        dirFulErr           -33     File directory full
        dskFulErr           -34     Disk is full
        nsvErr              -35     Volume not found
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename or attempt to move into
                                    a file
        fnfErr              -43     Source file or directory not found
        wPrErr              -44     Hardware volume lock
        fLckdErr            -45     File is locked
        vLckdErr            -46     Destination volume is read-only
        fBsyErr             -47     File busy, directory not empty, or
                                    working directory control block open
        dupFNErr            -48     Destination already exists
        paramErr            -50     Volume doesn't support this function,
                                    no default volume, or source and
        volOfflinErr        -53     Volume is offline
        fsRnErr             -59     Problem during rename
        dirNFErr            -120    Directory not found or incomplete pathname
        badMovErr           -122    Attempted to move directory into
                                    offspring
        wrgVolTypErr        -123    Not an HFS volume (it's a MFS volume)
        notAFileErr         -1302   The pathname is nil, the pathname
                                    is empty, or the pathname cannot refer
                                    to a filename
        diffVolErr          -1303   Files on different volumes
        afpAccessDenied     -5000   The user does not have the right to
                                    move the file  or directory
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
        afpSameObjectErr    -5038   Source and destination files are the same
    
    __________
    
    See also:   FSpMoveRenameCompat
}

{***************************************************************************}

FUNCTION FSpMoveRenameCompat({CONST}VAR srcSpec: FSSpec; {CONST}VAR dstSpec: FSSpec; copyName: Str255): OSErr;

{
    The FSpMoveRenameCompat function moves a file or directory and optionally
    renames it.  The source and destination locations must be on the same
    volume. This routine works even if the volume doesn't support MoveRename.
    
    srcSpec     input:  An FSSpec record specifying the source object.
    dstSpec     input:  An FSSpec record specifying the destination
                        directory.
    copyName    input:  Points to the new name if the object is to be
                        renamed or nil if the object isn't to be renamed.
    
    Result Codes
        noErr               0       No error
        dirFulErr           -33     File directory full
        dskFulErr           -34     Disk is full
        nsvErr              -35     Volume not found
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename or attempt to move into
                                    a file
        fnfErr              -43     Source file or directory not found
        wPrErr              -44     Hardware volume lock
        fLckdErr            -45     File is locked
        vLckdErr            -46     Destination volume is read-only
        fBsyErr             -47     File busy, directory not empty, or
                                    working directory control block open
        dupFNErr            -48     Destination already exists
        paramErr            -50     Volume doesn't support this function,
                                    no default volume, or source and
        volOfflinErr        -53     Volume is offline
        fsRnErr             -59     Problem during rename
        dirNFErr            -120    Directory not found or incomplete pathname
        badMovErr           -122    Attempted to move directory into
                                    offspring
        wrgVolTypErr        -123    Not an HFS volume (it's a MFS volume)
        notAFileErr         -1302   The pathname is nil, the pathname
                                    is empty, or the pathname cannot refer
                                    to a filename
        diffVolErr          -1303   Files on different volumes
        afpAccessDenied     -5000   The user does not have the right to
                                    move the file  or directory
        afpObjectTypeErr    -5025   Directory not found or incomplete pathname
        afpSameObjectErr    -5038   Source and destination files are the same
    
    __________
    
    See also:   HMoveRenameCompat
}

{***************************************************************************}

FUNCTION BuildAFPVolMountInfo(flags: INTEGER; nbpInterval: ByteParameter; nbpCount: ByteParameter; uamType: INTEGER; VAR zoneName: Str32; VAR serverName: Str31; VAR volName: Str27; VAR userName: Str31; VAR userPassword: Str8; VAR volPassword: Str8; VAR afpInfoPtr: AFPVolMountInfoPtr): OSErr;

{
    The BuildAFPVolMountInfo function allocates and initializes the fields
    of an AFPVolMountInfo record before using that record to call
    the VolumeMount function.
    
    flags           input:  The AFP mounting flags. 0 = normal mount;
                            set bit 0 to inhibit greeting messages.
    nbpInterval     input:  The interval used for VolumeMount's
                            NBP Lookup call. 7 is a good choice.
    nbpCount        input:  The retry count used for VolumeMount's
                            NBP Lookup call. 5 is a good choice.
    uamType         input:  The user authentication method to use.
    zoneName        input:  The AppleTalk zone name of the server.
    serverName      input:  The AFP server name.
    volName         input:  The AFP volume name.
    userName        input:  The user name (zero length Pascal string for
                            guest).
    userPassWord    input:  The user password (zero length Pascal string
                            if no user password)
    volPassWord     input:  The volume password (zero length Pascal string
                            if no volume password)
    afpInfoPtr      output: A pointer to the newly created and initialized
                            AFPVolMountInfo record. If the function fails to
                            create an AFPVolMountInfo record, it sets
                            afpInfoPtr to NULL and the function result is
                            memFullErr. Your program is responsible
                            for disposing of this pointer when it is finished
                            with it.
    
    Result Codes
        noErr               0       No error
        memFullErr          -108    memory full error
    
    __________
    
    Also see:   GetVolMountInfoSize, GetVolMountInfo, VolumeMount,
                RetrieveAFPVolMountInfo, BuildAFPXVolMountInfo,
                RetrieveAFPXVolMountInfo
}

{***************************************************************************}

FUNCTION RetrieveAFPVolMountInfo(afpInfoPtr: AFPVolMountInfoPtr; VAR flags: INTEGER; VAR uamType: INTEGER; zoneName: StringPtr; serverName: StringPtr; volName: StringPtr; userName: StringPtr): OSErr;

{
    The RetrieveAFPVolMountInfo function retrieves the AFP mounting
    information returned in an AFPVolMountInfo record by the
    GetVolMountInfo function.
    
    afpInfoPtr      input:  Pointer to AFPVolMountInfo record that contains
                            the AFP mounting information.
    flags           output: The AFP mounting flags.
    uamType         output: The user authentication method used.
    zoneName        output: The AppleTalk zone name of the server.
    serverName      output: The AFP server name.
    volName         output: The AFP volume name.
    userName        output: The user name (zero length Pascal string for
                            guest).
    
    Result Codes
        noErr               0       No error
        paramErr            -50     media field in AFP mounting information
                                    was not AppleShareMediaType
    
    __________
    
    Also see:   GetVolMountInfoSize, GetVolMountInfo, VolumeMount,
                BuildAFPVolMountInfo, BuildAFPXVolMountInfo,
                RetrieveAFPXVolMountInfo
}

{***************************************************************************}

FUNCTION BuildAFPXVolMountInfo(flags: INTEGER; nbpInterval: ByteParameter; nbpCount: ByteParameter; uamType: INTEGER; VAR zoneName: Str32; VAR serverName: Str31; VAR volName: Str27; VAR userName: Str31; VAR userPassword: Str8; VAR volPassword: Str8; VAR uamName: Str32; alternateAddressLength: UInt32; alternateAddress: UNIV Ptr; VAR afpXInfoPtr: AFPXVolMountInfoPtr): OSErr;

{
    The BuildAFPXVolMountInfo function allocates and initializes the fields
    of an AFPXVolMountInfo record before using that record to call
    the VolumeMount function.
    
    flags                   input:  The AFP mounting flags.
    nbpInterval             input:  The interval used for VolumeMount's
                                    NBP Lookup call. 7 is a good choice.
    nbpCount                input:  The retry count used for VolumeMount's
                                    NBP Lookup call. 5 is a good choice.
    uamType                 input:  The user authentication method to use.
    zoneName                input:  The AppleTalk zone name of the server.
    serverName              input:  The AFP server name.
    volName                 input:  The AFP volume name.
    userName                input:  The user name (zero length Pascal string
                                    for guest).
    userPassWord            input:  The user password (zero length Pascal
                                    string if no user password)
    volPassWord             input:  The volume password (zero length Pascal
                                    string if no volume password)
    uamName                 input:  The User Authentication Method name.
    alternateAddressLength  input:  Length of alternateAddress data.
    alternateAddress        input   The AFPAlternateAddress (variable length)
    afpXInfoPtr             output: A pointer to the newly created and
                                    initialized AFPVolMountInfo record.
                                    If the function fails to create an
                                    AFPVolMountInfo record, it sets
                                    afpInfoPtr to NULL and the function
                                    result is memFullErr. Your program is
                                    responsible for disposing of this pointer
                                    when it is finished with it.
    
    Result Codes
        noErr               0       No error
        memFullErr          -108    memory full error
    
    __________
    
    Also see:   GetVolMountInfoSize, GetVolMountInfo, VolumeMount,
                BuildAFPVolMountInfo, RetrieveAFPVolMountInfo,
                RetrieveAFPXVolMountInfo
}

{***************************************************************************}

FUNCTION RetrieveAFPXVolMountInfo(afpXInfoPtr: AFPXVolMountInfoPtr; VAR flags: INTEGER; VAR uamType: INTEGER; zoneName: StringPtr; serverName: StringPtr; volName: StringPtr; userName: StringPtr; uamName: StringPtr; VAR alternateAddressLength: UInt32; VAR alternateAddress: UNIV Ptr): OSErr;

{
    The RetrieveAFPXVolMountInfo function retrieves the AFP mounting
    information returned in an AFPXVolMountInfo record by the
    GetVolMountInfo function.
    
    afpXInfoPtr             input:  Pointer to AFPXVolMountInfo record that
                                    contains the AFP mounting information.
    flags                   output: The AFP mounting flags.
    uamType                 output: The user authentication method used.
    zoneName                output: The AppleTalk zone name of the server.
    serverName              output: The AFP server name.
    volName                 output: The AFP volume name.
    userName                output: The user name (zero length Pascal
                                    string for guest).
    uamName                 output: The User Authentication Method name.
    alternateAddressLength  output: Length of alternateAddress data returned.
    alternateAddress:       output: A pointer to the newly created and
                                    AFPAlternateAddress record (a variable
                                    length record). If the function fails to
                                    create an AFPAlternateAddress record,
                                    it sets alternateAddress to NULL and the
                                    function result is memFullErr. Your
                                    program is responsible for disposing of
                                    this pointer when it is finished with it.
    
    Result Codes
        noErr               0       No error
        paramErr            -50     media field in AFP mounting information
                                    was not AppleShareMediaType
        memFullErr          -108    memory full error
    
    __________
    
    Also see:   GetVolMountInfoSize, GetVolMountInfo, VolumeMount,
                BuildAFPVolMountInfo, RetrieveAFXVolMountInfo,
                BuildAFPXVolMountInfo
}

{***************************************************************************}

FUNCTION GetUGEntries(objType: INTEGER; entries: UGEntryPtr; reqEntryCount: LONGINT; VAR actEntryCount: LONGINT; VAR objID: LONGINT): OSErr;

{
    The GetUGEntries functions retrieves a list of user or group entries
    from the local file server.

    objType         input:  The object type: -1 = group; 0 = user
    UGEntries       input:  Pointer to array of UGEntry records where the list
                            is returned.
    reqEntryCount   input:  The number of elements in the UGEntries array.
    actEntryCount   output: The number of entries returned.
    objID           input:  The current index position. Set to 0 to start with
                            the first entry.
                    output: The index position to get the next entry. Pass this
                            value the next time you call GetUGEntries to start
                            where you left off.
    
    Result Codes
        noErr               0       No error    
        fnfErr              -43     No more users or groups 
        paramErr            -50     Function not supported; or, ioObjID is
                                    negative    

    __________
    
    Also see:   GetUGEntry
}

{***************************************************************************}



{$ALIGN RESET}
{$POP}

{$SETC UsingIncludes := MoreFilesExtrasIncludes}

{$ENDC} {__MOREFILESEXTRAS__}

{$IFC NOT UsingIncludes}
 END.
{$ENDC}
