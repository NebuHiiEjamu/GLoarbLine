/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
// Encapsulates the file access rights

#ifndef _H_CFileRights_
#define _H_CFileRights_

#if PRAGMA_ONCE
	#pragma once
#endif
			   
HL_Begin_Namespace_BigRedH

class CFileRights
{
	public:
					CFileRights()
						// throws nothing
						: mVisible(false)
						, mRead(false)
						, mWrite(false)
						{}
						
		void		SetRead(bool inOnOff)
						// throws nothing
						{ mRead = inOnOff; }
		bool		CanRead() const
						// throws nothing
						{ return mRead; }

		void		SetWrite(bool inOnOff)
						// throws nothing
						{ mWrite = inOnOff; }
		bool		CanWrite() const 
						// throws nothing
						{ return mWrite; }

		void		SetVisible(bool inOnOff)
						// throws nothing
						{ mVisible = inOnOff; }
		bool		IsVisible() const
						// throws nothing
						{ return mVisible; }
	private:
		bool		mVisible;
		bool		mRead;
		bool		mWrite;
}; // class CFileRights

HL_End_Namespace_BigRedH
#endif // _H_CFileRights_
