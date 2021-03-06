/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A reference to a local file (don't use relative paths)
//				  Volume:absolutepath:file

#ifndef _H_CFSLocalFileRefMac_
#define _H_CFSLocalFileRefMac_

#if PRAGMA_ONCE
	#pragma once
#endif
			   
#include "CFSLocalFileRef.h"
#include <Files.h>

HL_Begin_Namespace_BigRedH

class CFSLocalFileRefMac : public CFSLocalFileRef
{
	public:
											CFSLocalFileRefMac(const FSSpec& inFSpec,
															OSType inMacType = '????',
															OSType inMacCreator = '????' );
												// throws nothing
											CFSLocalFileRefMac(const CString& inFileName);
												// throws nothing
											CFSLocalFileRefMac(const CFSLocalFileRefMac& inOther);
												// throws nothing

		virtual	std::auto_ptr<CFSRef> 		Clone() const;
												// throws CFSException

		virtual const CString& 			GetName() const;
												// throws nothing

		virtual const FSSpec& 				GetFSpec() const;
												// throws CFSException
		virtual OSType		 				GetMacType() const
												// throws nothing
												{ return mMacType; }
		virtual void		 				SetMacType( OSType inType )
												// throws nothing
												{ mMacType = inType; }
		virtual OSType		 				GetMacCreator() const
												// throws nothing
												{ return mMacCreator; }
		virtual void		 				SetMacCreator( OSType inCreator )
												// throws nothing
												{ mMacCreator = inCreator; }

		virtual bool						IsEqual(const CFSRef& inOther) const;
												// throws nothing
		virtual bool						IsLessThan(const CFSRef& inOther) const;
												// throws nothing

		virtual void						LoadInputStream();
												// throws CFSException
		virtual std::auto_ptr<CInStream>&	GetInputStream();
												// throws CFSException
		virtual void						LoadOutputStream();
												// throws CFSException
		virtual std::auto_ptr<COutStream>&	GetOutputStream();
												// throws CFSException

		virtual void						LoadType();
												// throws CFSException
		virtual void 						SetFileType(const CString& inMime);
												// throws CFSException
		virtual CString					GetFileType();
												// throws CFSException

		virtual void 						LoadSize();
												// throws CFSException
		virtual UInt64 						GetSize();
												// throws CFSException

		virtual void						LoadRights();
												// throws CFSException
		virtual CFileRights&				GetRights();
												// throws CFSException
//		virtual void						SaveRights();
												// throws CFSException

		virtual void 						Delete();
												// throws CFSException

	protected:
		virtual void						SetupTypeCreator();
												// throws nothing
		virtual void						SetupSpec( bool inMustExist = true ) const;
												// throws CException

	private:
		std::auto_ptr<CInStream>			mInStream;
		std::auto_ptr<COutStream>			mOutStream;
		
		mutable FSSpec						mFSpec;
		mutable bool						mSpecSetup;
		OSType								mMacType, mMacCreator;
		CString							mFileName;
		CString							mMimeType;
		UInt64								mSize;
		CFileRights							mRights;

		bool								mSizeLoaded;
		bool								mTypeLoaded;
		bool								mRightsLoaded;
}; // class CFSLocalFileRefMac

HL_End_Namespace_BigRedH
#endif // _H_CFSLocalFileRefMac_
