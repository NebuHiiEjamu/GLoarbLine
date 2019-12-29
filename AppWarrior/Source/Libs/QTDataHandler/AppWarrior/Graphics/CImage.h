// =====================================================================
//	CImage.h                             (C) Hotline Communications 1999
// =====================================================================
// The offscreen buffer and image holder
//
// There are two constructors for this class; one which is public, and
// immediately allocates backing store, and another which is protected,
// and does not allocate backing store.  The actual function which
// allocates backing store is Create, which is called by the public
// constructor.  The point of the constructor which does not allocate
// backing store is that derivatives which load an image from a file 
// will not know the size of the backing store needed until after some 
// processing on the file; hence when they know the size of the image
// these derivatives can call Create.  Presumably, all this happens
// in the constructor of a derivative; all the members of this class
// assume that the constructor allocates backing store before returning.
//

#ifndef _H_CImage_
#define _H_CImage_

#if PRAGMA_ONCE
	#pragma once
#endif			  

#include "CPoint.h"
#include "CSize.h"
#include "CRect.h"
#include "CBroadcaster.h"
#include "CMessage.h"
#include "TProxy.h"

HL_Begin_Namespace_BigRedH

class CGraphicsPort;
class CImage : public CBroadcaster
{
	public:
		enum EImageDepth {
			eMillions
			,eMillionsAlpha
			,eThousands
			,eThousandsAlpha
			,eByteAlphaMap
		};
	
						CImage (
							CSize inSize
							,EImageDepth inBPP = eMillions);
							// throws CGraphicsException
									
	protected:
						CImage();
							// throws CGraphicsException
		void			Create (
							CSize inSize
							,EImageDepth inBPP);
							// throws CGraphicsException
	public:					
		virtual			~CImage();
							// throws nothing
								
			// ** image properties **
		virtual bool	IsValid() const;	
		CSize			GetSize() const;
		EImageDepth		GetColorDepth() const;
		virtual bool	HasAlpha() const
							{ return ((GetColorDepth() == eMillionsAlpha)
									|| (GetColorDepth() == eThousandsAlpha));
							}
											
		virtual TProxy<CGraphicsPort>
						GetGraphicsPort();
		
			// ** message sent when the image changes **
		enum			NotifyID
						{
							eChanged = 'imgc' 
						};
										
		typedef TObjectMessageS<CRect,CImage&>	
						ImageChanged;
						
						// helper function for derived classes		
		void			NotifyChanged(
							const CRect& inBounds);
		void 			NotifyImageComplete();
		
	protected:
		class			Imp;
		Imp*			mImp;
	
	friend class CGraphicsPort;
}; // class CImage

HL_End_Namespace_BigRedH
#endif // _H_CImage_
