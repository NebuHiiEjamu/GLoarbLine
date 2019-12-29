/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "CDecompressJpeg.h"


#define RGBA_RED    0
#define RGBA_GREEN  1
#define RGBA_BLUE   2
#define RGBA_ALPHA  3

#pragma options align=packed
typedef struct {
	Uint16 top;
	Uint16 left;
	Uint16 bottom;
	Uint16 right;
} SMacRect;
#pragma options align=reset

#pragma options align=packed
typedef struct {
  	SMacRect Bounds;
	Uint16 version;
	Uint16 packType;
	Uint32 packSize;
	Uint32 hRes;
	Uint32 vRes;
	Uint16 pixelType;
	Uint16 pixelSize;
	Uint16 cmpCount;
	Uint16 cmpSize;
	Uint32 planeBytes;
	Uint32 pmTable;
	Uint32 pmReserved;
} SMacPixMap;
#pragma options align=reset


// for reserved opcodes
#define res(length) { "reserved", (length), "reserved for Apple use" }

#define RGB_LEN     6
#define WORD_LEN   -1
#define NA          0

struct OpDef
{
	char *name;
	int   len;
	char *description;
};


class CDecompressPict : public CDecompressImage
{
	public:
		// constructor
		CDecompressPict(Uint32 inSize, bool inSilentFail = false);
		virtual ~CDecompressPict();
		
		virtual bool Decompress(const void *inData, Uint32 inDataSize);

	protected:
		Uint8 mStage;
		bool mReadOpCode;
	 	
  		Uint8 mVersion;        // PICT version number: 1 or 2
	  	Uint16 mOpCode;
  		Uint16 mRowBytes;      // Bytes per row in source when uncompressed
		Uint16 mPixelSizeOrCmpCount;
		bool mAlphaChannel;    // alpha channel only if 32 bpp

		CDecompressJpeg *mDecompressJpeg;
		Uint32 mJpegBegin;
		Uint32 mJpegReadRows;
		bool mJpegFirstImage;
		bool mJpegFirstSize;
		
		bool ReadPictImage(bool inFileBuffer, TFSRefObj* inPictFile=nil, void *inPictBuffer=nil, Uint32 inBufferSize=0);

		bool Clip();
		bool PixPat();
		bool SkipBits(SMacRect inBounds, Uint16 inRowBytes, Uint16 inPixelSize);
		bool SkipPolyOrRegion();
		bool BitsRect();
		bool BitsRegion();
		bool DoBitmap(bool inIsRegion);
		bool DoPixmap(bool inIsRegion);
		bool OpCode9A();
		bool DoOpCode9A();
		bool OpCode9B();
		bool DoOpCode9B();
		bool LongComment();

		bool InitJpeg(bool& outFound);
		bool SearchJpeg(bool& outFound);
		bool ReadJpeg();

		void CalcDestBPP(Uint16 SrcBPP);
		void InitPixmap(SMacPixMap inPixMap);
		void InitPixmap(Uint16 inWidth, Uint16 inHeight, bool inAlphaChannel);

		bool Unpack32Bits(Uint16 inRowBytes, int inNumBitPlanes);
		bool Unpack8Bits(Uint16 inRowBytes);
		bool UnpackBits(Uint16 inRowBytes, int inPixelSize);

		void ExpandBuf(Uint8 *pDestBuf, Uint8 *pSrcBuf, Uint16 Width, Uint16 bpp);
		void ExpandBuf8(Uint8 *pDestBuf, Uint8 *pSrcBuf, Uint16 Width, Uint16 bpp);
		void Expand1Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width);
		void Expand2Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width);
		void Expand4Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width);
		void Expand8Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width);
		void Expand16Bpp(Uint8 *pDest, Uint8 *pSrc, Uint16 Width);

		bool ReadPictImage();
		bool ReadVersion();
		bool ReadOpCode(Uint16& outOpCode);
		bool ReadByte(Uint8& outByte);
		bool ReadWord(Uint16& outWord);
		bool ReadDWord(Uint32& outDWord);
		bool ReadMacRect(SMacRect& outMacRect);
		bool ReadPixmap(SMacPixMap& outPixMap);
		Uint32 *ReadColourTable(Uint16& outNumColors);
};

