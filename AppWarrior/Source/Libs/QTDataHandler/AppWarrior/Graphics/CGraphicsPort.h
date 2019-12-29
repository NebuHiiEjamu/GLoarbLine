// =====================================================================
//	CGraphicsPort.h                      (C) Hotline Communications 1999
// =====================================================================
//
// Manages graphics object where all drawing is done.


#ifndef _H_CGraphicsPort_
#define _H_CGraphicsPort_

#if PRAGMA_ONCE
	#pragma once
#endif			  

#ifndef _H_CGraphicsException_
#include "CGraphicsException.h"
#endif

#include "CPoint.h"
#include "CRegion.h"
#include "CRect.h"
#include "CPen.h"
#include "CBrush.h"
#include "CFont.h"
#include "CImage.h"
#include "CColor.h"
#include "CString.h"

HL_Begin_Namespace_BigRedH

// A graphics port; be it a port on MacOS, a DC on Windows, this object is a metaphor
// for a place in which we can draw.

class CGraphicsPort
{
	public:
		virtual	 			~CGraphicsPort();
								// throws nothing
							
								// Is the graphicsport drawing to an offscreen image?	
		virtual bool		IsOffscreen() const = 0;
								// throws nothing

			// ** Surface **
								// Get the Local Origin in Surface space
		CPoint				GetOrigin() const;
								// throws nothing
							
								// Set the Local Origin in Surface space
		void				SetOrigin( const CPoint &inPoint );
								// throws nothing
								
								// Move the Local Origin in Surface space
		void				OffsetOriginBy( const CPoint &inPoint );
								// throws nothing

								// Transform from local to surface coordiantes
		CPoint				Transform(const CPoint& inPoint) const;
								// throws nothing
								
		CRect				Transform(const CRect& inRect) const;
								// throws nothing
								
								// Transform from surface to local
		CPoint				InvertTransform(const CPoint& inPoint) const;
								// throws nothing
								
		CRect				InvertTransform(const CRect& inRect) const;
								// throws nothing
								
								// SetClipRect only functions on offscreens!
		virtual void		SetClipRect( const CRect& inRect ) = 0;
								// throws nothing
								
								// Get the bounding rect of the clip region
		virtual CRect		GetClipRect() const = 0;
								// throws nothing
							
			// ** Operations Acceptable on Onscreen Ports ** 

								// Simple blast (with alpha, if inSrcImage has it)
		virtual void 		DrawImage( CImage& inSrcImage
									, const CRect& inSourceRect
									, const CRect& inDestRect ) = 0;
								// throws CGraphicsException
								
								// Tile the blasting of an image
		void				TileDrawImage( CImage& inSrcImage
									, const CRect& inDestRect );
								// throws CGraphicsException
						
						
								// Blast an image, with the specified alpha mask.
								// Pass inAlphaMask = 0, 
								// if you don't want alpha effects.
								// NOTE: Using inAlphaMask!=0 
								// is not acceptable for onscreens!
		virtual void 		AlphaMaskImage ( CImage& inSrcImage
									, const CRect& inSourceRect
									, const CRect& inDestRect
									, CImage* inAlphaMask ) = 0;	
								// throws CGraphicsException
									
								// Tile the alpha masking of an image
		void				TileAlphaMaskImage (CImage& inSrcImage
									, const CRect& inDestRect
									, CImage* inAlphaMask);
								// throws CGraphicsException
												
				// ** Operations only Acceptable on Offscreen Ports **

			// ** Pixels **
		virtual CColor		GetPixel(
								const CPoint& inPixel) const = 0;
								// throws nothing
								
		virtual void		SetPixel(
								const CPoint& inPixel
								,const CColor& inColor) = 0;
								// throws nothing
								
								// Flood-fill the entire port
		virtual void		SolidFill(const CColor& inColor) = 0;
								// throws CGraphicsException
								
			// ** Geometric Primitives **
		virtual void		DrawLine (const CPoint& inStart
									, const CPoint& inEnd
									, const CColor& inColor
									, int inThickness = 0) = 0;
								// throws CGraphicsException
								
		void 				DrawRect (const CRect& inRect
									, const CColor& inColor
									, int inThickness = 0);
								// throws CGraphicsException
								
		virtual void 		FillRect (const CRect& inRect
									, const CColor& inColor) = 0;
								// throws CGraphicsException
									
		virtual void		DrawOval (const CRect& inBounds
									, const CColor& inColor
									, int inThickness = 0) = 0;
								// throws CGraphicsException
								
		virtual void 		FillOval (const CRect& inBounds
									, const CColor& inColor) = 0;
								// throws CGraphicsException
								
		virtual void		DrawText ( const CString &inText
									 , const CFont& inFont
								     , const CPoint& inWhere) = 0;
								// throws CGraphicsException
											
		enum EPointMode {
			ePolyLine
			,ePolygon
			,ePolygonFill
			,eBezierDraft
			,eBezierDetail
		};
								// Draw the series of points in the specified mode.
		void				DrawPoints (int inPointCount
									, const CPoint* inPoints
									, const CColor& inColor
									, EPointMode inMode
									, int inThickness = 0);


								// Get the inline alpha into an alpha mask image					
		virtual CImage*		GetAlphaMask () = 0;


								// Blast an image, with the specified alpha value.
								// Ignores any alpha mask in the image.
		virtual void		AlphaDrawImage (CImage& inSrcImage
											, const CRect& inSourceRect
											, const CRect& inDestRect
											, ColorChannel inAlpha ) = 0;
											
								// Blast an image, with the specified color transparent
								// Ignores any alpha channel info on source image
		virtual void		ColorKeyedDrawImage (CImage& inSrcImage
											, const CRect& inSourceRect
											, const CRect& inDestRect
											, const CColor& inKeyColor ) = 0;

								// Tile the alpha drawing of an image										
		void				TileAlphaDrawImage (CImage& inSrcImage
											, const CRect& inDestRect
											, ColorChannel inAlpha);

								// Tile the color keyed drawing of an image
		void				TileColorKeyedDrawImage (CImage& inSrcImage
											, const CRect& inDestRect
											, const CColor& inKeyColor);		
		
			// ** Helpers & Pass-throughs **								

		// Convenience function for drawing an image with different common transforms
		enum EApplyImageOp {
			eCenter
			,eTile
			,eScale
		};										
		void				ApplyImage (CImage& inSrcImage
									, const CRect& inDestRect
									, EApplyImageOp inDrawOp);

		void				DrawImage( CImage& inSrcImage
										, const CRect& inSourceRect
										, const CPoint& inDestWhere );
		void				DrawImage( CImage& inSrcImage
										, const CRect& inDestRect );				
		void				DrawImage( CImage& inSrcImage
										, const CPoint& inDestWhere );
		void		 		AlphaMaskImage (CImage& inSrcImage
											, const CRect& inSourceRect
											, const CPoint& inDestWhere
											, CImage* inAlphaMask );
		void				AlphaMaskImage (CImage& inSrcImage
											, const CRect& inDestRect
											, CImage* inAlphaMask );
		void				AlphaMaskImage (CImage& inSrcImage
											, const CPoint& inDestWhere
											, CImage* inAlphaMask );							
		void				AlphaDrawImage (CImage& inSrcImage
											, const CRect& inSourceRect
											, const CPoint& inDestWhere
											, ColorChannel inAlpha );
		void				AlphaDrawImage (CImage& inSrcImage
											, const CRect& inDestRect
											, ColorChannel inAlpha );					
		void				AlphaDrawImage (CImage& inSrcImage
											, const CPoint& inDestWhere
											, ColorChannel inAlpha );								
		void				ColorKeyedDrawImage (CImage& inSrcImage
											, const CRect& inSourceRect
											, const CPoint& inDestWhere
											, const CColor& inKeyColor );
		void				ColorKeyedDrawImage (CImage& inSrcImage
											, const CRect& inDestRect
											, const CColor& inKeyColor );										
		void				ColorKeyedDrawImage (CImage& inSrcImage
											, const CPoint& inDestWhere
											, const CColor& inKeyColor );								
									
				// ** Pen **
		virtual CPen		GetPen() const = 0;
		virtual void		SetPen( const CPen &inPen ) = 0;

			// ** Brush **
		virtual CBrush		GetBrush() const = 0;
		virtual void		SetBrush( const CBrush &inBrush ) = 0;

										
	protected:	
								// Protected ctor - Derivatives only!
							CGraphicsPort();
							
		
								// Override to fill polygons
								// Called by DrawPoints
		virtual void		FillPolygon (int inPointCount
										, const CPoint* inPoints
										, const CColor& inColor) = 0;	
		
								// Helper to bit blasting functions
		static CImage*		ClipImageBlast(	CImage& inSrcImage
											, CRect& ioSrcRect
											, CRect& ioDestRect
											, const CRect& inClipRect);

								// Helpers to thick primitives;
								// Computes offset points for lines
		CPoint 				ComputeNormal (const CPoint& inA
										, const CPoint& inB
										, double inLength );
										
	private:
								// Draw a bezier curve
		void				DrawBezier (int inPointCount
									, const CPoint* inPoints
									, const CColor& inColor
									, int inSteps
									, int inThickness);
									
		CPoint				ComputeNormal (const CPoint& inA
										, const CPoint& inB
										, const CPoint& inC
										, double inLength );
	
		CPoint				EvaluateBezier (const CPoint& inA
									, const CPoint& inB
									, const CPoint& inC
									, const CPoint& inD
									, double inT);

								// The origin for the port
								// All origin manipulation is software,
								// NOT OS stuff.
		CPoint				mOrigin;
};

/*
#if 0
	#pragma mark StPortRegion
#endif
// =====================================================================
//  StPortRegion
// =====================================================================
//

class StPortRegion
{
public:
					StPortRegion( CGraphicsPort& inPort );
					StPortRegion( CGraphicsPort& inPort,
						const CRect& inRect );
					~StPortRegion();

private:
	CGraphicsPort	&mSavePort;
	CPoint			mSaveOrigin;
	CRegion			mSaveClipping;
};


#if 0
	#pragma mark StPortPen
#endif
// =====================================================================
//  StPortPen
// =====================================================================
//

class StPortPen
{
public:
					StPortPen( CGraphicsPort& inPort );
					StPortPen( CGraphicsPort& inPort,
						const CPen& inPen );
					~StPortPen();

private:
	CGraphicsPort	&mSavePort;
	CPen			mSavePen;
};
*/

HL_End_Namespace_BigRedH
#endif	// _H_CGraphicsPort_
