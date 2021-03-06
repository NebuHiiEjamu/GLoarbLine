/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A utility class for mapping between Content Type and File Extensions
// on Windows platform.

#include "UFileUtils.h"
#include "StString.h"
#include "CFSException.h"
#include "FullPath.h"


HL_Begin_Namespace_BigRedH

// ---------------------------------------------------------------------
//  GetTemporaryFolder                                  [public][static]
// ---------------------------------------------------------------------
// Returns the pathname to the temporary folder on the current platform.

CString
UFileUtils::GetTemporaryFolder()
{
	CString folderName;
	try {
		short vRefNum;
		long dirID;
		THROW_OS_( ::FindFolder( kOnSystemDisk, kTemporaryFolderType,
							kDontCreateFolder, &vRefNum, &dirID ) );
		short len;
		Handle path;
		THROW_OS_( GetFullPath( vRefNum, dirID, nil, &len, &path ) );
		if( path != nil ){
			::HLock( path );
			folderName = CString( *path, len );
			::HUnlock( path );
			::DisposeHandle( path );
		}
	} catch( ... ){
		RETHROW_FS_( eGetTempFolder, L"" );
	}
	return folderName;
}

HL_End_Namespace_BigRedH
