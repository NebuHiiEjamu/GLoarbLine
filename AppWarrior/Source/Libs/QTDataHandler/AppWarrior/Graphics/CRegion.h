// =====================================================================
//	CRegion.h                            (C) Hotline Communications 1999
// =====================================================================
//
// Manages region (non-regular) information for the graphics objects

#ifndef _H_CRegion_
#define _H_CRegion_

#if PRAGMA_ONCE
	#pragma once
#endif

#include "CPoint.h"
#include "CRect.h"

		    
HL_Begin_Namespace_BigRedH

class CRegion
{
	public:
						CRegion();
						CRegion( const CRegion &inOrig );
						CRegion( const CRect &inRect );
			 			~CRegion();

		bool			IsEmpty() const;
		CRect			GetBounds() const;
		void			OffsetBy( const CPoint &inPoint );
		void			ResetOrigin();

			// ** Containment **
		bool			ContainsPoint( const CPoint &inPoint ) const;
		bool			Intersects( const CRect &inRect ) const;
		bool			Intersects( const CRegion& inRegion ) const;

			// ** Operations **
		void			Union( const CRegion& inRegion );			// OR
		void			Difference( const CRegion& inRegion );		// DIFF
		void			Intersection( const CRegion& inRegion );	// AND
		void			ExclusiveOR( const CRegion& inRegion );		// XOR
		
			// ** Platform Specific Storage **
		class CRegionPS;
		CRegionPS*		GetPlatform() const
							{ return mPlatform; }


	private:
		CRegionPS			*mPlatform;
};

HL_End_Namespace_BigRedH
#endif	// _H_CRegion_
