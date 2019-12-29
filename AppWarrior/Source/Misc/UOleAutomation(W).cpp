#if WIN32

/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UOleAutomation.h"

#include <wchar.h>

static bool _CountArgsInFormat(Int8 *inFormat, Uint32 *outPn);
static Int8 *_GetNextVarType(Int8 *inFormat, VARTYPE *outVarType);


// Creates an instance of the Automation object and obtains it's IDispatch interface
bool UOleAutomation::CreateObject(LPOLESTR inProgID, IDispatch **outDispatch)
{
    *outDispatch = NULL;      
    CLSID clsid;					// CLSID of automation object
        
    // retrieve CLSID from the inProgID that the user specified
    HRESULT hr = ::CLSIDFromProgID(inProgID, &clsid);
    if (FAILED(hr))
        return false;
    
    return CreateObject(clsid, outDispatch);
}  

// Creates an instance of the Automation object and obtains it's IDispatch interface
bool UOleAutomation::CreateObject(CLSID& inClsid, IDispatch **outDispatch)
{
    *outDispatch = NULL;
    LPUNKNOWN pUnknown = NULL;		// IUnknown of automation object

    // create an instance of the automation object and ask for the IDispatch interface
    HRESULT hr = ::CoCreateInstance(inClsid, NULL, CLSCTX_SERVER, IID_IUnknown, (void**)&pUnknown);
    if (FAILED(hr))
        return false;
                   
    LPDISPATCH pDispatch = NULL;	// IDispatch of automation object
    hr = pUnknown->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    if (FAILED(hr))
    {
        pUnknown->Release();
        return false;
    }

    *outDispatch = pDispatch;
    pUnknown->Release();

    return true;     
}  

/* 
 * Invokes a property accessor function or method of an automation object
 *
 * Parameters:
 *
 *  inDispatch    IDispatch* of automation object
 *
 *  inFlags       Specfies if property is to be accessed or method to be invoked.
 *                Can hold DISPATCH_PROPERTYGET, DISPATCH_PROPERTYPUT, DISPATCH_METHOD,
 *                DISPATCH_PROPERTYPUTREF or DISPATCH_PROPERTYGET | DISPATCH_METHOD.
 *
 *  inProcName    Name of property or method.
 *
 *  inFormat      Format string that describes the variable list of parameters that follows. 
 *                The format string can contain the follwoing characters.
 *                & = mark the following format character as VT_BYREF 
 *                b = VT_BOOL
 *                i = VT_I2
 *                I = VT_I4
 *                r = VT_R2
 *                R = VT_R4
 *                c = VT_CY 
 *                s = VT_BSTR 		String pointer can be passed, BSTR will be allocated by this function
 *                e = VT_ERROR
 *                d = VT_DATE
 *                v = VT_VARIANT 	Use this to pass data types that are not described in the format string
 *                D = VT_DISPATCH
 *                U = VT_UNKNOWN
 *    
 * Usage examples:
 *
 *  HRESULT hr;  
 *  LPDISPATCH pdisp;   
 *  BSTR bstr;
 *  short i;
 *  BOOL b;   
 *  VARIANT v, v2;
 *
 *1. bstr = SysAllocString(OLESTR(""));
 *   hr = Invoke(pdisp, DISPATCH_METHOD, OLESTR("Method1"), "bis&b&i&s", TRUE, 2, (LPOLESTR)OLESTR("param"), (BOOL *)&b, (short *)&i, (BSTR *)&bstr);   
 *
 *2. VariantInit(&v);
 *   V_VT(&v) = VT_R8;
 *   V_R8(&v) = 12345.6789; 
 *   VariantInit(&v2);
 *   hr = Invoke(pdisp, DISPATCH_METHOD, OLESTR("Method2"), "v&v", v, (VARIANT *)&v2);
 */
 
bool UOleAutomation::Invoke(LPDISPATCH inDispatch, Uint16 inFlags, LPVARIANT outRet, LPOLESTR inProcName, Int8 *inFormat, ...)
{
    if (!inDispatch)
        return false;

    va_list argList;
    va_start(argList, inFormat);  
    
    DISPID dispid;
    VARIANTARG* pvarg = NULL;
    
    // get DISPID of property/method
    HRESULT hr = inDispatch->GetIDsOfNames(IID_NULL, &inProcName, 1, LOCALE_USER_DEFAULT, &dispid);
    if(FAILED(hr))
        return false;
               
    DISPPARAMS dispparams;
    UMemory::Clear(&dispparams, sizeof(DISPPARAMS));

    // determine number of arguments
    if (inFormat)
        _CountArgsInFormat(inFormat, (Uint32 *)&dispparams.cArgs);
    
    // property puts have a named argument that represents the value that the property is being assigned
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    if (inFlags & DISPATCH_PROPERTYPUT)
    {
        if (dispparams.cArgs == 0)
            return false;
        
        dispparams.cNamedArgs = 1;
        dispparams.rgdispidNamedArgs = &dispidNamed;
    }

    if (dispparams.cArgs != 0)
    {
        // allocate memory for all VARIANTARG parameters
        pvarg = new VARIANTARG[dispparams.cArgs];
        if(!pvarg)
            return false;   
            
        dispparams.rgvarg = pvarg;
        UMemory::Clear(pvarg, sizeof(VARIANTARG) * dispparams.cArgs);

        // get ready to walk vararg list
        Int8 *psz = inFormat;
        pvarg += dispparams.cArgs - 1;   // params go in opposite order
        
        while ((psz = _GetNextVarType(psz, &pvarg->vt)) != NULL)
        {
            if (pvarg < dispparams.rgvarg)
            {
                hr = ResultFromScode(E_INVALIDARG);
                goto cleanup;  
            }
            
            switch (pvarg->vt)
            {
            case VT_I2:
                V_I2(pvarg) = va_arg(argList, short);
                break;
            case VT_I4:
                V_I4(pvarg) = va_arg(argList, long);
                break;
            case VT_R4:
                V_R4(pvarg) = va_arg(argList, float);
                break; 
            case VT_DATE:
            case VT_R8:
                V_R8(pvarg) = va_arg(argList, double);
                break;
            case VT_CY:
                V_CY(pvarg) = va_arg(argList, CY);
                break;
            case VT_BSTR:
                V_BSTR(pvarg) = ::SysAllocString(va_arg(argList, OLECHAR FAR*));
                if (pvarg->bstrVal == NULL) 
                {
                    hr = ResultFromScode(E_OUTOFMEMORY);  
                    pvarg->vt = VT_EMPTY;
                    goto cleanup;  
                }
                break;
            case VT_DISPATCH:
                V_DISPATCH(pvarg) = va_arg(argList, LPDISPATCH);
                break;
            case VT_ERROR:
                V_ERROR(pvarg) = va_arg(argList, SCODE);
                break;
            case VT_BOOL:
                V_BOOL(pvarg) = va_arg(argList, BOOL) ? -1 : 0;
                break;
            case VT_VARIANT:
                *pvarg = va_arg(argList, VARIANTARG); 
                break;
            case VT_UNKNOWN:
                V_UNKNOWN(pvarg) = va_arg(argList, LPUNKNOWN);
                break;
            case VT_I2|VT_BYREF:
                V_I2REF(pvarg) = va_arg(argList, short FAR*);
                break;
            case VT_I4|VT_BYREF:
                V_I4REF(pvarg) = va_arg(argList, long FAR*);
                break;
            case VT_R4|VT_BYREF:
                V_R4REF(pvarg) = va_arg(argList, float FAR*);
                break;
            case VT_R8|VT_BYREF:
                V_R8REF(pvarg) = va_arg(argList, double FAR*);
                break;
            case VT_DATE|VT_BYREF:
                V_DATEREF(pvarg) = va_arg(argList, DATE FAR*);
                break;
            case VT_CY|VT_BYREF:
                V_CYREF(pvarg) = va_arg(argList, CY FAR*);
                break;
            case VT_BSTR|VT_BYREF:
                V_BSTRREF(pvarg) = va_arg(argList, BSTR FAR*);
                break;
            case VT_DISPATCH|VT_BYREF:
                V_DISPATCHREF(pvarg) = va_arg(argList, LPDISPATCH FAR*);
                break;
            case VT_ERROR|VT_BYREF:
                V_ERRORREF(pvarg) = va_arg(argList, SCODE FAR*);
                break;
            case VT_BOOL|VT_BYREF: 
                {
                    BOOL FAR* pbool = va_arg(argList, BOOL FAR*);
                    *pbool = 0;
                    V_BOOLREF(pvarg) = (VARIANT_BOOL FAR*)pbool;
                } 
                break;              
            case VT_VARIANT|VT_BYREF: 
                V_VARIANTREF(pvarg) = va_arg(argList, VARIANTARG FAR*);
                break;
            case VT_UNKNOWN|VT_BYREF:
                V_UNKNOWNREF(pvarg) = va_arg(argList, LPUNKNOWN FAR*);
                break;

            default:
                {
                    hr = ResultFromScode(E_INVALIDARG);
                    goto cleanup;  
                }
                break;
            }

            --pvarg; // get ready to fill next argument
        }
    } 
        
    // make the call 
    hr = inDispatch->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, inFlags, &dispparams, outRet, NULL, NULL);

cleanup:
    
    // cleanup any arguments that need cleanup
    if (dispparams.cArgs != 0)
    {
        VARIANTARG FAR* pvarg = dispparams.rgvarg;
        UINT cArgs = dispparams.cArgs;   
        
        while (cArgs--)
        {
            switch (pvarg->vt)
            {
            	case VT_BSTR:
                	::VariantClear(pvarg);
                	break;
            }
            
            ++pvarg;
        }
    }
    
    delete dispparams.rgvarg;
    va_end(argList);
    
    if (FAILED(hr))
    	return false;

    return true;   
}   

bool UOleAutomation::RegisterServer(const Int8 *inClsid, const Int8 *inProgID, bool inAppDll)
{
	if (!inClsid || !inProgID)
		return false;
	
	DWORD dwResult;
	
	// create HKEY_CLASSES_ROOT\inProgID\CLSID\inClsid
	HKEY hProgKey;
	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, inProgID, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hProgKey, &dwResult) != ERROR_SUCCESS)
		return false;

	if (RegSetValueEx(hProgKey, NULL, 0, REG_SZ, (BYTE *)inProgID, strlen(inProgID)) != ERROR_SUCCESS)
	{
		RegCloseKey(hProgKey);
		return false;
	}

	HKEY hClsidKey;
	if (RegCreateKeyEx(hProgKey, "CLSID", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hClsidKey, &dwResult) != ERROR_SUCCESS)
	{
		RegCloseKey(hProgKey);
		return false;
	}

	RegCloseKey(hProgKey);

	if (RegSetValueEx(hClsidKey, NULL, 0, REG_SZ, (BYTE *)inClsid, strlen(inClsid)) != ERROR_SUCCESS)
	{
		RegCloseKey(hClsidKey);
		return false;
	}

	RegCloseKey(hClsidKey);

	// create HKEY_CLASSES_ROOT\CLSID\inClsid...
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &hClsidKey) != ERROR_SUCCESS)
		return false;
		
	HKEY hClassKey;
	if (RegCreateKeyEx(hClsidKey, inClsid, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hClassKey, &dwResult) != ERROR_SUCCESS)
	{
		RegCloseKey(hClsidKey);
		return false;
	}

	RegCloseKey(hClsidKey);
	
	if (RegSetValueEx(hClassKey, NULL, 0, REG_SZ, (BYTE *)inProgID, strlen(inProgID)) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}

	Int8 *pServerType;
	if (inAppDll)
		pServerType = "LocalServer32";
	else
		pServerType = "InprocServer32";
	
	HKEY hServerKey;
	if (RegCreateKeyEx(hClassKey, pServerType, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hServerKey, &dwResult) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}

	Int8 bufPath[2048];
	Uint32 nPathSize = ::GetModuleFileName(NULL, bufPath, sizeof(bufPath));
	
	if (RegSetValueEx(hServerKey, NULL, 0, REG_SZ, (BYTE *)bufPath, nPathSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		RegCloseKey(hServerKey);
		return false;
	}
	
	RegCloseKey(hServerKey);

	if (RegCreateKeyEx(hClassKey, "ProgID", NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hProgKey, &dwResult) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}
	
	if (RegSetValueEx(hProgKey, NULL, 0, REG_SZ, (BYTE *)inProgID, strlen(inProgID)) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		RegCloseKey(hProgKey);
		return false;
	}
	
	RegCloseKey(hClassKey);
	RegCloseKey(hProgKey);

	return true;
}

bool UOleAutomation::IsRegisteredServer(const Int8 *inClsid, const Int8 *inProgID, bool inAppDll)
{
	if (!inClsid || !inProgID)
		return false;
		
	// check HKEY_CLASSES_ROOT\inProgID\CLSID\inClsid
	HKEY hProgKey;
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, inProgID, 0, KEY_ALL_ACCESS, &hProgKey) != ERROR_SUCCESS)
		return false;

	HKEY hClsidKey;
	if (RegOpenKeyEx(hProgKey, "CLSID", 0, KEY_ALL_ACCESS, &hClsidKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hProgKey);
		return false;
	}

	RegCloseKey(hProgKey);

	DWORD dwType;
	Int8 buf[2048];

	Uint32 nSize = sizeof(buf);
	if (RegQueryValueEx(hClsidKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hClsidKey);
		return false;
	}

	RegCloseKey(hClsidKey);

	nSize--;
	if (dwType != REG_SZ || nSize != strlen(inClsid) || !UMemory::Equal(inClsid, buf, nSize))
		return false;

	// check HKEY_CLASSES_ROOT\CLSID\inClsid...
	if (RegOpenKeyEx(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_ALL_ACCESS, &hClsidKey) != ERROR_SUCCESS)
		return false;
		
	HKEY hClassKey;
	if (RegOpenKeyEx(hClsidKey, inClsid, 0, KEY_ALL_ACCESS, &hClassKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hClsidKey);
		return false;
	}

	RegCloseKey(hClsidKey);
	
	nSize = sizeof(buf);
	if (RegQueryValueEx(hClassKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}

	nSize--;
	if (dwType != REG_SZ || nSize != strlen(inProgID) || !UMemory::Equal(inProgID, buf, nSize))
	{
		RegCloseKey(hClassKey);
		return false;
	}
	
	Int8 *pServerType;
	if (inAppDll)
		pServerType = "LocalServer32";
	else
		pServerType = "InprocServer32";
	
	HKEY hServerKey;
	if (RegOpenKeyEx(hClassKey, pServerType, 0, KEY_ALL_ACCESS, &hServerKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}
	
	nSize = sizeof(buf);
	if (RegQueryValueEx(hServerKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		RegCloseKey(hServerKey);
		return false;
	}
	
	RegCloseKey(hServerKey);

	Int8 bufPath[2048];
	Uint32 nPathSize = ::GetModuleFileName(NULL, bufPath, sizeof(bufPath));

	nSize--;
	if (dwType != REG_SZ || nPathSize != nSize || !UMemory::Equal(bufPath, buf, nSize))
	{
		RegCloseKey(hClassKey);
		return false;
	}

	if (RegOpenKeyEx(hClassKey, "ProgID", 0, KEY_ALL_ACCESS, &hProgKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		return false;
	}
		
	nSize = sizeof(buf);
	if (RegQueryValueEx(hProgKey, NULL, NULL, &dwType, (LPBYTE)buf, &nSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hClassKey);
		RegCloseKey(hProgKey);
		return false;
	}
	
	RegCloseKey(hClassKey);
	RegCloseKey(hProgKey);

	nSize--;
	if (dwType != REG_SZ || nSize != strlen(inProgID) || !UMemory::Equal(inProgID, buf, nSize))
		return false;
	
	return true;
}


#pragma mark -

static bool _CountArgsInFormat(Int8 *inFormat, Uint32 *outPn)
{
    *outPn = 0;

    if(!inFormat)
      return false;
    
    while (*inFormat)  
    {
       if (*inFormat == '&')
           inFormat++;

       switch(*inFormat)
       {
           case 'b':
           case 'i': 
           case 'I':
           case 'r': 
           case 'R':
           case 'c':
           case 's':
           case 'e':
           case 'd':
           case 'v':
           case 'D':
           case 'U':
               ++*outPn; 
               inFormat++;
               break;
           case '\0':  
           default:
               return false;   
        }
    }
    
    return true;
}

static Int8 *_GetNextVarType(Int8 *inFormat, VARTYPE *outVarType)
{   
    *outVarType = 0;
    
    if (*inFormat == '&') 
    {
         *outVarType = VT_BYREF; 
         inFormat++;
             
         if (!*inFormat)
             return NULL;    
    } 
    
    switch(*inFormat)
    {
        case 'b':
            *outVarType |= VT_BOOL;
            break;
        case 'i': 
            *outVarType |= VT_I2;
            break;
        case 'I': 
            *outVarType |= VT_I4;
            break;
        case 'r': 
            *outVarType |= VT_R4;
            break;
        case 'R': 
            *outVarType |= VT_R8;
            break;
        case 'c':
            *outVarType |= VT_CY;
            break;
        case 's': 
            *outVarType |= VT_BSTR;
            break;
        case 'e': 
            *outVarType |= VT_ERROR;
            break;
        case 'd': 
            *outVarType |= VT_DATE; 
            break;
        case 'v': 
            *outVarType |= VT_VARIANT;
            break;
        case 'U': 
            *outVarType |= VT_UNKNOWN; 
            break;
        case 'D': 
            *outVarType |= VT_DISPATCH;
            break;  
        case '\0':
             return NULL;     // end of Format string
        default:
            return NULL;
    } 
    
    return ++inFormat;  
}

#pragma mark -

extern Int8 *_gCmdLineStr;
extern Uint32 _gCmdLineStrSize;

//DEFINE_GUID(CLSID_PROTOCOL_HANDLER, 0xeb95f542, 0x3a00, 0x11d3, 0x90, 0x7, 0x0, 0x0, 0xb4, 0x5f, 0x53, 0x98);
EXTERN_C const GUID CLSID_PROTOCOL_HANDLER = {0xeb95f542, 0x3a00, 0x11d3, {0x90, 0x7, 0x0, 0x0, 0xb4, 0x5f, 0x53, 0x98}};
const Int8 *gClsid_Protocol_Handler = "{EB95F542-3A00-11d3-9007-0000B45F5398}";
Int8 *gProgID_Protocol_Handler = "Hotline.Protocol.1";

class _DIProtocolUnknown;
class _DIProtocolClassFactory;
_DIProtocolUnknown FAR *gProtocolUnknown = NULL;
_DIProtocolClassFactory FAR *gProtocolClassFactory = NULL;
Uint32 gRegisterUnknown = 0;
Uint32 gRegisterFactory = 0;

METHODDATA gMethodData[2];
INTERFACEDATA gInterfaceData;


class _DProtocolHandler
{
	public:
		_DProtocolHandler();
		
	    /* _DProtocolHandler methods */
	    STDMETHOD_ (short, Initialize) (LPCTSTR pProtocol, LPCTSTR pUrl);
	    STDMETHOD_ (void, Open) (LPCTSTR pUrl);
};

_DProtocolHandler::_DProtocolHandler()
{
}

STDMETHODIMP_ (short) _DProtocolHandler::Initialize(LPCTSTR pProtocol, LPCTSTR pUrl)
{
	if (!pProtocol || !pUrl)
		return 0;
	 	 
	Int8 bufProtocol[128];
 	Uint32 nProtocolSize = wcslen((WCHAR *)pProtocol);
  	::WideCharToMultiByte(CP_ACP, 0, (WCHAR *)pProtocol, nProtocolSize, bufProtocol, sizeof(bufProtocol), NULL, NULL);
 
	Int8 *pHotlineProtocol = "hotline";
	if (nProtocolSize == strlen(pHotlineProtocol) && !UMemory::Compare(bufProtocol, pHotlineProtocol, nProtocolSize))
		return 1;
		
	return 0;
}

STDMETHODIMP_ (void) _DProtocolHandler::Open(LPCTSTR pUrl)
{
	if (!pUrl)
		return;

	Int8 bufUrl[256];
	Uint32 nUrlSize = wcslen((WCHAR *)pUrl);
  	::WideCharToMultiByte(CP_ACP, 0, (WCHAR *)pUrl, nUrlSize, bufUrl, sizeof(bufUrl), NULL, NULL);

	if (_gCmdLineStr)
		::GlobalFree((HGLOBAL)_gCmdLineStr);

	_gCmdLineStr = (Int8 *)::GlobalAlloc(GMEM_FIXED, nUrlSize + 1);
	
	if (_gCmdLineStr)
	{
		::CopyMemory(_gCmdLineStr, bufUrl, nUrlSize + 1);
		_gCmdLineStrSize = nUrlSize;
			
		UApplication::PostMessage(msg_OpenDocuments);
	}
}


#pragma mark-

class _DIProtocolUnknown : public IUnknown
{
	public:
		_DIProtocolUnknown();
	    static _DIProtocolUnknown FAR *Create();

	   /* IUnknown methods */
	    STDMETHOD  (QueryInterface) (REFIID riid, void FAR* FAR* ppvObj);
	    STDMETHOD_ (ULONG, AddRef)  (void);
	    STDMETHOD_ (ULONG, Release) (void);
	    
	    _DProtocolHandler mProtocolHandler;
	    
	private:
		ULONG mRefCount;
		IUnknown FAR *mUnknownStdDisp;
};

_DIProtocolUnknown::_DIProtocolUnknown()
{
	mRefCount = 0;
	mUnknownStdDisp = NULL;
}

_DIProtocolUnknown FAR *_DIProtocolUnknown::Create()
{
    _DIProtocolUnknown FAR *pProtocolUnknown = new FAR _DIProtocolUnknown();
    if (pProtocolUnknown == NULL)
    	return NULL;
    
    pProtocolUnknown->AddRef();
    _DProtocolHandler FAR *pProtocolHandler = &pProtocolUnknown->mProtocolHandler;

    ITypeInfo FAR *pTypeInfo; 
 
    // build a TypeInfo for the functionality on this object that is being exposing for external programmability
    HRESULT hr = ::CreateDispTypeInfo(&gInterfaceData, LOCALE_SYSTEM_DEFAULT, &pTypeInfo);   
    if(hr != NOERROR)
    {
      	pProtocolUnknown->Release();
      	return NULL;
    }

    IUnknown FAR *pUnknowbStdDisp;

    // create and aggregate with an instance of the default implementation of IDispatch that is initialized with our pTypeInfo
    hr = ::CreateStdDispatch(pProtocolUnknown, pProtocolHandler, pTypeInfo, &pUnknowbStdDisp);
    pTypeInfo->Release();

    if(hr != NOERROR)
    {
    	pProtocolUnknown->Release();
      	return NULL;
    }

    pProtocolUnknown->mUnknownStdDisp = pUnknowbStdDisp;
    return pProtocolUnknown;
}

STDMETHODIMP _DIProtocolUnknown::QueryInterface(REFIID riid, void FAR* FAR* ppvObj)
{
	*ppvObj = NULL;
	
	if (riid == IID_IUnknown)
		*ppvObj = (LPUNKNOWN) this;
	
	if (riid == IID_IDispatch)
      return mUnknownStdDisp->QueryInterface(riid, ppvObj);
			
	if (*ppvObj == NULL)
		return E_NOINTERFACE;
		
	AddRef();
	return NOERROR;		
}

STDMETHODIMP_ (ULONG) _DIProtocolUnknown::AddRef()
{
	return ++mRefCount;
}

STDMETHODIMP_ (ULONG) _DIProtocolUnknown::Release()
{	
	if (0L != --mRefCount)
		return mRefCount;
		
	delete this;
	return 0L;
}


#pragma mark-

class _DIProtocolClassFactory : public IClassFactory
{
	public:
 		_DIProtocolClassFactory();
   	    static _DIProtocolClassFactory FAR *Create();

	   /* IUnknown methods */
	    STDMETHOD  (QueryInterface) (REFIID riid, void FAR* FAR* ppvObj);
	    STDMETHOD_ (ULONG, AddRef)  (void);
	    STDMETHOD_ (ULONG, Release) (void);
	    
	    /* IClassFactory methods */
	    STDMETHOD (CreateInstance) (IUnknown FAR* pUnkOuter, REFIID riid, void FAR* FAR* ppvObject);
	    STDMETHOD (LockServer) (BOOL fLock);
					
	private:
		ULONG mRefCount;
};

_DIProtocolClassFactory::_DIProtocolClassFactory()
{
	mRefCount = 0;
}

_DIProtocolClassFactory FAR *_DIProtocolClassFactory::Create()
{
    _DIProtocolClassFactory FAR *pProtocolClassFactory = new FAR _DIProtocolClassFactory();
    if (pProtocolClassFactory == NULL)
    	return NULL;
    
    pProtocolClassFactory->AddRef();

    return pProtocolClassFactory;
}

STDMETHODIMP _DIProtocolClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	*ppvObj = NULL;
	
	if (riid == IID_IUnknown)
		*ppvObj = (LPUNKNOWN) this;
		
	if (riid == IID_IClassFactory)
		*ppvObj = (LPCLASSFACTORY) this;
		
	if (*ppvObj == NULL)
		return E_NOINTERFACE;
		
	AddRef();
	return NOERROR;		
}

STDMETHODIMP_ (ULONG) _DIProtocolClassFactory::AddRef()
{
	return ++mRefCount;
}

STDMETHODIMP_ (ULONG) _DIProtocolClassFactory::Release()
{	
	if (0L != --mRefCount)
		return mRefCount;
		
	delete this;
	return 0L;
}

STDMETHODIMP _DIProtocolClassFactory::CreateInstance(IUnknown FAR *pUnkOuter, REFIID riid, void FAR* FAR* ppvObject)
{
	*ppvObject = NULL;
	
	if (pUnkOuter != NULL)
		return CLASS_E_NOAGGREGATION;
			
	return gProtocolUnknown->QueryInterface(riid, ppvObject);
}

STDMETHODIMP _DIProtocolClassFactory::LockServer(BOOL /* fLock */)
{		
	return NOERROR;
}

#pragma mark -

void _InitInterfaceData()
{
 	gMethodData[0].szName = OLESTR("Open");
 	gMethodData[0].ppdata = (PARAMDATA FAR *)UMemory::New(sizeof(PARAMDATA));
 	gMethodData[0].ppdata[0].szName = OLESTR("pUrl");
	gMethodData[0].ppdata[0].vt = VT_BSTR;
	gMethodData[0].dispid = 2;
	gMethodData[0].iMeth = 1;
	gMethodData[0].cc = CC_CDECL;
	gMethodData[0].cArgs = 1;
	gMethodData[0].wFlags = DISPATCH_METHOD;
	gMethodData[0].vtReturn = VT_EMPTY;

 	gMethodData[1].szName = OLESTR("Initialize");
 	gMethodData[1].ppdata = (PARAMDATA FAR *)UMemory::New(2 * sizeof(PARAMDATA));
 	gMethodData[1].ppdata[0].szName = OLESTR("pProtocol");
	gMethodData[1].ppdata[0].vt = VT_BSTR;
 	gMethodData[1].ppdata[1].szName = OLESTR("pUrl");
	gMethodData[1].ppdata[1].vt = VT_BSTR;
	gMethodData[1].dispid = 1;
	gMethodData[1].iMeth = 0;
	gMethodData[1].cc = CC_CDECL;
	gMethodData[1].cArgs = 2;
	gMethodData[1].wFlags = DISPATCH_METHOD;
	gMethodData[1].vtReturn = VT_I2;
	
	gInterfaceData.pmethdata = gMethodData;
	gInterfaceData.cMembers = 2;
}

bool _InitOleAutomationServer()
{
    if (!gProgID_Protocol_Handler)
    	return false;
    
   _InitInterfaceData();

    gProtocolUnknown = _DIProtocolUnknown::Create();
    if (gProtocolUnknown == NULL)
    	return false;
    	
    gProtocolClassFactory = _DIProtocolClassFactory::Create();
    if (gProtocolClassFactory == NULL)
    {
    	gProtocolUnknown->Release();
    	return false;
    }
  
    HRESULT hr = ::CoRegisterClassObject(CLSID_PROTOCOL_HANDLER, gProtocolClassFactory, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &gRegisterFactory);
    if (FAILED(hr))
    {
    	gProtocolClassFactory->Release();
    	gProtocolUnknown->Release();
    	return false;
    }

    hr = ::RegisterActiveObject(gProtocolUnknown, CLSID_PROTOCOL_HANDLER, NULL, &gRegisterUnknown);
    if (FAILED(hr))
    {
    	gProtocolClassFactory->Release();
    	gProtocolUnknown->Release();
  	    ::CoRevokeClassObject(gRegisterFactory);
    	return false;
    }
    
    if (!UOleAutomation::IsRegisteredServer(gClsid_Protocol_Handler, gProgID_Protocol_Handler))
   	    UOleAutomation::RegisterServer(gClsid_Protocol_Handler, gProgID_Protocol_Handler);
    
    return true;
}

void _UninitInterfaceData()
{
    UMemory::Dispose((TPtr)gMethodData[0].ppdata);
    gMethodData[0].ppdata = NULL;
    
    UMemory::Dispose((TPtr)gMethodData[1].ppdata);
    gMethodData[1].ppdata = NULL;

	gInterfaceData.pmethdata = NULL;
	gInterfaceData.cMembers = 0;
}

void _UninitOleAutomationServer()
{
    if (!gProgID_Protocol_Handler)
    	return;
    
    if (gRegisterUnknown != 0)
    	::RevokeActiveObject(gRegisterUnknown, NULL);

    if (gRegisterFactory != 0)
    	::CoRevokeClassObject(gRegisterFactory);

    if (gProtocolUnknown != NULL)
      	gProtocolUnknown->Release();
   
  	if (gProtocolClassFactory != NULL)
     	gProtocolClassFactory->Release();
     	
    _UninitInterfaceData();
}

#endif /* WIN32 */
