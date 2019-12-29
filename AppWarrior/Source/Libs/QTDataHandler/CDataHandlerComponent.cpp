// =====================================================================
//  CDataHandlerComponent                (C) Hotline Communications 2000
// =====================================================================
//
//  Implementation of CDataHandlerComponent
//  (platform-independed, but uses QuickTime SDK)


//-----------------------------------------------------------------------
// includes

#include "aw.h"
#undef PRAGMA_ONCE // to avoid macro redefinition warning
#include <Components.h>
#include <QuickTimeComponents.h>
#include "CDataHandlerComponent.h"
#include "CDataProvider.h"
#include "UNetUtil.h"
#include "StString.h"
#include "CThreadManager.h"
#include "StHandleLocker.h"


//-----------------------------------------------------------------------


HL_Begin_Namespace_BigRedH


std::auto_ptr<CDataHandlerComponent> CDataHandlerComponent::sInstance( nil );


// ---------------------------------------------------------------------
//  Instance                                            [public][static]
// ---------------------------------------------------------------------
// returns the unique instance of Data Handler Component

CDataHandlerComponent&
CDataHandlerComponent::Instance()
{
	if( sInstance.get() == 0 ){
		sInstance = std::auto_ptr<CDataHandlerComponent> ( new CDataHandlerComponent );
	}
	return *sInstance;
}


// ---------------------------------------------------------------------
//  CDataHandlerComponent                                    [protected]
// ---------------------------------------------------------------------
// constructor

CDataHandlerComponent::CDataHandlerComponent()
{
	UNetUtil::StartNetworking();
	CThreadManager::Instance();
}

// ---------------------------------------------------------------------
//  ~CDataHandlerComponent                             [public][virtual]
// ---------------------------------------------------------------------
// destructor

CDataHandlerComponent::~CDataHandlerComponent()
{
	std::map<CString, CDataProvider*>::iterator it;
	for (it=mProviders.begin(); it!=mProviders.end(); it++)
		delete it->second;

	UNetUtil::StopNetworking();
}



// ---------------------------------------------------------------------
//  NewConnection                                               [public]
// ---------------------------------------------------------------------
// Creates a new object for a given connection and adds it to the list 

CDataHandlerConnection*
CDataHandlerComponent::NewConnection()
{
	CDataHandlerConnection* connection = new CDataHandlerConnection;
	mConnections.push_back(connection);
	return connection;
}


// ---------------------------------------------------------------------
//  DeleteConnection                                            [public]
// ---------------------------------------------------------------------
// Removes the connection object from the internal list, then
// deletes that connection 
void
CDataHandlerComponent::DeleteConnection(CDataHandlerConnection* inConnection)
{
	mConnections.remove(inConnection);
	delete inConnection;
}


// ---------------------------------------------------------------------
//  CanDo                                                       [public]
// ---------------------------------------------------------------------
// responds if it can handle particular selector

bool
CDataHandlerComponent::CanDo(short inSelector)
{
	switch (inSelector)
	{
	
		// standart selectors: I can do that
		case kComponentOpenSelect:
		case kComponentCloseSelect:
		case kComponentCanDoSelect:
		case kComponentVersionSelect:

		// data handling selectors
		case kDataHCanUseDataRefSelect:
		case kDataHSetDataRefSelect:
		case kDataHGetFileNameSelect:
		case kDataHGetFileSizeSelect: 
		case kDataHGetAvailableFileSizeSelect: 
		case kDataHScheduleDataSelect:
//		case kDataHFinishDataSelect:
		case kDataHGetScheduleAheadTimeSelect:
		case kDataHOpenForReadSelect:   
		case kDataHCloseForReadSelect:
		case kDataHCompareDataRefSelect:
		case kDataHTaskSelect:

			return 1;	// yes, we can handle it
	}
	return 0;
}


// ---------------------------------------------------------------------
//  GetVersion                                                  [public]
// ---------------------------------------------------------------------
// returns version of component

UInt32 
CDataHandlerComponent::GetVersion()
{
	return (eTestDHldrComponentSpec << 16) | eTestDHldrComponentVersion;
}


// ---------------------------------------------------------------------
//  CanUseDataRef                                               [public]
// ---------------------------------------------------------------------
// returns true if can handle particular DataRef


bool 
CDataHandlerComponent::CanUseDataRef(Handle inDataref) 
{
	StHandleLocker lock( inDataref );
	CString hotline = "hotline://";
	CString org = CString(*(const char **)inDataref);
	return org.find(hotline)==0;
}


// ---------------------------------------------------------------------
//  IsReading                                                   [public]
// ---------------------------------------------------------------------
// checks if a provider is in the middle of doing a read


bool 
CDataHandlerComponent::IsReading( Str255 inTempFileName ) 
{
	CString tempFile( inTempFileName );
	CDataProvider *provider = mProviders[tempFile];
	if( provider )
		return provider->IsReading();
	else
		return false;
}


// ---------------------------------------------------------------------
//  CancelReading                                               [public]
// ---------------------------------------------------------------------
// cancels a read for a provider


void 
CDataHandlerComponent::CancelReading( Str255 inTempFileName ) 
{
	CString tempFile( inTempFileName );
	CDataProvider *provider = mProviders[tempFile];
	if( provider )
		provider->CancelReading();
}


// ---------------------------------------------------------------------
//  GetAheadTime                                                [public]
// ---------------------------------------------------------------------
// return number (in ms) of how long ahead should QuickTime read before 
// start playing

UInt32
CDataHandlerComponent::GetAheadTime() 
{
	return eTimeAheadMs;
}


// ---------------------------------------------------------------------
//  GetProvider											[public][static]
// ---------------------------------------------------------------------
// parses URL like hotline://www.xxx.com/movie.mov&size=12345&refnum=12345&port=5501&tempfile=abc123
// and returns data provider for such URL
CDataProvider*
CDataHandlerComponent::GetProvider(const CString& URL)
{
	using namespace std;
	// parameters to be determined
	CString server;
	CString file_name, temp_file_name, mime_type;
	UInt16 port;
	UInt32 file_size;
	UInt32 ref_num;
	UInt32 file_type;

	CString theURL = URL;

	// 1. find "hotline://" substring
	CString hotline( L"hotline://" );
	SInt32 hotline_pos = theURL.find( hotline );
	if (hotline_pos!=0)
		return nil; // not hotline string

	hotline_pos += hotline.length();

	// 2. find first slash after hotline://
	SInt32 first_slash = theURL.find( L'/', hotline_pos );
	if (first_slash<0)
		return nil; // no slash after server name

	// 3. get server name 
	server = CString( theURL.substr(hotline_pos, first_slash - hotline_pos ) );

	// 4. get file name 
	SInt32 ampersand_pos = theURL.find( L'&' );
	if( ampersand_pos != theURL.npos ){
		file_name = CString( theURL.substr( first_slash + 1, ampersand_pos - first_slash - 1) );
	}

	// 5. find parameters followed by "&" in URL
	while (ampersand_pos>=0)
	{
		UInt32 next_ampersand = theURL.find( L'&', ampersand_pos+1 );
		CString param = next_ampersand>=0 ?
			theURL.substr(ampersand_pos, next_ampersand-ampersand_pos):
			theURL.substr(ampersand_pos);
		if( param.find( L"filesize=" ) != param.npos ){
			StCStyleString cstr(param.substr(param.find( L'=' )+1) );
			file_size = strtoul( cstr, nil, 10 );
		} else if( param.find( L"refnum=") != param.npos ){
			StCStyleString cstr( param.substr(param.find( L'=' )+1) );
			ref_num = strtoul( cstr, nil, 10 );
		} else if( param.find( L"port=" ) != param.npos ){
			StCStyleString cstr( param.substr(param.find( L'=' )+1) );
			port = strtoul( cstr, nil, 10 );
		} else if( param.find( L"tempfile=" ) != param.npos ){
			temp_file_name = CString( param.substr(param.find( L'=' )+1) );
		} else if( param.find( L"mimetype=" ) != param.npos ){
			mime_type = CString( param.substr(param.find( L'=' )+1) );
		} else if( param.find( L"filetype=" ) != param.npos ){
			StCStyleString cstr( param.substr(param.find( L'=' )+1) );
			file_type = strtoul( cstr, nil, 10 );
		}

		ampersand_pos = next_ampersand;
	}

	CDataProvider* provider = mProviders[temp_file_name];
	if (!provider)
	{
		provider = new CDataProvider(server, file_name, port, file_size,
									ref_num, temp_file_name, mime_type, file_type );
		mProviders[temp_file_name] = provider;
	}

	if (provider)
		provider->AddRef();

	return provider;
}


void 
CDataHandlerComponent::DeleteProvider(CDataProvider* inProvider)
{
	std::map<CString, CDataProvider*>::iterator it;
	for (it=mProviders.begin(); it!=mProviders.end(); it++)
		if (it->second == inProvider)
		{
			mProviders[it->first] = nil;
			return;
		}
}



// for debugging purpose: from the given error
char* 
CDataHandlerComponent::GetSelectorName(int number)
{
	switch (number) 
	{
		case kComponentOpenSelect: return "kComponentOpenSelect";
		case kComponentCloseSelect: return "kComponentCloseSelect";
		case kComponentCanDoSelect: return "kComponentCanDoSelect";
		case kComponentVersionSelect: return "kComponentVersionSelect";

		case kDataHGetDataSelect: return "kDataHGetDataSelect";
		case kDataHPutDataSelect: return "kDataHPutDataSelect";
		case kDataHFlushDataSelect: return "kDataHFlushDataSelect";
		case kDataHOpenForWriteSelect: return "kDataHOpenForWriteSelect";
		case kDataHCloseForWriteSelect: return "kDataHCloseForWriteSelect";
		case kDataHOpenForReadSelect: return "kDataHOpenForReadSelect";
		case kDataHCloseForReadSelect: return "kDataHCloseForReadSelect";
		case kDataHSetDataRefSelect: return "kDataHSetDataRefSelect";
		case kDataHGetDataRefSelect: return "kDataHGetDataRefSelect";
		case kDataHCompareDataRefSelect: return "kDataHCompareDataRefSelect";
		case kDataHTaskSelect: return "kDataHTaskSelect";
		case kDataHScheduleDataSelect: return "kDataHScheduleDataSelect";
		case kDataHFinishDataSelect: return "kDataHFinishDataSelect";
		case kDataHFlushCacheSelect: return "kDataHFlushCacheSelect";
		case kDataHResolveDataRefSelect: return "kDataHResolveDataRefSelect";
		case kDataHGetFileSizeSelect: return "kDataHGetFileSizeSelect";
		case kDataHCanUseDataRefSelect: return "kDataHCanUseDataRefSelect";
		case kDataHGetVolumeListSelect: return "kDataHGetVolumeListSelect";
		case kDataHWriteSelect: return "kDataHWriteSelect";
		case kDataHPreextendSelect: return "kDataHPreextendSelect";
		case kDataHSetFileSizeSelect: return "kDataHSetFileSizeSelect";
		case kDataHGetFreeSpaceSelect: return "kDataHGetFreeSpaceSelect";
		case kDataHCreateFileSelect: return "kDataHCreateFileSelect";
		case kDataHGetPreferredBlockSizeSelect:  return "kDataHGetPreferredBlockSizeSelect";
		case kDataHGetDeviceIndexSelect: return "kDataHGetDeviceIndexSelect";
		case kDataHIsStreamingDataHandlerSelect: return "kDataHIsStreamingDataHandlerSelect";
		case kDataHGetDataInBufferSelect: return "kDataHGetDataInBufferSelect";
		case kDataHGetScheduleAheadTimeSelect: return "kDataHGetScheduleAheadTimeSelect";
		case kDataHSetCacheSizeLimitSelect: return "kDataHSetCacheSizeLimitSelect";
		case kDataHGetCacheSizeLimitSelect: return "kDataHGetCacheSizeLimitSelect";
		case kDataHGetMovieSelect: return "kDataHGetMovieSelect";
		case kDataHAddMovieSelect: return "kDataHAddMovieSelect";
		case kDataHUpdateMovieSelect: return "kDataHUpdateMovieSelect";
		case kDataHDoesBufferSelect: return "kDataHDoesBufferSelect";
		case kDataHGetFileNameSelect: return "kDataHGetFileNameSelect";
		case kDataHGetAvailableFileSizeSelect: return "kDataHGetAvailableFileSizeSelect";
		case kDataHGetMacOSFileTypeSelect: return "kDataHGetMacOSFileTypeSelect";
		case kDataHGetMIMETypeSelect: return "kDataHGetMIMETypeSelect";
		case kDataHSetDataRefWithAnchorSelect: return "kDataHSetDataRefWithAnchorSelect";
		case kDataHGetDataRefWithAnchorSelect: return "kDataHGetDataRefWithAnchorSelect";
		case kDataHSetMacOSFileTypeSelect: return "kDataHSetMacOSFileTypeSelect";
		case kDataHSetTimeBaseSelect: return "kDataHSetTimeBaseSelect";
/*
		case kDataHGetInfoFlagsSelect: return "kDataHGetInfoFlagsSelect";
		case kDataHScheduleData64Select: return "kDataHScheduleData64Select";
		case kDataHWrite64Select: return "kDataHWrite64Select";
		case kDataHGetFileSize64Select: return "kDataHGetFileSize64Select";
		case kDataHPreextend64Select: return "kDataHPreextend64Select";
		case kDataHSetFileSize64Select: return "kDataHSetFileSize64Select";
		case kDataHGetFreeSpace64Select: return "kDataHGetFreeSpace64Select";
		case kDataHAppend64Select: return "kDataHAppend64Select";
		case kDataHReadAsyncSelect: return "kDataHReadAsyncSelect";
		case kDataHPollReadSelect: return "kDataHPollReadSelect";
		case kDataHGetDataAvailabilitySelect: return "kDataHGetDataAvailabilitySelect";
		case kDataHGetFileSizeAsyncSelect: return "kDataHGetFileSizeAsyncSelect";
*/
		case kDataHPlaybackHintsSelect: return "kDataHPlaybackHintsSelect";
	}
	return "Unknown";
}




HL_End_Namespace_BigRedH
