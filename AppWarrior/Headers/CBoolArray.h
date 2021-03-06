/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UMemory.h"

class CBoolArray
{
	public:
		// construction
		CBoolArray();
		~CBoolArray();
	
		// add and remove items
		Uint32 InsertItems(Uint32 inIndex, Uint32 inCount, bool inData);
		Uint32 InsertItem(Uint32 inIndex, bool inData);
		Uint32 AddItem(bool inData);
		void RemoveItems(Uint32 inIndex, Uint32 inCount);
		void RemoveItem(Uint32 inIndex);
		void Clear();
		Uint32 GetItemCount() const;
		
		// rearrange items
		void MoveItems(Uint32 inToIndex, Uint32 inFromIndex, Uint32 inCount);
		void MoveItem(Uint32 inToIndex, Uint32 inFromIndex);
		void SwapItems(Uint32 inIndexA, Uint32 inIndexB, Uint32 inCount = 1);
		
		// set and get items
		void SetItems(Uint32 inIndex, Uint32 inCount, bool inData);
		void SetItem(Uint32 i, bool b);
		bool GetItem(Uint32 i) const;
		Uint32 GetFirstSet() const;
		Uint32 GetLastSet() const;
		
		// misc
		void SetDataHandle(THdl inHdl, Uint32 inItemCount);
		
	protected:
		THdl mData;
		Uint32 mDataSize;
		Uint32 mCount;
		
		// misc
		void ShrinkData(Uint32 inLessItems);
		void ExpandData(Uint32 inExtraItems);
		void ShiftItems(Uint32 inStartPos, Uint32 inEndPos, Uint32 inDestPos);
};		


