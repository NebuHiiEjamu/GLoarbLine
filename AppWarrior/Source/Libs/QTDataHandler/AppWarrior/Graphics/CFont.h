// =====================================================================
//	CFont.h                              (C) Hotline Communications 1999
// =====================================================================
//
// Manages font info for drawing text

#ifndef _H_CFont_
#define _H_CFont_

#if PRAGMA_ONCE
	#pragma once
#endif			   

#include "CColor.h"
#include "CString.h"
#include "TProxy.h"
#include "CRect.h"

HL_Begin_Namespace_BigRedH

// Font measurement structure
class CFontMetrics {
	public:
		SInt32			GetAscent() const 
							{ return mAscent; }
		SInt32 			GetDescent() const 
							{ return mDescent; }
		SInt32 			GetMaxCharacterWidth() const
							{ return mMaxCharacterWidth; }
		SInt32 			GetLeading() const
							{ return mLeading; }

	private:
						CFontMetrics() 
							{}
	
		SInt32 mAscent;
		SInt32 mDescent;
		SInt32 mMaxCharacterWidth;
		SInt32 mLeading;
		
		friend class CFontMac;
		friend class CFontWin;
};

// Platform-independant base for font functions
class CGraphicsPort;
class CFont
{
	public:
		virtual 		~CFont() {}
		virtual CFont*	Clone() const = 0;	

		virtual CRect	MeasureText( const CString& inText
									,const TProxy<CGraphicsPort>& inPort) const = 0;

		virtual const CFontMetrics&
						GetMetrics() const = 0;
						
						
		const CColor& 	GetColor() const { return mColor; }
		virtual void	SetColor(const CColor& inColor) = 0;

		int				GetSize() const { return mSize; }
		virtual void	SetSize(int inSize) = 0;
		
		bool			IsItalic() const { return mItalic; }
		bool			IsBold() const 	{ return mBold; }
		bool			IsUnderline() const { return mUnderline; }
		virtual void	SetItalic(bool inState) = 0;
		virtual void	SetBold(bool inState) = 0;
		virtual void	SetUnderline(bool inState) = 0;
		
		// Font face details are fairly complex.
		
		static const CFont&	
						GetSystemFont() 
							{ return *sSystemFont; }
		
	protected:
						CFont() 
							{}
						CFont(const CFont&);

		CColor			mColor;
		int 			mSize;
		bool			mItalic;
		bool			mBold;
		bool			mUnderline;
		
		static CFont* 	sSystemFont;
		
	private:
		CFont&			operator= (const CFont&);
};

HL_End_Namespace_BigRedH
#endif	// _H_CFont_
