/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// A reference to a file system item like a file, folder, whatever else

#ifndef _H_CFSFileRef_
#define _H_CFSFileRef_

#include "CFSRef.h"
#include "CListener.h"
#include "CInStream.h"
#include "COutStream.h"
#include "CRefObj.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CFlattenable;
class CFSFileRef : public CFSRef
				 , public CListener // listens to CMemoryObjectCache notifications
{
	public:
		virtual bool			 			IsContainer() const
												// throw nothing
												{ return false; }

		virtual void						LoadInputStream() = 0;
												// throw CFSException
		virtual std::auto_ptr<CInStream>&	GetInputStream() = 0;
												// throw CFSException

		virtual void						LoadOutputStream() = 0;
												// throw CFSException
		virtual std::auto_ptr<COutStream>&	GetOutputStream() = 0;
												// throw CFSException
	
			// ** delayed operations **
		void 								LoadObject();
												// throw CFSException
		CRefObj<CFlattenable>&				GetLoadedObject();
												// throw CFSException
	
		virtual void						LoadType()=0;
												// throw CFSException
		virtual void 						SetFileType(const CString& inMime)=0;
												// throw CFSException
		virtual CString					GetFileType()=0;
												// throw CFSException
		
		virtual void						LoadSize()=0;
												// throw CFSException
		virtual UInt64						GetSize()=0;
												// throw CFSException

			// ** ID's and Msg types for notifications **
		enum 								NotifyID
												{
												    eType	= 'fsTy'
												  , eSize	= 'fsSz'
												  , eInput	= 'fsIn'
												  , eOutput	= 'fsOt'
												  , eObject	= 'fsOb'
												};
	protected:
											CFSFileRef()
												// throw nothing
												{};
											CFSFileRef(CFSFileRef&)
												// throw nothing
												{};
		void								ListenToMessage( const CMessage &inMessage );
												// throw CMessageException
}; // class CFSFileRef 

HL_End_Namespace_BigRedH
#endif // _H_CFSFileRef_