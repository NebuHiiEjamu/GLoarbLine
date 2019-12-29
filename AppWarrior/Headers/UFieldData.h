/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "UMemory.h"

/*
 * Types
 */

typedef class TFieldDataObj *TFieldData;

/*
 * Field Data
 */

class UFieldData
{
	public:
		// new and dispose
		static TFieldData New(THdl inData = nil, Uint32 inHeaderSize = 0);
		static void Dispose(TFieldData inRef);
		
		// ID<->index conversion
		static Uint16 GetFieldID(TFieldData inRef, Uint16 inIndex);
		static Uint16 GetFieldIndex(TFieldData inRef, Uint16 inID);
		static Uint16 GetFieldCount(TFieldData inRef);

		// read and write fields by index
		static Uint32 ReadFieldByIndex(TFieldData inRef, Uint16 inIndex, Uint32 inOffset, void *outData, Uint32 inMaxSize);
		static Uint32 WriteFieldByIndex(TFieldData inRef, Uint16 inIndex, Uint32 inOffset, const void *inData, Uint32 inDataSize);
		static void SetFieldSizeByIndex(TFieldData inRef, Uint16 inIndex, Uint16 inSize);
		static Uint32 GetFieldSizeByIndex(TFieldData inRef, Uint16 inIndex);
		static void SetFieldByIndex(TFieldData inRef, Uint16 inIndex, const void *inData, Uint32 inDataSize);
		static Uint32 GetFieldByIndex(TFieldData inRef, Uint16 inIndex, void *outData, Uint32 inMaxSize);
		static void DeleteFieldByIndex(TFieldData inRef, Uint16 inIndex);

		// read and write fields by ID
		static Uint32 ReadField(TFieldData inRef, Uint16 inID, Uint32 inOffset, void *outData, Uint32 inMaxSize);
		static Uint32 WriteField(TFieldData inRef, Uint16 inID, Uint32 inOffset, const void *inData, Uint32 inDataSize);
		static void SetFieldSize(TFieldData inRef, Uint16 inID, Uint16 inSize);
		static Uint32 GetFieldSize(TFieldData inRef, Uint16 inID);
		static void SetField(TFieldData inRef, Uint16 inID, const void *inData, Uint32 inDataSize);
		static void DeleteField(TFieldData inRef, Uint16 inID);
		static void DeleteAllFields(TFieldData inRef);
		
		// add fields
		static void AddField(TFieldData inRef, Uint16 inID, const void *inData, Uint32 inDataSize);
		static void AddInteger(TFieldData inRef, Uint16 inID, Int32 inInteger);
		static void AddPString(TFieldData inRef, Uint16 inID, const Uint8 inPString[]);
		static void AddCString(TFieldData inRef, Uint16 inID, const Int8 inCString[]);
		
		// get fields
		static Uint32 GetField(TFieldData inRef, Uint16 inID, void *outData, Uint32 inMaxSize);
		static Int32 GetInteger(TFieldData inRef, Uint16 inID);
		static Int32 GetIntegerByIndex(TFieldData inRef, Uint16 inIndex);
		static void GetPString(TFieldData inRef, Uint16 inID, Uint8 *outPString, Uint32 inBufSize = 256);
		static void GetCString(TFieldData inRef, Uint16 inID, Int8 *outCString, Uint32 inBufSize);

		// field info
		static bool GetFieldInfo(TFieldData inRef, Uint16 inID, Uint16& outIndex, Uint32& outSize, Uint32& outOffset);
		static bool GetFieldInfoByIndex(TFieldData inRef, Uint16 inIndex, Uint16& outID, Uint32& outSize, Uint32& outOffset);
		
		// data handle that stores field data
		static THdl GetDataHandle(TFieldData inRef);
		static THdl DetachDataHandle(TFieldData inRef);
		static void SetDataHandle(TFieldData inRef, THdl inHdl, Uint32 inHeaderSize = 0);
		static void DataChanged(TFieldData inRef);
};

/*
 * Stack TFieldData
 */

class StFieldData
{
	public:
		StFieldData(THdl inData = nil, Uint32 inHeaderSize = 0)	{	mRef = UFieldData::New(inData, inHeaderSize);	}
		~StFieldData()											{	UFieldData::Dispose(mRef);						}
		operator TFieldData()									{	return mRef;									}
		TFieldDataObj *operator->() const						{	return mRef;									}

	private:
		TFieldData mRef;
};

/*
 * UFieldData Object Interface
 */

class TFieldDataObj
{
	public:
		Uint16 GetFieldID(Uint16 inIndex)																	{	return UFieldData::GetFieldID(this, inIndex);										}
		Uint16 GetFieldIndex(Uint16 inID)																	{	return UFieldData::GetFieldIndex(this, inID);										}
		Uint16 GetFieldCount()																				{	return UFieldData::GetFieldCount(this);												}

		Uint32 ReadFieldByIndex(Uint16 inIndex, Uint32 inOffset, void *outData, Uint32 inMaxSize)			{	return UFieldData::ReadFieldByIndex(this, inIndex, inOffset, outData, inMaxSize);	}
		Uint32 WriteFieldByIndex(Uint16 inIndex, Uint32 inOffset, const void *inData, Uint32 inDataSize)	{	return UFieldData::WriteFieldByIndex(this, inIndex, inOffset, inData, inDataSize);	}
		void SetFieldSizeByIndex(Uint16 inIndex, Uint16 inSize)												{	UFieldData::SetFieldSizeByIndex(this, inIndex, inSize);								}
		Uint32 GetFieldSizeByIndex(Uint16 inIndex)															{	return UFieldData::GetFieldSizeByIndex(this, inIndex);								}
		void SetFieldByIndex(Uint16 inIndex, const void *inData, Uint32 inDataSize)							{	UFieldData::SetFieldByIndex(this, inIndex, inData, inDataSize);						}
		Uint32 GetFieldByIndex(Uint16 inIndex, void *outData, Uint32 inMaxSize)								{	return UFieldData::GetFieldByIndex(this, inIndex, outData, inMaxSize);				}
		void DeleteFieldByIndex(Uint16 inIndex)																{	UFieldData::DeleteFieldByIndex(this, inIndex);										}

		Uint32 ReadField(Uint16 inID, Uint32 inOffset, void *outData, Uint32 inMaxSize)						{	return UFieldData::ReadField(this, inID, inOffset, outData, inMaxSize);				}
		Uint32 WriteField(Uint16 inID, Uint32 inOffset, const void *inData, Uint32 inDataSize)				{	return UFieldData::WriteField(this, inID, inOffset, inData, inDataSize);			}
		void SetFieldSize(Uint16 inID, Uint16 inSize)														{	UFieldData::SetFieldSize(this, inID, inSize);										}
		Uint32 GetFieldSize(Uint16 inID)																	{	return UFieldData::GetFieldSize(this, inID);										}
		void SetField(Uint16 inID, const void *inData, Uint32 inDataSize)									{	UFieldData::SetField(this, inID, inData, inDataSize);								}
		void DeleteField(Uint16 inID)																		{	UFieldData::DeleteField(this, inID);												}
		void DeleteAllFields()																				{	UFieldData::DeleteAllFields(this);													}
		
		void AddField(Uint16 inID, const void *inData, Uint32 inDataSize)									{	UFieldData::AddField(this, inID, inData, inDataSize);								}
		void AddInteger(Uint16 inID, Int32 inInteger)														{	UFieldData::AddInteger(this, inID, inInteger);										}
		void AddPString(Uint16 inID, const Uint8 inPString[])												{	UFieldData::AddPString(this, inID, inPString);										}
		void AddCString(Uint16 inID, const Int8 inCString[])												{	UFieldData::AddCString(this, inID, inCString);										}
		
		Uint32 GetField(Uint16 inID, void *outData, Uint32 inMaxSize)										{	return UFieldData::GetField(this, inID, outData, inMaxSize);						}
		Int32 GetInteger(Uint16 inID)																		{	return UFieldData::GetInteger(this, inID);											}
		Int32 GetIntegerByIndex(Uint16 inIndex)																{	return UFieldData::GetIntegerByIndex(this, inIndex);								}
		void GetPString(Uint16 inID, Uint8 *outPString, Uint32 inBufSize = 256)								{	UFieldData::GetPString(this, inID, outPString, inBufSize);							}
		void GetCString(Uint16 inID, Int8 *outCString, Uint32 inBufSize)									{	UFieldData::GetCString(this, inID, outCString, inBufSize);							}

		bool GetFieldInfo(Uint16 inID, Uint16& outIndex, Uint32& outSize, Uint32& outOffset)				{	return UFieldData::GetFieldInfo(this, inID, outIndex, outSize, outOffset);			}
		bool GetFieldInfoByIndex(Uint16 inIndex, Uint16& outID, Uint32& outSize, Uint32& outOffset)			{	return UFieldData::GetFieldInfoByIndex(this, inIndex, outID, outSize, outOffset);	}
		
		THdl GetDataHandle()																				{	return UFieldData::GetDataHandle(this);												}
		THdl DetachDataHandle()																				{	return UFieldData::DetachDataHandle(this);											}
		void SetDataHandle(THdl inHdl, Uint32 inHeaderSize = 0)												{	UFieldData::SetDataHandle(this, inHdl, inHeaderSize);								}
		void DataChanged()																					{	UFieldData::DataChanged(this);														}
		
		void operator delete(void *p)																		{	UFieldData::Dispose((TFieldData)p);													}
	protected:
		TFieldDataObj() {}		// force creation via UFieldData
};











