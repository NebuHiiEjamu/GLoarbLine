/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// Storage space for all objects created from an FSFileRef

#ifndef _H_CMemoryObjectCache_
#define _H_CMemoryObjectCache_

#include "CBroadcaster.h"
#include "CMessage.h"
#include "CFSFileRef.h"
#include "CRefObj.h"
#include "CMutex.h"
#include "CFlattenable.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CFSFileRef;
// Storage space for all objects created from an FSFileRef
class CMemoryObjectCache : public CBroadcaster
{
	public:
			//** singleton management **
		static CMemoryObjectCache&  Instance();
										// throws CPersistenceException
		static void					ClearInstance();
										// throws CPersistenceException

			//** destructor **
									~CMemoryObjectCache();
										// throws nothing

			//** object management **
		void 						LoadObject(CFSFileRef& inRef);
										// throws CPersistenceException
		CRefObj<CFlattenable>&		GetLoadedObject(CFSFileRef& inRef);
										// throws CPersistenceException

			//** notification management **
		enum 						NotifyID
										{ eObject = 'mcOb' };
		class Message : public CMessage
		{
			public:
									Message(  UInt32 inID
											, CMemoryObjectCache& inSource
											, CFSFileRef& inDestination
											, bool inSuccess );
										// throw ???
									Message( const Message& inMsg );
										// throw ???
									~Message();
										// throw nothing
				virtual CMessage*	Clone() const;
										// throw ???
				bool				VerifySource(CMemoryObjectCache& inSource) const
										// throw nothing
										{ return &mSource == &inSource; }
				CFSFileRef&			GetFSFileRef() const 
										// throw nothing
										{ return *mDestination; }
				bool				GetSuccess() const 
										// throw nothing
										{ return mSuccess; }
			private:
				CMemoryObjectCache&	mSource;
				CFSFileRef*			mDestination; // owned
				bool				mSuccess;	
		}; // class Message

	protected:
			//** constructor **
									CMemoryObjectCache();	
	private:
		static std::auto_ptr<CMemoryObjectCache> sInstance;

			//** items  **   (a state machine used to download and store the Image,Sound,...)
		class Item : public CListener
				   , public CRefObj<CFlattenable>
		{
			public:
									Item(CMemoryObjectCache& inParent, const CFSFileRef& inRef);
										// throw nothing
									~Item();
										// throw nothing
										// override required by CRefObj
				CFlattenable*		GetSibling()
										// throw nothing
										{ return mFlattenable.get(); }
				CFSFileRef&			GetFileRef() const
										// throw nothing
										{ return *mFileRef.get(); }
				bool				IsComplete()
										// throw nothing
										{ return mState == eCompleted; }
			protected:
				friend class 		CMemoryObjectCache;
				void				NextState();				
										// throw CPersistenceException
				void				ListenToMessage( const CMessage &inMessage );
										// throw CMessageException
			private:
				enum 			   	State
										{
											    eStart
											  , eLoadType
											  , eLoadInStream
											  , eCompleted 
										};
				State							mState;
				CMemoryObjectCache& 			mParent;
				std::auto_ptr<CFSFileRef>		mFileRef;
				std::auto_ptr<CFlattenable>		mFlattenable;
		}; // class Item
		friend class Item;
			// Item less provides a comparison operator for std::map
		struct ItemLess
		{
			bool		operator()(const CFSFileRef *const & x, const CFSFileRef * const & y) const
							// throw nothing
							{ return x->IsLessThan(*y); }
		}; // class ItemLess
			//	since Items is a keeper for Item we have to provide a 
			//	destructor that deletes each item from the map
		class Items : public std::map<CFSFileRef*,Item*,ItemLess>
		{
			public:
				~Items();
		};
		
		Items						mItems;
		CMutex						mLock;

		void 						OnCompleted(Item& inItem, bool inSuccess);
										// throw CPersistenceException
}; // class CMemoryObjectCache

HL_End_Namespace_BigRedH
#endif //_H_CMemoryObjectCache_
