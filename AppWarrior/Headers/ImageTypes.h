/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once

#define END_OF_FB 	    0xFFFFFFFF
#define END_OF_BYTES    0xFFFFFFFE


/*
 * Image Error Codes
 */

enum {
	errorType_Image						= 10,
	imgError_Unknown					= 100,
	imgError_InsufficientMemory			= imgError_Unknown + 1,
	imgError_UnsupportedFormat			= imgError_Unknown + 2,
	imgError_UnknownFormat				= imgError_Unknown + 3,
	imgError_InvalidFileStructure		= imgError_Unknown + 4,
	imgError_UnexpectedEnd				= imgError_Unknown + 5,
	imgError_ReadingError				= imgError_Unknown + 6,
	imgError_WritingError				= imgError_Unknown + 7,
	imgError_InvalidPixmap				= imgError_Unknown + 8,
	imgError_NotGifFile					= imgError_Unknown + 9,
	imgError_BadSymbolSize				= imgError_Unknown + 10,
	imgError_NotBitmapFile				= imgError_Unknown + 11,
	imgError_VersionNumberMissing		= imgError_Unknown + 12,
	imgError_IllegalVersionNumber		= imgError_Unknown + 13,
	imgError_VectorData					= imgError_Unknown + 14,
	imgError_QuicktimeData				= imgError_Unknown + 15,
	imgError_CorruptJpegImage			= imgError_Unknown + 16,
	imgError_UnknownPatternType			= imgError_Unknown + 17,
	imgError_InvalidColorMap			= imgError_Unknown + 18,
	imgError_InvalidColorTable			= imgError_Unknown + 19,
	imgError_ColorFoundTwice			= imgError_Unknown + 20
};


// ------------------------------GIF---------------------------------------------------------- //

#pragma options align=packed
typedef struct {
	Uint16 width, depth, bits;
	Uint8 flags;
	Uint16 background;
	Uint8 palette[768];
	} GifInfo;
#pragma options align=reset

#pragma options align=packed
typedef struct {
	Int8 sig[6];
	Uint16 screenwidth,screendepth;
	Uint8 flags,background,aspect;
	} GIFHEADER;
#pragma options align=reset

#pragma options align=packed
typedef struct {
	Uint16 left,top,width,depth;
	Uint8 flags;
	} IMAGEBLOCK;
#pragma options align=reset

#pragma options align=packed
typedef struct {
	Uint8 blocksize;
	Uint8 flags;
	Uint16 delay;
	Uint8 transparent_colour;
	Uint8 terminator;
	} CONTROLBLOCK;
#pragma options align=reset

#pragma options align=packed
typedef struct {
	Uint8 blocksize;
	Uint16 left,top;
	Uint16 gridwidth,gridheight;
	Uint8 cellwidth,cellheight;
	Int8 forecolour,backcolour;
	} PLAINTEXT;
#pragma options align=reset

#pragma options align=packed
typedef struct {
	Uint8 blocksize;
	Int8 applstring[8];
	Int8 authentication[3];
	} APPLICATION;
#pragma options align=reset

#pragma options align=packed
typedef struct{
	TImage image;
	TImage mask;
	IMAGEBLOCK imgBlock;
	CONTROLBLOCK ctrBlock;
} SImageInfo;
#pragma options align=reset


// ------------------------------BITMAP---------------------------------------------------------- //

#pragma options align=packed
typedef struct tagBITMAPFILEHEAD { 
	Uint16 bfType; 
    Uint32 bfSize; 
    Uint16 bfReserved1; 
    Uint16 bfReserved2; 
    Uint32 bfOffBits; 
} BITMAPFILEHEAD; 
#pragma options align=reset

#pragma options align=packed
typedef struct tagBITMAPINFOHEAD{ 
   Uint32 biSize; 
   Uint32 biWidth; 
   Uint32 biHeight; 
   Uint16 biPlanes; 
   Uint16 biBitCount;
   Uint32 biCompression; 
   Uint32 biSizeImage; 
   Uint32 biXPelsPerMeter; 
   Uint32 biYPelsPerMeter; 
   Uint32 biClrUsed; 
   Uint32 biClrImportant; 
} BITMAPINFOHEAD; 
#pragma options align=reset

#pragma options align=packed
typedef struct tagBITMAPRGB { 
    Uint8 rgbBlue; 
    Uint8 rgbGreen; 
    Uint8 rgbRed; 
    Uint8 rgbReserved; 
} BITMAPRGB; 
#pragma options align=reset

