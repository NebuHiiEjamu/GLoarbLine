/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
#ifndef _H_CRefObj_
#define _H_CRefObj_

#include "CListener.h"
#include "CPersistenceException.h"

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

// Pools of reference counted objects store pairs like   { CRefObj, Obj* }
// The way objects refered by TRef look like
// this object can be dynamically	
// casted to CImage, CSound or something else
//?? since Release does only decrement the counter the object is not deleted
//   when the reference count reaches 0 => somebody else should do that
//   Fo Images it's CMemoryObjectCache's job
template <class B>
class CRefObj	
{				
	public:
					CRefObj() 
						// throws nothing
						: mRefCount(0)
						{}
		virtual		~CRefObj()
						// throws nothing
						{ ASSERT(mRefCount==0);	}
		void 		AddRef() 
						// throws nothing
						{ mRefCount++; }
		void 		Release() 
						// throws nothing
						{ --mRefCount; }
		UInt32		GetRefCount()
						// throws nothing
						{ return mRefCount; }						
		virtual B*	GetSibling() = 0;
						// throws ???
	private:
		UInt32 		mRefCount;	
};	// class CRefObj			   


/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// reference counted object pointer
// points to a CRefObj that can be dynamically casted to a T

template <class T, class B>
class TRef
{
	public:
					TRef()
						// throws nothing
						: mRef(nil)
						, mQuick(nil)
						{}
					TRef(CRefObj<B>& inObject)
						// throws nothing
						: mRef(&inObject)
						, mQuick(nil)
						{ AddRef(); }
					TRef(TRef& inOther)
						// throws nothing
						: mRef(inOther.mRef)
						, mQuick(nil)
						{ AddRef(); }
					~TRef()
						// throws nothing
						{ try { Release(); } catch (...) {} }

		void 		SetRef(CRefObj<B>& inObject)
						// throws nothing
						{	// release the old mRef and replace with the new one
							Release(); 
							mRef = &inObject;
							ASSERT(mRef != nil);
							AddRef(); 
						}
						
		void 		AddRef()
						// throws nothing
						{ 
							if (mRef != nil)
							{
								mRef->AddRef(); 
								B* b = mRef->GetSibling();
								ASSERT(b != nil);
								mQuick = dynamic_cast <T*> (b);
								// we should have the object here
								ASSERT(mQuick != nil);

								if (mQuick == nil)
									THROW_UNKNOWN_PERSISTENCE_(eCreateObject);
							}	
						}

		void		Release()
						// throws nothing
						{
							if (mRef != nil)
							{
								CRefObj<B>* tmp = mRef; // don't remove this
								mRef   = nil;		 // you want mRef to
								mQuick = nil;		 // be nil at the time
								tmp->Release(); 	 // Release is called	
							}	
						}						

		T* 			operator -> () 
						// throws nothing
						{ ASSERT(mQuick!=nil); return mQuick; }
					operator T& () 
						// throws nothing
						{ ASSERT(mQuick!=nil); return *mQuick; }
					operator T* () 
						// throws nothing
						{ ASSERT(mQuick!=nil); return mQuick; }

		bool		IsValid()
						// throws ???
						{ return (mQuick!=nil)&&(mQuick->IsValid()); }				
						
	private:
		CRefObj<B>*	mRef;
		T*			mQuick; // mQuick = dynamic_cast <T*> (mRef)
}; // class TRef


/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// a TRef that can listen to object creation messages
class CFSFileRef;

template <class T, class B, class Parent>
class TListenerRef : public TRef<T,B>
				   , public CListener
{
	public:
					TListenerRef(Parent& inParent)
						// throws nothing
						: mParent(inParent)	
						{}
					TListenerRef(Parent& inParent, T& inObject)
						// throws nothing
						: TRef(inObject)
						, mParent(inParent)	
						{}
					TListenerRef(TListenerRef& inOther)
						// throws nothing
						: TRef(inOther)
						, mParent(inOther.mParent)	
						{}

		void 		LoadFrom(CFSFileRef& inName)
						// throws ???
						{
							mName = std::auto_ptr<CFSFileRef>(static_cast<CFSFileRef*>(inName.Clone().release()));
							mName->AddListener(*this);
							mName->LoadObject();
						}
	protected: 
						// CListener::ListenToMessage override
		void		ListenToMessage( const CMessage &inMessage );
						// throws CMessageException
	private:
		Parent&						mParent;
		std::auto_ptr<CFSFileRef>	mName;
}; // class TListenerRef


// ---------------------------------------------------------------------
//  ListenToMessage												[public]
// ---------------------------------------------------------------------
// listens to object created notification from CFSFileRef

template <class T, class B, class Parent>
void
TListenerRef<T,B,Parent>::ListenToMessage( const CMessage &inMessage )
{
	try
	{
		if (inMessage.GetID() == CFSFileRef::eObject)
		{
			const CFSFileRef::Message* msg 
					= static_cast< const CFSFileRef::Message* > (&inMessage);
			if (mName.get() == &msg->GetSource())
			{
				mName->RemoveListener(*this);
				if (msg->GetMsg()) // successfull load
				{
					SetRef(mName->GetLoadedObject());
					mParent.ObjectAvailableNotification(*this);
				}
			}	
		}	
	}
	catch (...) { RETHROW_MESSAGE_(eListenToMessage); }
}				

HL_End_Namespace_BigRedH
#endif // _H_CRefObj_
