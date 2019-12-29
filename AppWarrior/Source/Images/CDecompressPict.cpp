/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CDecompressPict.h"


static OpDef OpTable[] =
  {
    /* 0x00 */	{ "NOP",               0, "nop" },
    /* 0x01 */	{ "Clip",             NA, "clip" },
    /* 0x02 */	{ "BkPat",             8, "background pattern" },
    /* 0x03 */	{ "TxFont",            2, "text font (word)" },
    /* 0x04 */	{ "TxFace",            1, "text face (byte)" },
    /* 0x05 */	{ "TxMode",            2, "text mode (word)" },
    /* 0x06 */	{ "SpExtra",           4, "space extra (fixed point)" },
    /* 0x07 */	{ "PnSize",            4, "pen size (point)" },
    /* 0x08 */	{ "PnMode",            2, "pen mode (word)" },
    /* 0x09 */	{ "PnPat",             8, "pen pattern" },
    /* 0x0a */	{ "FillPat",           8, "fill pattern" },
    /* 0x0b */	{ "OvSize",            4, "oval size (point)" },
    /* 0x0c */	{ "Origin",            4, "dh, dv (word)" },
    /* 0x0d */	{ "TxSize",            2, "text size (word)" },
    /* 0x0e */	{ "FgColor",           4, "foreground color (longword)" },
    /* 0x0f */	{ "BkColor",           4, "background color (longword)" },
    /* 0x10 */	{ "TxRatio",           8, "numerator (point), denominator (point)" },
    /* 0x11 */	{ "Version",           1, "version (byte)" },
    /* 0x12 */	{ "BkPixPat",         NA, "color background pattern" },
    /* 0x13 */	{ "PnPixPat",         NA, "color pen pattern" },
    /* 0x14 */	{ "FillPixPat",       NA, "color fill pattern" },
    /* 0x15 */	{ "PnLocHFrac",        2, "fractional pen position" },
    /* 0x16 */	{ "ChExtra",           2, "extra for each character" },
    /* 0x17 */	res(0),
    /* 0x18 */	res(0),
    /* 0x19 */	res(0),
    /* 0x1a */	{ "RGBFgCol",    RGB_LEN, "RGB foreColor" },
    /* 0x1b */	{ "RGBBkCol",    RGB_LEN, "RGB backColor" },
    /* 0x1c */	{ "HiliteMode",        0, "hilite mode flag" },
    /* 0x1d */	{ "HiliteColor", RGB_LEN, "RGB hilite color" },
    /* 0x1e */	{ "DefHilite",         0, "Use default hilite color" },
    /* 0x1f */	{ "OpColor",           6, "RGB OpColor for arithmetic modes" },
    /* 0x20 */	{ "Line",              8, "pnLoc (point), newPt (point)" },
    /* 0x21 */	{ "LineFrom",          4, "newPt (point)" },
    /* 0x22 */	{ "ShortLine",         6, "pnLoc (point, dh, dv (-128 .. 127))" },
    /* 0x23 */	{ "ShortLineFrom",     2, "dh, dv (-128 .. 127)" },
    /* 0x24 */	res(WORD_LEN),
    /* 0x25 */	res(WORD_LEN),
    /* 0x26 */	res(WORD_LEN),
    /* 0x27 */	res(WORD_LEN),
    /* 0x28 */	{ "LongText",         NA, "txLoc (point), count (0..255), text" },
    /* 0x29 */	{ "DHText",           NA, "dh (0..255), count (0..255), text" },
    /* 0x2a */	{ "DVText",           NA, "dv (0..255), count (0..255), text" },
    /* 0x2b */	{ "DHDVText",         NA, "dh, dv (0..255), count (0..255), text" },
    /* 0x2c */	res(WORD_LEN),
    /* 0x2d */	res(WORD_LEN),
    /* 0x2e */	res(WORD_LEN),
    /* 0x2f */	res(WORD_LEN),
    /* 0x30 */	{ "frameRect",         8, "rect" },
    /* 0x31 */	{ "paintRect",         8, "rect" },
    /* 0x32 */	{ "eraseRect",         8, "rect" },
    /* 0x33 */	{ "invertRect",        8, "rect" },
    /* 0x34 */	{ "fillRect",          8, "rect" },
    /* 0x35 */	res(8),
    /* 0x36 */	res(8),
    /* 0x37 */	res(8),
    /* 0x38 */	{ "frameSameRect",     0, "rect" },
    /* 0x39 */	{ "paintSameRect",     0, "rect" },
    /* 0x3a */	{ "eraseSameRect",     0, "rect" },
    /* 0x3b */	{ "invertSameRect",    0, "rect" },
    /* 0x3c */	{ "fillSameRect",      0, "rect" },
    /* 0x3d */	res(0),
    /* 0x3e */	res(0),
    /* 0x3f */	res(0),
    /* 0x40 */	{ "frameRRect",        8, "rect" },
    /* 0x41 */	{ "paintRRect",        8, "rect" },
    /* 0x42 */	{ "eraseRRect",        8, "rect" },
    /* 0x43 */	{ "invertRRect",       8, "rect" },
    /* 0x44 */	{ "fillRRrect",        8, "rect" },
    /* 0x45 */	res(8),
    /* 0x46 */	res(8),
    /* 0x47 */	res(8),
    /* 0x48 */	{ "frameSameRRect",    0, "rect" },
    /* 0x49 */	{ "paintSameRRect",    0, "rect" },
    /* 0x4a */	{ "eraseSameRRect",    0, "rect" },
    /* 0x4b */	{ "invertSameRRect",   0, "rect" },
    /* 0x4c */	{ "fillSameRRect",     0, "rect" },
    /* 0x4d */	res(0),
    /* 0x4e */	res(0),
    /* 0x4f */	res(0),
    /* 0x50 */	{ "frameOval",         8, "rect" },
    /* 0x51 */	{ "paintOval",         8, "rect" },
    /* 0x52 */	{ "eraseOval",         8, "rect" },
    /* 0x53 */	{ "invertOval",        8, "rect" },
    /* 0x54 */	{ "fillOval",          8, "rect" },
    /* 0x55 */	res(8),
    /* 0x56 */	res(8),
    /* 0x57 */	res(8),
    /* 0x58 */	{ "frameSameOval",     0, "rect" },
    /* 0x59 */	{ "paintSameOval",     0, "rect" },
    /* 0x5a */	{ "eraseSameOval",     0, "rect" },
    /* 0x5b */	{ "invertSameOval",    0, "rect" },
    /* 0x5c */	{ "fillSameOval",      0, "rect" },
    /* 0x5d */	res(0),
    /* 0x5e */	res(0),
    /* 0x5f */	res(0),
    /* 0x60 */	{ "frameArc",         12, "rect, startAngle, arcAngle" },
    /* 0x61 */	{ "paintArc",         12, "rect, startAngle, arcAngle" },
    /* 0x62 */	{ "eraseArc",         12, "rect, startAngle, arcAngle" },
    /* 0x63 */	{ "invertArc",        12, "rect, startAngle, arcAngle" },
    /* 0x64 */	{ "fillArc",          12, "rect, startAngle, arcAngle" },
    /* 0x65 */	res(12),
    /* 0x66 */	res(12),
    /* 0x67 */	res(12),
    /* 0x68 */	{ "frameSameArc",      4, "rect, startAngle, arcAngle" },
    /* 0x69 */	{ "paintSameArc",      4, "rect, startAngle, arcAngle" },
    /* 0x6a */	{ "eraseSameArc",      4, "rect, startAngle, arcAngle" },
    /* 0x6b */	{ "invertSameArc",     4, "rect, startAngle, arcAngle" },
    /* 0x6c */	{ "fillSameArc",       4, "rect, startAngle, arcAngle" },
    /* 0x6d */	res(4),
    /* 0x6e */	res(4),
    /* 0x6f */	res(4),
    /* 0x70 */	{ "framePoly",        NA, "poly" },
    /* 0x71 */	{ "paintPoly",        NA, "poly" },
    /* 0x72 */	{ "erasePoly",        NA, "poly" },
    /* 0x73 */	{ "invertPoly",       NA, "poly" },
    /* 0x74 */	{ "fillPoly",         NA, "poly" },
    /* 0x75 */	res(NA),
    /* 0x76 */	res(NA),
    /* 0x77 */	res(NA),
    /* 0x78 */	{ "frameSamePoly",     0, "poly (NYI)" },
    /* 0x79 */	{ "paintSamePoly",     0, "poly (NYI)" },
    /* 0x7a */	{ "eraseSamePoly",     0, "poly (NYI)" },
    /* 0x7b */	{ "invertSamePoly",    0, "poly (NYI)" },
    /* 0x7c */	{ "fillSamePoly",      0, "poly (NYI)" },
    /* 0x7d */	res(0),
    /* 0x7e */	res(0),
    /* 0x7f */	res(0),
    /* 0x80 */	{ "frameRgn",         NA, "region" },
    /* 0x81 */	{ "paintRgn",         NA, "region" },
    /* 0x82 */	{ "eraseRgn",         NA, "region" },
    /* 0x83 */	{ "invertRgn",        NA, "region" },
    /* 0x84 */	{ "fillRgn",          NA, "region" },
    /* 0x85 */	res(NA),
    /* 0x86 */	res(NA),
    /* 0x87 */	res(NA),
    /* 0x88 */	{ "frameSameRgn",      0, "region (NYI)" },
    /* 0x89 */	{ "paintSameRgn",      0, "region (NYI)" },
    /* 0x8a */	{ "eraseSameRgn",      0, "region (NYI)" },
    /* 0x8b */	{ "invertSameRgn",     0, "region (NYI)" },
    /* 0x8c */	{ "fillSameRgn",       0, "region (NYI)" },
    /* 0x8d */	res(0),
    /* 0x8e */	res(0),
    /* 0x8f */	res(0),
    /* 0x90 */	{ "BitsRect",         NA, "copybits, rect clipped" },

    /* 0x91 */	{ "BitsRgn",          NA, "copybits, rgn clipped" },
    /* 0x92 */	res(WORD_LEN),
    /* 0x93 */	res(WORD_LEN),
    /* 0x94 */	res(WORD_LEN),
    /* 0x95 */	res(WORD_LEN),
    /* 0x96 */	res(WORD_LEN),
    /* 0x97 */	res(WORD_LEN),
    /* 0x98 */	{ "PackBitsRect",     NA, "packed copybits, rect clipped" },
    /* 0x99 */	{ "PackBitsRgn",      NA, "packed copybits, rgn clipped" },
    /* 0x9a */	{ "Opcode_9A",        NA, "the mysterious opcode 9A" },
    /* 0x9b */	res(WORD_LEN),
    /* 0x9c */	res(WORD_LEN),
    /* 0x9d */	res(WORD_LEN),
    /* 0x9e */	res(WORD_LEN),
    /* 0x9f */	res(WORD_LEN),
    /* 0xa0 */	{ "ShortComment",      2, "kind (word)" },
    /* 0xa1 */	{ "LongComment",      NA, "kind (word), size (word), data" }
  };


CDecompressPict::CDecompressPict(Uint32 inSize, bool inSilentFail) 
	: CDecompressImage(inSize, inSilentFail, false)
{
	mStage = 1;
	mReadOpCode = true;
	 	
  	mVersion = 0;
	mOpCode = 0;
	mRowBytes = 0;
	mPixelSizeOrCmpCount = 0;
	mAlphaChannel = false;
	
	mOffset = 512;        // skip empty 512 byte header
	
	mDecompressJpeg = nil;
	mJpegBegin = 0;
	mJpegReadRows = 0;
	mJpegFirstImage = true;
	mJpegFirstSize = true;
}

CDecompressPict::~CDecompressPict()
{
	if(mDecompressJpeg)
		delete mDecompressJpeg;
}

bool CDecompressPict::Decompress(const void *inData, Uint32 inDataSize)
{
	mBuffer = inData;
	mBytesInBuffer = inDataSize;

	try
	{
		return ReadPictImage();
	}
	catch(SError& inError)
	{
		if (mSilentFail)
			inError.silent = true;
			
		throw; 
	}
	
	return false;
}

bool CDecompressPict::ReadPictImage()
{
	Uint32 nOldReadRows = mReadRows;

startagain:
	
	switch (mStage)
	{
		case 1:
			Uint16 nPicSize;             // version 1 picture size, ignored in version 2
			if(ReadWord(nPicSize))	{ mStage = 2; goto startagain;}
			break;
			
		case 2:
		 	SMacRect PictFrame;
			if(ReadMacRect(PictFrame)) { mStage = 3; goto startagain;}
			break;
			
		case 3:
			Uint8 ch;
			while(ReadByte(ch) && ch == 0){};
			if(ch != 0)
			{
  				if (ch != 0x11)
					Fail(errorType_Image, imgError_VersionNumberMissing);
                
   				mStage = 4; goto startagain;
			}
			break;
			
		case 4:
			if(ReadVersion()) { mStage = 5; goto startagain; }
			break;
			
		case 5:
			if(mReadOpCode && !ReadOpCode(mOpCode))
				return false;
					
			mReadOpCode = true;
			
		  	if(mOpCode == 0xFF || mOpCode == 0xFFFF)
		  	{
				Fail(errorType_Image, imgError_VectorData);
		  	}
	  		else if (mOpCode < 0xA2)
	  		{
		  		switch (mOpCode)
  				{
					case 0x01: 
    	      			if(Clip()) goto startagain;
    	      			else mReadOpCode = false;		
    	      			break;
    	      			
    	      	    case 0x12:
			        case 0x13:
			        case 0x14:
			           	if(PixPat()) goto startagain;
			           	else mReadOpCode = false;
				        break;
				
				    case 0x70:
				    case 0x71:
				    case 0x72:
				    case 0x73:
				    case 0x74:
				    case 0x75:
				    case 0x76:
				    case 0x77:
				       	if(SkipPolyOrRegion()) goto startagain;
				       	else mReadOpCode = false;
			        	break;
			        				        	
		        	case 0x90:
        			case 0x98:
          				if(BitsRect()) goto startagain;
						else mReadOpCode = false;
          				break;
     
     			   	case 0x91:
        			case 0x99:
          				if(BitsRegion()) goto startagain;
						else mReadOpCode = false;
          				break;
				
				   	case 0x9A:
          				if(OpCode9A()) goto startagain;
						else mReadOpCode = false;          				
          				break;    
				    
				   	case 0x9B: 
          				if(OpCode9B()) goto startagain;
						else mReadOpCode = false;          				
        				break;    

	      	  		case 0xA1:
				        if(LongComment()) goto startagain;
   				       	else mReadOpCode = false;
 			            break;
			    				        
		    		default:
          				// no function => skip to next OpCode
			          	if(OpTable[mOpCode].len == WORD_LEN)
			          	{
			   				Uint16 nWord;
			   				if(ReadWord(nWord))
			   				{
								mOffset += nWord;
								goto startagain;
							}
							
								mReadOpCode = false;					
            			}
          				else
          				{
            				mOffset += OpTable[mOpCode].len;
            				goto startagain;
            			}
		    	};
		    }
		    else if(mOpCode == 0xC00)
		    {
				mOffset += 24;				
				goto startagain;
			}
    		else if(mOpCode == 0x8200) 	// is a JPEG image
	  		{
			 	mStage = 9; 
			 	goto startagain;
    		}
    		else if(mOpCode >= 0xA2 && mOpCode <= 0xAF)
    		{
			   	Uint16 nWord;
			   	if(ReadWord(nWord))
			   	{
					mOffset += nWord;
					goto startagain;
				}
				
					mReadOpCode = false;					
			}
    		else if((mOpCode >= 0xB0 && mOpCode <= 0xCF) || (mOpCode >= 0x8000 && mOpCode <= 0x80FF))
    		{
	      		// just a reserved OpCode, no data
	      		goto startagain;
    		}
    		else if((mOpCode >= 0xD0 && mOpCode <= 0xFE) || (mOpCode >= 8100 && mOpCode <= 0xFFFF))
    		{
			   	Uint32 nDWord;
			   	if(ReadDWord(nDWord))
			   	{
					mOffset += nDWord;
					goto startagain;
				}
				
					mReadOpCode = false;					
    		}
      		else if(mOpCode >= 0x100 && mOpCode <= 0x7FFF)
      		{
				mOffset += (mOpCode >> 7) & 255;    
				goto startagain;
			}
    		else
    		{
				Fail(errorType_Image, imgError_UnknownFormat);
		    }
			break;
			 
		case 6:
      		if(Unpack32Bits(mRowBytes, mPixelSizeOrCmpCount))
      		{	mStage = 11; goto startagain;}
      		break;

		case 7:
	      	if(Unpack8Bits(mRowBytes))
      		{	mStage = 11; goto startagain;}
      		break;

		case 8:
	   	    if(UnpackBits(mRowBytes, mPixelSizeOrCmpCount))
		    {	mStage = 11; goto startagain;}
      		break;

		case 9: 
 			bool bFound;
 			if(InitJpeg(bFound))
 			{
				if(bFound)
				{	mStage = 10; goto startagain;	}
    		  	else
    			{	mStage = 11; goto startagain;	}
    		}
			break;
			
		case 10:
			if(ReadJpeg())
			{	mStage = 9; goto startagain;	}
			break;
		
		case 11: 
			mIsComplete = true;
			break;
	};
		
	return (mReadRows > nOldReadRows);
}
		

					// OpCode functions

// skips clipping rectangle or region
bool CDecompressPict::Clip()
{
	Uint16 nLen;
	if(ReadWord(nLen))
	{
  		if(nLen == 0x000A)
    		mOffset += 8;
   		else
	   		mOffset += nLen-2;
	   		
	    return true;
	}
    
    return false;
}

// skips pattern definition.
bool CDecompressPict::PixPat() 
{
	Uint32 nOffset = mOffset;

	Uint16 nPatType;
	if(!ReadWord(nPatType))
		return false;
	
	switch (nPatType)
    {
    	case 2:
	   		mOffset += 13;
       		break;
        		
      	case 1:
	   		mOffset += 8;
	        
	        Uint16 rowBytes;
	        if(!ReadWord(rowBytes))
	        {
		        mOffset = nOffset;
		        return false;
	        }
    	    
    	    SMacPixMap pixMap;
        	if(!ReadPixmap(pixMap))
        	{
		        mOffset = nOffset;
		        return false;
        	}

			Uint16 nNumColors;
	        Uint32 *pColorTab = ReadColourTable(nNumColors);
   	    	if(!pColorTab)
 	    	{
		        mOffset = nOffset;
		        return false;
   	    	}
   	    	
	       		UMemory::Dispose((TPtr)pColorTab);

   	    	if(!SkipBits(pixMap.Bounds, rowBytes, pixMap.pixelSize))
   	    	{
		        mOffset = nOffset;
		        return false;
   	    	}
	        break;
      
      	default:
			Fail(errorType_Image, imgError_UnknownPatternType);
    };
    
    return true;
}

// skips unneeded packbits
bool CDecompressPict::SkipBits(SMacRect inBounds, Uint16 inRowBytes, Uint16 inPixelSize)
{
	Uint16 nHeight = inBounds.bottom - inBounds.top;
  	Uint16 nWidth = inBounds.right - inBounds.left;

  	// high bit of rowBytes is flag
  	if(inPixelSize <= 8)
    	inRowBytes &= 0x7FFF;

  	Uint16 nPixWidth = nWidth;

  	if(inPixelSize == 16)
    	nPixWidth *= 2;

 	if(inRowBytes == 0)
    	inRowBytes = nPixWidth;

  	if(inRowBytes < 8)
  	{
  		mOffset += inRowBytes*nHeight;
  	}
  	else
  	{
    	for(Uint16 i = 0; i < nHeight; i++)
    	{
      		if(inRowBytes > 250)
	  		{	      		
	  			Uint16 nLineLen;            // length of source line in bytes.
        		if(!ReadWord(nLineLen))
        			return false;
        		
   		  		mOffset += nLineLen;
        	}
       		else
       		{
	  			Uint8 nLineLen;            // length of source line in bytes.
        		if(!ReadByte(nLineLen))
        			return false;
        		
   		  		mOffset += nLineLen;
        	}
    	}
  	}
  	
  	return true;
}

bool CDecompressPict::SkipPolyOrRegion()
{
   	Uint16 nWord;
   	if(ReadWord(nWord))
   	{
		nWord -= 2;
		mOffset += nWord;
		
		return true;
	}
	
	return false;					
}

// bitmap/pixmap data clipped by a rectangle
bool CDecompressPict::BitsRect()
{
	Uint32 nOffset = mOffset;

  	if(!ReadWord(mRowBytes))
  		return false;
                         
  	if(mRowBytes & 0x8000)
  	{
   		if(!DoPixmap(false))
   		{
	        mOffset = nOffset;
	        return false;   		
   		}
   	}
  	else
  	{
   		if(!DoBitmap(false))
   		{
	        mOffset = nOffset;
	        return false;   		
   		}
   	}
   	   
	return true;
}

// bitmap/pixmap data clipped by a region - Untested...
bool CDecompressPict::BitsRegion()
{
	Uint32 nOffset = mOffset;

  	if(!ReadWord(mRowBytes))
  		return false;
  	
	if(mRowBytes & 0x8000)
	{
    	if(!DoPixmap(true))
   		{
	        mOffset = nOffset;
	        return false;   		
   		}
    }
   	else
   	{
    	if(!DoBitmap(true))
   		{
	        mOffset = nOffset;
	        return false;   		
   		}
    }
  
   return true;
}

// decode version 1 bitmap
bool CDecompressPict::DoBitmap(bool inIsRegion)
{
  	SMacRect Bounds;
  	if(!ReadMacRect(Bounds))
  		return false;

  	// ignore source & destination rectangle as well as transfer mode
  	SMacRect TempRect;
  	if(!ReadMacRect(TempRect))
  		return false;
  		
  	if(!ReadMacRect(TempRect))
  		return false;
  		
  	Uint16 nMode;
	if(!ReadWord(nMode))
		return false;

  	if(inIsRegion && !SkipPolyOrRegion())
  		return false;

	try
	{
		mPixmap.colorTab = (Uint32*)UMemory::NewClear(8);
	}
	catch(...)
	{
		Fail(errorType_Image, imgError_InsufficientMemory);
	}

	mPixmap.colorTab[0] = 0xFFFFFFFF;
	mPixmap.colorTab[1] = 0x00000000;
  	((Uint8*)mPixmap.colorTab)[4+RGBA_ALPHA] = 0xFF;

  	CalcDestBPP(1);
 	InitPixmap(Bounds.right-Bounds.left, Bounds.bottom - Bounds.top, false);

  	mPixelSizeOrCmpCount = 1;
  	mStage = 8;
  	
  	return true;
}

// decode version 2 pixmap
bool CDecompressPict::DoPixmap(bool inIsRegion)
{
	SMacPixMap PixMap;
  	if(!ReadPixmap(PixMap))
  		return false;

  	Uint16 nNumColors; 
  	mPixmap.colorTab = ReadColourTable(nNumColors);
	if(!mPixmap.colorTab)
		return false;
	
	mPixmap.colorCount = nNumColors;

  	// ignore source & destination rectangle as well as transfer mode
  	SMacRect TempRect;
  	if(!ReadMacRect(TempRect))
  		return false;
  		
  	if(!ReadMacRect(TempRect))
  		return false;
  	
  	Uint16 nMode;
	if(!ReadWord(nMode))
		return false;

  	if(inIsRegion && !SkipPolyOrRegion())
  		return false;

	CalcDestBPP(PixMap.pixelSize);
  	InitPixmap(PixMap);
	
  	switch (PixMap.pixelSize)
  	{
    	case 32:
      		mPixelSizeOrCmpCount = PixMap.cmpCount;
      		mStage = 6;
      		break;
      		
    	case 8:
      		mPixelSizeOrCmpCount = 0;
	    	mStage = 7;
      		break;
      		
	    default:
       		mPixelSizeOrCmpCount = PixMap.pixelSize;
		    mStage = 8;
  	};
  	
  	return true;
}

bool CDecompressPict::OpCode9A()
{
	Uint32 nOffset = mOffset;

	if(!DoOpCode9A())
	{
        mOffset = nOffset;
        return false;   		
	}
	
	return true;
}

bool CDecompressPict::DoOpCode9A()
{
	mOffset += 6;

	SMacPixMap PixMap;
  	if(!ReadPixmap(PixMap))
  		return false;

  	// ignore source & destination rectangle as well as transfer mode
  	SMacRect TempRect;
  	if(!ReadMacRect(TempRect))
  		return false;
  	
  	if(!ReadMacRect(TempRect))
  		return false;
  		
  	Uint16 nMode;
  	if(!ReadWord(nMode))
  		return false;

  	CalcDestBPP(PixMap.pixelSize);
  	InitPixmap(PixMap);
  	
  	// do the actual unpacking
  	switch(PixMap.pixelSize)
  	{
    	case 32:
      		mPixelSizeOrCmpCount = PixMap.cmpCount;
      		mStage = 6;
      		break;
      		
    	case 8:
      		mPixelSizeOrCmpCount = 0;
  			mStage = 7;
      		break;
      		
    	default:
       		mPixelSizeOrCmpCount = PixMap.pixelSize;
			mStage = 8;
  	};
  	
  	return true;
}

bool CDecompressPict::OpCode9B()
{
	Uint32 nOffset = mOffset;

	if(!DoOpCode9B())
	{
        mOffset = nOffset;
        return false;   		
	}
	
	return true;
}

bool CDecompressPict::DoOpCode9B() 
{
	mOffset += 6;

	SMacPixMap PixMap;
  	if(!ReadPixmap(PixMap))
  		return false;

  	// ignore source & destination rectangle as well as transfer mode
  	SMacRect TempRect;
  	if(!ReadMacRect(TempRect))
  		return false;
  	
  	if(!ReadMacRect(TempRect))
  		return false;
  		
 	mOffset += 2;
	if(!SkipPolyOrRegion())
		return false;
  	
  	CalcDestBPP(PixMap.pixelSize);
  	InitPixmap(PixMap);

  	// do the actual unpacking
  	switch(PixMap.pixelSize)
  	{
    	case 32:
      		mPixelSizeOrCmpCount = PixMap.cmpCount;
      		mStage = 6;
      		break;
      		
    	case 8:
      		mPixelSizeOrCmpCount = 0;
  			mStage = 7;
      		break;
      		
    	default:
       		mPixelSizeOrCmpCount = PixMap.pixelSize;
			mStage = 8;
  	};
  	
  	return true;
}

bool CDecompressPict::LongComment()
{
	if(mOffset + 4 > mBytesInBuffer)
		return false;

	mOffset += 2;

  	Uint16 nLen;
  	ReadWord(nLen);
 
 	if(nLen > 0)
 		mOffset += nLen;
  	
  	return true;
}

bool CDecompressPict::InitJpeg(bool& outFound)
{
 	if(!SearchJpeg(outFound))
		return false;
 	 			
	if(outFound)
	{
		mUpdateRows = mReadRows;
		mJpegReadRows = mReadRows;
		mJpegBegin = mOffset;
		mJpegFirstSize = true;
		
		if(mDecompressJpeg)
			delete mDecompressJpeg;
		
		mDecompressJpeg = new CDecompressJpeg(mBufferSize - mJpegBegin);
  	}
  	else
 	{
		if(!mPixmap.width || !mPixmap.height || !mImage)
			Fail(errorType_Image, imgError_QuicktimeData);
    }
    
    return true;
}

bool CDecompressPict::SearchJpeg(bool& outFound)
{
	outFound = false;
						
	if(mJpegBegin) // JPEG library don't give me a correct offset
	{
		mOffset = mJpegBegin + 100;
		mJpegBegin = 0;
	}
	
	while(!outFound && mOffset + 3 < mBufferSize)
	{
		Uint8 nByte1, nByte2, nByte3;
		if(!ReadByte(nByte1) || !ReadByte(nByte2) || !ReadByte(nByte3))
		{
			mOffset -= 2;
			return false;
		}
					
		if(nByte1 == 0xFF && nByte2 == 0xD8 && nByte3 == 0xFF)
			outFound = true;
    	else
	    	mOffset -= 2;
	}
	
	if(outFound)
		mOffset -= 3;
	
	return true;
}

bool CDecompressPict::ReadJpeg()
{
	if(!mDecompressJpeg)
		return true;
			
	try
	{
		if(mDecompressJpeg->Decompress((Uint8*)mBuffer + mJpegBegin, mBytesInBuffer - mJpegBegin))
		{
			if(mJpegFirstSize)
			{
				Uint32 nJpegWidth, nJpegHeight;
				if(mDecompressJpeg->GetSize(nJpegWidth, nJpegHeight))
				{
					if(!mPixmap.width)
						mPixmap.width = nJpegWidth;
					else if(mPixmap.width != nJpegWidth)
					{
						delete mDecompressJpeg;
						mDecompressJpeg = nil;
						
						return true;
					}
					
					mPixmap.height += nJpegHeight;
					
					if(!mJpegFirstImage)
					{
						TImage tempImage = mImage;
						mImage = nil;
														
						InitCompatibleImage();
						
						UGraphics::CopyPixels(mImage, SPoint(0,0), tempImage, SPoint(0,0), mPixmap.width, mUpdateRows);
						UGraphics::DisposeImage(tempImage);
					}
					
					mJpegFirstSize = false;
				}
			}
			
			if(!mJpegFirstSize)
			{
				mOffset = mJpegBegin + mDecompressJpeg->mOffset;
				mReadRows = mJpegReadRows + mDecompressJpeg->mReadRows;
			
				if(mJpegFirstImage)
				{
					mImage = mDecompressJpeg->GetImagePtr();
				}
				else if(mReadRows > mUpdateRows)
				{
					UGraphics::CopyPixels(mImage, SPoint(0, mUpdateRows), mDecompressJpeg->GetImagePtr(), SPoint(0, mUpdateRows - mJpegReadRows), mPixmap.width, mReadRows - mUpdateRows);
					mUpdateRows = mReadRows;
				}
			}
		}

		if(mOffset >= mBufferSize || mDecompressJpeg->IsComplete())
		{
			TImage tempImage = mDecompressJpeg->DetachImagePtr();
			if(mJpegFirstImage)
				mImage = tempImage;
			else
				UGraphics::DisposeImage(tempImage);
			
			if(mJpegFirstImage)
				mJpegFirstImage = false;

			delete mDecompressJpeg;
			mDecompressJpeg = nil;

			return true;
		}
	}
	catch(...)
	{
		delete mDecompressJpeg;
		mDecompressJpeg = nil;
		
		Fail(errorType_Image, imgError_CorruptJpegImage);
	}
	
	return false;
}

void CDecompressPict::CalcDestBPP(Uint16 SrcBPP)
{
   	if(SrcBPP <= 8)
   		mPixmap.depth = 8;
   	else
   		mPixmap.depth = 32;
}

void CDecompressPict::InitPixmap(SMacPixMap inPixMap)
{
  	if(mPixmap.depth == 32)
  	{	 
  		// 32-bit output.
    	if(inPixMap.cmpCount == 4)
    		// alpha channel in picture.
      		InitPixmap(inPixMap.Bounds.right-inPixMap.Bounds.left, inPixMap.Bounds.bottom-inPixMap.Bounds.top, true);
    	else
    		// no alpha channel in picture.
      		InitPixmap(inPixMap.Bounds.right-inPixMap.Bounds.left, inPixMap.Bounds.bottom-inPixMap.Bounds.top, false);
	}
  	else
  		// 8-bit-output.
   	 	InitPixmap(inPixMap.Bounds.right - inPixMap.Bounds.left, inPixMap.Bounds.bottom - inPixMap.Bounds.top, false);
}

void CDecompressPict::InitPixmap(Uint16 inWidth, Uint16 inHeight, bool inAlphaChannel)
{
	mPixmap.width = inWidth;
	mPixmap.height = inHeight;
	mPixmap.rowBytes = inWidth*(mPixmap.depth/8);
	
	try
	{
		mPixmap.data = UMemory::NewClear(mPixmap.rowBytes*mPixmap.height);
	}
	catch(...)
	{
		Fail(errorType_Image, imgError_InsufficientMemory);
	}
	
	mAlphaChannel = inAlphaChannel;
}

bool CDecompressPict::Unpack32Bits(Uint16 inRowBytes, int inNumBitPlanes) // 3 if RGB, 4 if RGBA
{
	Uint32 nOffset = mOffset;

  	Uint32 nBytesPerRow = mPixmap.width*inNumBitPlanes;        // bytes per row when uncompressed
  	if(inRowBytes == 0)
    	inRowBytes = nBytesPerRow;

	// temporary buffer for line data. In this buffer, pixels are uncompressed but still plane-oriented
	Uint8 *pLineBuf;
	try
	{
		pLineBuf = (Uint8*)UMemory::NewClear(nBytesPerRow);
	}
	catch(...)
	{
		Fail(errorType_Image, imgError_InsufficientMemory);
	}
	
	while(mReadRows < mPixmap.height) 
    { 
	    Uint16 nLineLen;            // length of source line in bytes
    	if(inRowBytes > 250)
    	{
        	if(!ReadWord(nLineLen))
        	{
	          	UMemory::Dispose((TPtr)pLineBuf);
		        mOffset = nOffset;
		        return false;   		
        	}
        }
       	else
       	{
        	Uint8 nLineLenTemp;
        	if(!ReadByte(nLineLenTemp))
        	{
			   	UMemory::Dispose((TPtr)pLineBuf);
		        mOffset = nOffset;
		        return false;   		
        	}
        	
        	nLineLen = nLineLenTemp;
        }

      	// pointer to source line
     	Uint8 *pSrcLine;
     	try
     	{
     		pSrcLine = (Uint8*)UMemory::NewClear(nLineLen);
     	}
     	catch(...)
     	{
   		   	UMemory::Dispose((TPtr)pLineBuf);
			Fail(errorType_Image, imgError_InsufficientMemory);
		}
	  	
	  	try
	  	{
	  		if(ReadImage(pSrcLine, nLineLen) == END_OF_BYTES)
	  		{
   		   		UMemory::Dispose((TPtr)pLineBuf);
	       		UMemory::Dispose((TPtr)pSrcLine);
	       
		        mOffset = nOffset;
	    	    return false;   		
	  		}
	  	}
	  	catch(...)
	  	{
	   		UMemory::Dispose((TPtr)pLineBuf);
       		UMemory::Dispose((TPtr)pSrcLine);
	       
	        mOffset = nOffset;
			throw;
	  	}
  	  
  	  	// current location in pLineBuf
  	  	Uint8 *pBuf = pLineBuf;

      	// unpack bytewise RLE
      	for(Uint16 i = 0; i < nLineLen; )
      	{
        	Uint8 nFlagCounter = pSrcLine[i];
        	if(nFlagCounter & 0x80)
        	{
          		if(nFlagCounter == 0x80)
           			i++;
          		else
          		{ 
            		Uint8 nLen = ((nFlagCounter ^ 255) & 255) + 2;
            		
            		UMemory::Fill(pBuf, nLen, *(pSrcLine+i+1));
            		pBuf += nLen;
            		i += 2;
          		}
        	}
       		else
        	{ 
        		Uint8 nLen = (nFlagCounter & 255) + 1;
           		if (nLen > nLineLen - i - 1)
           			nLen = nLineLen - i - 1;
          		
           		UMemory::Copy(pBuf, pSrcLine+i+1, nLen);
          		pBuf += nLen;
          		i += nLen + 1;
        	}
      	}
  		
  		UMemory::Dispose((TPtr)pSrcLine);

      	// convert plane-oriented data into pixel-oriented data
      	Uint8 *pDestLine = (Uint8*)mPixmap.data + mReadRows*mPixmap.rowBytes;
      	pBuf = pLineBuf;
      	
      	if(inNumBitPlanes == 3)		// no alpha channel
      	{ 
        	for(Uint16 i = 0; i < mPixmap.width; i++)
        	{ 
         		*(pDestLine+RGBA_RED) = *pBuf;                     		// Red
          		*(pDestLine+RGBA_GREEN) = *(pBuf + mPixmap.width);  	// Green
         		*(pDestLine+RGBA_BLUE) = *(pBuf + mPixmap.width*2); 	// Blue
         		*(pDestLine+RGBA_ALPHA) = 0xFF;                     	// Alpha
         		pDestLine +=4;
          		pBuf++;
        	}
      	}
      	else						// image with alpha channel
      	{
        	for (Uint16 i = 0; i < mPixmap.width; i++)
       		{ 
          		*(pDestLine+RGBA_BLUE) = *(pBuf + mPixmap.width*3);  	// Blue
          		*(pDestLine+RGBA_GREEN) = *(pBuf + mPixmap.width*2); 	// Green
          		*(pDestLine+RGBA_RED) = *(pBuf + mPixmap.width);     	// Red
         		*(pDestLine+RGBA_ALPHA) = *pBuf;                     	// Alpha
         		pDestLine +=4;
          		pBuf++;
        	}
      	}
	
		mReadRows++;		
		nOffset = mOffset;
    }

   	UMemory::Dispose((TPtr)pLineBuf);

  	return true;
}

bool CDecompressPict::Unpack8Bits(Uint16 inRowBytes)
{
	Uint32 nOffset = mOffset;

  	// high bit of rowBytes is flag
  	inRowBytes &= 0x7FFF;
  	if(inRowBytes == 0)
    	inRowBytes = mPixmap.width;

  	Uint8* pLineBuf;
  	try
  	{
  		pLineBuf = (Uint8*)UMemory::NewClear(inRowBytes * 4);    
  	}
  	catch(...)
  	{
		Fail(errorType_Image, imgError_InsufficientMemory);
	}

    if(inRowBytes < 8)
    {
      	// pointer to source line
     	Uint8 *pSrcLine;
     	try
     	{
     		pSrcLine = (Uint8*)UMemory::NewClear(inRowBytes);
     	}
     	catch(...)
     	{
   		   	UMemory::Dispose((TPtr)pLineBuf);
			Fail(errorType_Image, imgError_InsufficientMemory);
		}

		while(mReadRows < mPixmap.height) 
      	{
     	  	try
     	  	{
	     	  	if(ReadImage(pSrcLine, inRowBytes) == END_OF_BYTES)
		  		{
   			   		UMemory::Dispose((TPtr)pLineBuf);
       				UMemory::Dispose((TPtr)pSrcLine);
	       
	        		mOffset = nOffset;
	        		return false;   		
	  			}
	  		}
	  		catch(...)
	  		{
   		   		UMemory::Dispose((TPtr)pLineBuf);
    			UMemory::Dispose((TPtr)pSrcLine);
	       
	       		mOffset = nOffset;
	       		throw;  
	  		}

           	Uint8 *pDestLine = (Uint8*)mPixmap.data + mReadRows*mPixmap.rowBytes;	
 
        	if(mPixmap.depth == 32)
        	{
          		try
          		{
          			ExpandBuf(pDestLine, pSrcLine, mPixmap.width, 8);
          		}
          		catch(...)
          		{
          		   	UMemory::Dispose((TPtr)pLineBuf);
       			    UMemory::Dispose((TPtr)pSrcLine);

          		    throw;
          		}
        	}
        	else
        	{
          		UMemory::Copy(pDestLine, pSrcLine, mPixmap.width);
      	    }
      	
			mReadRows++;		
			nOffset = mOffset;
      	}
      	
      	UMemory::Dispose((TPtr)pSrcLine);
    }
    else
    {
		while(mReadRows < mPixmap.height) 
      	{ 
      	    Uint16 nLineLen;            // length of source line in bytes
    		if(inRowBytes > 250)
    		{
	        	if(!ReadWord(nLineLen))
	        	{
   	          		UMemory::Dispose((TPtr)pLineBuf);
		       	 	mOffset = nOffset;
		        	return false;   		
		        }
	        }
       		else
       		{
        		Uint8 nLineLenTemp;
	        	if(!ReadByte(nLineLenTemp))
	        	{
	          		UMemory::Dispose((TPtr)pLineBuf);
		       	 	mOffset = nOffset;
		        	return false;   		    	
	        	}
	        	
    	    	nLineLen = nLineLenTemp;
	      	}
  	
  	     	Uint8 *pSrcLine;
  	     	try
  	     	{
  	     		pSrcLine = (Uint8*)UMemory::NewClear(nLineLen);
  	     	}
	     	catch(...)
	     	{
   			   	UMemory::Dispose((TPtr)pLineBuf);
				Fail(errorType_Image, imgError_InsufficientMemory);
			}
   		
   		  	try
   		  	{
	   		  	if(ReadImage(pSrcLine, nLineLen) == END_OF_BYTES)
		  		{
   			   		UMemory::Dispose((TPtr)pLineBuf);
       				UMemory::Dispose((TPtr)pSrcLine);
	       
	        		mOffset = nOffset;
	        		return false;   		
	  			}
	  		}
	  		catch(...)
	  		{
		   		UMemory::Dispose((TPtr)pLineBuf);
   				UMemory::Dispose((TPtr)pSrcLine);
	       
        		mOffset = nOffset;
        		throw;
	  		}
        	
        	Uint8* pDestPixel = pLineBuf;

        	for(Uint16 i = 0; i < nLineLen; )
        	{
          		Uint8 nFlagCounter = pSrcLine[i];

          		if(nFlagCounter & 0x80)
          		{
            		if(nFlagCounter == 0x80)
              			i++;
            		else
            		{
              			Uint16 nLen = ((nFlagCounter ^ 255) & 255) + 2;

	              		if(mPixmap.depth == 32)
              			{
                			Uint32 PixValue = mPixmap.colorTab[*(pSrcLine+i+1)];
                			for(Uint16 k = 0; k < nLen; k++)
                  				*(Uint32 *)(pDestPixel+k*4) = PixValue;

                			pDestPixel += nLen*4;
              			}	
              			else
              			{
                			Uint8 PixValue = *(pSrcLine+i+1);
                			for(Uint16 k = 0; k < nLen; k++)
                  				*(pDestPixel+k) = PixValue;

		                	pDestPixel += nLen;
              			}
              
             			i += 2;
            		}
          		}
          		else
          		{
	           		Uint16 nLen = (nFlagCounter & 255) + 1;
               		if (nLen > nLineLen - i - 1)
            			nLen = nLineLen - i - 1;
            		
            		if(mPixmap.depth == 32)
            		{
              			for(Uint16 k=0; k<nLen; k++)
              			{
                			*((Uint32 *)pDestPixel) = mPixmap.colorTab[*(pSrcLine+i+k+1)];
			                pDestPixel += 4;
            		    }
            		}
            		else
            		{
              			for(Uint16 k=0; k<nLen; k++)
              			{
                			*(pDestPixel) = *(pSrcLine+i+k+1);
                			pDestPixel++;
              			}
            		}
            
            		i += nLen + 1;
          		}
        	}
        
           	UMemory::Dispose((TPtr)pSrcLine);
           	
           	Uint8 *pDestLine = (Uint8*)mPixmap.data + mReadRows*mPixmap.rowBytes;
        	if(mPixmap.depth == 32)
          		UMemory::Copy(pDestLine, pLineBuf, 4*mPixmap.width);
        	else
          		UMemory::Copy(pDestLine, pLineBuf, mPixmap.width);
          		
  			mReadRows++;		
			nOffset = mOffset;
      	}
    }

	UMemory::Dispose((TPtr)pLineBuf);

	return true;
}

bool CDecompressPict::UnpackBits(Uint16 inRowBytes, int inPixelSize)
{
	Uint32 nOffset = mOffset;

  	// high bit of rowBytes is flag
  	if(inPixelSize <= 8)
    	inRowBytes &= 0x7FFF;

  	Uint16 nPixWidth = mPixmap.width;
  	Int16 nPkpixSize = 1;      
	if(inPixelSize == 16)    
	{
    	nPkpixSize = 2;
    	nPixWidth *= 2;
  	}

  	if(inRowBytes == 0)
    	inRowBytes = nPixWidth;

    Int16 PixelPerRLEUnit;
    Uint8 *pLineBuf;
    switch (inPixelSize)
    {
    	case 1:
        	PixelPerRLEUnit = 8;
        	pLineBuf = (Uint8*)UMemory::NewClear((inRowBytes+1) * 32);
        	break;
     	 
     	 case 2:
        	PixelPerRLEUnit = 4;
        	pLineBuf = (Uint8*)UMemory::NewClear((inRowBytes+1) * 16);
        	break;
      
      	case 4:
        	PixelPerRLEUnit = 2;
        	pLineBuf = (Uint8*)UMemory::NewClear((inRowBytes+1) * 8);
        	break;
      
      	case 8:
        	PixelPerRLEUnit = 1;
        	pLineBuf = (Uint8*)UMemory::NewClear(inRowBytes * 4);
        	break;
      	
      	case 16:
        	PixelPerRLEUnit = 1;
       		pLineBuf = (Uint8*)UMemory::NewClear(inRowBytes * 2 + 4);
       	 	break;
       	 	
      	default:
			Fail(errorType_Image, imgError_UnknownFormat);
    };

  	if(!pLineBuf)
		Fail(errorType_Image, imgError_InsufficientMemory);

    if(inRowBytes < 8)
    {
    	Uint8 *pSrcLine;
    	try
    	{
    		pSrcLine = (Uint8*)UMemory::NewClear(inRowBytes);
    	}
     	catch(...)
     	{
   		   	UMemory::Dispose((TPtr)pLineBuf);
			Fail(errorType_Image, imgError_InsufficientMemory);
		}
     	
		while(mReadRows < mPixmap.height) 
      	{
     	  	try
     	  	{
     	  		if(ReadImage(pSrcLine, inRowBytes) == END_OF_BYTES)
	  			{
   		   			UMemory::Dispose((TPtr)pLineBuf);
       				UMemory::Dispose((TPtr)pSrcLine);
	       
	        		mOffset = nOffset;
	        		return false;   		
	  			}
	  		}
	  		catch(...)
	  		{
  	   			UMemory::Dispose((TPtr)pLineBuf);
    			UMemory::Dispose((TPtr)pSrcLine);
	       
	       		mOffset = nOffset;
	       		throw;
	  		}

        	Uint8 *pDestLine = (Uint8*)mPixmap.data + mReadRows*mPixmap.rowBytes;
        
        	try
        	{
        		if(mPixmap.depth == 32)
          			ExpandBuf(pDestLine, pSrcLine, mPixmap.width, inPixelSize);
        		else
          			ExpandBuf8(pDestLine, pSrcLine, mPixmap.width, inPixelSize);
          	}
          	catch(...)
          	{
          	   	UMemory::Dispose((TPtr)pLineBuf);
       		    UMemory::Dispose((TPtr)pSrcLine);

          	    throw;
          	}
          		
			mReadRows++;		
			nOffset = mOffset;
      	}
      	
      	UMemory::Dispose((TPtr)pSrcLine);
	}
   	else
    {
		while(mReadRows < mPixmap.height) 
      	{
      	    Uint16 nLineLen;            // length of source line in bytes
    		if(inRowBytes > 250)
    		{
	        	if(!ReadWord(nLineLen))
	        	{
	          		UMemory::Dispose((TPtr)pLineBuf);
		       	 	mOffset = nOffset;
		        	return false;   		    		        	
	        	}
	        }
       		else
       		{
        		Uint8 nLineLenTemp;
	        	if(!ReadByte(nLineLenTemp))
	        	{
	        		UMemory::Dispose((TPtr)pLineBuf);
		       	 	mOffset = nOffset;
		        	return false;   		    	
	        	}
	        	
    	    	nLineLen = nLineLenTemp;
	      	}
  	
  	     	Uint8 *pSrcLine;
  	     	try
  	     	{
  	     		pSrcLine = (Uint8*)UMemory::NewClear(nLineLen);
  	     	}
	     	catch(...)
	     	{
   			   	UMemory::Dispose((TPtr)pLineBuf);
				Fail(errorType_Image, imgError_InsufficientMemory);
			}
   		
   		  	try
   		  	{
   		  		if(ReadImage(pSrcLine, nLineLen) == END_OF_BYTES)
	  			{
   		   			UMemory::Dispose((TPtr)pLineBuf);
       				UMemory::Dispose((TPtr)pSrcLine);
	       
	        		mOffset = nOffset;
	        		return false;   		
	  			}
	  		}
	  		catch(...)
	  		{
	   			UMemory::Dispose((TPtr)pLineBuf);
    			UMemory::Dispose((TPtr)pSrcLine);
	       
	       		mOffset = nOffset;
	       		throw;
	  		}
   		  	
        	Uint8* pBuf = pLineBuf;

        	for(Uint16 i = 0; i < nLineLen; )
        	{
          		Uint8 nFlagCounter = pSrcLine[i];
          		if(nFlagCounter & 0x80)
          		{
            		if(nFlagCounter == 0x80)
              			i++;
            		else
            		{
              			Uint16 nLen = ((nFlagCounter ^ 255) & 255) + 2;

              			if(mPixmap.depth == 32)
              			{
                			try
                			{
                				ExpandBuf(pBuf, pSrcLine+i+1, 1, inPixelSize);
                			}
                			catch(...)
          		            {
          		   	            UMemory::Dispose((TPtr)pLineBuf);
       			                UMemory::Dispose((TPtr)pSrcLine);

          		                throw;
          		            }
               			
                			for (Uint16 k = 1; k < nLen; k++)
                  				UMemory::Copy(pBuf+(k*4*PixelPerRLEUnit), pBuf, 4*PixelPerRLEUnit);
                			
			                pBuf += nLen*4*PixelPerRLEUnit;
            			}
              			else
              			{
                			try
                			{
                				ExpandBuf8(pBuf, pSrcLine+i+1, 1, inPixelSize);
			                }
			                catch(...)
          		            {
          		   	            UMemory::Dispose((TPtr)pLineBuf);
       			                UMemory::Dispose((TPtr)pSrcLine);

          		                throw;
          		            }
			                
			                for(Uint16 k = 1; k < nLen; k++)
                  				UMemory::Copy(pBuf+(k*PixelPerRLEUnit), pBuf, PixelPerRLEUnit);
                
                			pBuf += nLen*PixelPerRLEUnit;
              			}
              			
              			i += 1 + nPkpixSize;
            		}
          		}
          		else
          		{
            		Uint16 nLen = (nFlagCounter & 255) + 1;
            		if (nLen > (nLineLen - i - 1)/nPkpixSize)
            			nLen = (nLineLen - i - 1)/nPkpixSize;

            		if(mPixmap.depth == 32)
            		{
              			try
              			{
              				ExpandBuf(pBuf, pSrcLine+i+1, nLen, inPixelSize);
              			}
              			catch(...)
          		        {
          		   	        UMemory::Dispose((TPtr)pLineBuf);
       			            UMemory::Dispose((TPtr)pSrcLine);

          		            throw;
          		        }
              			
			            pBuf += nLen*4*PixelPerRLEUnit;
            		}
            		else
            		{
              			try
            			{
              				ExpandBuf8(pBuf, pSrcLine+i+1, nLen, inPixelSize);
              			}
              			catch(...)
          		        {
          		   	        UMemory::Dispose((TPtr)pLineBuf);
       			            UMemory::Dispose((TPtr)pSrcLine);

          		            throw;
          		        }
             			
		 	            pBuf += nLen*PixelPerRLEUnit;
            		}
            		
            		i += nLen * nPkpixSize + 1;
          		}
        	}
        
           	UMemory::Dispose((TPtr)pSrcLine);
           	
           	Uint8 *pDestLine = (Uint8*)mPixmap.data + mReadRows*mPixmap.rowBytes;
        	if(mPixmap.depth == 32)
          		UMemory::Copy(pDestLine, pLineBuf, 4*mPixmap.width);
        	else
          		UMemory::Copy(pDestLine, pLineBuf, mPixmap.width);
          		
  			mReadRows++;		
			nOffset = mOffset;
      	}
	}

	UMemory::Dispose((TPtr)pLineBuf);

	return true;
}

void CDecompressPict::ExpandBuf(Uint8 *pDestBuf, Uint8 *pSrcBuf, Uint16 Width, Uint16 bpp)
{
  	switch (bpp)
  	{
    	case 16:
			Expand16Bpp(pDestBuf, pSrcBuf, Width);
      		break;
      		
    	case 8:
      		Expand8Bpp(pDestBuf, pSrcBuf, Width);
      		break;
      		
    	case 4:
      		Expand4Bpp(pDestBuf, pSrcBuf, Width*2);
      		break;
      		
    	case 2:
      		Expand2Bpp(pDestBuf, pSrcBuf, Width*4);
      		break;
      		
    	case 1:
      		Expand1Bpp(pDestBuf, pSrcBuf, Width*8);
      		break;
      		
    	default:
			Fail(errorType_Image, imgError_UnknownFormat);
	};
}

void CDecompressPict::ExpandBuf8(Uint8 *pDestBuf, Uint8 *pSrcBuf, Uint16 Width, Uint16 bpp)
{
	Uint8 *pSrc  = pSrcBuf;
  	Uint8 *pDest = pDestBuf;

  	switch (bpp)
  	{
    	case 8:
      		UMemory::Copy(pDestBuf, pSrcBuf, Width);
      		break;
      		
    	case 4:
      		for(Uint16 i=0; i<Width; i++)
      		{
        		*pDest = (*pSrc >> 4) & 15;
        		*(pDest+1) = (*pSrc & 15);
        		pSrc++;
        		pDest += 2;
      		}
      
      		if (Width & 1)
      		{
        		*pDest = (*pSrc >> 4) & 15;
        		pDest++;
      		}
      		break;
      
    	case 2:
      		for(Uint16 i=0; i<Width; i++)
      		{
        		*pDest = (*pSrc >> 6) & 3;
        		*(pDest+1) = (*pSrc >> 4) & 3;
        		*(pDest+2) = (*pSrc >> 2) & 3;
        		*(pDest+3) = (*pSrc & 3);
        		pSrc++;
        		pDest += 4;
      		}
      
      		if (Width & 3)  
        		for(Uint16 i=6; i>8-(Width & 3)*2; i-=2)
        		{
          			*pDest = (*pSrc >> i) & 3;
          			pDest++;
        		}
      		
      		break;
      		
    	case 1:
      		for(Uint16 i=0; i<Width; i++)
      		{
        		*pDest = (*pSrc >> 7) & 1;
        		*(pDest+1) = (*pSrc >> 6) & 1;
        		*(pDest+2) = (*pSrc >> 5) & 1;
        		*(pDest+3) = (*pSrc >> 4) & 1;
        		*(pDest+4) = (*pSrc >> 3) & 1;
        		*(pDest+5) = (*pSrc >> 2) & 1;
        		*(pDest+6) = (*pSrc >> 1) & 1;
        		*(pDest+7) = *pSrc  & 1;
        		pSrc++;
        		pDest += 8;
      		}
      		
      		if (Width & 7)  
        		for(Uint16 i=7; i>(8-Width & 7); i--)
       			{
          			*pDest = (*pSrc >> i) & 1;
          			pDest++;
        		}
      		
      		break;
      		
    	default:
			Fail(errorType_Image, imgError_UnknownFormat);
	};
}

void CDecompressPict::Expand1Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width)
{
	for(Uint16 i=0; i<Width/8; i++)
  	{
    	*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> 7) & 1));
    	*((Uint32 *)(pDest+4)) = *(mPixmap.colorTab+((*pSrc >> 6) & 1));
   		*((Uint32 *)(pDest+8)) = *(mPixmap.colorTab+((*pSrc >> 5) & 1));
    	*((Uint32 *)(pDest+12)) = *(mPixmap.colorTab+((*pSrc >> 4) & 1));
    	*((Uint32 *)(pDest+16)) = *(mPixmap.colorTab+((*pSrc >> 3) & 1));
    	*((Uint32 *)(pDest+20)) = *(mPixmap.colorTab+((*pSrc >> 2) & 1));
    	*((Uint32 *)(pDest+24)) = *(mPixmap.colorTab+((*pSrc >> 1) & 1));
    	*((Uint32 *)(pDest+28)) = *(mPixmap.colorTab+(*pSrc & 1));
    	pSrc++;
    	pDest += 32;
  	}
  
  	if (Width & 7)  
    	for(Uint16 i=7; i > (8-Width & 7); i--)
    	{
      		*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> i) & 1));
      		pDest += 4;
    	}
}

void CDecompressPict::Expand2Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width)
{
  	for(Uint16 i=0; i<Width/4; i++)
  	{
    	*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> 6) & 3));
    	*((Uint32 *)(pDest+4)) = *(mPixmap.colorTab+((*pSrc >> 4) & 3));
    	*((Uint32 *)(pDest+8)) = *(mPixmap.colorTab+((*pSrc >> 2) & 3));
    	*((Uint32 *)(pDest+12)) = *(mPixmap.colorTab+(*pSrc & 3));
    	pSrc++;
    	pDest += 16;
  	}
  
  	if (Width & 3) 
    	for(Uint16 i=6; i > 8-(Width & 3)*2; i-=2)
   	 	{
      		*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> i) & 3));
      		pDest += 4;
    	}
}

void CDecompressPict::Expand4Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width)
{
  	for(Uint16 i=0; i<Width/2; i++)
  	{
    	*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> 4) & 15));
    	*((Uint32 *)(pDest+4)) = *(mPixmap.colorTab+(*pSrc & 15));
    	pSrc++;
    	pDest += 8;
  	}
  	
  	if (Width & 1)
  	{
    	*((Uint32 *)pDest) = *(mPixmap.colorTab+((*pSrc >> 4) & 15));
    	pDest += 4;
  	}
}

void CDecompressPict::Expand8Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width)
{
  	for(Uint16 i=0; i<Width; i++)
  	{
    	*((Uint32 *)pDest) = *(mPixmap.colorTab+*pSrc);
    	pSrc++;
    	pDest += 4;
  	}
}

void CDecompressPict::Expand16Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width)
{
	for (Uint16 i=0; i<Width; i++)
 	{
    	Uint16 Src = pSrc[1]+(pSrc[0]<<8);
    	*(pDest + RGBA_BLUE) = ((Src) & 31)*8;          // Blue
    	*(pDest + RGBA_GREEN) = ((Src >> 5) & 31)*8;    // Green
    	*(pDest + RGBA_RED) = ((Src  >> 10) & 31)*8;    // Red
    	*(pDest + RGBA_ALPHA) = 0xFF;                   // Alpha
    	pSrc += 2;
    	pDest += 4;
    }
}

					// Read function


bool CDecompressPict::ReadVersion()
{
	if(mOffset + 2 > mBytesInBuffer)
		return false;

	Uint8 ch1;
	ReadByte(ch1);
	
  	switch (ch1)        
  	{
    	case 1:	
    		mVersion = 1;
			break;
    	
   		case 2:
			Uint8 ch2;
  			ReadByte(ch2);
      				
      		if(ch2 != 0xFF)
				Fail(errorType_Image, imgError_IllegalVersionNumber);
      					
      		mVersion = 2;
			break;

    	default:
			Fail(errorType_Image, imgError_IllegalVersionNumber);
	};
		
	return true;
}

// moves to an even byte position in the file and returns the opcode found
bool CDecompressPict::ReadOpCode(Uint16& outOpCode)
{
  	if(mVersion == 1)
  	{
   		 Uint8 nOpCode;
   		 if(ReadByte(nOpCode))
   		 {
   		 outOpCode = nOpCode;
   		 	return true;
   		 }
   	} 
  	else if(mVersion == 2)
  	{
  	 	if(mOffset%2)
			mOffset++;

    	if(ReadWord(outOpCode))
    		return true;
    }
    
    return false;
}

bool CDecompressPict::ReadByte(Uint8& outByte)
{
	if(mOffset + 1 > mBytesInBuffer)
		return false;
	
	ReadImage(&outByte, 1);
	
	return true;
}

bool CDecompressPict::ReadWord(Uint16& outWord)
{
	if(mOffset + 2 > mBytesInBuffer)
		return false;
	
	ReadImage(&outWord, 2);

	#if CONVERT_INTS
	outWord = swap_int(outWord);
	#endif

	return true;
}

bool CDecompressPict::ReadDWord(Uint32& outDWord)
{
	if(mOffset + 4 > mBytesInBuffer)
		return false;
	
	ReadImage(&outDWord, 4);

	#if CONVERT_INTS
	outDWord = swap_int(outDWord);
	#endif

	return true;
}

bool CDecompressPict::ReadMacRect(SMacRect& outMacRect)
{
	if(mOffset + 8 > mBytesInBuffer)
		return false;
	
	ReadImage(&outMacRect.top, 2);
	ReadImage(&outMacRect.left, 2);
	ReadImage(&outMacRect.bottom, 2);
	ReadImage(&outMacRect.right, 2);

	#if CONVERT_INTS
	outMacRect.top = swap_int(outMacRect.top);
	outMacRect.left = swap_int(outMacRect.left);
	outMacRect.bottom = swap_int(outMacRect.bottom);
	outMacRect.right = swap_int(outMacRect.right);
	#endif
	
	return true;
}

bool CDecompressPict::ReadPixmap(SMacPixMap& outPixMap)
{
	if(mOffset + sizeof(SMacPixMap) > mBytesInBuffer)
		return false;

 	ReadMacRect(outPixMap.Bounds);
  	ReadWord(outPixMap.version);
  	ReadWord(outPixMap.packType);
  	ReadDWord(outPixMap.packSize);
  	ReadDWord(outPixMap.hRes);
  	ReadDWord(outPixMap.vRes);
  	ReadWord(outPixMap.pixelType);
  	ReadWord(outPixMap.pixelSize);
  	ReadWord(outPixMap.cmpCount);
  	ReadWord(outPixMap.cmpSize);
  	ReadDWord(outPixMap.planeBytes);
  	ReadDWord(outPixMap.pmTable);
  	ReadDWord(outPixMap.pmReserved);
  	
  	return true;
}

// Reads a mac colour table into a bitmap palette
Uint32 *CDecompressPict::ReadColourTable(Uint16& outNumColors)
{
  	Uint32 ctSeed;
  	if(!ReadDWord(ctSeed))
  		return nil;
  
  	Uint16 ctFlags;
 	if(!ReadWord(ctFlags))
 		return nil;
  
  	if(!ReadWord(outNumColors))
  		return nil;
  		
	outNumColors += 1;

	Uint32 *pColTable;
	try
	{
		pColTable = (Uint32*)UMemory::NewClear(sizeof(Uint32)*outNumColors);
	}
	catch(...)
  	{
		Fail(errorType_Image, imgError_InsufficientMemory);
	}

	for(Uint16 i=0; i<outNumColors; i++)
  	{
    	Uint16 nVal;
    	if(!ReadWord(nVal))
    	{
    		UMemory::Dispose((TPtr)pColTable);
    		return nil;
    	}
    	
    	if(ctFlags & 0x8000)
      		nVal = i;
      
    	if(nVal >= outNumColors)
    	{
      		UMemory::Dispose((TPtr)pColTable);
			Fail(errorType_Image, imgError_InvalidColorTable);
    	}

	    // Mac colour tables contain 16-bit values for R, G, and B...
    	Uint16 nColor;
    	if(!ReadWord(nColor))
    	{
    		UMemory::Dispose((TPtr)pColTable);
    		return nil;
    	}

    	*(((Uint8*)(pColTable+nVal))+RGBA_RED) = nColor >> 8;
	    
    	if(!ReadWord(nColor))
    	{
    		UMemory::Dispose((TPtr)pColTable);
    		return nil;
    	}
	   
	    *(((Uint8*)(pColTable+nVal))+RGBA_GREEN) = nColor >> 8;
    	
    	if(!ReadWord(nColor))
    	{
    		UMemory::Dispose((TPtr)pColTable);
    		return nil;
    	}
    	
    	*(((Uint8*)(pColTable+nVal))+RGBA_BLUE) = nColor >> 8;
	    
	    *(((Uint8*)(pColTable+nVal))+RGBA_ALPHA) = 0x00;
	}

  	return pColTable;
}


