// =====================================================================
//	CColor.h               		         (C) Hotline Communications 1999
// =====================================================================
//
// Platform-independant definition of a color.

#ifndef _H_CColor_
#define _H_CColor_

#if PRAGMA_ONCE
	#pragma once
#endif

HL_Begin_Namespace_BigRedH

// 8 bit channels for color; will convert to whatever colorspace is appropriate.
// Note that this is a typedef on purpose!  For printing support, we may go to 16 bpc later!
typedef unsigned char ColorChannel;

// Definition of a color in a platform-independant way.
class CColor {
	public:
		inline			CColor ();
		inline			CColor (
							ColorChannel inR
							,ColorChannel inG
							,ColorChannel inB
							,ColorChannel inA = 0xFF);
	
		inline bool		operator== (
							const CColor& other) const;
	
		inline bool		operator!= (
							const CColor& other) const;
						
		ColorChannel	GetRed() const
							{ return mRed; }
		ColorChannel	GetGreen() const
							{ return mGreen; }
		ColorChannel	GetBlue() const
							{ return mBlue; }
		ColorChannel	GetAlpha() const
							{ return mAlpha; }

		void			SetRed(ColorChannel inR)
							{ mRed = inR; }
		void			SetGreen(ColorChannel inG)
							{ mGreen = inG; }
		void			SetBlue (ColorChannel inB)
							{ mBlue = inB; }
		void			SetAlpha (ColorChannel inA)
							{ mAlpha = inA; }
		
						// Blend when inAlpha + inInvAlpha == 0xFF
		inline CColor	Blend255 (
							const CColor& inColor
							, unsigned char inAlpha
							, unsigned char inInvAlpha) const;
							
		
		// A few useful color constants defined here
		static const CColor White;	
		static const CColor Black;
		static const CColor Red;
		static const CColor Green;
		static const CColor Blue;
		static const CColor Cyan;

private:
		ColorChannel 	mRed;
		ColorChannel	mGreen;
		ColorChannel	mBlue;
		ColorChannel 	mAlpha;
		
		// Helpers
		static inline unsigned long
						Div255 (unsigned long k);
						
		static inline unsigned char
						BlendChannel255 (
							unsigned char inA
							, unsigned char inB
							, unsigned char inAlpha
							, unsigned char inInvAlpha);
};

// These puppies are inline so temporaries can be optimized out (in theory)
// I don't know if the compiler actually does it, but it should 
// at least for return value optimization.
// Short reason: inline because it's faster. ;)

// ---------------------------------------------------------------------
//  CColor	  	                                        [inline][public]
// ---------------------------------------------------------------------
// 
// Default ctor.
inline
CColor::CColor ()
: mRed(0), mGreen(0), mBlue(0), mAlpha(0xFF)
{}

// ---------------------------------------------------------------------
//  CColor	  	                                        [inline][public]
// ---------------------------------------------------------------------
// 
// Construct a color with given channel values.
inline
CColor::CColor (ColorChannel inR,ColorChannel inG,ColorChannel inB,ColorChannel inA)
: mRed(inR), mGreen(inG), mBlue(inB), mAlpha(inA)
{}

// ---------------------------------------------------------------------
//  operator== 	                                        [inline][public]
// ---------------------------------------------------------------------
// 
// Compare two colors for equality.

inline bool
CColor::operator== (const CColor& inOther) const
{
	return ((mRed == inOther.mRed)
			&& (mGreen == inOther.mGreen)
			&& (mBlue == inOther.mBlue));
}


// ---------------------------------------------------------------------
//  operator!= 	                                        [inline][public]
// ---------------------------------------------------------------------
// 
// Compare two colors for equality.

inline bool
CColor::operator!= (const CColor& inOther) const
{
	return !(*this == inOther);
}

// ---------------------------------------------------------------------
// Div255			                           [private][static][inline]
// ---------------------------------------------------------------------
// This computes k/255, using two adds and two shifts
// Using 129 for the 'magic number' does proper rounding for alpha blends,
// while using 1 for the number results in a "true" k/255
inline unsigned long
CColor::Div255 (unsigned long k)
{	
	return ((129 + k + (k >> 8)) >> 8);
}

// ---------------------------------------------------------------------
// BlendChannel255	                           [private][static][inline]
// ---------------------------------------------------------------------
// Perform blending of two input (A and B), with specified alpha.
// inAlpha + inInvAlpha == 0xFF, otherwise everything goes bad.
// Essentially and 8-bit channel alpha blend, and the core of all
// our alpha functions.

inline unsigned char 
CColor::BlendChannel255 ( unsigned char inA
					,unsigned char inB
					,unsigned char inAlpha
					,unsigned char inInvAlpha)
{	
	unsigned long tempChannel = static_cast<unsigned long>(inA) * inAlpha
							+ static_cast<unsigned long>(inB) * inInvAlpha;
	// Handle overflow correctly (having overflow cause black pixels is ugly)
	if (tempChannel >= 255UL * 255) return 255;
	else return static_cast<unsigned char>(Div255(tempChannel));
}

// ---------------------------------------------------------------------
// Blend255		                                		[inline][public]
// ---------------------------------------------------------------------

inline CColor
CColor::Blend255 (const CColor& inColor, unsigned char inAlpha, unsigned char inInvAlpha) const
{
	return CColor ( BlendChannel255(mRed, inColor.GetRed(), inAlpha, inInvAlpha)
					,BlendChannel255(mGreen, inColor.GetGreen(), inAlpha, inInvAlpha)
					,BlendChannel255(mBlue, inColor.GetBlue(), inAlpha, inInvAlpha)
					,0xFF);
}

HL_End_Namespace_BigRedH

#endif //_H_CColor_