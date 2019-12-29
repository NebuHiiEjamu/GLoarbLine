//======================================================================
//	CRegionPS_W.h                        (C) Hotline Communications 1999
//======================================================================
//
// Data holder for Win32 specific regions

#ifndef _H_CRegionPS_W_
#define _H_CRegionPS_W_

#if PRAGMA_ONCE
	#pragma once
#endif
				    
#include "CModuleWin.h"
#include "CRegion.h"


HL_Begin_Namespace_BigRedH

class CRegion::CRegionPS
{
public:
						CRegionPS( const CRect& inRect );
						~CRegionPS();

						// Return a copy of the object
	CRegionPS*			Clone();
						// Return a copy of the object
			    		operator HRGN() const
			    			{ return mHandle; }

		// ** Region Style Handling **
	      				// WIN32 styles = NULLREGION, SIMPLEREGION,
	      				//				COMPLEXREGION, ERROR
	int					GetStyle()	const
							{ return mStyle; }
	void				SetStyle(int inStyle)
							{ mStyle = inStyle; } 

private:
			    		CRegionPS( HRGN inHandle, int inStyle ){}

	HRGN  				mHandle;
	int   				mStyle;
};

HL_End_Namespace_BigRedH
#endif	// _H_CRegionPS_W_
