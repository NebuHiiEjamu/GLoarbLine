/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "CDecompressImage.h"


class CDecompressGif : public CDecompressImage
{
	public:
		// constructor
		CDecompressGif(Uint32 inSize, bool inSilentFail = false);
		virtual ~CDecompressGif();
		
		virtual bool Decompress(const void *inData, Uint32 inDataSize);
		
		GifInfo GetGifInfo()		{	return mGifInfo;			}
		IMAGEBLOCK GetImgBlock()	{ 	return mImgBlock;   		}
		CONTROLBLOCK GetCtrBlock()	{	return mCtrBlock;			}
		bool GetTransColor(Uint32& outTransColor);
		TImage GetTransMask();
		
		bool IsCompleteImage()		{	return mIsCompleteImage;	}

	protected:
		GIFHEADER mGifHeader;      		
		GifInfo mGifInfo;
		GifInfo mImageInfo;
		IMAGEBLOCK mImgBlock;        		
		CONTROLBLOCK mCtrBlock;
		Uint32 mTransColor;
	
		bool mIsCompleteImage;

		Uint8 mStage;  		            
		Uint8 mImageStage;			    
		Uint8 mUnpackStage;       	    
		Uint8 mExtensionStage;			

		Uint8 mCodeSize;           		

		Uint16 mBits2; 	       			// Bits1 plus 1 
		Uint16 mCodeSize1;     			// Current code size in bits 
		Uint16 mCodeSize2;     			// Next codesize 
		Int16 mNextCode;      			// Next available table entry 
		Int16 mThisCode;       			// Code being expanded 
		Int16 mOldToken;      			// Last symbol decoded 
		Int16 mCurrentCode;    			// Code just read 
		Int16 mOldCode;        			// Code read before this one 
		Uint16 mBitsLeft;      			// Number of bits left in *mpCurrentByte 
		Uint8 mBlockSize;      			// Bytes in next block 
		Uint16 mLine;	       			// next line to write 
		Uint16 mByte;          			// next byte to write 
		Uint16 mPass;	       			// pass number for interlaced pictures 

		Uint8 *mpCurrentByte;			// Pointer to current byte in read buffer 
		Uint8 *mpLastByte;     			// Pointer past last byte in read buffer 
		Uint8 mpReadBuffer[255];		// Read buffer 
		Uint8 *mpStack;       			// Stack pointer into firstcodestack 
		Uint8 *mLineBuffer;    			// place to store the current line 

		Uint8 mpFirstCodeStack[4096];   // Stack for first codes 
		Uint8 mpLastCodeStack[4096];    // Statck for previous code 
		Uint16 mpCodeStack[4096];       // Stack for links

		bool ReadGifImage();
		void InitPixmap();
		void AllocMemory();
		bool UnpackGif();
		bool PutExtension();
		bool UnpackImage(Uint8 bits);
		void PutLine(Uint8 *pLine, Uint16 nLine);
};

