// =====================================================================
//	CSize.h                              (C) Hotline Communications 1999
// =====================================================================
//
// Manages size info


#ifndef _H_CSize_
#define _H_CSize_

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH

class CSize
{
	public:
							CSize( )
								: mX(0), mY(0) 
								{}
							CSize( SInt32 inX, SInt32 inY )
								: mX(inX), mY(inY) 
								{}

			// ** Accessors **
		SInt32				GetX() const
								{ return mX; }
		SInt32				GetH() const
								{ return mX; }
		SInt32				GetWidth() const
								{ return mX; }
		void				SetX( SInt32 inX )
								{ mX = inX; }
		void				SetH( SInt32 inX )
								{ mX = inX; }
		void				SetWidth( SInt32 inX )
								{ mX = inX; }
		SInt32				GetY() const
								{ return mY; }
		SInt32				GetV() const
								{ return mY; }
		SInt32				GetHeight() const
								{ return mY; }
		void				SetY( SInt32 inY )
								{ mY = inY; }
		void				SetV( SInt32 inY )
								{ mY = inY; }
		void				SetHeight( SInt32 inY )
								{ mY = inY; }

			// ** Operators **
		CSize&				operator+=( const CSize &inRHS )
								{ mX += inRHS.mX; mY += inRHS.mY; return *this; }
		CSize&				operator+=( SInt32 inRHS )
								{ mX += inRHS; mY += inRHS; return *this; }
		CSize&				operator-=( const CSize &inRHS )
								{ mX -= inRHS.mX; mY -= inRHS.mY; return *this; }
		CSize&				operator-=( SInt32 inRHS )
								{ mX -= inRHS; mY -= inRHS; return *this; }
								
		bool				operator==( const CSize& other ) const
								{ return ((mX == other.mX) && (mY == other.mY)); }
		bool				operator!=( const CSize& other ) const
								{ return !((*this) == other); }
	private:
		SInt32				mX, mY;
}; // class CSize

HL_End_Namespace_BigRedH
#endif	// _H_CSize_
