/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CDecompressBitmap.h"


CDecompressBitmap::CDecompressBitmap(Uint32 inSize, bool inSilentFail) 
	: CDecompressImage(inSize, inSilentFail, true)
{
	mStage = 1;
	mColorBuf = nil;
}

CDecompressBitmap::~CDecompressBitmap()
{
	if(mColorBuf)
		UMemory::Dispose((TPtr)mColorBuf);
}

bool CDecompressBitmap::Decompress(const void *inData, Uint32 inDataSize)
{
	mBuffer = inData;
	mBytesInBuffer = inDataSize;

	try
	{
		return ReadBitmapImage();
	}
	catch(SError& inError)
	{
		if(mColorBuf)
		{
			UMemory::Dispose((TPtr)mColorBuf);
			mColorBuf = nil;
		}
		
		if (mSilentFail)
			inError.silent = true;
			
		throw; 
	}
	
	return false;
}

bool CDecompressBitmap::ReadBitmapImage()
{
	Uint32 nOldReadRows = mReadRows;
	
startagain:
	
	switch (mStage) 
	{
		case 1:
			BITMAPFILEHEAD bfh;
			if(ReadImage(&bfh, sizeof(BITMAPFILEHEAD)) != END_OF_BYTES)
			{
				if(UMemory::Compare(&bfh.bfType, "BM", 2)) 
					Fail(errorType_Image, imgError_NotBitmapFile);

				mStage = 2;
				goto startagain;
			}
			break;
			
		case 2:
			BITMAPINFOHEAD bih;
			if(ReadImage(&bih, sizeof(BITMAPINFOHEAD)) != END_OF_BYTES)
			{
				#if !CONVERT_INTS
				bih.biCompression = swap_int(bih.biCompression);
				#endif

				if(bih.biCompression)
					Fail(errorType_Image, imgError_UnsupportedFormat);
				
				#if !CONVERT_INTS
				bih.biWidth = swap_int(bih.biWidth);
				bih.biHeight = swap_int(bih.biHeight);
				bih.biBitCount = swap_int(bih.biBitCount);
				bih.biClrUsed = swap_int(bih.biClrUsed);
				#endif
				
				mPixmap.width = bih.biWidth;
				mPixmap.height = bih.biHeight;
				mPixmap.depth = bih.biBitCount; 
				mPixmap.rowBytes = (bih.biWidth*bih.biBitCount + 31)/32 * 4;
 
				if(bih.biClrUsed)
					mPixmap.colorCount = bih.biClrUsed;
				else
					switch (bih.biBitCount)
					{
						case 1 : mPixmap.colorCount = 2; break;
						case 4 : mPixmap.colorCount = 16; break;
						case 8 : mPixmap.colorCount = 256; break;
						default: mPixmap.colorCount = 0;
					};
	
				if(mPixmap.colorCount)
				{
					try
					{
						mColorBuf = (BITMAPRGB*)UMemory::NewClear(mPixmap.colorCount*sizeof(BITMAPRGB));
					}
					catch(...)
					{
						Fail(errorType_Image, imgError_InsufficientMemory);
					}
	
					try
					{
						mPixmap.colorTab = (Uint32*)UMemory::NewClear(mPixmap.colorCount*4);
					}
					catch(...)
					{
						Fail(errorType_Image, imgError_InsufficientMemory);
					}
			
					mStage = 3;
					goto startagain;
				}
				else
				{
					mStage = 4;
					goto startagain;
				}
			}
			break;
			
		case 3:	
			if(ReadImage(mColorBuf, mPixmap.colorCount*sizeof(BITMAPRGB)) != END_OF_BYTES)
			{
				Uint8 *p = (Uint8 *)mPixmap.colorTab;
				for(Uint16 i=0; i<mPixmap.colorCount; i++)
				{
					*p++ = mColorBuf[i].rgbRed;    
					*p++ = mColorBuf[i].rgbGreen; 
					*p++ = mColorBuf[i].rgbBlue;  
					*p++ = 0;
				}

				UMemory::Dispose((TPtr)mColorBuf);
				mColorBuf = nil;

				mStage = 4;
				goto startagain;
			}
			break;
			
		case 4:
			try
			{
				mPixmap.data = UMemory::NewClear(mPixmap.rowBytes*mPixmap.height);
			}
			catch(...)
			{
				Fail(errorType_Image, imgError_InsufficientMemory);
			}
			
			mStage = 5;
			goto startagain;
			break;
			
		case 5:
	 		while(mReadRows < mPixmap.height) 
			{
				void *data = (Uint8*)mPixmap.data + (mPixmap.height-mReadRows-1)*mPixmap.rowBytes;
				if(ReadImage(data, mPixmap.rowBytes) != END_OF_BYTES)
				{
					if(mPixmap.depth == 16)
						for(Uint16 i=0; i<mPixmap.width; i++)
							*((Uint16*)data + i) = (*((Uint16*)data + i) >> 1) | (*((Uint16*)data + i) << 15);
										
					if(mPixmap.depth == 24)
						for(Uint16 i=0; i<mPixmap.width; i++)
						{
							Uint8 nTempColor = *((Uint8*)data + 3*i);
							*((Uint8*)data + 3*i) = *((Uint8*)data + 3*i + 2);
							*((Uint8*)data + 3*i + 2) = nTempColor;
						}

					mReadRows++;					
				}
				else
					break;
			}
	
			if(mReadRows == mPixmap.height)
			{
				mStage = 6;
				goto startagain;
			}
			break;
			
		case 6: 
			mIsComplete = true;
			break;	
	};
	
	return (mReadRows > nOldReadRows);
}
