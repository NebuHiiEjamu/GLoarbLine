// =====================================================================
//  StHandleLocker.h                     (C) Hotline Communications 2000
// =====================================================================
// 

#ifndef _H_StHandleLocker_
#define _H_StHandleLocker_

#include <MacMemory.h>

HL_Begin_Namespace_BigRedH


class StHandleLocker
{
public:
    
					StHandleLocker( Handle inHandle )
							:mHandle(inHandle)
						{ mWasUnlocked = (::HGetState( mHandle ) & 0x80) == 0;
							if( mWasUnlocked ) ::HLock( mHandle ); }
		

					~StHandleLocker()
						{ if( mWasUnlocked ) ::HUnlock( mHandle ); }

private:
	Handle			mHandle;
	bool			mWasUnlocked;
};

HL_End_Namespace_BigRedH

#endif //_H_StHandleLocker_
