#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"


class UOleAutomation
{
	public:
		// automation client
		static bool CreateObject(LPOLESTR inProgID, IDispatch **outDispatch);
		static bool CreateObject(CLSID& inClsid, IDispatch **outDispatch);
		static bool Invoke(LPDISPATCH inDispatch, Uint16 inFlags, LPVARIANT outRet, LPOLESTR inProcName, Int8 *inFormat, ...);

		// automation server
		static bool RegisterServer(const Int8 *inClsid, const Int8 *inProgID, bool inAppDll = true);
		static bool IsRegisteredServer(const Int8 *inClsid, const Int8 *inProgID, bool inAppDll = true);		
};

#endif /* WIN32 */
