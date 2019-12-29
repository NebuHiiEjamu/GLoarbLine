#ifndef _H_CGraphicsScopeWin_
#define _H_CGraphicsScopeWin_

#include "UGraphicsWin.h"

HL_Begin_Namespace_BigRedH

class StGDIObjWin {
public:
	StGDIObjWin (HGDIOBJ inObj)
		: mObj(inObj)
	{}
	~StGDIObjWin ()
	{
		::DeleteObject(mObj);
	}
	
private:
	StGDIObjWin (const StGDIObjWin&);
	StGDIObjWin& operator= (const StGDIObjWin&);
	
	HGDIOBJ mObj;
};

class StGDISelectWin {
public:
	StGDISelectWin (HDC inDC, HGDIOBJ inObj) 
		: mDC (inDC)
	{
		mObj = ::SelectObject(mDC, inObj);	
	}
	
	~StGDISelectWin()
	{
		::SelectObject(mDC, mObj);
	}

private:
	StGDISelectWin (const StGDISelectWin&);
	StGDISelectWin& operator= (const StGDISelectWin&);
	
	HDC mDC;
	HGDIOBJ mObj;
};

class StWindowDC {
	public:
						StWindowDC (HDC inDC)
							: mDC(inDC)
							{}
	
						~StWindowDC ()
							{
								::DeleteDC(mDC);
							}

	private:
		HDC mDC;
	
						StWindowDC(const StWindowDC&);
						StWindowDC& operator= (const StWindowDC&);
};

class StBkModeWin {
	public:
						StBkModeWin (::HDC inDC, int inMode)
							: mDC(inDC)
							{
								mOldMode = ::GetBkMode(mDC);
								::SetBkMode(mDC, inMode);
							}
							
						~StBkModeWin ()
							{
								::SetBkMode(mDC, mOldMode);
							}
							
	private:
		HDC mDC;
		int mOldMode;
};

HL_End_Namespace_BigRedH

#endif // _H_CGraphicsScopeWin_