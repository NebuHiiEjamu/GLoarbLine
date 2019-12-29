/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#ifndef _H_CResourceList_
#define _H_CResourceList_

#include "CFlattenable.h"
#include "CResourceFile.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CResourceList: public CFlattenable
{
	public:
										CResourceList();
											// throws ???
										~CResourceList();	
											// throws nothing

		UInt32							GetSize() const;
											// throws nothing

		UInt32							GetResourceIdOf
											(UInt32 inIndex) 
											const;
											// throws CResourceException

		CResourceFile::Type				GetResourceTypeOf
											(UInt32 inIndex) 
											const;
											// throws CResourceException

		CRefObj<CFlattenable>&			GetResourceObject
											( UInt32 inIndex )
											const;
											// throws CResourceException
	protected:
			//** CFlattenable overrides **
		virtual void 					AttachInStream
											(std::auto_ptr<CInStream>& inInStream);
											// throws CMessageException
		virtual void					ListenToMessage
											( const CMessage &inMessage );
											// throws CMessageException
	private:
		void							TryToReadSomeMore();
											// throws ???

		void							AttachResourceFile
											(CResourceFile& inResFile);
											// throws nothing
		friend class					CResourceFile;
									
		struct ListEntry
		{
			CResourceFile::Type			mResType;
			UInt32 						mResID;
		};

		std::vector<ListEntry>			mResList;
		CResourceFile*					mResFile;
};

HL_End_Namespace_BigRedH
#endif // _H_CResourceList_
