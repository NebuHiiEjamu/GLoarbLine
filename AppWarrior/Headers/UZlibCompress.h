/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

enum
{
	zCompressLevel_None		= 0,
	zCompressLevel_Fastest	= 1,
	zCompressLevel_Best		= 9,
	
	zCompressLevel_Default	= -1
};


typedef class TZCompressObj *TZCompress;
typedef class TZDecompressObj *TZDecompress;

	
class UZlibCompress
{
	public:
		// constructor
		static TZCompress New(Int32 inLevel = zCompressLevel_Default);
		static void Dispose(TZCompress inRef);
		
		static TPtr Compress(TZCompress inRef, const void *inData, Uint32 inDataSize, Uint32 &outSize, bool inFinish);
		
		// low-level interface
		static Uint32 Process(TZCompress inRef, Uint8 *&ioRawData, Uint32 &ioRawSize, Uint8 *outCompressData, Uint32 inCompressMaxSize, bool inFinish = false);
};


class UZlibDecompress
{
	public:
		static TZDecompress New();
		static void Dispose(TZDecompress inRef);
		
		static TPtr Decompress(TZDecompress inRef, const void *inData, Uint32 inDataSize, Uint32 &outSize);
		
		// low-level interface
		static Uint32 Process(TZDecompress inRef, Uint8 *&ioCompressedData, Uint32 &ioCompressedDataSize, Uint8 *outRawData, Uint32 inRawMaxSize);

};



// object interface
class TZCompressObj
{
	public:
		TPtr Compress(const void *inData, Uint32 inDataSize, Uint32 &outSize, bool inFinish = false)										{	return UZlibCompress::Compress(this, inData, inDataSize, outSize, inFinish);								}


		Uint32 Process(Uint8 *&ioRawData, Uint32 &ioRawSize, Uint8 *outCompressData, Uint32 inCompressMaxSize, bool inFinish = false)		{	return UZlibCompress::Process(this, ioRawData, ioRawSize, outCompressData, inCompressMaxSize, inFinish);	}

		void operator delete(void *p)																										{	UZlibCompress::Dispose((TZCompress)p);																		}
	protected:
		TZCompressObj() {}				// force creation via UZlibCompress
};


class TZDecompressObj
{
	public:
		TPtr Decompress(const void *inData, Uint32 inDataSize, Uint32 &outSize)												{	return UZlibDecompress::Decompress(this, inData, inDataSize, outSize);										}

	
		Uint32 Process(Uint8 *&ioCompressedData, Uint32 &ioCompressedDataSize, Uint8 *outRawData, Uint32 inRawMaxSize)		{	return UZlibDecompress::Process(this, ioCompressedData, ioCompressedDataSize, outRawData, inRawMaxSize);	}

		void operator delete(void *p)																						{	UZlibDecompress::Dispose((TZDecompress)p);																	}
	protected:
		TZDecompressObj() {}				// force creation via UZlibDecompress
};

