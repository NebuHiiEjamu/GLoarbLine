/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A class that is used to unflatten a streamed file.
// A file is created in the container the forks - data(all) and resource(Mac)
// Once the file is created, 
#ifndef _H_CUnflattenFile_
#define _H_CUnflattenFile_

#include "CBroadcaster.h"
#include "CFSException.h"
#include "CPipe.h"

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH
class CFSLocalFolderRef;
class CFSLocalFileRef;
class CLocalFile;


class CUnflattenFile : public CBroadcaster
{
	public:
				 				 		CUnflattenFile( const CString &inContainerName );
											// throws ???
				 				 		CUnflattenFile( const CString &inContainerName,
				 				 						const CString &inAltFileName );
											// throws ???
//				 				 		CUnflattenFile( const CFSLocalFolderRef &inContainer );
											// throws ???
//				 				 		CUnflattenFile( const CFSLocalFolderRef &inContainer,
//														const CString &inAltFileName );
											// throws ???
		virtual 				 		~CUnflattenFile();
											// throws nothing

			//** Opening and closing the file **
		void					 		ProcessData( UInt8 *inBuffer, UInt32 inSize );
											// throws CFSException

			//** Getting file info **
		enum							NotifyID {
											eRefLoaded = 'RefL'
											, eDataCreated = 'CrDt'
											, eResourceCreated = 'CrRs'
		};

		CFSLocalFileRef&		 		GetFileRef();
											// throws CFSException

		UInt32					 		GetDataForkSize();
											// throws CFSException

	private:
//		const CFSLocalFolderRef&			mContainer;
		CString							mContainer;
		CString							mAltFileName;
		CPipe								mBuffer;
		CFSLocalFileRef*					mFileRef;
		CLocalFile*							mFile;
		bool								mDataForkCreated;
		UInt32								mDataForkSize;
		
		enum EState {
			eState_Header,
			eState_ForkHeader,
			eState_Fork,
			eState_Done
		} mState;
		
			// ** Header **
		UInt32								mHeaderFormat;
		UInt16								mHeaderVers;
		UInt16								mHeaderForkCount;
		UInt16								mCurrentFork;

			// ** Fork Header **
		UInt32								mForkType;
		UInt32								mForkCompressionType;
		UInt32								mForkSize;
		UInt32								mCurrentDataSize;

			// ** Fork Info **
		UInt32								mInfoPlatform;
		UInt32								mInfoTypeSig;
		UInt32								mInfoCreatorSig;
		UInt32								mInfoFlags;
		UInt32								mInfoPlatformFlags;
		struct STimeStamp {
			UInt16				mYear;
			UInt16				mMilliSecs;
			UInt32				mSecs;
		} 									mInfoCreateDate,
											mInfoModifyDate;
		UInt16								mInfoNameScript;
		UInt16								mInfoNameSize;

}; // class CUnflattenFile 

HL_End_Namespace_BigRedH
#endif // _H_CUnflattenFile_
