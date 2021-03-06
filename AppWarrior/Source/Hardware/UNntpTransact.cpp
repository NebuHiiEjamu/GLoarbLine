/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
 
#include "UNntpTransact.h"


/*
 * Structures and Types
 */

struct SEncodingInfo {
	bool bEncodingBase64;
	Int8 csFlavor[32];
	Uint8 psName[64];
};

struct SPostArticleInfo {
	Uint8 psArticleID[64];
	Uint8 psArticleName[64];
	Uint8 psPosterName[64];
	Uint8 psPosterAddr[64];
	Uint8 psOrganization[64];
	Uint8 psNewsReader[64];
	SCalendarDate stPostDate;
	CPtrList<Uint8> pParentIDsList;
	CPtrList<SArticleData> pDataList;
};
 
struct SNntpTransact {
	TTransport tpt;
	TMessageProc msgProc;
	void *msgProcContext;
	
	void *pBuffer;
	Uint32 nBufferSize;
	CPtrList<Uint32> pOffsetList;
	CPtrList<SArticleData> pDataList;
	SPostArticleInfo *pPostInfo;
	
	Uint8 nCommandID;
	bool bIsCommandExecuted;
	bool bIsCommandError;
	Uint8 psCommandError[128];

	Uint8 psServerAddr[256];
	Uint8 psUserLogin[32];
	Uint8 psUserPassword[32];
	bool bIsPostingAllowed;
	SNewsGroupInfo stSelectedGroup;			// SelectGroup
	SNewsArticleInfo stCurrentArticle;		// GetArticle, GetArticleHeader, GetArticleData
	SEncodingInfo stCurrentEncoding;		// GetArticle, GetArticleHeader
	Uint32 nSelectedArticleNumber;			// SelectArticle, SelectNextArticle, SelectPrevArticle
	Uint8 psSelectedArticleID[64];	
};

#define TRN		((SNntpTransact *)inTrn)

/*
 * Function Prototypes
 */

static void _NntpMessageHandler(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);

static void _StartNntpConnect(TNntpTransact inTrn, Uint16 inPort, Uint8 *inAddress, Uint32 inMaxSecs);
static bool _CheckServer(TNntpTransact inTrn, const Uint8 *inServerAddr, const Uint8 *inLogin, const Uint8 *inPassword, bool& outSameServer);
static bool _ConvertArticleDate(const Uint8 *inDate, SCalendarDate& outDate);
static Uint32 _ConvertPostDate(SCalendarDate& inPostDate, Uint8 *outDate, Uint32 inMaxSize);
static void _ProcessErrors(TNntpTransact inTrn, bool inCorruptData = false, bool inNotEnoughMemory = false);

static bool _Process_StartConnect(TNntpTransact inTrn);
static bool _Process_UserLogin(TNntpTransact inTrn);
static bool _Process_UserPassword(TNntpTransact inTrn);
static bool _Process_ListGroups(TNntpTransact inTrn);
static bool _Process_SelectGroup(TNntpTransact inTrn);
static bool _Process_ListArticles(TNntpTransact inTrn);
static bool _Process_SelectArticle(TNntpTransact inTrn);
static bool _Process_GetArticle(TNntpTransact inTrn);
static bool _Process_GetArticleHeader(TNntpTransact inTrn);
static bool _Process_GetArticleData(TNntpTransact inTrn);
static bool _Process_ExistsArticle(TNntpTransact inTrn);
static bool _Process_PostArticle(TNntpTransact inTrn);

static bool _SetOffsetList(TNntpTransact inTrn);
static bool _GetNewsItem(TNntpTransact inTrn, Uint32 inIndex, const Uint8 *&outFieldBegin, const Uint8 *&outFieldEnd);
static bool _GetGroup(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsGroupInfo& outGroupInfo);
static bool _GetSelectedGroup(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsGroupInfo& outGroupInfo);
static bool _GetArticle(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo);
static bool _GetArticleID(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint32& outArticleNumber, Uint8 *outArticleID, Uint32 inMaxSize);
static bool _GetCurrentArticle(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo, SEncodingInfo& outEncodingInfo);
static bool _GetArticlePoster(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo);
static Uint32 _GenerateName(const Int8 *inFlavor, Uint32& inNameNumber, Uint8 *outName, Uint32 inMaxSize);
static const Uint8 *_CheckMultiPart(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint8 *outMultiPartID, Uint32 inMaxSize);
static const Uint8 *_CheckMultiPartContent(TNntpTransact inTrn, const Uint8 *inMultiPartID, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint32& ioNameNumber);
static bool _GetArticleItem(TNntpTransact inTrn, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd);
static bool _GetArticleData(TNntpTransact inTrn, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd);
static bool _SetArticleItem(TNntpTransact inTrn, SArticleData *inArticleData);
static bool _SetPostArticleData(TNntpTransact inTrn);

static bool _WriteNntpData(void*& ioData, Uint32& ioDataSize, Uint32& ioDataOffset, const Uint8 *inNewData, Uint32 inNewDataSize);
static bool _AdjustNntpData(void*& ioData, Uint32& ioDataSize, Uint32 inDataOffset);
static void _ClearNntpFlags(TNntpTransact inTrn);
static void _ClearNntpTransport(TNntpTransact inTrn);
static void _ClearNntpBuffer(TNntpTransact inTrn);
static void _ClearNntpData(TNntpTransact inTrn);
static void _ClearNntpPost(TNntpTransact inTrn);
static void _ClearNntpCurrent(TNntpTransact inTrn);
static void _ClearNntpSelected(TNntpTransact inTrn);
static void _ClearNntpInfo(TNntpTransact inTrn);


/* -------------------------------------------------------------------------- */

TNntpTransact UNntpTransact::New()
{
	SNntpTransact *trn = (SNntpTransact *)UMemory::NewClear(sizeof(SNntpTransact));
	
	return (TNntpTransact)trn;
}

void UNntpTransact::Dispose(TNntpTransact inTrn)
{
	if (inTrn)
	{
		if (TRN->msgProc)
			UApplication::FlushMessages(TRN->msgProc, TRN->msgProcContext, inTrn);
		
		_ClearNntpTransport(inTrn);
		_ClearNntpBuffer(inTrn);
		_ClearNntpData(inTrn);
		_ClearNntpPost(inTrn);
		
		UMemory::Dispose((TPtr)inTrn);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UNntpTransact::SetMessageHandler(TNntpTransact inTrn, TMessageProc inProc, void *inContext)
{
	Require(inTrn);
	
	TRN->msgProc = inProc;
	TRN->msgProcContext = inContext;
}

void UNntpTransact::PostMessage(TNntpTransact inTrn, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inTrn);
	
	if (TRN->msgProc)
		UApplication::PostMessage(inMsg, inData, inDataSize, inPriority, TRN->msgProc, TRN->msgProcContext, inTrn);
}

void UNntpTransact::ReplaceMessage(TNntpTransact inTrn, Uint32 inMsg, const void *inData, Uint32 inDataSize, Int16 inPriority)
{
	Require(inTrn);
	
	if (TRN->msgProc)
		UApplication::ReplaceMessage(inMsg, inData, inDataSize, inPriority, TRN->msgProc, TRN->msgProcContext, inTrn);
}


/* -------------------------------------------------------------------------- */
#pragma mark -

void UNntpTransact::MakeNewTransport(TNntpTransact inTrn)
{
	Require(inTrn);
	
	_ClearNntpTransport(inTrn);
    
    try
    {
    	TRN->tpt = UTransport::New(transport_TCPIP);
    	UTransport::SetMessageHandler(TRN->tpt, _NntpMessageHandler, inTrn);
    }
    catch(...)
    {
    	UTransport::Dispose(TRN->tpt);
	  	TRN->tpt = nil;
    }
}

TTransport UNntpTransact::GetTransport(TNntpTransact inTrn)
{
	Require(inTrn);
	
	return TRN->tpt;
}


/* -------------------------------------------------------------------------- */
#pragma mark -

bool UNntpTransact::IsConnected(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (!TRN->tpt || TRN->nCommandID < kNntp_Connected)
		return false;

	return UTransport::IsConnected(TRN->tpt);
}

bool UNntpTransact::StartConnect(TNntpTransact inTrn, const Uint8 *inServerAddr,  const Uint8 *inLogin, const Uint8 *inPassword, Uint32 inMaxSecs)
{
	bool bSameServer;
	if (!_CheckServer(inTrn, inServerAddr, inLogin, inPassword, bSameServer))
		return false;
	
	_ClearNntpFlags(inTrn);
	_ClearNntpInfo(inTrn);

	if (!TRN->tpt || !TRN->tpt->IsConnected() || !bSameServer)
	{
		TRN->nCommandID = kNntp_StartConnect;
		_StartNntpConnect(inTrn, port_number_NNTP, TRN->psServerAddr, inMaxSecs);
	}
	else
	{
		TRN->nCommandID = kNntp_Connected;
		TRN->bIsCommandExecuted = true;
		UNntpTransact::PostMessage(inTrn, msg_ConnectionEstablished, nil, 0, priority_Normal);
	}

	return true;
}

void UNntpTransact::Disconnect(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (TRN->tpt)
		UTransport::Disconnect(TRN->tpt);
}

void UNntpTransact::StartDisconnect(TNntpTransact inTrn)
{
	Require(inTrn);

	if (TRN->tpt)
		UTransport::StartDisconnect(TRN->tpt);
}

bool UNntpTransact::IsDisconnecting(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (TRN->tpt)
		return UTransport::IsDisconnecting(TRN->tpt);
		
	return false;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool UNntpTransact::IsPostingAllowed(TNntpTransact inTrn)
{
	Require(inTrn);

	return (IsConnected(inTrn) && TRN->bIsPostingAllowed);
}

Uint32 UNntpTransact::GetConnectedServer(TNntpTransact inTrn, Uint8 *outServerAddr, Uint32 inMaxSize)
{
	Require(inTrn);
	
	if (!TRN->psServerAddr[0] || !outServerAddr || !inMaxSize)
		return 0;
		
	return UMemory::Copy(outServerAddr, TRN->psServerAddr + 1, TRN->psServerAddr[0] > inMaxSize ? inMaxSize : TRN->psServerAddr[0]);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// return kNntp_... constants
Uint8 UNntpTransact::GetCommandID(TNntpTransact inTrn)
{
	Require(inTrn);

	return TRN->nCommandID;
}

bool UNntpTransact::IsCommandExecuted(TNntpTransact inTrn)
{
	Require(inTrn);
	
	return TRN->bIsCommandExecuted;
}

bool UNntpTransact::IsCommandError(TNntpTransact inTrn)
{
	Require(inTrn);
		
	return TRN->bIsCommandError;
}

Uint8 *UNntpTransact::GetCommandError(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsCommandError(inTrn))
		return nil;

	return TRN->psCommandError;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpListGroups message
bool UNntpTransact::Command_ListGroups(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn))
		return false;
	
	_ClearNntpInfo(inTrn);

	Uint8 bufListCommand[6];
	Uint32 nCommandSize = UMemory::Copy(bufListCommand, "LIST\r\n", 6);
	
	TRN->tpt->Send(bufListCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_ListGroups;
	
	return true;
}

// generate msg_NntpListGroups message
bool UNntpTransact::Command_ListGroups(TNntpTransact inTrn, const SCalendarDate& inSinceDate)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !inSinceDate.IsValid())
		return false;
	
	_ClearNntpInfo(inTrn);

	SCalendarDate stSinceDate = inSinceDate + UDateTime::GetGMTDelta();

	Uint8 psYear[5];
	psYear[0] = UText::IntegerToText(psYear + 1, sizeof(psYear) - 1, stSinceDate.year);
	if (psYear[0] > 2)
		psYear[0] = UMemory::Copy(psYear + 1, psYear + psYear[0] - 1, 2);
	
	Uint8 bufNewgroupsCommand[64];
	Uint32 nCommandSize = UText::Format(bufNewgroupsCommand, sizeof(bufNewgroupsCommand), "NEWGROUPS %#s%2.2li%2.2li %2.2li%2.2li%2.2li GMT\r\n", psYear, stSinceDate.month, stSinceDate.day, stSinceDate.hour, stSinceDate.minute, stSinceDate.second);
	
	TRN->tpt->Send(bufNewgroupsCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_ListGroups;
	
	return true;
}

Uint32 UNntpTransact::GetGroupCount(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (GetCommandID(inTrn) != kNntp_ListGroups)
		return 0;

	return TRN->pOffsetList.GetItemCount();
}

bool UNntpTransact::GetGroup(TNntpTransact inTrn, SNewsGroupInfo& outGroupInfo, Uint32 inIndex)
{
	Require(inTrn);

	ClearStruct(outGroupInfo);
	
	if (GetCommandID(inTrn) != kNntp_ListGroups || !inIndex)
		return false;
		
	const Uint8 *pFieldBegin, *pFieldEnd;
	if (!_GetNewsItem(inTrn, inIndex, pFieldBegin, pFieldEnd))
		return false;
	
	return _GetGroup(pFieldBegin, pFieldEnd, outGroupInfo);
}

bool UNntpTransact::GetNextGroup(TNntpTransact inTrn, SNewsGroupInfo& outGroupInfo, Uint32& ioIndex)
{
	Require(inTrn);

	ioIndex++;
	return GetGroup(inTrn, outGroupInfo, ioIndex);
}

bool UNntpTransact::GetPrevGroup(TNntpTransact inTrn, SNewsGroupInfo& outGroupInfo, Uint32& ioIndex)
{
	Require(inTrn);

	if (!ioIndex)
		return false;
	
	ioIndex--;
	return GetGroup(inTrn, outGroupInfo, ioIndex);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpSelectGroup message
bool UNntpTransact::Command_SelectGroup(TNntpTransact inTrn, const Uint8 *inGroupName, bool inIsPostingAllowed)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !inGroupName || !inGroupName[0])
		return false;

	_ClearNntpInfo(inTrn);
	
	Uint8 bufGroupCommand[264];
	Uint32 nCommandSize = UText::Format(bufGroupCommand, sizeof(bufGroupCommand), "GROUP %#s\r\n", inGroupName);
	
	TRN->tpt->Send(bufGroupCommand, nCommandSize);
	TRN->stSelectedGroup.bIsPostingAllowed = inIsPostingAllowed;
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_SelectGroup;

	return true;
}

bool UNntpTransact::IsSelectedGroup(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!TRN->stSelectedGroup.psGroupName[0])
		return false;

	return true;
}

bool UNntpTransact::IsGroupPostingAllowed(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsSelectedGroup(inTrn))
		return false;

	return TRN->stSelectedGroup.bIsPostingAllowed;
}

bool UNntpTransact::GetSelectedGroup(TNntpTransact inTrn, SNewsGroupInfo& outGroupInfo)
{
	Require(inTrn);

	if (!IsSelectedGroup(inTrn))
	{
		ClearStruct(outGroupInfo);
		return false;
	}

	outGroupInfo = TRN->stSelectedGroup;
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpListArticles message
bool UNntpTransact::Command_ListArticles(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn))
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufXoverCommand[32];
	Uint32 nCommandSize = UText::Format(bufXoverCommand, sizeof(bufXoverCommand), "XOVER %lu-%lu\r\n", TRN->stSelectedGroup.nFirstArticleNumber, TRN->stSelectedGroup.nLastArticleNumber);
	
	TRN->tpt->Send(bufXoverCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_ListArticles;
	
	return true;
}

// generate msg_NntpListArticles message (this will list just ID's for new messages)
bool UNntpTransact::Command_ListArticles(TNntpTransact inTrn, const SCalendarDate& inSinceDate)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inSinceDate.IsValid())
		return false;
	
	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);

	SCalendarDate stSinceDate = inSinceDate + UDateTime::GetGMTDelta();

	Uint8 psYear[5];
	psYear[0] = UText::IntegerToText(psYear + 1, sizeof(psYear) - 1, stSinceDate.year);
	if (psYear[0] > 2)
		psYear[0] = UMemory::Copy(psYear + 1, psYear + psYear[0] - 1, 2);

	Uint8 bufNewnewsCommand[128];
	Uint32 nCommandSize = UText::Format(bufNewnewsCommand, sizeof(bufNewnewsCommand), "NEWNEWS %#s %#s%2.2li%2.2li %2.2li%2.2li%2.2li GMT\r\n", TRN->stSelectedGroup.psGroupName, psYear, stSinceDate.month, stSinceDate.day, stSinceDate.hour, stSinceDate.minute, stSinceDate.second);
	
	TRN->tpt->Send(bufNewnewsCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_ListArticles;
	
	return true;
}

Uint32 UNntpTransact::GetArticleCount(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (GetCommandID(inTrn) != kNntp_ListArticles)
		return 0;

	return TRN->pOffsetList.GetItemCount();
}

bool UNntpTransact::GetArticle(TNntpTransact inTrn, SNewsArticleInfo& outArticleInfo, Uint32 inIndex)
{
	Require(inTrn);

	ClearStruct(outArticleInfo);
	
	if (GetCommandID(inTrn) != kNntp_ListArticles || !inIndex)
		return false;
		
	const Uint8 *pFieldBegin, *pFieldEnd;
	if (!_GetNewsItem(inTrn, inIndex, pFieldBegin, pFieldEnd))
		return false;
	
	return _GetArticle(pFieldBegin, pFieldEnd, outArticleInfo);
}

bool UNntpTransact::GetNextArticle(TNntpTransact inTrn, SNewsArticleInfo& outArticleInfo, Uint32& ioIndex)
{
	Require(inTrn);

	ioIndex++;
	return GetArticle(inTrn, outArticleInfo, ioIndex);
}

bool UNntpTransact::GetPrevArticle(TNntpTransact inTrn, SNewsArticleInfo& outArticleInfo, Uint32& ioIndex)
{
	Require(inTrn);

	if (!ioIndex)
		return false;
	
	ioIndex--;
	return GetArticle(inTrn, outArticleInfo, ioIndex);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpSelectArticle message
bool UNntpTransact::Command_SelectArticle(TNntpTransact inTrn, Uint32 inArticleID)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn))
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufStatCommand[32];
	Uint32 nCommandSize = UText::Format(bufStatCommand, sizeof(bufStatCommand), "STAT %lu\r\n", inArticleID);

	TRN->tpt->Send(bufStatCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_SelectArticle;
	
	return true;
}

// generate msg_NntpSelectArticle message
bool UNntpTransact::Command_SelectNextArticle(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn))
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufNextCommand[6];
	Uint32 nCommandSize = UMemory::Copy(bufNextCommand, "NEXT\r\n", 6);
	
	TRN->tpt->Send(bufNextCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_SelectArticle;
	
	return true;
}

// generate msg_NntpSelectArticle message
bool UNntpTransact::Command_SelectPrevArticle(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn))
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufLastCommand[6];
	Uint32 nCommandSize = UMemory::Copy(bufLastCommand, "LAST\r\n", 6);
	
	TRN->tpt->Send(bufLastCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_SelectArticle;
	
	return true;
}

bool UNntpTransact::IsSelectedArticle(TNntpTransact inTrn)
{
	Require(inTrn);

//	Command_SelectGroup() select first article and set just TRN->nSelectedArticleNumber (TRN->psSelectedArticleID is empty)

	if (!TRN->nSelectedArticleNumber)
		return false;

	return true;
}

Uint32 UNntpTransact::GetSelectedArticleNumber(TNntpTransact inTrn)
{
	Require(inTrn);
	
	if (!IsSelectedArticle(inTrn))
		return 0;

	return TRN->nSelectedArticleNumber;
}

Uint32 UNntpTransact::GetSelectedArticleID(TNntpTransact inTrn, Uint8 *outArticleID, Uint32 inMaxSize)
{
	Require(inTrn);

	if (!IsSelectedArticle(inTrn) || !outArticleID || !inMaxSize)
		return 0;
	
	return UMemory::Copy(outArticleID, TRN->psSelectedArticleID + 1, (TRN->psSelectedArticleID[0] > inMaxSize ? inMaxSize : TRN->psSelectedArticleID[0]));
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpGetArticle message
bool UNntpTransact::Command_GetSelectedArticle(TNntpTransact inTrn)
{
	Require(inTrn);

	return Command_GetArticle(inTrn, TRN->nSelectedArticleNumber);
}

// generate msg_NntpGetArticle message
bool UNntpTransact::Command_GetArticle(TNntpTransact inTrn, Uint32 inArticleNumber)	// set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleNumber)
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufArticleCommand[32];
	Uint32 nCommandSize = UText::Format(bufArticleCommand, sizeof(bufArticleCommand), "ARTICLE %lu\r\n", inArticleNumber);
	
	TRN->tpt->Send(bufArticleCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticle;
	
	return true;
}

// generate msg_NntpGetArticle message
bool UNntpTransact::Command_GetArticle(TNntpTransact inTrn, const Uint8 *inArticleID)	// don't set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleID || !inArticleID[0])
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);

	Uint8 bufArticleCommand[128];
	Uint32 nCommandSize = UText::Format(bufArticleCommand, sizeof(bufArticleCommand), "ARTICLE <%#s>\r\n", inArticleID);
	
	TRN->tpt->Send(bufArticleCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticle;
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpGetArticleHeader message
bool UNntpTransact::Command_GetSelectedArticleHeader(TNntpTransact inTrn)
{
	Require(inTrn);

	return Command_GetArticleHeader(inTrn, TRN->nSelectedArticleNumber);
}

// generate msg_NntpGetArticleHeader message
bool UNntpTransact::Command_GetArticleHeader(TNntpTransact inTrn, Uint32 inArticleNumber)	// set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleNumber)
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufHeadCommand[32];
	Uint32 nCommandSize = UText::Format(bufHeadCommand, sizeof(bufHeadCommand), "HEAD %lu\r\n", inArticleNumber);
	
	TRN->tpt->Send(bufHeadCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticleHeader;
	
	return true;
}

// generate msg_NntpGetArticleHeader message
bool UNntpTransact::Command_GetArticleHeader(TNntpTransact inTrn, const Uint8 *inArticleID)	// don't set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleID || !inArticleID[0])
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);

	Uint8 bufHeadCommand[128];
	Uint32 nCommandSize = UText::Format(bufHeadCommand, sizeof(bufHeadCommand), "HEAD <%#s>\r\n", inArticleID);
	
	TRN->tpt->Send(bufHeadCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticleHeader;
	
	return true;
}

bool UNntpTransact::IsCurrentArticle(TNntpTransact inTrn)
{
	Require(inTrn);

	// stCurrentArticle.nArticleNumber is 0 when we use "inArticleID" to get the article
	if (!TRN->stCurrentArticle.psArticleID[0])
		return false;

	return true;
}

bool UNntpTransact::GetArticleInfo(TNntpTransact inTrn, SNewsArticleInfo& outArticleInfo)
{
	Require(inTrn);

	if (!IsCurrentArticle(inTrn))
	{
		ClearStruct(outArticleInfo);
		return false;
	}

	outArticleInfo = TRN->stCurrentArticle;
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpGetArticleData message
bool UNntpTransact::Command_GetSelectedArticleData(TNntpTransact inTrn)
{
	Require(inTrn);

	return 	Command_GetArticleData(inTrn, TRN->nSelectedArticleNumber);
}

// generate msg_NntpGetArticleData message
bool UNntpTransact::Command_GetArticleData(TNntpTransact inTrn, Uint32 inArticleNumber)	// set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleNumber)
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);

	Uint8 bufBodyCommand[32];
	Uint32 nCommandSize = UText::Format(bufBodyCommand, sizeof(bufBodyCommand), "BODY %lu\r\n", inArticleNumber);
	
	TRN->tpt->Send(bufBodyCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticleData;

	return true;
}

// generate msg_NntpGetArticleData message
bool UNntpTransact::Command_GetArticleData(TNntpTransact inTrn, const Uint8 *inArticleID)	// don't set server article pointer
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleID || !inArticleID[0])
		return false;

	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpCurrent(inTrn);

	Uint8 bufBodyCommand[128];
	Uint32 nCommandSize = UText::Format(bufBodyCommand, sizeof(bufBodyCommand), "BODY <%#s>\r\n", inArticleID);
	
	TRN->tpt->Send(bufBodyCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_GetArticleData;

	return true;
}

bool UNntpTransact::IsArticleText(TNntpTransact inTrn)
{
	Require(inTrn);

	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (TRN->pDataList.GetNext(pArticleData, i))
	{	
		if (!UMemory::Compare(pArticleData->psName + 1, pArticleData->psName[0], ".body", 5) &&
		    !UMemory::Compare(pArticleData->csFlavor, strlen(pArticleData->csFlavor), "text/plain", 10))
		    return true;
	}
	
	return false;
}

Uint32 UNntpTransact::GetDataSize(TNntpTransact inTrn)
{
	Require(inTrn);

	if (TRN->nCommandID != kNntp_GetArticle && TRN->nCommandID != kNntp_GetArticleData)
		return 0;

	if (!IsCommandExecuted(inTrn))
		return TRN->nBufferSize;

	Uint32 nDataSize = 0;

	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (TRN->pDataList.GetNext(pArticleData, i))
		nDataSize += pArticleData->nDataSize;
		
	return nDataSize;
}

Uint32 UNntpTransact::GetDataCount(TNntpTransact inTrn)
{
	Require(inTrn);

	return TRN->pDataList.GetItemCount();
}

SArticleData *UNntpTransact::GetData(TNntpTransact inTrn,  Uint32 inIndex)
{
	Require(inTrn);

	return TRN->pDataList.GetItem(inIndex);
}

bool UNntpTransact::GetNextData(TNntpTransact inTrn, SArticleData*& outArticleData, Uint32& ioIndex)
{
	Require(inTrn);

	return TRN->pDataList.GetNext(outArticleData, ioIndex);
}

bool UNntpTransact::GetPrevData(TNntpTransact inTrn, SArticleData*& outArticleData, Uint32& ioIndex)
{
	Require(inTrn);

	return TRN->pDataList.GetPrev(outArticleData, ioIndex);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpExistsArticle message
bool UNntpTransact::Command_ExistsArticle(TNntpTransact inTrn, const Uint8 *inArticleID)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || !IsSelectedGroup(inTrn) || !inArticleID || !inArticleID[0])
		return false;

	_ClearNntpBuffer(inTrn);

	Uint8 bufIHaveCommand[128];
	Uint32 nCommandSize = UText::Format(bufIHaveCommand, sizeof(bufIHaveCommand), "IHAVE <%#s>\r\n", inArticleID);
	
	TRN->tpt->Send(bufIHaveCommand, nCommandSize);
		
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_ExistsArticle;

	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

// generate msg_NntpPostArticle message
bool UNntpTransact::Command_PostArticleStart(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn))
		return false;
	
	_ClearNntpBuffer(inTrn);
	_ClearNntpPost(inTrn);
	
	TRN->pPostInfo = (SPostArticleInfo *)UMemory::NewClear(sizeof(SPostArticleInfo));

	Uint8 bufPostCommand[6];
	Uint32 nCommandSize = UMemory::Copy(bufPostCommand, "POST\r\n", 6);
	
	TRN->tpt->Send(bufPostCommand, nCommandSize);
	
	_ClearNntpFlags(inTrn);
	TRN->nCommandID = kNntp_PostArticle;

	return true;
}

// must to supply ID's for all parents in follow-up order
bool UNntpTransact::AddParentID(TNntpTransact inTrn, const Uint8 *inParentID)
{
	Require(inTrn);
	
	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inParentID || !inParentID[0])
		return false;
		
	Uint8 *pParentID = (Uint8 *)UMemory::New(inParentID[0] + 1);
	UMemory::Copy(pParentID, inParentID, inParentID[0] + 1);

	TRN->pPostInfo->pParentIDsList.AddItem(pParentID);
	
	return true;
}

bool UNntpTransact::SetArticleID(TNntpTransact inTrn, const Uint8 *inArticleID)
{
	Require(inTrn);
	
	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inArticleID || !inArticleID[0])
		return false;

	TRN->pPostInfo->psArticleID[0] = UMemory::Copy(TRN->pPostInfo->psArticleID + 1, inArticleID + 1, inArticleID[0] > sizeof(TRN->pPostInfo->psArticleID) - 1 ? sizeof(TRN->pPostInfo->psArticleID) - 1 : inArticleID[0]);
	return true;
}

bool UNntpTransact::SetOrganization(TNntpTransact inTrn, const Uint8 *inOrganization)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inOrganization || !inOrganization[0])
		return false;

	TRN->pPostInfo->psOrganization[0] = UMemory::Copy(TRN->pPostInfo->psOrganization + 1, inOrganization + 1, inOrganization[0] > sizeof(TRN->pPostInfo->psOrganization) - 1 ? sizeof(TRN->pPostInfo->psOrganization) - 1 : inOrganization[0]);
	return true;
}

bool UNntpTransact::SetNewsReader(TNntpTransact inTrn, const Uint8 *inNewsReader)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inNewsReader || !inNewsReader[0])
		return false;

	TRN->pPostInfo->psNewsReader[0] = UMemory::Copy(TRN->pPostInfo->psNewsReader + 1, inNewsReader + 1, inNewsReader[0] > sizeof(TRN->pPostInfo->psNewsReader) - 1 ? sizeof(TRN->pPostInfo->psNewsReader) - 1 : inNewsReader[0]);
	return true;
}

bool UNntpTransact::SetPostDate(TNntpTransact inTrn, SCalendarDate& inPostDate)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inPostDate.IsValid())
		return false;
	
	TRN->pPostInfo->stPostDate = inPostDate;
	return true;
}

bool UNntpTransact::SetPostInfo(TNntpTransact inTrn, const Uint8 *inArticleName, const Uint8 *inPosterName, const Uint8 *inPosterAddr)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inArticleName || !inArticleName[0] || !inPosterName || !inPosterName[0])
		return false;

	TRN->pPostInfo->psArticleName[0] = UMemory::Copy(TRN->pPostInfo->psArticleName + 1, inArticleName + 1, inArticleName[0] > sizeof(TRN->pPostInfo->psArticleName) - 1 ? sizeof(TRN->pPostInfo->psArticleName) - 1 : inArticleName[0]);
	TRN->pPostInfo->psPosterName[0] = UMemory::Copy(TRN->pPostInfo->psPosterName + 1, inPosterName + 1, inPosterName[0] > sizeof(TRN->pPostInfo->psPosterName) - 1 ? sizeof(TRN->pPostInfo->psPosterName) - 1 : inPosterName[0]);

	if (inPosterAddr && inPosterAddr[0])
		TRN->pPostInfo->psPosterAddr[0] = UMemory::Copy(TRN->pPostInfo->psPosterAddr + 1, inPosterAddr + 1, inPosterAddr[0] > sizeof(TRN->pPostInfo->psPosterAddr) - 1 ? sizeof(TRN->pPostInfo->psPosterAddr) - 1 : inPosterAddr[0]);

	return true;
}

// take ownership of inData
bool UNntpTransact::AddPostData(TNntpTransact inTrn, const Int8 *inFlavor, const Uint8 *inName, void *inData, Uint32 inDataSize)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!inFlavor || !strlen(inFlavor) || !inName || !inName[0] || !inData || !inDataSize)
		return false;
	
	SArticleData *pArticleData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
	
	pArticleData->csFlavor[UMemory::Copy(pArticleData->csFlavor, inFlavor, strlen(inFlavor) > sizeof(pArticleData->csFlavor) - 1 ?  sizeof(pArticleData->csFlavor) - 1 : strlen(inFlavor))] = 0;
	pArticleData->psName[0] = UMemory::Copy(pArticleData->psName + 1, inName + 1, inName[0] > sizeof(pArticleData->psName) - 1 ? sizeof(pArticleData->psName) - 1 : inName[0]);
	pArticleData->nDataSize = inDataSize;
	pArticleData->pData = inData;

	TRN->pPostInfo->pDataList.AddItem(pArticleData);
	
	return true;
}

// generate msg_NntpPostArticle message
bool UNntpTransact::Command_PostArticleFinish(TNntpTransact inTrn)
{
	Require(inTrn);

	if (!IsConnected(inTrn) || !IsCommandExecuted(inTrn) || IsCommandError(inTrn) || GetCommandID(inTrn) != kNntp_PostArticle || !TRN->pPostInfo)
		return false;

	if (!_SetPostArticleData(inTrn))
	{
		_ClearNntpBuffer(inTrn);
		return false;
	}

	TRN->tpt->Send(TRN->pBuffer, TRN->nBufferSize);

	_ClearNntpFlags(inTrn);
	_ClearNntpBuffer(inTrn);
	_ClearNntpPost(inTrn);
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static void _NntpMessageHandler(void *inContext, void */*inObject*/, Uint32 inMsg, const void *inData, Uint32 inDataSize)
{
	SNntpTransact *pNntpTransact = (SNntpTransact *)inContext;
	
	switch (inMsg)
	{
		case msg_ConnectionEstablished:		
			return;		// don't post msg_ConnectionEstablished here				

		case msg_ConnectionRefused:
			_ClearNntpTransport((TNntpTransact)pNntpTransact);
			_ClearNntpBuffer((TNntpTransact)pNntpTransact);
			break;

		case msg_DataArrived:
		case msg_ConnectionClosed:
			if (pNntpTransact->tpt && !pNntpTransact->bIsCommandExecuted)
			{
				Uint32 nRecSize = pNntpTransact->tpt->GetReceiveSize();
		    	if (!nRecSize)
		    	{
    				if (!pNntpTransact->tpt->IsConnected())
		    			pNntpTransact->bIsCommandExecuted = true;
		    	
		    		break;
				}
			
				try
				{
					if (!pNntpTransact->pBuffer)
						pNntpTransact->pBuffer = UMemory::NewClear(nRecSize);
					else
						pNntpTransact->pBuffer = UMemory::Reallocate((TPtr)pNntpTransact->pBuffer, pNntpTransact->nBufferSize + nRecSize);
				}
				catch(...)
				{
					_ClearNntpTransport((TNntpTransact)pNntpTransact);
					_ClearNntpBuffer((TNntpTransact)pNntpTransact);
				
					pNntpTransact->bIsCommandExecuted = true;
					_ProcessErrors((TNntpTransact)pNntpTransact, false, true);
					UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpError, nil, 0, priority_Normal);

					throw;
    			}
    			
    			pNntpTransact->nBufferSize += pNntpTransact->tpt->Receive((Uint8 *)pNntpTransact->pBuffer + pNntpTransact->nBufferSize, nRecSize);
    			
    			switch (pNntpTransact->nCommandID)
    			{
    				case kNntp_StartConnect:
    					if (_Process_StartConnect((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_ConnectionEstablished, nil, 0, priority_Normal);
    						return;
    					}
    					break;
    			
    				case kNntp_UserLogin:
    					if (_Process_UserLogin((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_ConnectionEstablished, nil, 0, priority_Normal);
    						return;
    					}
    					break;

    				case kNntp_UserPassword:
    					if (_Process_UserPassword((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_ConnectionEstablished, nil, 0, priority_Normal);
    						return;
    					}
    					break;

    				case kNntp_ListGroups:
    					if (_Process_ListGroups((TNntpTransact)pNntpTransact) || !pNntpTransact->bIsCommandError)
    					{
  							// this message is posted every time we receive more data
  							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpListGroups, nil, 0, priority_Normal);
    						return;
    					}
    					break;

    				case kNntp_SelectGroup:
    					if (_Process_SelectGroup((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpSelectGroup, nil, 0, priority_Normal);
    						return;
    					}
    					break;
    					
    				case kNntp_ListArticles:
      					if (_Process_ListArticles((TNntpTransact)pNntpTransact) || !pNntpTransact->bIsCommandError)
      					{
  							// this message is posted every time we receive more data
							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpListArticles, nil, 0, priority_Normal);
    						return;
    					}
    					break;

  					case kNntp_SelectArticle:
    					if (_Process_SelectArticle((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpSelectArticle, nil, 0, priority_Normal);
    						return;
    					}
    					break;

      				case kNntp_GetArticle:
    					if (_Process_GetArticle((TNntpTransact)pNntpTransact) || !pNntpTransact->bIsCommandError)
    					{
  							// this message is posted every time we receive more data
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpGetArticle, nil, 0, priority_Normal);
    						return;
    					}
    					break;
			
    				case kNntp_GetArticleHeader:
    					if (_Process_GetArticleHeader((TNntpTransact)pNntpTransact))
   						{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpGetArticleHeader, nil, 0, priority_Normal);
    						return;
    					}
    					break;
		
    				case kNntp_GetArticleData:
    					if (_Process_GetArticleData((TNntpTransact)pNntpTransact) || !pNntpTransact->bIsCommandError)
    					{
  							// this message is posted every time we receive more data
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpGetArticleData, nil, 0, priority_Normal);
   							return;
   						}
   						break; 
   						
    				case kNntp_ExistsArticle:
    					if (_Process_ExistsArticle((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpExistsArticle, nil, 0, priority_Normal);
   							return;
   						}
   						break;  					 					

    				case kNntp_PostArticle:
    					if (_Process_PostArticle((TNntpTransact)pNntpTransact))
    					{
   							UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpPostArticle, nil, 0, priority_Normal);
   							return;
   						}
   						break;  					 					
    			};
			}
    		
    		if (pNntpTransact->bIsCommandError)
				UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_NntpError, nil, 0, priority_Normal);

    		if (inMsg == msg_ConnectionClosed)
    			UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, msg_ConnectionClosed, nil, 0, priority_Normal);
    		
    		return;
    		
//		case msg_DataTimeOut:
//		case msg_ReadyToSend:
//		case msg_ConnectionRequest:
    };

	UNntpTransact::PostMessage((TNntpTransact)pNntpTransact, inMsg, inData, inDataSize, priority_Normal);
}


/* -------------------------------------------------------------------------- */
#pragma mark -

static void _StartNntpConnect(TNntpTransact inTrn, Uint16 inPort, Uint8 *inAddress, Uint32 inMaxSecs)
{
	Require(inTrn);

	SInternetNameAddress stAddress;
	stAddress.type = kInternetNameAddressType;
	stAddress.port = inPort;
	UMemory::Copy(stAddress.name, inAddress, inAddress[0] + 1);

	// a TTransport can only be connected ONCE so we must make a new one
	UNntpTransact::MakeNewTransport(inTrn);
	UTransport::StartConnect(TRN->tpt, &stAddress, stAddress.name[0] + 5, inMaxSecs);
}

static bool _CheckServer(TNntpTransact inTrn, const Uint8 *inServerAddr, const Uint8 *inLogin, const Uint8 *inPassword, bool& outSameServer)
{
	outSameServer = false;
	Require(inTrn);

	if (!inServerAddr || !inServerAddr[0])
		return false;
	
	if (!UText::CompareInsensitive(inServerAddr + 1, inServerAddr[0], TRN->psServerAddr + 1, TRN->psServerAddr[0]) &&
	   (((!inLogin || !inLogin[0]) && !TRN->psUserLogin[0]) || (inLogin && inLogin[0] && !UMemory::Compare(inLogin + 1, inLogin[0], TRN->psUserLogin + 1, TRN->psUserLogin[0]))) &&
	   (((!inPassword || !inPassword[0]) && !TRN->psUserPassword[0]) || (inPassword && inPassword[0] && !UMemory::Compare(inPassword + 1, inPassword[0], TRN->psUserPassword + 1, TRN->psUserPassword[0]))))
	{		
		outSameServer = true;
		return true;
	}
	
	TRN->psServerAddr[0] = UMemory::Copy(TRN->psServerAddr + 1, inServerAddr + 1, inServerAddr[0] > sizeof(TRN->psServerAddr) - 1 ? sizeof(TRN->psServerAddr) - 1 : inServerAddr[0]);

	if (inLogin && inLogin[0])
		TRN->psUserLogin[0] = UMemory::Copy(TRN->psUserLogin + 1, inLogin + 1, inLogin[0] > sizeof(TRN->psUserLogin) - 1 ? sizeof(TRN->psUserLogin) - 1 : inLogin[0]);
	else
		TRN->psUserLogin[0] = 0;

	if (inPassword && inPassword[0])
		TRN->psUserPassword[0] = UMemory::Copy(TRN->psUserPassword + 1, inPassword + 1, inPassword[0] > sizeof(TRN->psUserPassword) - 1 ? sizeof(TRN->psUserPassword) - 1 : inPassword[0]);
	else
		TRN->psUserPassword[0] = 0;

	return true;
}

// Wdy, DD Mon YYYY HH:MM:SS (+,-)HHMM
// DD Mon YYYY HH:MM:SS GMT 
// Wdy, DD Mon YYYY HH:MM:SS
static bool _ConvertArticleDate(const Uint8 *inDate, SCalendarDate& outDate)
{
	ClearStruct(outDate);

	if (!inDate || !inDate[0])
		return false;
	
	const Uint8 *pDateEnd = inDate + inDate[0] + 1;

	Uint8 nWeekDay = 0;
	const Uint8 *pWeekDayBegin = inDate + 1;
	const Uint8 *pWeekDayEnd = UMemory::Search(", ", 2, pWeekDayBegin, pDateEnd - pWeekDayBegin);
	if (pWeekDayEnd)
	{
		Uint8 *pWeekDayList1[7] = {"\pMonday", "\pTuesday", "\pWednesday", "\pThursday", "\pFriday", "\pSaturday", "\pSunday"};
		Int8 *pWeekDayList2[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

		while (nWeekDay < 7)
		{
			if (!UText::CompareInsensitive(pWeekDayBegin, pWeekDayEnd - pWeekDayBegin, pWeekDayList1[nWeekDay] + 1, pWeekDayList1[nWeekDay][0]) ||
				!UText::CompareInsensitive(pWeekDayBegin, pWeekDayEnd - pWeekDayBegin, pWeekDayList2[nWeekDay], 3))
				break;
				
			nWeekDay++;
		}
	
		if (nWeekDay >= 7)
			nWeekDay = 0;		// unknown weekday
		else
			nWeekDay++;
	}

	const Uint8 *pDayBegin;
	if (pWeekDayEnd)
		pDayBegin = pWeekDayEnd + 2;
	else
		pDayBegin = inDate + 1;
	
	const Uint8 *pDayEnd = UMemory::SearchByte(' ', pDayBegin, pDateEnd - pDayBegin);
	if (!pDayEnd)
		return false;
	
	Int8 nDay = UText::TextToInteger(pDayBegin, pDayEnd - pDayBegin, 0, 10);
	if (nDay < 1 || nDay > 31)
		return false;

	const Uint8 *pMonthBegin = pDayEnd + 1;
	const Uint8 *pMonthEnd = UMemory::SearchByte(' ', pMonthBegin, pDateEnd - pMonthBegin);
	if (!pMonthEnd || pMonthEnd - pMonthBegin != 3)
		return false;
	
	Int8 *pMonthList[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	Uint8 nMonth = 0;
	while (nMonth < 12)
	{
		if (!UText::CompareInsensitive(pMonthBegin, pMonthList[nMonth], 3))
			break;
			
		nMonth++;
	}
	
	if (nMonth >= 12)
		return false;
	
	nMonth++;
		
	const Uint8 *pYearBegin = pMonthEnd + 1;
	const Uint8 *pYearEnd = UMemory::SearchByte(' ', pYearBegin, pDateEnd - pYearBegin);
	if (!pYearEnd)
		return false;

	Int16 nYear = UText::TextToInteger(pYearBegin, pYearEnd - pYearBegin);
	if (nYear <= 0)
		return false;

	const Uint8 *pHourBegin = pYearEnd + 1;
	const Uint8 *pHourEnd = UMemory::SearchByte(':', pHourBegin, pDateEnd - pHourBegin);
	if (!pHourEnd)
		return false;

	Int16 nHour = UText::TextToInteger(pHourBegin, pHourEnd - pHourBegin, 0, 10);
	if (nHour < 0 || nHour > 23)
		return false;

	const Uint8 *pMinuteBegin = pHourEnd + 1;
	const Uint8 *pMinuteEnd = UMemory::SearchByte(':', pMinuteBegin, pDateEnd - pMinuteBegin);
	if (!pMinuteEnd)
		return false;

	Int16 nMinute = UText::TextToInteger(pMinuteBegin, pMinuteEnd - pMinuteBegin, 0, 10);
	if (nMinute < 0 || nMinute > 59)
		return false;

	const Uint8 *pSecondBegin = pMinuteEnd + 1;
	const Uint8 *pSecondEnd = UMemory::SearchByte(' ', pSecondBegin, pDateEnd - pSecondBegin);
	if (!pSecondEnd)
	{
		if (pDateEnd - pSecondBegin > 2)
			return false;
			
		pSecondEnd = pDayEnd;
	}

	Int16 nSecond = UText::TextToInteger(pSecondBegin, pSecondEnd - pSecondBegin, 0, 10);
	if (nSecond < 0 || nSecond > 59)
		return false;

	outDate.year = nYear;
	outDate.month = nMonth;
	outDate.day = nDay;
	outDate.hour = nHour;
	outDate.minute = nMinute;
	outDate.second = nSecond;
	
	if (nWeekDay)
		outDate.weekDay = nWeekDay;
	else
		outDate.weekDay= UDateTime::CalculateWeekDay(outDate.year, outDate.month, outDate.day);

	// convert from GMT to local time
	Int32 nGMTDelta = UDateTime::GetGMTDelta();
	
	if (pDayEnd > pSecondEnd)
	{
		const Uint8 *pTimeDepl = pSecondEnd + 1;
		if (pDateEnd - pTimeDepl >= 5 && (*pTimeDepl == '+' || *pTimeDepl == '-'))
		{	
			Uint32 nSecondsDepl = (UText::TextToInteger(pTimeDepl + 1, 2, 0, 10) * 60 + UText::TextToInteger(pTimeDepl + 3, 2, 0, 10)) * 60;
		
			if (*pTimeDepl == '+')	// reverse sign
				nGMTDelta -= nSecondsDepl;
			else
				nGMTDelta += nSecondsDepl;
		}
	}
		
	outDate += nGMTDelta;
	return true;
}

// Wdy, DD Mon YYYY HH:MM:SS (+,-)HHMM
static Uint32 _ConvertPostDate(SCalendarDate& inPostDate, Uint8 *outDate, Uint32 inMaxSize)
{
	if (!outDate || !inMaxSize)
		return 0;
	
	Uint8 psTimeDepl[6];
	psTimeDepl[0] = 5;
	Int32 nGMTDelta = UDateTime::GetGMTDelta();
	
	Uint32 nTimeDepl;
	if (nGMTDelta >= 0)
	{
		nTimeDepl = nGMTDelta;
		psTimeDepl[1] = '+';
	}
	else
	{
		nTimeDepl = -nGMTDelta;
		psTimeDepl[1] = '-';
	}

	Uint32 nHourDepl = nTimeDepl/3600;
	nTimeDepl -= nHourDepl * 3600;
	Uint32 nMinuteDepl = nTimeDepl/60;
		
	UText::Format(psTimeDepl + 2, 4, "%2.2lu%2.2lu", nHourDepl, nMinuteDepl);
		
	if (!inPostDate.IsValid())
		UDateTime::GetCalendarDate(calendar_Gregorian, inPostDate);
		
	Uint8 *pWeekDayList[7] = {"\pMon", "\pTue", "\pWed", "\pThu", "\pFri", "\pSat", "\pSun"};
	Uint8 *pMonthList[12] = {"\pJan", "\pFeb", "\pMar", "\pApr", "\pMay", "\pJun", "\pJul", "\pAug", "\pSep", "\pOct", "\pNov", "\pDec"};
	
	return UText::Format(outDate, inMaxSize, "%#s, %li %#s %li %2.2li:%2.2li:%2.2li %#s", pWeekDayList[inPostDate.weekDay - 1], inPostDate.day, pMonthList[inPostDate.month - 1], inPostDate.year, inPostDate.hour, inPostDate.minute, inPostDate.second, psTimeDepl);
}

static void _ProcessErrors(TNntpTransact inTrn, bool inCorruptData, bool inNotEnoughMemory)
{
	if (!TRN->bIsCommandError)
		TRN->bIsCommandError = true;

	if (inCorruptData)
		TRN->psCommandError[0] = UMemory::Copy(TRN->psCommandError + 1, "Received corrupt data", 21);
	else if (inNotEnoughMemory)
		TRN->psCommandError[0] = UMemory::Copy(TRN->psCommandError + 1, "Not enough memory", 17);
	else if (TRN->pBuffer && TRN->nBufferSize > 6)
		TRN->psCommandError[0] = UMemory::Copy(TRN->psCommandError + 1, (Uint8 *)TRN->pBuffer + 4, TRN->nBufferSize - 6 > sizeof(TRN->psCommandError) - 1 ? sizeof(TRN->psCommandError) - 1 : TRN->nBufferSize - 6);
	else
		TRN->psCommandError[0] = 0;
	
	_ClearNntpBuffer(inTrn);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static bool _Process_StartConnect(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	if (!UMemory::Compare(TRN->pBuffer, "200", 3))			// "200 server ready - posting allowed"
		TRN->bIsPostingAllowed = true;
	else if (!UMemory::Compare(TRN->pBuffer, "201", 3))		// "201 server ready - no posting allowed"
		TRN->bIsPostingAllowed = false;
	else
		TRN->bIsCommandError = true;
		
	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		TRN->bIsCommandExecuted = true;
		return false;
	}
	
	_ClearNntpBuffer(inTrn);
			
	if (TRN->psUserLogin[0])
	{
		Uint8 bufLogin[64];
		Uint32 nCommandSize = UText::Format(bufLogin, sizeof(bufLogin), "AUTHINFO USER %#s\r\n", TRN->psUserLogin);
	
		TRN->tpt->Send(bufLogin, nCommandSize);
			
		TRN->nCommandID = kNntp_UserLogin;
		return false;
	}
	
	TRN->nCommandID = kNntp_Connected;
	TRN->bIsCommandExecuted = true;
	
	return true;
}

static bool _Process_UserLogin(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	bool bSendPassword;
	if (!UMemory::Compare(TRN->pBuffer, "381", 3))		// "381 PASS required" "381 More Authentication Required"
		bSendPassword = true;
	else if (!UMemory::Compare(TRN->pBuffer, "281", 3))	// "281 Authentication Accepted"
		bSendPassword = false;
	else
		TRN->bIsCommandError = true;
		
	if (TRN->bIsCommandError || (bSendPassword && !TRN->psUserPassword[0]))
	{
		_ProcessErrors(inTrn);		
		TRN->bIsCommandExecuted = true;
		return false;
	}
	
	_ClearNntpBuffer(inTrn);
		
	if (bSendPassword)
	{
		Uint8 bufPassword[64];
		Uint32 nCommandSize = UText::Format(bufPassword, sizeof(bufPassword), "AUTHINFO PASS %#s\r\n", TRN->psUserPassword);
	
		TRN->tpt->Send(bufPassword, nCommandSize);
		
		TRN->nCommandID = kNntp_UserPassword;
		return false;
	}
	
	TRN->nCommandID = kNntp_Connected;
	TRN->bIsCommandExecuted = true;
	
	return true;
}

static bool _Process_UserPassword(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	TRN->bIsCommandExecuted = true;

	if (UMemory::Compare(TRN->pBuffer, "281", 3))	// "281 Authentication Accepted"
	{
		_ProcessErrors(inTrn);	
		return false;
	}
	
	_ClearNntpBuffer(inTrn);
	
	TRN->nCommandID = kNntp_Connected;
	return true;
}

static bool _Process_ListGroups(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3)
		false;

	// "215 list of newsgroups follows" "231 list of new newsgroups follows"
	if (!UMemory::Compare(TRN->pBuffer, "215", 3) || !UMemory::Compare(TRN->pBuffer, "231", 3))
	{
		if (!_SetOffsetList(inTrn))
	 		return false;	
	}
	else 
	{
		if (!UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
 			return false;
 			
 		TRN->bIsCommandError = true;
 	}

	TRN->bIsCommandExecuted = true;

	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		return false;
	}

	return true;
}

static bool _Process_SelectGroup(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	TRN->bIsCommandExecuted = true;

	if (UMemory::Compare(TRN->pBuffer, "211", 3))	// "211 'num' 'first' 'last' 'name' group selected"
	{
		_ProcessErrors(inTrn);
		ClearStruct(TRN->stSelectedGroup);

		return false;
	}
	
	if (!_GetSelectedGroup((Uint8 *)TRN->pBuffer, (Uint8 *)TRN->pBuffer + TRN->nBufferSize, TRN->stSelectedGroup))
	{
		_ProcessErrors(inTrn, true);
		ClearStruct(TRN->stSelectedGroup);

		return false;
	}

	TRN->nSelectedArticleNumber = TRN->stSelectedGroup.nFirstArticleNumber;
	TRN->psSelectedArticleID[0] = 0;	//	don't have any information about article ID
	
	_ClearNntpBuffer(inTrn);
	return true;
}

static bool _Process_ListArticles(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3)
		false;

	// "224 Overview information Follows" "230 list of new articles by message-id follows"
	if (!UMemory::Compare(TRN->pBuffer, "224", 3) || !UMemory::Compare(TRN->pBuffer, "230", 3))
	{
		if (!_SetOffsetList(inTrn))
	 		return false;	
	}
	else 
	{
		if (!UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
 			return false;
 			
 		TRN->bIsCommandError = true;
 	}

	TRN->bIsCommandExecuted = true;

	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		return false;
	}

	return true;
}

static bool _Process_SelectArticle(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	TRN->bIsCommandExecuted = true;

	if (UMemory::Compare(TRN->pBuffer, "223", 3))		// "223 'num' <'id'> article retrieved - request text separately"
	{
		_ProcessErrors(inTrn);
		_ClearNntpSelected(inTrn);
		
		return false;
	}

	if (!_GetArticleID((Uint8 *)TRN->pBuffer, (Uint8 *)TRN->pBuffer + TRN->nBufferSize, TRN->nSelectedArticleNumber, TRN->psSelectedArticleID, sizeof(TRN->psSelectedArticleID)))
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpSelected(inTrn);
		
		return false;
	}

	_ClearNntpBuffer(inTrn);
	return true;
}

static bool _Process_GetArticle(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3)
		false;

	// "220 'num' <'id'> article retrieved - head and body follow"
	if (!UMemory::Compare(TRN->pBuffer, "220", 3) || !UMemory::Compare(TRN->pBuffer, "221", 3) || !UMemory::Compare(TRN->pBuffer, "222", 3))
	{
		if (!UMemory::Search("\r\n.\r\n", 5, TRN->pBuffer, TRN->nBufferSize))
	 		return false;	
	}
	else 
	{
		if (!UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
 			return false;
 			
 		TRN->bIsCommandError = true;
 	}

	TRN->bIsCommandExecuted = true;

	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);

		return false;
	}
	
	if (!UMemory::Compare(TRN->pBuffer, "221", 3))			// "221 'num' <'id'> article retrieved - head follows"
		return _Process_GetArticleHeader(inTrn);
	else if (!UMemory::Compare(TRN->pBuffer, "222", 3))		// "222 'num' <'id'> article retrieved - body follows"
		return _Process_GetArticleData(inTrn);

	const Uint8 *pArticleData = UMemory::Search("\r\n\r\n", 4, TRN->pBuffer, TRN->nBufferSize);
	if (!pArticleData)
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);
		
		return false;
	}

	pArticleData += 2;
	Uint32 nHeaderSize = pArticleData - TRN->pBuffer;

	if (!_GetCurrentArticle((Uint8 *)TRN->pBuffer, (Uint8 *)TRN->pBuffer + nHeaderSize, TRN->stCurrentArticle, TRN->stCurrentEncoding))
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);
		
		return false;
	}

	if (!TRN->nSelectedArticleNumber)
	{
		TRN->nSelectedArticleNumber = TRN->stCurrentArticle.nArticleNumber;
		UMemory::Copy(TRN->psSelectedArticleID, TRN->stCurrentArticle.psArticleID, TRN->stCurrentArticle.psArticleID[0] + 1);
	}
	
	pArticleData += 2;
	if (!_GetArticleData(inTrn, pArticleData, (Uint8 *)TRN->pBuffer + TRN->nBufferSize))
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);
		
		return false;
	}
	
	_ClearNntpBuffer(inTrn);
	return true;
}

static bool _Process_GetArticleHeader(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3)
		false;

	if (!UMemory::Compare(TRN->pBuffer, "221", 3))		// "221 'num' <'id'> article retrieved - head follows"
	{
		if (!UMemory::Search("\r\n.\r\n", 5, TRN->pBuffer, TRN->nBufferSize))
	 		return false;	
	}
	else 
	{
		if (!UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
 			return false;
 			
 		TRN->bIsCommandError = true;
 	}

	TRN->bIsCommandExecuted = true;

	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);
		
		return false;
	}

	if (!_GetCurrentArticle((Uint8 *)TRN->pBuffer, (Uint8 *)TRN->pBuffer + TRN->nBufferSize, TRN->stCurrentArticle, TRN->stCurrentEncoding))
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		_ClearNntpCurrent(inTrn);
		
		return false;
	}

	if (!TRN->nSelectedArticleNumber)
	{
		TRN->nSelectedArticleNumber = TRN->stCurrentArticle.nArticleNumber;
		UMemory::Copy(TRN->psSelectedArticleID, TRN->stCurrentArticle.psArticleID, TRN->stCurrentArticle.psArticleID[0] + 1);
	}

	_ClearNntpBuffer(inTrn);
	return true;
}

static bool _Process_GetArticleData(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3)
		false;

	if (!UMemory::Compare(TRN->pBuffer, "222", 3))		// "222 'num' <'id'> article retrieved - body follows"
	{
		if (!UMemory::Search("\r\n.\r\n", 5, TRN->pBuffer, TRN->nBufferSize))
	 		return false;	
	}
	else 
	{
		if (!UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
 			return false;
 			
 		TRN->bIsCommandError = true;
 	}

	TRN->bIsCommandExecuted = true;

	if (TRN->bIsCommandError)
	{
		_ProcessErrors(inTrn);
		_ClearNntpData(inTrn);
		
		return false;
	}

	const Uint8 *pArticleData = UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize);
	if (!pArticleData)
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		
		return false;
	}

	pArticleData += 2;
	_GetArticleID((Uint8 *)TRN->pBuffer, pArticleData, TRN->stCurrentArticle.nArticleNumber, TRN->stCurrentArticle.psArticleID, sizeof(TRN->stCurrentArticle.psArticleID));
	
	if (!TRN->nSelectedArticleNumber)
	{
		TRN->nSelectedArticleNumber = TRN->stCurrentArticle.nArticleNumber;
		UMemory::Copy(TRN->psSelectedArticleID, TRN->stCurrentArticle.psArticleID, TRN->stCurrentArticle.psArticleID[0] + 1);
	}

	if (!UMemory::Compare(pArticleData, "\r\n", 2))
		pArticleData += 2;
	
	if (!_GetArticleData(inTrn, pArticleData, (Uint8 *)TRN->pBuffer + TRN->nBufferSize))
	{
		_ProcessErrors(inTrn, true);
		_ClearNntpData(inTrn);
		
		return false;
	}
	
	_ClearNntpBuffer(inTrn);	
	return true;
}

static bool _Process_ExistsArticle(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	TRN->bIsCommandExecuted = true;

	if (UMemory::Compare(TRN->pBuffer, "335", 3))	// "335 send article to be transferred"
	{
		_ProcessErrors(inTrn);
		return false;
	}

	_ClearNntpBuffer(inTrn);
	return true;
}

static bool _Process_PostArticle(TNntpTransact inTrn)
{
	if (TRN->nBufferSize <= 3 || !UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize))
	 	return false;

	TRN->bIsCommandExecuted = true;

	// "240 article posted ok" 
	// "340 send article to be posted"
	if (UMemory::Compare(TRN->pBuffer, "240", 3) && UMemory::Compare(TRN->pBuffer, "340", 3))
	{
		_ProcessErrors(inTrn);
		return false;
	}

	_ClearNntpBuffer(inTrn);
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static bool _SetOffsetList(TNntpTransact inTrn)
{
	if (!TRN->pBuffer || !TRN->nBufferSize)
		return false;

	const Uint8 *pFieldBegin;
	Uint32 nCount = TRN->pOffsetList.GetItemCount();
		
	if (nCount)
	{
		Uint32 *pLastOffset = TRN->pOffsetList.GetItem(nCount);
		if (!pLastOffset)
			return false;
		
		pFieldBegin = (Uint8 *)TRN->pBuffer + *pLastOffset;
	}
	else	
	{
		pFieldBegin = UMemory::Search("\r\n", 2, TRN->pBuffer, TRN->nBufferSize);
		if (!pFieldBegin)
			return false;
			
		pFieldBegin += 2;
	}

	if (!UMemory::Compare(pFieldBegin, ".\r\n", 3))
		return true;				

	Uint8 *pDataEnd = (Uint8 *)TRN->pBuffer + TRN->nBufferSize;
	
	while (true)
	{
		pFieldBegin = UMemory::Search("\r\n", 2, pFieldBegin, pDataEnd - pFieldBegin);
		if (!pFieldBegin)
			break;

		pFieldBegin += 2;
		if (!UMemory::Compare(pFieldBegin, ".\r\n", 3))
			return true;				
	
		Uint32 *pOffset = (Uint32 *)UMemory::New(sizeof(Uint32));
		*pOffset = pFieldBegin - TRN->pBuffer;
		
		TRN->pOffsetList.AddItem(pOffset);		
	}

	return false;
}

static bool _GetNewsItem(TNntpTransact inTrn, Uint32 inIndex, const Uint8 *&outFieldBegin, const Uint8 *&outFieldEnd)
{
	if (!TRN->pBuffer || !TRN->nBufferSize)
		return false;

	Uint32 *pOffset = TRN->pOffsetList.GetItem(inIndex);
	if (!pOffset || *pOffset > TRN->nBufferSize)
		return false;

	const Uint8 *pFieldBegin = (Uint8 *)TRN->pBuffer + *pOffset;
	const Uint8 *pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, (Uint8 *)TRN->pBuffer + TRN->nBufferSize - pFieldBegin);
	if (!pFieldEnd)
		return false;

	outFieldBegin = pFieldBegin;
	outFieldEnd = pFieldEnd;

	return true;
}

static bool _GetGroup(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsGroupInfo& outGroupInfo)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;
		
	// search group name
	const Uint8 *pNameEnd = UMemory::SearchByte(' ', inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pNameEnd)
		return false;
		
	outGroupInfo.psGroupName[0] = UMemory::Copy(outGroupInfo.psGroupName + 1, inFieldBegin, pNameEnd - inFieldBegin > sizeof(outGroupInfo.psGroupName) - 1 ? sizeof(outGroupInfo.psGroupName) - 1 :  pNameEnd - inFieldBegin);
	
	// search last article number
	const Uint8 *pLastNumberBegin = pNameEnd + 1;
	const Uint8 *pLastNumberEnd = UMemory::SearchByte(' ', pLastNumberBegin, inFieldEnd - pLastNumberBegin);
	if (!pLastNumberEnd)
		return false;

	outGroupInfo.nLastArticleNumber = UText::TextToUInteger(pLastNumberBegin, pLastNumberEnd - pLastNumberBegin, 0, 10);

	// search first article number
	const Uint8 *pFirstNumberBegin = pLastNumberEnd + 1;
	const Uint8 *pFirstNumberEnd = UMemory::SearchByte(' ', pFirstNumberBegin, inFieldEnd - pFirstNumberBegin);
	if (!pFirstNumberEnd)
		return false;

	outGroupInfo.nFirstArticleNumber = UText::TextToUInteger(pFirstNumberBegin, pFirstNumberEnd - pFirstNumberBegin, 0, 10);

	// don't have info about estimated number of articles in group so calculate if posible
	if (outGroupInfo.nLastArticleNumber > outGroupInfo.nFirstArticleNumber)
		outGroupInfo.nArticleCount = outGroupInfo.nLastArticleNumber - outGroupInfo.nFirstArticleNumber;
	else
		outGroupInfo.nArticleCount = 0;

	// set posting flag
	const Uint8 *pPostingAllowed = pFirstNumberEnd + 1;
	if (pPostingAllowed >= inFieldEnd)
		return false;

	if (*pPostingAllowed == 'n')	// can be 'y'(allowed), 'n'(not allowed), 'm'(???)
		outGroupInfo.bIsPostingAllowed = false;
	else
		outGroupInfo.bIsPostingAllowed = true;

	return true;
}

static bool _GetSelectedGroup(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsGroupInfo& outGroupInfo)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;

	// skip response ID (211)
	const Uint8 *pCountBegin = UMemory::SearchByte(' ', inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pCountBegin)
		return false;
	
	// search estimated number of articles in group
	pCountBegin++;
	const Uint8 *pCountEnd = UMemory::SearchByte(' ', pCountBegin, inFieldEnd - pCountBegin);
	if (!pCountEnd)
		return false;

	outGroupInfo.nArticleCount = UText::TextToUInteger(pCountBegin, pCountEnd - pCountBegin);

	// search first article number
	const Uint8 *pFirstNumberBegin = pCountEnd + 1;
	const Uint8 *pFirstNumberEnd = UMemory::SearchByte(' ', pFirstNumberBegin, inFieldEnd - pFirstNumberBegin);
	if (!pFirstNumberEnd)
		return false;

	outGroupInfo.nFirstArticleNumber = UText::TextToUInteger(pFirstNumberBegin, pFirstNumberEnd - pFirstNumberBegin);
	
	// search last article number
	const Uint8 *pLastNumberBegin = pFirstNumberEnd + 1;
	const Uint8 *pLastNumberEnd = UMemory::SearchByte(' ', pLastNumberBegin, inFieldEnd - pLastNumberBegin);
	if (!pLastNumberEnd)
		return false;

	outGroupInfo.nLastArticleNumber = UText::TextToUInteger(pLastNumberBegin, pLastNumberEnd - pLastNumberBegin);

	// set group name
	const Uint8 *pNameBegin = pLastNumberEnd + 1;
	const Uint8 *pNameEnd = UMemory::Search("\r\n", 2, pNameBegin, inFieldEnd - pNameBegin);
	if (!pNameEnd)
		return false;
		
	outGroupInfo.psGroupName[0] = UMemory::Copy(outGroupInfo.psGroupName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(outGroupInfo.psGroupName) - 1 ? sizeof(outGroupInfo.psGroupName) - 1 :  pNameEnd - pNameBegin);

	// outGroupInfo.bIsPostingAllowed flag was set in UNntpTransact::SelectGroup()

	return true;
}

static bool _GetArticle(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;
	
	if (*inFieldBegin == '<' && *(inFieldEnd - 1) == '>')	// NEWNEWS command list just ID's for new messages
	{
		outArticleInfo.psArticleID[0] = UMemory::Copy(outArticleInfo.psArticleID + 1, inFieldBegin + 1, inFieldEnd - inFieldBegin - 2 > sizeof(outArticleInfo.psArticleID) - 1 ? sizeof(outArticleInfo.psArticleID) - 1 : inFieldEnd - inFieldBegin - 2);
		return true;
	}
		
	// search article number
	const Uint8 *pArticleNumberEnd = UMemory::SearchByte('\t', inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pArticleNumberEnd)
		return false;
	
	outArticleInfo.nArticleNumber = UText::TextToUInteger(inFieldBegin, pArticleNumberEnd - inFieldBegin);

	// search article name
	const Uint8 *pNameBegin = pArticleNumberEnd + 1;
	const Uint8 *pNameEnd = UMemory::SearchByte('\t', pNameBegin, inFieldEnd - pNameBegin);
	if (!pNameEnd)
		return false;
		
	outArticleInfo.psArticleName[0] = UMemory::Copy(outArticleInfo.psArticleName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(outArticleInfo.psArticleName) - 1 ? sizeof(outArticleInfo.psArticleName) - 1 :  pNameEnd - pNameBegin);

	// search poster
	const Uint8 *pPosterBegin = pNameEnd + 1;
	const Uint8 *pPosterEnd = UMemory::SearchByte('\t', pPosterBegin, inFieldEnd - pPosterBegin);
	if (!pPosterEnd)
		return false;
		
	if (!_GetArticlePoster(pPosterBegin, pPosterEnd, outArticleInfo))
		return false;

	// search date
	const Uint8 *pDateBegin = pPosterEnd + 1;
	const Uint8 *pDateEnd = UMemory::SearchByte('\t', pDateBegin, inFieldEnd - pDateBegin);
	if (!pDateEnd)
		return false;
		
	Uint8 psArticleDate[64];
	psArticleDate[0] = UMemory::Copy(psArticleDate + 1, pDateBegin, pDateEnd - pDateBegin > sizeof(psArticleDate) - 1 ? sizeof(psArticleDate) - 1 :  pDateEnd - pDateBegin);
	_ConvertArticleDate(psArticleDate, outArticleInfo.stArticleDate);
	
	// search article ID
	const Uint8 *pArticleIDBegin = pDateEnd + 1;
	if (*pArticleIDBegin != '<')
		return false;

	pArticleIDBegin++;
	const Uint8 *pArticleIDEnd = UMemory::SearchByte('\t', pArticleIDBegin, inFieldEnd - pArticleIDBegin);
	if (!pArticleIDEnd || *(pArticleIDEnd - 1) != '>')
		return false;
	
	pArticleIDEnd--;
	outArticleInfo.psArticleID[0] = UMemory::Copy(outArticleInfo.psArticleID + 1, pArticleIDBegin, pArticleIDEnd - pArticleIDBegin > sizeof(outArticleInfo.psArticleID) - 1 ? sizeof(outArticleInfo.psArticleID) - 1 : pArticleIDEnd - pArticleIDBegin);
	
	// search parent ID (we have a list with ID's for all parents but we need just the last one)
	outArticleInfo.psParentID[0] = 0;
	
	const Uint8 *pParentID = pArticleIDEnd + 2;
	const Uint8 *pParentIDEnd = UMemory::SearchByte('\t', pParentID, inFieldEnd - pParentID);
	if (pParentIDEnd > pParentID && *(pParentIDEnd - 1) == '>')
	{
		pParentIDEnd--;
		const Uint8 *pParentIDBegin = pParentIDEnd;
		while (--pParentIDBegin >= pParentID)
		{
			if (*pParentIDBegin == '<')
			{
				pParentIDBegin++;
				outArticleInfo.psParentID[0] = UMemory::Copy(outArticleInfo.psParentID + 1, pParentIDBegin, pParentIDEnd - pParentIDBegin > sizeof(outArticleInfo.psParentID) - 1 ? sizeof(outArticleInfo.psParentID) - 1 : pParentIDEnd - pParentIDBegin);
				break;
			}
		}
	}
		
	return true;
}

static bool _GetCurrentArticle(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo, SEncodingInfo& outEncodingInfo)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;

	// search article ID
	if (!_GetArticleID(inFieldBegin, inFieldEnd, outArticleInfo.nArticleNumber, outArticleInfo.psArticleID, sizeof(outArticleInfo.psArticleID)))
		return false;

	// search article name
	const Uint8 *pNameBegin = UText::SearchInsensitive("Subject: ", 9, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pNameBegin)
		return false;
	
	pNameBegin += 9;
	const Uint8 *pNameEnd = UMemory::Search("\r\n", 2, pNameBegin, inFieldEnd - pNameBegin);
	if (!pNameEnd)
		return false;
		
	outArticleInfo.psArticleName[0] = UMemory::Copy(outArticleInfo.psArticleName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(outArticleInfo.psArticleName) - 1 ? sizeof(outArticleInfo.psArticleName) - 1 :  pNameEnd - pNameBegin);

	// search poster
	const Uint8 *pPosterBegin = UText::SearchInsensitive("From: ", 6, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pPosterBegin)
		return false;
	
	pPosterBegin += 6;
	const Uint8 *pPosterEnd = UMemory::Search("\r\n", 2, pPosterBegin, inFieldEnd - pPosterBegin);
	if (!pPosterEnd)
		return false;
		
	if (!_GetArticlePoster(pPosterBegin, pPosterEnd, outArticleInfo))
		return false;

	// search date
	const Uint8 *pDateBegin = UText::SearchInsensitive("Date: ", 6, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pDateBegin)
		return false;
	
	pDateBegin += 6;
	const Uint8 *pDateEnd = UMemory::Search("\r\n", 2, pDateBegin, inFieldEnd - pDateBegin);
	if (!pDateEnd)
		return false;
		
	Uint8 psArticleDate[64];
	psArticleDate[0] = UMemory::Copy(psArticleDate + 1, pDateBegin, pDateEnd - pDateBegin > sizeof(psArticleDate) - 1 ? sizeof(psArticleDate) - 1 :  pDateEnd - pDateBegin);
	_ConvertArticleDate(psArticleDate, outArticleInfo.stArticleDate);

	// search parent ID (we have a list with ID's for all parents but we need just the last one)
	outArticleInfo.psParentID[0] = 0;
		
	const Uint8 *pParentID = UText::SearchInsensitive("References: ", 12, inFieldBegin, inFieldEnd - inFieldBegin);
	if (pParentID)
	{
		pParentID += 12;
		const Uint8 *pParentIDEnd = UMemory::Search("\r\n", 2, pParentID, inFieldEnd - pParentID);
		while (pParentIDEnd && *(pParentIDEnd + 2) == '\t')
		{
			pParentIDEnd += 3;
			pParentIDEnd = UMemory::Search("\r\n", 2, pParentIDEnd, inFieldEnd - pParentIDEnd);
		}

		if (pParentIDEnd && *(pParentIDEnd - 1) == '>')
		{
			pParentIDEnd--;
			const Uint8 *pParentIDBegin = pParentIDEnd;
			
			while (--pParentIDBegin >= pParentID)
			{
				if (*pParentIDBegin == '<')
				{
					pParentIDBegin++;
					outArticleInfo.psParentID[0] = UMemory::Copy(outArticleInfo.psParentID + 1, pParentIDBegin, pParentIDEnd - pParentIDBegin > sizeof(outArticleInfo.psParentID) - 1 ? sizeof(outArticleInfo.psParentID) - 1 : pParentIDEnd - pParentIDBegin);
					break;
				}
			}
		}
	}
	
	ClearStruct(outEncodingInfo);

	// search "Content-type:"
	const Uint8 *pContentBegin = UText::SearchInsensitive("Content-type: ", 14, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pContentBegin)
		return true;
	
	pContentBegin += 14;
	const Uint8 *pContentEnd = UMemory::Search("\r\n", 2, pContentBegin, inFieldEnd - pContentBegin);
	if (!pContentEnd)
		return true;
	
	// search flavor
	const Uint8 *pFlavourBegin = pContentBegin;
	const Uint8 *pFlavourEnd = UMemory::SearchByte(';', pFlavourBegin, pContentEnd - pFlavourBegin);
	
	pNameBegin = nil;
	if (pFlavourEnd)
		pNameBegin = pFlavourEnd + 1;
	else
		pFlavourEnd = pContentEnd;
	
	Uint32 nFlavorSize = UMemory::Copy(outEncodingInfo.csFlavor, pFlavourBegin, pFlavourEnd - pFlavourBegin > sizeof(outEncodingInfo.csFlavor) - 1 ? sizeof(outEncodingInfo.csFlavor) - 1 : pFlavourEnd - pFlavourBegin);
	UText::MakeLowercase(outEncodingInfo.csFlavor, nFlavorSize);
	*(outEncodingInfo.csFlavor + nFlavorSize) = 0;
			
	// search name
	if (pNameBegin)
	{
		pNameBegin = UText::SearchInsensitive("name=", 5, pNameBegin, pContentEnd - pNameBegin);
		if (pNameBegin)
		{
			pNameBegin += 5;
			if (*pNameBegin == '"')
				pNameBegin++;
				
			const Uint8 *pNameEnd = pContentEnd;
			if (*(pNameEnd - 1) == '"')
				pNameEnd--;
	
			outEncodingInfo.psName[0] = UMemory::Copy(outEncodingInfo.psName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(outEncodingInfo.psName) - 1 ? sizeof(outEncodingInfo.psName) - 1 : pNameEnd - pNameBegin);
		}
	}
	
	if (!outEncodingInfo.psName[0])
	{
		Uint32 nNameNumber = 1;
		outEncodingInfo.psName[0] = _GenerateName(outEncodingInfo.csFlavor, nNameNumber, outEncodingInfo.psName + 1, sizeof(outEncodingInfo.psName) - 1);
	
		if (!outEncodingInfo.psName[0])
			return true;
	}

	// search "Content-transfer-encoding:"
	const Uint8 *pEncodingBegin = UText::SearchInsensitive("Content-transfer-encoding: ", 27, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pEncodingBegin)
		return true;
		
	// search encoding
	if (!UText::CompareInsensitive(pEncodingBegin + 27, "base64", 6))
		outEncodingInfo.bEncodingBase64 = true;
	
	return true;
}

static bool _GetArticleID(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint32& outArticleNumber, Uint8 *outArticleID, Uint32 inMaxSize)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;

	// search article number
	const Uint8 *pArticleNumberBegin = UMemory::SearchByte(' ', inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pArticleNumberBegin)
		return false;

	pArticleNumberBegin += 1;
	const Uint8 *pArticleNumberEnd = UMemory::SearchByte(' ', pArticleNumberBegin, inFieldEnd - pArticleNumberBegin);
	if (!pArticleNumberEnd)
		return false;

	outArticleNumber = UText::TextToUInteger(pArticleNumberBegin, pArticleNumberEnd - pArticleNumberBegin);

	// search article ID
	const Uint8 *pArticleIDBegin = UMemory::SearchByte('<', pArticleNumberEnd, inFieldEnd - pArticleNumberEnd);
	if (!pArticleIDBegin)
		return false;

	pArticleIDBegin++;
	const Uint8 *pArticleIDEnd = UMemory::Search("\r\n", 2, pArticleIDBegin, inFieldEnd - pArticleIDBegin);
	if (!pArticleIDEnd || *(pArticleIDEnd - 1) != '>')
		return false;
	
	pArticleIDEnd--;
	outArticleID[0] = UMemory::Copy(outArticleID + 1, pArticleIDBegin, pArticleIDEnd - pArticleIDBegin > inMaxSize - 1 ? inMaxSize - 1 : pArticleIDEnd - pArticleIDBegin);

	return true;
}

// Mark Horton
// mark@cbosgd.UUCP
// <mark@cbosgd.UUCP>
// mark@cbosgd.UUCP(Mark Horton)
// mark@cbosgd.UUCP (Mark Horton)
// Mark Horton <mark@cbosgd.UUCP>
// "Mark Horton" <mark@cbosgd.UUCP>
// Mark<mark@cbosgd.UUCP>
static bool _GetArticlePoster(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, SNewsArticleInfo& outArticleInfo)
{
	if (!inFieldBegin || !inFieldEnd)
		return false;
	
	if (inFieldBegin >= inFieldEnd)		// if don't have poster name and address
	{
		outArticleInfo.psPosterName[0] = 0;
		outArticleInfo.psPosterAddr[0] = 0;
	}
	else if (*(inFieldEnd - 1) != '>' && *(inFieldEnd - 1) != ')')	// if don't have poster name or address
	{
		if (UMemory::SearchByte('@', inFieldBegin, inFieldEnd - inFieldBegin))
		{
			outArticleInfo.psPosterName[0] = 0;
			outArticleInfo.psPosterAddr[0] = UMemory::Copy(outArticleInfo.psPosterAddr + 1, inFieldBegin, inFieldEnd - inFieldBegin > sizeof(outArticleInfo.psPosterAddr) - 1 ? sizeof(outArticleInfo.psPosterAddr) - 1 :  inFieldEnd - inFieldBegin);
		}
		else
		{
			outArticleInfo.psPosterName[0] = UMemory::Copy(outArticleInfo.psPosterName + 1, inFieldBegin, inFieldEnd - inFieldBegin > sizeof(outArticleInfo.psPosterName) - 1 ? sizeof(outArticleInfo.psPosterName) - 1 : inFieldEnd - inFieldBegin);
			outArticleInfo.psPosterAddr[0] = 0;
		}		
	}
	else if (*(inFieldEnd - 1) == ')')
	{
		if (*inFieldBegin == '(')
		{
			outArticleInfo.psPosterName[0] = UMemory::Copy(outArticleInfo.psPosterName + 1, inFieldBegin + 1, inFieldEnd - inFieldBegin - 2 > sizeof(outArticleInfo.psPosterName) - 1 ? sizeof(outArticleInfo.psPosterName) - 1 : inFieldEnd - inFieldBegin - 2);
			outArticleInfo.psPosterAddr[0] = 0;
		}
		else
		{
			// search poster address
			const Uint8 *pPosterAddrBegin = inFieldBegin;
			if (*pPosterAddrBegin == '<')
				pPosterAddrBegin++;
		
			const Uint8 *pPosterAddrEnd = UMemory::Search(" (", 2, pPosterAddrBegin, inFieldEnd - pPosterAddrBegin);
			if (!pPosterAddrEnd)
			{
				pPosterAddrEnd = UMemory::SearchByte('(', pPosterAddrBegin, inFieldEnd - pPosterAddrBegin);
				if (!pPosterAddrEnd)
					return false;
			}
			
			// search poster name
			const Uint8 *pPosterNameBegin;
			if (*pPosterAddrEnd == '(')
				pPosterNameBegin = pPosterAddrEnd + 1;
			else
				pPosterNameBegin = pPosterAddrEnd + 2;
			
			if (*pPosterAddrEnd == '>')
				pPosterAddrEnd--;

			outArticleInfo.psPosterAddr[0] = UMemory::Copy(outArticleInfo.psPosterAddr + 1, pPosterAddrBegin, pPosterAddrEnd - pPosterAddrBegin > sizeof(outArticleInfo.psPosterAddr) - 1 ? sizeof(outArticleInfo.psPosterAddr) - 1 : pPosterAddrEnd - pPosterAddrBegin);
			outArticleInfo.psPosterName[0] = UMemory::Copy(outArticleInfo.psPosterName + 1, pPosterNameBegin, inFieldEnd - 1 - pPosterNameBegin > sizeof(outArticleInfo.psPosterName) - 1 ? sizeof(outArticleInfo.psPosterName) - 1 : inFieldEnd - 1 - pPosterNameBegin);
		}
	}
	else 
	{
		if (*inFieldBegin == '<')			// if don't have poster name
		{
			outArticleInfo.psPosterName[0] = 0;
			outArticleInfo.psPosterAddr[0] = UMemory::Copy(outArticleInfo.psPosterAddr + 1, inFieldBegin + 1, inFieldEnd - inFieldBegin - 2 > sizeof(outArticleInfo.psPosterAddr) - 1 ? sizeof(outArticleInfo.psPosterAddr) - 1 :  inFieldEnd - inFieldBegin - 2);
		}
		else
		{
			// search poster name
			const Uint8 *pPosterNameBegin = UMemory::SearchByte('"', inFieldBegin, inFieldEnd - inFieldBegin);
			const Uint8 *pPosterNameEnd = nil;

			if (pPosterNameBegin)
			{
				pPosterNameBegin += 1;
				pPosterNameEnd = UMemory::SearchByte('"', pPosterNameBegin, inFieldEnd - pPosterNameBegin);
			}
			else
			{
				pPosterNameBegin = inFieldBegin;
				pPosterNameEnd = UMemory::SearchByte(' ', pPosterNameBegin, inFieldEnd - pPosterNameBegin);
			}

			if (!pPosterNameEnd)
			{
				pPosterNameEnd = UMemory::SearchByte('<', pPosterNameBegin, inFieldEnd - pPosterNameBegin);
			
				if (!pPosterNameEnd)
					return false;
			}
			
			outArticleInfo.psPosterName[0] = UMemory::Copy(outArticleInfo.psPosterName + 1, pPosterNameBegin, pPosterNameEnd - pPosterNameBegin > sizeof(outArticleInfo.psPosterName) - 1 ? sizeof(outArticleInfo.psPosterName) - 1 :  pPosterNameEnd - pPosterNameBegin);

			// search poster address
			pPosterNameEnd += 1;
			const Uint8 *pPosterAddrBegin = UMemory::SearchByte('<', pPosterNameEnd, inFieldEnd - pPosterNameEnd);
			if (pPosterAddrBegin)
			{
				pPosterAddrBegin += 1;
				outArticleInfo.psPosterAddr[0] = UMemory::Copy(outArticleInfo.psPosterAddr + 1, pPosterAddrBegin, inFieldEnd - 1 - pPosterAddrBegin > sizeof(outArticleInfo.psPosterAddr) - 1 ? sizeof(outArticleInfo.psPosterAddr) - 1 :  inFieldEnd - 1 - pPosterAddrBegin);
			}
			else
				outArticleInfo.psPosterAddr[0] = 0;
		}
	}
	
	return true;
}

static Uint32 _GenerateName(const Int8 *inFlavor, Uint32& inNameNumber, Uint8 *outName, Uint32 inMaxSize)
{
	if (!inFlavor || !strlen(inFlavor) || !outName || !inMaxSize)
		return 0;

	if (!UText::CompareInsensitive(inFlavor, "text/plain", 10) || !UText::CompareInsensitive(inFlavor, "multipart/", 10))
		return 0;
	
	// if the attachment don't have a name we generate one
	Uint8 psExtension[5];
	const Int8 *pExtension = UMime::ConvertMime_Extension(inFlavor);
	psExtension[0] = UMemory::Copy(psExtension + 1, pExtension, strlen(pExtension) > sizeof(psExtension) - 1 ? sizeof(psExtension) - 1 : strlen(pExtension));
		
	return UText::Format(outName, inMaxSize, "HHH%5.5lu%#s", inNameNumber++, psExtension);
}

static const Uint8 *_CheckMultiPart(const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint8 *outMultiPartID, Uint32 inMaxSize)
{
	outMultiPartID[0] = 0;

	// search multi part ID
	const Uint8 *pMultiPartBegin = inFieldBegin;
	const Uint8 *pMultiPartEnd = UMemory::Search("\r\n", 2, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pMultiPartEnd)
		return pMultiPartBegin;
		
	const Uint8 *pFieldBegin = pMultiPartEnd + 2;
	if (!UMemory::Compare(pFieldBegin, "\r\n", 2))
		pFieldBegin += 2;
	
	const Uint8 *pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, inFieldEnd - pFieldBegin);
	if (!pFieldEnd)
		return pMultiPartBegin;

	// search "Content-type:"
	if (UText::CompareInsensitive(pFieldBegin, "Content-type: ", 14))
	{
		const Uint8 *pMultiPartSaved = pFieldBegin;

		pFieldBegin = pFieldEnd + 2;
		pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, inFieldEnd - pFieldBegin);
		if (!pFieldEnd)
			return pMultiPartBegin;
		
		if (UText::CompareInsensitive(pFieldBegin, "Content-type: ", 14))
		{
			pFieldEnd = UMemory::Search("\r\n\r\n", 4, inFieldBegin, inFieldEnd - inFieldBegin);
			if (!pFieldEnd)
				return pMultiPartBegin;

			pFieldBegin = pFieldEnd + 4;
			pMultiPartSaved = pFieldBegin;
			
			pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, inFieldEnd - pFieldBegin);
			if (!pFieldEnd)
				return pMultiPartBegin;

			pFieldBegin = pFieldEnd + 2;
			pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, inFieldEnd - pFieldBegin);
			if (!pFieldEnd || UText::CompareInsensitive(pFieldBegin, "Content-type: ", 14))
				return pMultiPartBegin;
		}
		
		pMultiPartBegin = pMultiPartSaved;
		pMultiPartEnd = pFieldBegin - 2;
	}
	
	// search "Content-transfer-encoding:"
	pFieldBegin = pFieldEnd + 2;
	pFieldBegin = UText::SearchInsensitive("Content-transfer-encoding: ", 27, pFieldBegin, inFieldEnd - pFieldBegin);

	if (pFieldBegin)
		outMultiPartID[0] = UMemory::Copy(outMultiPartID + 1, pMultiPartBegin, pMultiPartEnd - pMultiPartBegin > inMaxSize - 1 ? inMaxSize - 1 :  pMultiPartEnd - pMultiPartBegin);
	
	return pMultiPartBegin;
}

static const Uint8 *_CheckMultiPartContent(TNntpTransact inTrn, const Uint8 *inMultiPartID, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd, Uint32& ioNameNumber)
{
	// search "Content-type:"
	const Uint8 *pFieldEnd = UMemory::Search("\r\n", 2, inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pFieldEnd || UText::CompareInsensitive(inFieldBegin, "Content-type: ", 14))
		return nil;
		
	// search flavor
	const Uint8 *pFlavourBegin = inFieldBegin + 14;
	const Uint8 *pFlavourEnd = UMemory::SearchByte(';', pFlavourBegin, pFieldEnd - pFlavourBegin);
	
	const Uint8 *pNameBegin = nil;
	if (pFlavourEnd)
		pNameBegin = pFlavourEnd + 1;
	else
		pFlavourEnd = pFieldEnd;
	
	Int8 csFlavor[32];
	Uint32 nFlavorSize = UMemory::Copy(csFlavor, pFlavourBegin, pFlavourEnd - pFlavourBegin > sizeof(csFlavor) - 1 ? sizeof(csFlavor) - 1 : pFlavourEnd - pFlavourBegin);
	UText::MakeLowercase(csFlavor, nFlavorSize);
	*(csFlavor + nFlavorSize) = 0;
			
	// search name
	Uint8 psName[64];
	psName[0] = 0;
	
	if (pNameBegin)
	{
		if (!UMemory::Compare(pNameBegin, "\r\n", 2))
		{
			pNameBegin += 2;
			
			pFieldEnd = UMemory::Search("\r\n", 2, pNameBegin, inFieldEnd - pNameBegin);
			if (!pFieldEnd)
				return nil;
		}
				
		pNameBegin = UText::SearchInsensitive("name=", 5, pNameBegin, pFieldEnd - pNameBegin);
		if (pNameBegin)
		{
			pNameBegin += 5;
			if (*pNameBegin == '"')
				pNameBegin++;
				
			const Uint8 *pNameEnd = pFieldEnd;
			if (*(pNameEnd - 1) == '"')
				pNameEnd--;
	
			psName[0] = UMemory::Copy(psName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(psName) - 1 ? sizeof(psName) - 1 : pNameEnd - pNameBegin);
		}
	}
	
	if (!psName[0])
	{
		psName[0] = _GenerateName(csFlavor, ioNameNumber, psName + 1, sizeof(psName) - 1);
	
		if (!psName[0])
			return nil;
	}

	// search "Content-transfer-encoding:"
	const Uint8 *pFieldBegin = pFieldEnd + 2;
	pFieldBegin = UText::SearchInsensitive("Content-transfer-encoding: ", 27, pFieldBegin, inFieldEnd - pFieldBegin);
	if (!pFieldBegin)
		return nil;
	
	// search encoding
	bool bEncodingBase64 = false;
	if (!UText::CompareInsensitive(pFieldBegin + 27, "base64", 6))
		bEncodingBase64 = true;
	
	// search data begin
	pFieldEnd = UMemory::Search("\r\n\r\n", 4, pFieldBegin, inFieldEnd - pFieldBegin);
	if (!pFieldEnd)
		return nil;

	// search data end
	pFieldBegin = pFieldEnd + 4;
	pFieldEnd = UMemory::Search(inMultiPartID + 1, inMultiPartID[0], pFieldBegin, inFieldEnd - pFieldBegin);
	if (!pFieldEnd || UMemory::Compare(pFieldEnd - 2, "\r\n", 2))
		return nil;

	const Uint8 *pMultiPartID = pFieldEnd;
	
	pFieldEnd -= 2;
	if (!UMemory::Compare(pFieldEnd - 2, "\r\n", 2))
		pFieldEnd -= 2;

	Uint32 nDataSize;
	void *pData = nil;
	
	if (bEncodingBase64)
	{
		// decode data
		pData= UDigest::Base64_Decode(pFieldBegin, pFieldEnd - pFieldBegin, nDataSize);
	}
	else
	{
		nDataSize = pFieldEnd - pFieldBegin;
		pData = UMemory::New(pFieldBegin, nDataSize);
	}

	if (!pData)
		return pMultiPartID;

	SArticleData *pArticleData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
	
	UMemory::Copy(pArticleData->csFlavor, csFlavor, strlen(csFlavor) + 1);
	UMemory::Copy(pArticleData->psName, psName, psName[0] + 1);
	pArticleData->nDataSize = nDataSize;
	pArticleData->pData = pData;
		
	TRN->pDataList.AddItem(pArticleData);

	return pMultiPartID;
}

static bool _GetArticleItem(TNntpTransact inTrn, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;

//	if (UMemory::Compare(inFieldBegin, "644", 3) && UMemory::Compare(inFieldBegin, "666", 3) &&
//		UMemory::Compare(inFieldBegin, "755", 3))												// ???
//		return false;

	// search name
	const Uint8 *pNameBegin = UMemory::SearchByte(' ', inFieldBegin, inFieldEnd - inFieldBegin);
	if (!pNameBegin)
		return false;
	
	pNameBegin += 1;
	const Uint8 *pNameEnd = UMemory::Search("\r\n", 2, pNameBegin, inFieldEnd - pNameBegin);
	if (!pNameEnd)
		return false;

	Uint8 psName[64];
	psName[0] = UMemory::Copy(psName + 1, pNameBegin, pNameEnd - pNameBegin > sizeof(psName) - 1 ? sizeof(psName) - 1 : pNameEnd - pNameBegin);
	
	const Uint8 *pDataBegin = pNameEnd + 2;
	
	// decode data
	Uint32 nDataSize;
	void *pData = nil;
	
	pData= UDigest::UU_Decode(pDataBegin, inFieldEnd - pDataBegin, nDataSize);
	if (!pData)
		return false;
				
	SArticleData *pArticleData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
	
	const Int8 *pFlavor = UMime::ConvertNameExtension_Mime(psName);
	UMemory::Copy(pArticleData->csFlavor, pFlavor, strlen(pFlavor) + 1);
	UMemory::Copy(pArticleData->psName, psName, psName[0] + 1);
	pArticleData->nDataSize = nDataSize;
	pArticleData->pData = pData;
		
	TRN->pDataList.AddItem(pArticleData);
		
	return true;
}

static bool _GetArticleData(TNntpTransact inTrn, const Uint8 *inFieldBegin, const Uint8 *inFieldEnd)
{
	if (!inFieldBegin || !inFieldEnd || inFieldBegin >= inFieldEnd)
		return false;

	// we have just one attachment here
	if (TRN->stCurrentEncoding.psName[0])
	{
		Uint32 nDataSize;
		void *pData = nil;
	
		if (TRN->stCurrentEncoding.bEncodingBase64)
		{
			// decode data
			pData= UDigest::Base64_Decode(inFieldBegin, inFieldEnd - inFieldBegin, nDataSize);
		}
		else
		{
			nDataSize = inFieldEnd - inFieldBegin;
			pData = UMemory::New(inFieldBegin, nDataSize);
		}

		if (!pData)
			return false;

		SArticleData *pArticleData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
		
		UMemory::Copy(pArticleData->csFlavor, TRN->stCurrentEncoding.csFlavor, strlen(TRN->stCurrentEncoding.csFlavor) + 1);
		UMemory::Copy(pArticleData->psName, TRN->stCurrentEncoding.psName, TRN->stCurrentEncoding.psName[0] + 1);
		pArticleData->nDataSize = nDataSize;
		pArticleData->pData = pData;

		TRN->pDataList.AddItem(pArticleData);

		return true;
	}

	void *pData = nil;
	Uint32 nDataSize = 0;
	Uint32 nDataOffset = 0;
		
	Uint32 nNameNumber = 1;	// if the attachment don't have a name we generate one
	
	Uint8 psMultiPartID[64];
	const Uint8 *pFieldBegin = _CheckMultiPart(inFieldBegin, inFieldEnd, psMultiPartID, sizeof(psMultiPartID));
	const Uint8 *pFieldEnd = nil;
	
	while (true)
	{
		pFieldEnd = UMemory::Search("\r\n", 2, pFieldBegin, inFieldEnd - pFieldBegin);
		if (!pFieldEnd)
			break;

		// if multi part message
		if (psMultiPartID[0] && pFieldEnd - pFieldBegin >= psMultiPartID[0] && !UMemory::Compare(pFieldBegin, psMultiPartID + 1, psMultiPartID[0]))
		{
			if (pFieldEnd - pFieldBegin - psMultiPartID[0] == 2 && !UMemory::Compare(pFieldEnd - 2, "--", 2))
				break;
		
			const Uint8 *pItemEnd = UMemory::Search("\r\n\r\n", 4, pFieldBegin, inFieldEnd - pFieldBegin);
			if (pItemEnd)
			{
				pFieldBegin = _CheckMultiPartContent(inTrn, psMultiPartID, pFieldEnd + 2, inFieldEnd, nNameNumber);
				if (!pFieldBegin)
					pFieldBegin = pItemEnd + 4;
					
				if (!UMemory::Compare(pFieldBegin, ".\r\n", 3))
					break;

				continue;
			}
		}
		
		// if image or another attachment
		if (!UText::CompareInsensitive(pFieldBegin, "begin ", 6))
		{
			const Uint8 *pItemBegin = pFieldBegin + 6;
			const Uint8 *pItemEnd = UText::SearchInsensitive("\r\nend\r\n", 7, pItemBegin, inFieldEnd - pItemBegin);
			if (!pItemEnd)
				pItemEnd = inFieldEnd;
			
			if (_GetArticleItem(inTrn, pItemBegin, pItemEnd))
			{
				if (pItemEnd == inFieldEnd)
					break;
				
				pFieldBegin = pItemEnd + 7;
				if (!UMemory::Compare(pFieldBegin, ".\r\n", 3))
					break;

				continue;
			}
		}
		
		// if misc attachment (skip for now)
		if (!UText::CompareInsensitive(pFieldBegin, "begin:", 6))
		{
			const Uint8 *pItemBegin = pFieldBegin + 6;
			const Uint8 *pItemEnd = UText::SearchInsensitive("\r\nend:", 6, pItemBegin, inFieldEnd - pItemBegin);
			
			if (pItemEnd)
			{
				pItemEnd += 6;
				pItemEnd = UMemory::Search("\r\n", 2, pItemEnd, inFieldEnd - pItemEnd);
				
				if (pItemEnd)
				{
					pFieldBegin = pItemEnd + 2;
					if (!UMemory::Compare(pFieldBegin, ".\r\n", 3))
						break;

					continue;
				}
			}
		}
		
		if (!_WriteNntpData(pData, nDataSize, nDataOffset, pFieldBegin, pFieldEnd - pFieldBegin))
			return false;
		
		if (!UMemory::Compare(pFieldEnd, "\r\n.\r\n", 5))
			break;
					
		Uint8 cr = '\r';	// replace CR-LF with CR
		if (!_WriteNntpData(pData, nDataSize, nDataOffset, &cr, 1))
			return false;
		
		pFieldBegin = pFieldEnd + 2;
	}

	if (pData)
	{
		if (!_AdjustNntpData(pData, nDataSize, nDataOffset))
			return false;
	
		SArticleData *pArticleData = (SArticleData *)UMemory::NewClear(sizeof(SArticleData));
		
		UMemory::Copy(pArticleData->csFlavor, "text/plain", 11);
		pArticleData->psName[0] = UMemory::Copy(pArticleData->psName + 1, ".body", 5);
		pArticleData->nDataSize = nDataSize;
		pArticleData->pData = pData;
		
		TRN->pDataList.InsertItem(1, pArticleData);
	}
	
	return true;
}

static bool _SetArticleItem(TNntpTransact inTrn, SArticleData *inArticleData)
{
	if (!inArticleData->psName[0] || !inArticleData->pData)
		return false;
	
	// add CR-LF
	if (TRN->pBuffer)
	{
		TRN->nBufferSize += 2;
		TRN->pBuffer = UMemory::Reallocate((TPtr)TRN->pBuffer, TRN->nBufferSize);
		if (!TRN->pBuffer)
			return false;

		*((Uint8 *)TRN->pBuffer + TRN->nBufferSize - 2) = '\r';
		*((Uint8 *)TRN->pBuffer + TRN->nBufferSize - 1) = '\n';
	}
	
	if (!UText::CompareInsensitive(inArticleData->csFlavor, strlen(inArticleData->csFlavor), "text/plain", 10))
	{
		const Uint8 *pDataEnd = (Uint8 *)inArticleData->pData + inArticleData->nDataSize;
		const Uint8 *pFieldBegin = (Uint8 *)inArticleData->pData;
		const Uint8 *pFieldEnd = nil;

		// replace CR with CR-LF
		while (true)
		{
			pFieldEnd = UMemory::SearchByte('\r', pFieldBegin, pDataEnd - pFieldBegin);
			if (!pFieldEnd)
				pFieldEnd = pDataEnd;
		
			Uint32 nFieldSize = pFieldEnd - pFieldBegin;
			if (pFieldEnd != pDataEnd)
				nFieldSize += 2;
		
			TRN->pBuffer = UMemory::Reallocate((TPtr)TRN->pBuffer, TRN->nBufferSize + nFieldSize);
			if (!TRN->pBuffer)
				return false;
			
			if (pFieldEnd > pFieldBegin)
				UMemory::Copy((Uint8 *)TRN->pBuffer + TRN->nBufferSize, pFieldBegin, pFieldEnd - pFieldBegin);

			TRN->nBufferSize += nFieldSize;
			
			if (pFieldEnd == pDataEnd)
				break;
			
			*((Uint8 *)TRN->pBuffer + TRN->nBufferSize - 2) = '\r';
			*((Uint8 *)TRN->pBuffer + TRN->nBufferSize - 1) = '\n';
			
			pFieldBegin = pFieldEnd + 1;
			if (pFieldBegin == pDataEnd)
				break;
		}
	}
	else
	{
		Uint32 nDataSize;
		void *pData = UDigest::UU_Encode(inArticleData->pData, inArticleData->nDataSize, nDataSize);
		if (!pData)
			return false;
		
		Uint8 psDataHeader[128];
		psDataHeader[0] = UText::Format(psDataHeader + 1, sizeof(psDataHeader) - 1, "\r\nbegin 666 %#s\r\n", inArticleData->psName);

		TRN->pBuffer = UMemory::Reallocate((TPtr)TRN->pBuffer, TRN->nBufferSize + psDataHeader[0] + nDataSize + 7);
		if (!TRN->pBuffer)
		{	
			UMemory::Dispose((TPtr)pData);
			return false;
		}
	
		TRN->nBufferSize += UMemory::Copy((Uint8 *)TRN->pBuffer + TRN->nBufferSize, psDataHeader + 1, psDataHeader[0]);	
		TRN->nBufferSize += UMemory::Copy((Uint8 *)TRN->pBuffer + TRN->nBufferSize, pData, nDataSize);	
		TRN->nBufferSize += UMemory::Copy((Uint8 *)TRN->pBuffer + TRN->nBufferSize, "\r\nend\r\n", 7);

		UMemory::Dispose((TPtr)pData);
	}
	
	return true;	
}

static bool _SetPostArticleData(TNntpTransact inTrn)
{
	_ClearNntpBuffer(inTrn);

	if (!TRN->pPostInfo->psArticleName[0] || !TRN->pPostInfo->psPosterName[0] || !TRN->pPostInfo->pDataList.GetItemCount())
		return false;

	// set date
	Uint8 psDate[64];
	psDate[0] = _ConvertPostDate(TRN->pPostInfo->stPostDate, psDate + 1, sizeof(psDate) - 1);

	// set poster
	Uint8 psPoster[132];
	if (TRN->pPostInfo->psPosterAddr[0])
		psPoster[0] = UText::Format(psPoster + 1, sizeof(psPoster) - 1, "\"%#s\" <%#s>", TRN->pPostInfo->psPosterName, TRN->pPostInfo->psPosterAddr);
	else
		UMemory::Copy(psPoster, TRN->pPostInfo->psPosterName, TRN->pPostInfo->psPosterName[0] + 1);
	
	Uint8 bufHeader[1024];
	Uint32 nHeaderSize = UText::Format(bufHeader, sizeof(bufHeader), "Date: %#s\r\nFrom: %#s\r\nNewsgroups: %#s\r\nSubject: %#s\r\n", psDate, psPoster, TRN->stSelectedGroup.psGroupName, TRN->pPostInfo->psArticleName);
	
	// set article ID
	if (TRN->pPostInfo->psArticleID[0])
		nHeaderSize += UText::Format(bufHeader + nHeaderSize, sizeof(bufHeader) - nHeaderSize, "Message-ID: <%#s>\r\n", TRN->pPostInfo->psArticleID);
	
	// set parent ID's
	if (TRN->pPostInfo->pParentIDsList.GetItemCount())
	{
		nHeaderSize += UMemory::Copy(bufHeader + nHeaderSize, "References:", 11);
		
		Uint32 i = 0;
		Uint8 *pParentID;
		
		while (TRN->pPostInfo->pParentIDsList.GetNext(pParentID, i))
			nHeaderSize += UText::Format(bufHeader + nHeaderSize, sizeof(bufHeader) - nHeaderSize, " <%#s>", pParentID);

		nHeaderSize += UMemory::Copy(bufHeader + nHeaderSize, "\r\n", 2);
	}	

	// set organization
	if (TRN->pPostInfo->psOrganization[0])
		nHeaderSize += UText::Format(bufHeader + nHeaderSize, sizeof(bufHeader) - nHeaderSize, "Organization: %#s\r\n", TRN->pPostInfo->psOrganization);
	
	// set news reader
	if (TRN->pPostInfo->psNewsReader[0])
		nHeaderSize += UText::Format(bufHeader + nHeaderSize, sizeof(bufHeader) - nHeaderSize, "X-Newsreader: %#s\r\n", TRN->pPostInfo->psNewsReader);

	nHeaderSize += UMemory::Copy(bufHeader + nHeaderSize, "Mime-version: 1.0\r\n", 19);
	nHeaderSize += UMemory::Copy(bufHeader + nHeaderSize, "Content-type: text/plain; charset=\"US-ASCII\"\r\n", 46);
	nHeaderSize += UMemory::Copy(bufHeader + nHeaderSize, "Content-transfer-encoding: 7bit\r\n\r\n", 35);

	TRN->pBuffer = UMemory::New(nHeaderSize);
	if (!TRN->pBuffer)
		return false;
		
	TRN->nBufferSize = UMemory::Copy(TRN->pBuffer, bufHeader, nHeaderSize);
	
	// set data
	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (TRN->pPostInfo->pDataList.GetNext(pArticleData, i))
	{
		if (!_SetArticleItem(inTrn, pArticleData))
			return false;
	}
	
	TRN->pBuffer = UMemory::Reallocate((TPtr)TRN->pBuffer, TRN->nBufferSize + 5);
	if (!TRN->pBuffer)
		return false;

	TRN->nBufferSize += UMemory::Copy((Uint8 *)TRN->pBuffer + TRN->nBufferSize, "\r\n.\r\n", 5);
	
	return true;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

static bool _WriteNntpData(void*& ioData, Uint32& ioDataSize, Uint32& ioDataOffset, const Uint8 *inNewData, Uint32 inNewDataSize)
{
	if (!inNewDataSize)
		return true;
	
	if (ioDataOffset > ioDataSize || !inNewData)
		return false;
	
	if (ioDataOffset + inNewDataSize > ioDataSize)
	{
		ioDataSize += RoundUp8(inNewDataSize + 1024);
		
		if (!ioData)
			ioData = UMemory::New(ioDataSize);
		else
			ioData = UMemory::Reallocate((TPtr)ioData, ioDataSize);
			
		if (!ioData)
			return false;
	}

	ioDataOffset += UMemory::Copy((Uint8 *)ioData + ioDataOffset, inNewData, inNewDataSize);
	
	return true;
}

static bool _AdjustNntpData(void*& ioData, Uint32& ioDataSize, Uint32 inDataOffset)
{
	if (!ioData || !ioDataSize || !inDataOffset || inDataOffset >= ioDataSize)
		return false;
	
	ioDataSize = inDataOffset;
	ioData = UMemory::Reallocate((TPtr)ioData, ioDataSize);

	if (!ioData)
		return false;
	
	return true;	
}

static void _ClearNntpFlags(TNntpTransact inTrn)
{
	TRN->bIsCommandExecuted = false;
	TRN->bIsCommandError = false;
	TRN->psCommandError[0] = 0;
}

static void _ClearNntpTransport(TNntpTransact inTrn)
{
	if (TRN->tpt)
	{		
		UTransport::Dispose(TRN->tpt);
		TRN->tpt = nil;
	}
}

static void _ClearNntpBuffer(TNntpTransact inTrn)
{
	if (TRN->pBuffer)
	{
		UMemory::Dispose((TPtr)TRN->pBuffer);
		TRN->pBuffer = nil;
	}	
	
	TRN->nBufferSize = 0;	
		
	Uint32 i = 0;
	Uint32 *pOffset;
	
	while (TRN->pOffsetList.GetNext(pOffset, i))
		UMemory::Dispose((TPtr)pOffset);

	TRN->pOffsetList.Clear();
}

static void _ClearNntpData(TNntpTransact inTrn)
{
	Uint32 i = 0;
	SArticleData *pArticleData;
	
	while (TRN->pDataList.GetNext(pArticleData, i))
	{
		UMemory::Dispose((TPtr)pArticleData->pData);
		UMemory::Dispose((TPtr)pArticleData);
	}

	TRN->pDataList.Clear();
}

static void _ClearNntpPost(TNntpTransact inTrn)
{
	if (!TRN->pPostInfo)
		return;
	
	Uint32 i = 0;
	Uint8 *pParentID;
	
	while (TRN->pPostInfo->pParentIDsList.GetNext(pParentID, i))
		UMemory::Dispose((TPtr)pParentID);

	TRN->pPostInfo->pParentIDsList.Clear();
	
	i = 0;
	SArticleData *pArticleData;
	
	while (TRN->pPostInfo->pDataList.GetNext(pArticleData, i))
	{
		UMemory::Dispose((TPtr)pArticleData->pData);
		UMemory::Dispose((TPtr)pArticleData);
	}

	TRN->pPostInfo->pDataList.Clear();

	UMemory::Dispose((TPtr)TRN->pPostInfo);
	TRN->pPostInfo = nil;
}

static void _ClearNntpCurrent(TNntpTransact inTrn)
{
	ClearStruct(TRN->stCurrentArticle);
	ClearStruct(TRN->stCurrentEncoding);	
}

static void _ClearNntpSelected(TNntpTransact inTrn)
{
	TRN->nSelectedArticleNumber = 0;
	TRN->psSelectedArticleID[0] = 0;
}

static void _ClearNntpInfo(TNntpTransact inTrn)
{
	_ClearNntpBuffer(inTrn);
	_ClearNntpData(inTrn);
	_ClearNntpPost(inTrn);

	ClearStruct(TRN->stSelectedGroup);
	_ClearNntpCurrent(inTrn);
	_ClearNntpSelected(inTrn);
}

