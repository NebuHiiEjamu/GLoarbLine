/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "MoreTypes.h"
#include "UMemory.h"

// void pointer list class
class CVoidPtrList
{
	public:
		// init and dispose
		CVoidPtrList();

		~CVoidPtrList();
		
		void SetData(TPtr inPtr, Uint32 inCount, Uint32 inMaxCount);
		
		void **GetArrayPtr() const									
		{	return mData; }

		// add and remove items
		Uint32 AddItem(void *inPtr);
		void Preallocate(Uint32 inCount);
		Uint32 InsertItem(Uint32 inIndex, void *inPtr);
		Uint32 RemoveItem(void *inPtr);
		void *RemoveItem(Uint32 inIndex);
		void Clear();
		void Truncate(Uint32 inCount);

		// set and get items
		void SetItem(Uint32 inIndex, void *inPtr)					{	if (inIndex && inIndex <= mCount) mData[inIndex-1] = inPtr;		}
		void *GetItem(Uint32 inIndex) const							{	return (inIndex && inIndex <= mCount) ? mData[inIndex-1] : 0;	}
		Uint32 GetItemCount() const									{	return mCount;													}
		Uint32 GetItemIndex(void *inPtr) const;
		
		// rearrange list
		void MoveForward(void *inPtr);
		void MoveBackward(void *inPtr);
		void MoveToFront(void *inPtr);
		void MoveToBack(void *inPtr);
		
		// searching
		bool GetNext(void*& ioPtr, Uint32& ioIndex) const			{	return ioIndex++, ((ioIndex <= mCount) ? ioPtr = mData[ioIndex-1], true : false);													}
		bool GetPrev(void*& ioPtr, Uint32& ioIndex) const			{	return ((ioIndex == 0) ? ioIndex = mCount : ioIndex--), ((ioIndex && ioIndex <= mCount) ? ioPtr = mData[ioIndex-1], true : false);	}
		bool IsInList(void *inPtr) const							{	return UMemory::SearchLongArray((Uint32)inPtr, (Uint32 *)mData, mCount) != 0;														}

		// sorting
		void Sort(TComparePtrsProc inProc, void *inRef = nil);
		bool SortedSearch(void *inPtr, Uint32& outIndex, TComparePtrsProc inProc, void *inRef = nil);

	protected:
		void **mData;
		Uint32 mCount, mMaxCount;
};

// strongly typed pointer list template class
template <class T>
class CPtrList : public CVoidPtrList
{
	public:
		T **GetArrayPtr() const							{	return (T **)mData;									}

		Uint32 AddItem(T *inPtr)						{	return CVoidPtrList::AddItem(inPtr);				}
		Uint32 InsertItem(Uint32 inIndex, T *inPtr)		{	return CVoidPtrList::InsertItem(inIndex, inPtr);	}
		Uint32 RemoveItem(T *inPtr)						{	return CVoidPtrList::RemoveItem(inPtr);				}
		T *RemoveItem(Uint32 inIndex)					{	return (T *)CVoidPtrList::RemoveItem(inIndex);		}

		void SetItem(Uint32 inIndex, T *inPtr)			{	CVoidPtrList::SetItem(inIndex, inPtr);				}
		T *GetItem(Uint32 inIndex) const				{	return (T*)CVoidPtrList::GetItem(inIndex);			}
		Uint32 GetItemIndex(T *inPtr) const				{	return CVoidPtrList::GetItemIndex(inPtr);			}
		
		void MoveForward(T *inPtr)						{	CVoidPtrList::MoveForward(inPtr);					}
		void MoveBackward(T *inPtr)						{	CVoidPtrList::MoveBackward(inPtr);					}
		void MoveToFront(T *inPtr)						{	CVoidPtrList::MoveToFront(inPtr);					}
		void MoveToBack(T *inPtr)						{	CVoidPtrList::MoveToBack(inPtr);					}
		
		bool GetNext(T*& ioPtr, Uint32& ioIndex) const	{	return CVoidPtrList::GetNext((void*&)ioPtr, ioIndex);		}
		bool GetPrev(T*& ioPtr, Uint32& ioIndex) const	{	return CVoidPtrList::GetPrev((void*&)ioPtr, ioIndex);		}
		bool IsInList(T *inPtr) const					{	return CVoidPtrList::IsInList(inPtr);				}

		bool SortedSearch(T *inPtr, Uint32& outIndex, TComparePtrsProc inProc, void *inRef = nil)				{	return CVoidPtrList::SortedSearch(inPtr, outIndex, inProc, inRef);	}
};
 

