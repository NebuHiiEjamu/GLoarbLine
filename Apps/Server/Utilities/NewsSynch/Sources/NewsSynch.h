/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma def_inherited on

#include "CButtonPopupView.h"
#include "CMenuListView.h"


#if WIN32
#define LIST_BACKGROUND_OPTION	0
#elif MACINTOSH
#define LIST_BACKGROUND_OPTION	scrollerOption_NoBkgnd
#endif

#define NEW_NEWS_FORMAT			0
#define APPLICATION_NAME		"\pHotline News Synchronizer"

#define DEFAULT_SYNCH_HOURS		24
#define DEFAULT_PURGE_DAYS		90
#define DEFAULT_MAX_ARTICLES	800

#define MAX_SYNCH_HOURS			1000
#define MAX_PURGE_DAYS			5000
#define MAX_MAX_ARTICLES		50000


#pragma mark ₯₯ Constants ₯₯

#pragma mark View IDs
enum {
	viewID_Refresh			= 1001,
	viewID_AddSynch			= 1002,
	viewID_Synch			= 1003,
	viewID_Options			= 1004,
	viewID_StopTask			= 1005,

	viewID_SelectServer1	= 1006,
	viewID_ServerAddr1		= 1007,
	viewID_GroupFilter1		= 1008,
	viewID_GroupList1		= 1009,
	viewID_ArticleList1		= 1010,
	viewID_SortArticles1	= 1011,
	viewID_Attachment1		= 1012,

	viewID_SelectServer2	= 1013,
	viewID_SelectBundle2	= 1014,
	viewID_GroupFilter2		= 1015,
	viewID_GroupList2		= 1016,
	viewID_ArticleList2		= 1017,
	viewID_SortArticles2	= 1018,
	viewID_Attachment2		= 1019
};

#pragma mark Synch Options
enum {
	myOpt_LogToFile			= 0x01,
	myOpt_OldNewsFormat		= 0x02,
	myOpt_SortDescending1	= 0x04,
	myOpt_SortDescending2	= 0x08
};

enum {
	myOpt_SynchNewsHotline	= 1,
	myOpt_SynchNewsServer	= 2,
	myOpt_SynchNewsBoth		= 3
};

enum {
	kMyPrefsVersion			= 1,
	kMySynchVersion			= 1,

	kMaxSynchs				= 500,
	kMaxLogs				= 50
};

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Structures ₯₯

#pragma mark SMyOptions
struct SMyOptions {
	Uint32 nOpts;
	Uint32 nPurgeDays;
	Uint32 nMaxArticles;
	Uint8 psEmailAddress[32];
	Uint8 psOrganization[64];
	void *pAddArticleText;
	Uint32 nArticleTextSize;
};

#pragma mark SMyGroupInfo1
struct SMyGroupInfo1 {
	SNewsGroupInfo stGroupInfo;
	Uint32 nNameSize;
};

#pragma mark SMyArticleInfo1
struct SMyArticleInfo1
{
	SNewsArticleInfo stArticleInfo;
	Uint32 nTitleSize;
};

#pragma mark SMyGroupInfo2
struct SMyGroupInfo2 
{
	Uint32 nArticleCount;
	Uint8 psGroupName[256];
	Uint32 nNameSize;
};

#pragma mark SMyDataInfo2
struct SMyDataInfo2
{
	Int8 csFlavor[32];
	Uint8 psName[64];
};

#pragma mark SMyArticleInfo2
struct SMyArticleInfo2
{
	Uint32 nArticleID;
	Uint32 nFlags;
	Uint8 psTitle[32];
	Uint8 psPoster[32];
	SDateTimeStamp stDateTime;
	CPtrList<SMyDataInfo2> lDataList;
	Uint32 nDataSize;
	Uint32 nTitleSize;
};

#pragma mark SMyArticleIDsInfo1
struct SMyArticleIDsInfo1 
{
	bool bSynchronized;
	Uint32 nArticleID;		// news server number
	Uint8 psArticleID[64];	// news server ID
	Uint8 psParentID[64];	// news server parent ID
	SCalendarDate stArticleDate;
};

#pragma mark SMyArticleIDsInfo2
struct SMyArticleIDsInfo2 
{
	bool bSynchronized;
	Uint32 nArticleID;		// hotline ID
	Uint8 psArticleID[64];	// news server ID
	SCalendarDate stArticleDate;
	CPtrList<SMyDataInfo2> lDataList;
	Uint32 nDataSize;
};

#pragma mark SPurgeInfo
struct SPurgeInfo 
{
	bool bTreeFlag;			// true -> nTreeIndex from mArticleIDsTree1, false -> nTreeIndex from mArticleIDsTree2
	Uint32 nArticleID;
	SCalendarDate stArticleDate;
};

#pragma mark SMyServerInfo
#pragma options align=packed
struct SMyServerInfo 
{
	Uint8 psServerAddr[64];
	Uint8 psUserLogin[32];
	Uint8 psUserPassword[32];
};
#pragma options align=reset

#pragma mark SMySynchInfo
#pragma options align=packed
struct SMySynchInfo 
{
	Uint8 nActive;
	SMyServerInfo stServerInfo;
	Uint8 psGroupName[64];
	Uint8 psHotlineServer[32];
	Uint8 nSynchType;
	Uint32 nSynchHours;
	Uint32 nPurgeDays;
	Uint32 nMaxArticles;
	SCalendarDate stLastSynch;
	TFSRefObj* pHotlineFile;
};
#pragma options align=reset

#pragma mark SMyOffSynchInfo
struct SMyOffSynchInfo {
	Uint32 nDateOffset;
	SMySynchInfo *pSynchInfo;
};

#pragma mark SMyLogInfo
struct SMyLogInfo {
	Uint32 nLogID;
	Uint8 *psLogText;
	SCalendarDate stLogDate;
};

#pragma mark SMyServerBundleInfo
struct SMyServerBundleInfo {
	Uint8 psServerName[32];
	CPtrList<TFSRefObj> lBundleList;
};

class CMyTask;
#pragma mark SMyTaskInfo
struct SMyTaskInfo {
	CMyTask *pTask;
	Uint32 nProgVal, nProgMax;
	Uint32 nTextSize;
	Uint8 *pTextData;
};

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Tasks ₯₯

#pragma mark CMyTask
class CMyTask
{
	public:
		CMyTask(const Uint8 *inDesc);
		virtual ~CMyTask();
		
		virtual void Process();
		bool IsFinished()			{	return mIsFinished;		}
		
	protected:
		Uint16 mStage;
		Uint16 mIsFinished		: 1;
		
		void ShowProgress(Uint32 inVal, Uint32 inMax, const Uint8 *inDesc = nil);
		void Finish();
};

#pragma mark CMyConnectTask
class CMyConnectTask : public CMyTask
{
	public:
		CMyConnectTask(SMyServerInfo& inServerInfo);
		virtual ~CMyConnectTask();
		
		bool IsConnected()		{	return mNntpTransact->IsConnected();	}
		bool IsConnectTask(const Uint8 *inServerAddr);
		void FinishTimerArmed();
		
		Uint32 GetLogID()		{	return mLogID;							}
		void LogStop();
		void SelectLog();

		virtual void Process();
				
	protected:
		StNntpTransact mNntpTransact;		
		Uint8 mServerAddr[64];
		
		Uint32 mLogID;
		void MakeLog(const Uint8 *inLogText);
		void AddLog(const Uint8 *inLogText);
		void LogRefused();
		void LogClosed();
		void LogError(bool inShowProgress = true);
	
		bool mFinishTask;
		TTimer mFinishTimer;
		static void FinishTimer(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
};

#pragma mark CMyListGroupsTask
class CMyListGroupsTask : public CMyConnectTask
{
	public:
		CMyListGroupsTask(SMyServerInfo& inServerInfo);
		
		bool IsListGroupsTask(const Uint8 *inServerAddr);
		virtual void Process();	

	protected:
		Uint32 mGroupIndex;
		
		void LogReceived();
};

#pragma mark CMyListArticlesTask
class CMyListArticlesTask : public CMyConnectTask
{
	public:
		CMyListArticlesTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName);
		virtual ~CMyListArticlesTask();
		
		bool IsListArticlesTask(const Uint8 *inServerAddr, const Uint8 *inGroupName);
		virtual void Process();	

	protected:
		Uint32 mArticleIndex;
		Uint8 mGroupName[64];
		
		CPtrList<SMyArticleInfo1> mTempArticleList;
		
		void LogReceived();
};

#pragma mark CMyGetArticleTask
class CMyGetArticleTask : public CMyConnectTask
{
	public:
		CMyGetArticleTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName, const Uint8 *inArticleName, Uint32 inArticleID);
		
		bool IsGetArticleTask(const Uint8 *inServerAddr, const Uint8 *inGroupName, Uint32 inArticleID);
		virtual void Process();	

	protected:
		Uint8 mGroupName[64];
		Uint8 mArticleName[64];
		Uint32 mArticleID;
		Uint32 mArticleSize;
};

#pragma mark CMySynchGroupTask
class CMySynchGroupTask : public CMyConnectTask
{
	public:
		CMySynchGroupTask(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, Uint32 inPurgeDays, Uint32 inMaxArticles);
		virtual ~CMySynchGroupTask();
	
		bool IsSynchGroupTask(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile);
		bool IsSynchGroupTask(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2);
	
	protected:
		Uint32 mArticleIndex1;
		Uint8 mGroupName1[64];
		Uint8 mGroupName2[256];
		TFSRefObj* mSynchFile2;
		
		SCalendarDate mPurgeDate;
		Uint32 mMaxArticles;
				
		CPtrTree<SMyArticleIDsInfo1> mArticleIDsTree1;
		CPtrList<SMyArticleIDsInfo1> mTempArticleIDsList1;
		CPtrTree<SMyArticleIDsInfo2> mArticleIDsTree2;
		
		Uint32 mArticleID1;
		Uint32 mTreeIndex1;
		Uint32 mTreeIndex2;

		bool SetArticleIDsTree1(bool inFinish);
		void SetArticleIDsTree1(CPtrList<SMyArticleIDsInfo1>& inTempArticleIDsList, CPtrList<SMyArticleIDsInfo1>& inSearchArticleIDsList);
		bool SetArticleIDsTree2();
		
		bool SelectNextArticle1();
		bool SelectNextArticle2();
		bool CreateGroup2();

		Uint32 GetParentID2(const Uint8 *inArticleID, const Uint8 *inParentID, Uint32& outParentIndex);
		Uint32 AddArticleID2(Uint32 inParentIndex, Uint32 inArticleID1, const Uint8 *inArticleID2, const SCalendarDate& inArticleDate);

		bool SynchArticle1(Uint32& outSynchSize);
		bool SynchArticle2(Uint32& outSynchSize);

		void MakeMaxArticles();
		void PurgeArticles();
		void MarkSynchronized();
		
		Uint32 GetPurgeDate(CPtrList<SPurgeInfo>& inPurgeList, SCalendarDate& outPurgeDate);
		void ClearDataList(SMyArticleIDsInfo2 *inArticleIDsInfo);
};

#pragma mark CMySynchGroupTask1
class CMySynchGroupTask1 : public CMySynchGroupTask
{
	public:
		CMySynchGroupTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles);
		CMySynchGroupTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles);
		
		virtual void Process();
		
	protected:
		Uint32 mSynchCount1;
		Uint32 mSynchSize1;
		Uint32 mTempSize1;

		void LogSynchronized();
};

#pragma mark CMySynchGroupTask2
class CMySynchGroupTask2 : public CMySynchGroupTask
{
	public:
		CMySynchGroupTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles);
		CMySynchGroupTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles);
		
		virtual void Process();

	protected:
		Uint32 mSynchCount2;
		Uint32 mSynchSize2;

		void LogSynchronized();		
};

#pragma mark CMySynchGroupTask21
class CMySynchGroupTask21 : public CMySynchGroupTask
{
	public:
		CMySynchGroupTask21(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, Uint32 inPurgeDays, Uint32 inMaxArticles);
		CMySynchGroupTask21(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inPurgeDays, Uint32 inMaxArticles);

		virtual void Process();
		
	protected:
		Uint32 mSynchCount1;
		Uint32 mSynchSize1;
		Uint32 mTempSize1;
		
		Uint32 mSynchCount2;
		Uint32 mSynchSize2;

		void LogSynchronized1();
		void LogSynchronized2();
};

#pragma mark CMySynchArticleTask1
class CMySynchArticleTask1 : public CMyConnectTask
{
	public:
		CMySynchArticleTask1(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, CPtrList<Uint8>& inParentIDs1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2, const Uint8 *inArticleName2, CPtrList<SMyDataInfo2>& inDataList2);
		virtual ~CMySynchArticleTask1();
		
		bool IsSynchArticleTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2);
		virtual void Process();
		
	protected:
		Uint8 mGroupName1[64];
		TFSRefObj* mSynchFile2;

		Uint32 mArticleID2;
		Uint8 mArticleName2[64];
		CPtrList<Uint8> mParentIDs1;
		CPtrList<SMyDataInfo2> mDataList2;
		
		Uint32 mSynchSize1;
		
		bool SynchArticle1();
		void LogSynchronized();
};

#pragma mark CMySynchArticleTask2
class CMySynchArticleTask2 : public CMyConnectTask
{
	public:
		CMySynchArticleTask2(SMyServerInfo& inServerInfo, const Uint8 *inGroupName1, Uint32 inArticleID1, const Uint8 *inArticleName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inParentID2);
		virtual ~CMySynchArticleTask2();
		
		bool IsSynchArticleTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, Uint32 inArticleID1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2);
		virtual void Process();
		
	protected:
		Uint8 mGroupName1[64];
		TFSRefObj* mSynchFile2;

		Uint32 mArticleID1;
		Uint8 mArticleName1[64];
		Uint32 mParentID2;
		
		Uint32 mSynchSize2;
		bool SynchArticle2();
		void LogSynchronized();
};


/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Views ₯₯

#pragma mark CMyListStatusView
template <class T> class CMyListStatusView : public CGeneralTabbedListView<T>
{
	public:
		CMyListStatusView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CMyListStatusView();
		
		void SetStatus(const Uint8 *inStatMsg);
		
		virtual Uint32 GetFullHeight() const;
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

	protected:
		Uint8 *mStatMsg;
};

template<class T> CMyListStatusView<T>::CMyListStatusView(CViewHandler *inHandler, const SRect& inBounds)
	: CGeneralTabbedListView(inHandler, inBounds)
{
#if WIN32
	mTabHeight = 18;
#endif

	mStatMsg = nil;
}

template<class T> CMyListStatusView<T>::~CMyListStatusView()
{
	if (mStatMsg)
		UMemory::Dispose((TPtr)mStatMsg);
}

template<class T> void CMyListStatusView<T>::SetStatus(const Uint8 *inStatMsg)
{
	if (mStatMsg)
	{
		UMemory::Dispose((TPtr)mStatMsg);
		mStatMsg = nil;
	}
	
	if (inStatMsg && inStatMsg[0])
		mStatMsg = (Uint8 *)UMemory::New(inStatMsg, inStatMsg[0] + 1);
	
	if (!GetItemCount())
		SetFullHeight();

	Refresh();
}

template<class T> Uint32 CMyListStatusView<T>::GetFullHeight() const
{
	if (mHandler && !GetItemCount())
	{
		CScrollerView *pHandler = dynamic_cast<CScrollerView *>(mHandler);
		if (pHandler)
		{
			SRect stBounds;
			pHandler->GetBounds(stBounds);
			
			return stBounds.GetHeight() - 6;
		}		
	}

	return CGeneralTabbedListView::GetFullHeight();
}

template<class T> void CMyListStatusView<T>::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	// draw the status msg
	if (!GetItemCount() && mStatMsg)
	{
		UGraphics::SetFont(inImage, kDefaultFont, nil, 9);
	#if MACINTOSH
		inImage->SetInkMode(mode_Or);
	#endif

		inImage->SetInkColor(color_Gray);
		
		SRect stRect = mBounds;
		stRect.Enlarge(-4);
		stRect.top += mTabHeight;
		if (stRect.bottom < stRect.top)
			stRect.bottom = stRect.top;
		
		inImage->DrawTextBox(stRect, inUpdateRect, mStatMsg + 1, mStatMsg[0], 0, textAlign_Left);
	}

	CGeneralTabbedListView::Draw(inImage, inUpdateRect, inDepth);
}


#pragma mark CMyTreeStatusView
template <class T> class CMyTreeStatusView : public CTabbedTreeView<T>
{
	public:
		CMyTreeStatusView(CViewHandler *inHandler, const SRect& inBounds);
		~CMyTreeStatusView();
		
		void SetStatus(const Uint8 *inStatMsg);

		virtual Uint32 GetFullHeight() const;
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

	protected:
		Uint8 *mStatMsg;
};

template<class T> CMyTreeStatusView<T>::CMyTreeStatusView(CViewHandler *inHandler, const SRect& inBounds)
	: CTabbedTreeView(inHandler, inBounds, 240, 241)
{
#if WIN32	
	mTabHeight = 18;
#endif

	mLevelWidth = 20;

	mStatMsg = nil;
}

template<class T> CMyTreeStatusView<T>::~CMyTreeStatusView()
{
	if (mStatMsg)
		UMemory::Dispose((TPtr)mStatMsg);
}

template<class T> void CMyTreeStatusView<T>::SetStatus(const Uint8 *inStatMsg)
{
	if (mStatMsg)
	{
		UMemory::Dispose((TPtr)mStatMsg);
		mStatMsg = nil;
	}
	
	if (inStatMsg && inStatMsg[0])
		mStatMsg = (Uint8 *)UMemory::New(inStatMsg, inStatMsg[0] + 1);
	
	if (!GetTreeCount())
		SetFullHeight();

	Refresh();
}

template<class T> Uint32 CMyTreeStatusView<T>::GetFullHeight() const
{
	if (mHandler && !GetTreeCount())
	{
		CScrollerView *pHandler = dynamic_cast<CScrollerView *>(mHandler);
		if (pHandler)
		{
			SRect stBounds;
			pHandler->GetBounds(stBounds);
			
			return stBounds.GetHeight() - 6;
		}		
	}

	return CTabbedTreeView::GetFullHeight();
}

template<class T> void CMyTreeStatusView<T>::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth)
{
	// draw the status msg
	if (!GetItemCount() && mStatMsg)
	{
		UGraphics::SetFont(inImage, kDefaultFont, nil, 9);
	#if MACINTOSH
		inImage->SetInkMode(mode_Or);
	#endif

		inImage->SetInkColor(color_Gray);
		
		SRect stRect = mBounds;
		stRect.Enlarge(-4);
		stRect.top += mTabHeight;
		if (stRect.bottom < stRect.top)
			stRect.bottom = stRect.top;
		
		inImage->DrawTextBox(stRect, inUpdateRect, mStatMsg + 1, mStatMsg[0], 0, textAlign_Left);
	}

	CTabbedTreeView::Draw(inImage, inUpdateRect, inDepth);
}

#pragma mark CMyGroupArticle
class CMyGroupArticle
{
	public:
		CMyGroupArticle(bool inGroupArticle);
		virtual ~CMyGroupArticle();

		virtual void SearchText(const Uint8 *inText) = 0;
		virtual void SetDropIndex(const SDragMsgData& inInfo) = 0;
	
	protected:
		TIcon mRootIcon;

		Uint32 mDropIndex;
		Uint32 mKeyDownTime;
		TTimer mSelectTimer;

		void StartSelectTimer();
		virtual void ExecuteSelectTimer() = 0;
		static void SelectTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
};

#pragma mark CMyGroupListView1
class CMyGroupListView1 : public CMyListStatusView<SMyGroupInfo1>, public CMyGroupArticle
{
	public:
		CMyGroupListView1(CViewHandler *inHandler, const SRect &inBounds);
		virtual ~CMyGroupListView1();

		void SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2);
		void GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2);
		
		virtual void SearchText(const Uint8 *inText);
		virtual void SetDropIndex(const SDragMsgData& inInfo);
		Uint32 GetDropGroupName(Uint8 *outGroupName, Uint32 inMaxSize);

		void AddGroup(SMyGroupInfo1 *inGroupInfo, Uint8 *inFilterText, CLabelView *inGroupCount);
		void FilterGroupList(Uint8 *inFilterText, CLabelView *inGroupCount);
 		void DeleteAll(CLabelView *inGroupCount = nil);

		// key events
		virtual bool KeyDown(const SKeyMsgData& inInfo);

		// list events
		virtual void SetItemSelect(Uint32 inItem, bool inSelect);
		
		// drag events
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragMove(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);
			
	protected:
		CPtrList<SMyGroupInfo1> mGroupList;

		void DisplayGroupCount(CLabelView *inGroupCount);
		
		virtual void ExecuteSelectTimer();
		virtual void ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const CPtrList<SRect>& inTabRectList, Uint32 inOptions = 0);
};

#pragma mark CMyArticleTreeView1
class CMyArticleTreeView1 : public CMyGroupArticle, public CMyTreeStatusView<SMyArticleInfo1>
{
	public:
		CMyArticleTreeView1(CViewHandler *inHandler, const SRect &inBounds);
		virtual ~CMyArticleTreeView1();

		void SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3, Uint8 inTabPercent4, Uint8 inTabPercent5);
		void GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3, Uint8& outTabPercent4, Uint8& outTabPercent5);

		void DeleteAll();
		void SortArticles(bool inDescending = false);

		virtual void SearchText(const Uint8 *inText);
		virtual void SetDropIndex(const SDragMsgData& inInfo);
		bool GetDropArticleID(CPtrList<Uint8>& outParentIDsList);

		// key events
		virtual bool KeyDown(const SKeyMsgData& inInfo);

		// tree events
		virtual void SelectionChanged(Uint32 inTreeIndex, SMyArticleInfo1 *inTreeItem, bool inIsSelected);

		// drag events
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragMove(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);
				
	protected:
		static Int32 CompareArticles(void *inItemA, void *inItemB, void *inRef);
		
		virtual void ExecuteSelectTimer();
		virtual void ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyArticleInfo1 *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions = 0);
};

#pragma mark CMyGroupListView2
class CMyGroupListView2 : public CMyListStatusView<SMyGroupInfo2>, public CMyGroupArticle
{
	public:
		CMyGroupListView2(CViewHandler *inHandler, const SRect &inBounds);
		virtual ~CMyGroupListView2();
			
		void SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2);
		void GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2);

		virtual void SearchText(const Uint8 *inText);
		virtual void SetDropIndex(const SDragMsgData& inInfo);
		Uint32 GetDropGroupName(Uint8 *outGroupName, Uint32 inMaxSize);

		void AddGroup(SMyGroupInfo2 *inGroupInfo, Uint8 *inFilterText, CLabelView *inGroupCount);
		void FilterGroupList(Uint8 *inFilterText, CLabelView *inGroupCount);
 		void DeleteAll(CLabelView *inGroupCount = nil);

		// key events
		virtual bool KeyDown(const SKeyMsgData& inInfo);

		// list events
		virtual void SetItemSelect(Uint32 inItem, bool inSelect);
		
		// drag events
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragMove(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);
			
	protected:
		CPtrList<SMyGroupInfo2> mGroupList;

		void DisplayGroupCount(CLabelView *inGroupCount);
	
		virtual void ExecuteSelectTimer();
		virtual void ItemDraw(Uint32 inListIndex, TImage inImage, const SRect& inBounds, const CPtrList<SRect>& inTabRectList, Uint32 inOptions = 0);
};

#pragma mark CMyArticleTreeView2
class CMyArticleTreeView2 : public CMyGroupArticle, public CMyTreeStatusView<SMyArticleInfo2>
{
	public:
		CMyArticleTreeView2(CViewHandler *inHandler, const SRect &inBounds);
		virtual ~CMyArticleTreeView2();
		
		void SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3, Uint8 inTabPercent4, Uint8 inTabPercent5);
		void GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3, Uint8& outTabPercent4, Uint8& outTabPercent5);

		void DeleteAll();
		Uint32 SetItemsFromData(const Uint8 *inData, Uint32 inDataSize);
		void SortArticles(bool inDescending = false);

		virtual void SearchText(const Uint8 *inText);
		virtual void SetDropIndex(const SDragMsgData& inInfo);
		Uint32 GetDropArticleID();
		SMyArticleInfo2 *GetSelectedArticle();
		CPtrList<SMyDataInfo2> *GetDataList(Uint32 inArticleID);
	
		// key events
		virtual bool KeyDown(const SKeyMsgData& inInfo);

		// tree events
		virtual void SelectionChanged(Uint32 inTreeIndex, SMyArticleInfo2 *inTreeItem, bool inIsSelected);

		// drag events
		virtual void DragEnter(const SDragMsgData& inInfo);
		virtual void DragMove(const SDragMsgData& inInfo);
		virtual void DragLeave(const SDragMsgData& inInfo);

	protected:
		static Int32 CompareArticles(void *inItemA, void *inItemB, void *inRef);
		
		virtual void ExecuteSelectTimer();
		virtual void ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyArticleInfo2 *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions = 0);
};

#pragma mark CMySynchListView
class CMyOptionsWin;
class CMySynchListView : public CGeneralCheckListView<SMySynchInfo>
{
	public:
		CMySynchListView(CViewHandler *inHandler, const SRect &inBounds, CMyOptionsWin *inWin);
		virtual ~CMySynchListView();
		
		SMySynchInfo *GetSelectedSynch();
		SMySynchInfo *GetNearSelectedSynch();
	
		// list events
		virtual void SetItemSelect(Uint32 inItem, bool inSelect);		
		
		// mouse events
		virtual void MouseDown(const SMouseMsgData& inInfo);
		
	protected:
		CMyOptionsWin *mWin;

		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);
};

#pragma mark CMyLogListView
class CMyLogListView : public CTabbedTreeView<SMyLogInfo>
{
	public:
		CMyLogListView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CMyLogListView();
		
		void SetTabs(Uint8 inTabPercent1, Uint8 inTabPercent2, Uint8 inTabPercent3);
		void GetTabs(Uint8& outTabPercent1, Uint8& outTabPercent2, Uint8& outTabPercent3);

		Uint32 MakeLog(const Uint8 *inLogText);
		void AddLog(Uint32 inLogID, const Uint8 *inLogText);
		void SelectLog(Uint32 inLogID);

		// tree events
		virtual void SelectionChanged(Uint32 inTreeIndex, SMyLogInfo *inTreeItem, bool inIsSelected);

	protected:
		Uint32 nLogCount;
	
		void DeleteFirstLog();
		virtual void ItemDraw(Uint32 inTreeIndex, Uint32 inTreeLevel, SMyLogInfo *inTreeItem, STreeViewItem *inTreeViewItem, TImage inImage, const CPtrList<SRect>& inTabRectList, Uint32 inOptions = 0);
};

#pragma mark CMyTaskListView
class CMyTaskListView : public CListView
{
	public:
		CMyTaskListView(CViewHandler *inHandler, const SRect& inBounds);
		virtual ~CMyTaskListView();

		virtual Uint32 GetItemCount() const;
		
		void AddTask(CMyTask *inTask, const Uint8 *inDesc);
		void RemoveTask(CMyTask *inTask);
		
		void SetTaskProgress(CMyTask *inTask, Uint32 inVal, Uint32 inMax, const Uint8 *inDesc);
		CMyTask *GetSelectedTask();
		void ShowFinishedBar(CMyTask *inTask);
		void SelectTask(Uint32 inLogID);
	
		// list events
		virtual void SetItemSelect(Uint32 inItem, bool inSelect);
	
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

	protected:
		CPtrList<SMyTaskInfo> mTaskList;

		SMyTaskInfo *TaskToInfo(CMyTask *inTask, Uint32& outIndex);
		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions);		
};

#pragma mark CMyServerListView1
class CMyServerListView1 : public CGeneralListView<SMyServerInfo>
{
	public:
		CMyServerListView1(CViewHandler *inHandler, const SRect &inBounds);
			
	protected:
		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);		
};

#pragma mark CMyServerListView2
class CMySelectServerWin2;
class CMyServerListView2 : public CGeneralListView<SMyServerBundleInfo>
{
	public:
		CMyServerListView2(CViewHandler *inHandler, const SRect &inBounds, CMySelectServerWin2 *inWin);
		
		// list events
		virtual void SetItemSelect(Uint32 inItem, bool inSelect);
	
	protected:
		CMySelectServerWin2 *mWin;
	
		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);		
};

#pragma mark CMyBundleListView
class CMyBundleListView : public CGeneralListView<TFSRefObj>
{
	public:
		CMyBundleListView(CViewHandler *inHandler, const SRect &inBounds);
		virtual ~CMyBundleListView();
			
	protected:
		virtual void ItemDraw(Uint32 inItem, TImage inImage, const SRect& inBounds, const SRect& inUpdateRect, Uint32 inOptions = 0);		
};

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Windows ₯₯

#pragma mark CMySearchText
class CMySearchText
{
	public:
		CMySearchText();
		virtual ~CMySearchText();

		void MakeSearchText(CContainerView *inContainerView, const SRect& inBounds);
		
	protected:
		TTimer mSearchTimer1;
		TTimer mSearchTimer2;
		
		CLabelView *searchText;
	
		bool SearchText_KeyDown(const SKeyMsgData& inInfo);
		virtual void SearchText(const Uint8 *inText) = 0;
		
		static void SearchTimer1(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
		static void SearchTimer2(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);	
};

#pragma mark CMySynchWin
class CMySynchWin : public CWindow, public CDragAndDropHandler, public CMySearchText
{
	public:
		CMySynchWin();
		virtual ~CMySynchWin();
	
		void SetInternalBounds(const SRect inViewsRect[14], const Uint8 inTabPercent[17]);
		void GetInternalBounds(SRect outViewsRect[14], Uint8 outTabPercent[17]);
		
		void SetServerInfo(SMyServerInfo *inServerInfo);
		Uint32 GetServerAddr(Uint8 *outServerAddr, Uint32 inMaxSize);
		Uint32 GetUserLogin(Uint8 *outUserLogin, Uint32 inMaxSize);
		Uint32 GetUserPassword(Uint8 *outUserPassword, Uint32 inMaxSize);
		Uint32 GetFilterText1(Uint8 *outFilterText, Uint32 inMaxSize);
		Uint32 GetFilterText2(Uint8 *outFilterText, Uint32 inMaxSize);

		CLabelView *GetServerNameView()				{	return mViews.pServerName;		}
		CLabelView *GetBundleNameView()				{	return mViews.pBundleName;		}

		CMyGroupListView1 *GetGroupListView1()		{	return mViews.pGroupList1;		}
		CMyArticleTreeView1 *GetArticleTreeView1()	{	return mViews.pArticleTree1;	}
		CTextView *GetArticleTextView1()			{	return mViews.pArticleText1;	}
	
		CMyGroupListView2 *GetGroupListView2()		{	return mViews.pGroupList2;		}
		CMyArticleTreeView2 *GetArticleTreeView2()	{	return mViews.pArticleTree2;	}
		CTextView *GetArticleTextView2()			{	return mViews.pArticleText2;	}
		
		CMyLogListView *GetLogListView()			{	return mViews.pLogList;			}
		CMyTaskListView *GetTaskListView()			{	return mViews.pTaskList;		}
		
		void AddGroup1(SMyGroupInfo1 *inGroupInfo, Uint8 *inFilterText);
		void FilterGroupList1(bool inFilter);
 		void DeleteAllGroup1();

		void AddGroup2(SMyGroupInfo2 *inGroupInfo, Uint8 *inFilterText);
		void FilterGroupList2(bool inFilter);
 		void DeleteAllGroup2();

		void UpdateSortBtn1(bool inDescending);
		void UpdateSortBtn2(bool inDescending);

 		void DeleteAllArticle1();
 		void DeleteAllArticle2();

		void AddAttachment1(SArticleData *inAttachmentData);
		SArticleData *GetSelectedAttachment1();
		void DeleteArticle1();

		void AddAttachment2(const Uint8 *inAttachmentName);
		Uint32 GetSelectedAttachment2(Uint8 *outAttachmentName, Uint32 inMaxSize);
		void DeleteArticle2();

		virtual void SearchText(const Uint8 *inText);
		virtual void KeyDown(const SKeyMsgData& inInfo);

		virtual void HandleDrag(CDragAndDroppable *inDD, const SMouseMsgData& inInfo);
		virtual bool HandleDrop(CDragAndDroppable *inDD, const SDragMsgData& inInfo);

	private:
	
		struct {
			CPaneView *pPaneView2, *pPaneView3, *pPaneView4, *pPaneView5, *pPaneView6, *pPaneView7;
			
			CTextView *pServerAddr, *pUserLogin;
			CPasswordTextView *pUserPassword;
			CLabelView *pServerName, *pBundleName;

			CTextView *pGroupFilter1;
			CLabelView *pGroupCount1;
			
			CContainerView *pGroupVc1;
			CMyGroupListView1 *pGroupList1;
			
			CScrollerView *pArticleTreeScr1;
			CMyArticleTreeView1 *pArticleTree1;
			CButtonView *pSortArticlesBtn1;

			CContainerView *pArticleVc1;
			CTextView *pArticleText1;
			CButtonPopupView *pAttachmentButton1;
			CMenuListView *pAttachmentList1;

			CTextView *pGroupFilter2;
			CLabelView *pGroupCount2;
			
			CContainerView *pGroupVc2;
			CMyGroupListView2 *pGroupList2;
			
			CScrollerView *pArticleTreeScr2;
			CMyArticleTreeView2 *pArticleTree2;
			CButtonView *pSortArticlesBtn2;
			
			CContainerView *pArticleVc2;
			CTextView *pArticleText2;
			CButtonPopupView *pAttachmentButton2;
			CMenuListView *pAttachmentList2;
			
			CScrollerView *pLogListScr;
			CMyLogListView *pLogList;
			
			CScrollerView *pTaskListScr;
			CMyTaskListView *pTaskList;
		} mViews;
		
		CPtrList<SArticleData> mAttachmentList1;
		TTimer mFilterTimer1, mFilterTimer2;

		CLabelView *MakeGroupCount(CContainerView *inContainerView, const SRect& inBounds);

		void FilterGroupList1();
		void FilterGroupList2();
		static void FilterTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
};

#pragma mark CMySynchList
class CMySynchList
{
	public:
		CMySynchList();
		virtual ~CMySynchList();
	
		void AddSynch(SMySynchInfo *inSynchInfo);
		void DeleteSynch(SMySynchInfo *inSynchInfo);
		bool GetNextSynch(SMySynchInfo*& ioSynchInfo, Uint32& ioIndex);
	
	private:
		CPtrList<SMySynchInfo> mSynchList;
};

#pragma mark CMyOptionsWin
class CMyOptionsWin : public CWindow
{
	public:
		CMyOptionsWin();
		virtual ~CMyOptionsWin();
			
		void SetSynchInfo(const SMySynchInfo *inSynchInfo);
		void SetInfo(const SMyOptions& inOptions, CPtrList<SMySynchInfo> *inSynchList, const Uint8 *inNewsServer, const Uint8 *inHotlineServer);
		void GetInfo(SMyOptions& outOptions);
		
		void AddSynch(const SMySynchInfo *inSynchInfo = nil);
		void EditSynch();
		void DeleteSynch();
		void MakeSynch();

		void FilterList();
		void RefreshList();

	protected:
		struct {
			CTabbedView *tabs;
					
			CTextView *emailText, *organizationText, *addArticleText, *purgeDaysGenText, *maxArticlesGenText;
			CCheckBoxView *logToFile, *oldNewsFormat;
			
			CScrollerView *synchScroll;
			CTextView *pSynchFilter, *serverText1, *serverText2, *loginText1, *bundleText2, *groupText1, *groupText2;
			CLabelView *pSynchCount, *passwordLbl1, *synchHoursLbl, *purgeDaysLbl, *maxArticlesLbl, *lastSynchLbl, *nextSynchLbl;
			CCheckBoxView *showNewsServer, *showHotlineServer;
			CMySynchListView *synchList;			
		} mViews;
		
		CPtrList<SMySynchInfo> *mSynchList;
		CMySynchList mNewSynchList;
		TTimer mFilterTimer;
		
		Uint8 mDispPassword[8];

		void AddSynch(const SMySynchInfo& inSynchInfo);
		void EditSynch(const SMySynchInfo& inSynchInfo);
		void EditSynch(const SMySynchInfo& inSynchInfo, SMySynchInfo *inEditSynchInfo, bool bActivate = false);
		void AddEditSynch(bool inAddEditSynch, const SMySynchInfo *inSynchInfo = nil);
		
		void MakeNewSynch();
		void RefreshList(const Uint8 *inNewsServer, const Uint8 *inHotlineServer, SMySynchInfo *inSelSynchInfo = nil);
		void RefreshSynchList(const Uint8 *inNewsServer, const Uint8 *inHotlineServer, SMySynchInfo *inSelSynchInfo = nil);
		void DisplaySynchCount();
		void ClearSynchInfo();

		void SetSynchDate(const SCalendarDate& inLastSynch, Uint32 inHours);
		Uint32 GetPurgeDays();
		Uint32 GetMaxArticles();

		CContainerView *MakeGeneralTab();
		CContainerView *MakeSynchTab();		
		CLabelView *MakeSynchCount(CContainerView *inContainerView, const SRect& inBounds);

		static void FilterTimerProc(void *inContext, void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
};

#pragma mark CMyAddEditSynchWin
class CMyAddEditSynchWin : public CWindow
{
	public:
		CMyAddEditSynchWin(bool inAddEditSynch, Uint32 inPurgeDays, Uint32 inMaxArticles);
		virtual ~CMyAddEditSynchWin();
				
		void SetSynchInfo(const SMySynchInfo& inSynchInfo);
		bool GetSynchInfo(SMySynchInfo& outSynchInfo);
		void SelectBundle();
		void SelectGroup();

	protected:
		struct {
			CTextView *serverText1, *serverText2, *loginText1, *bundleText2, *groupText1, *groupText2, 
					  *synchHoursText, *purgeDaysText, *maxArticlesText;
			CPasswordTextView *passwordText1;
		} mViews;				

		TFSRefObj* mBundleRef;
};

#pragma mark CMySelectServerWin1
class CMySelectServerWin1 : public CWindow
{
	public:
		CMySelectServerWin1();
				
		CMyServerListView1 *GetServerListView()		{	return mViews.serverList;	}

	protected:
		struct {
			CMyServerListView1 *serverList;
		} mViews;
};

#pragma mark CMySelectServerWin2
class CMySelectServerWin2 : public CWindow
{
	public:
		CMySelectServerWin2();
				
		void SetBundleList(SMyServerBundleInfo *inServerBundleInfo);

		CMyServerListView2 *GetServerListView()		{	return mViews.serverList;	}
		bool GetSelectedServer(SMyServerBundleInfo*& outServerBundleInfo, TFSRefObj*& outBundle);

	protected:
		struct {
			CMyServerListView2 *serverList;
			CMyBundleListView *bundleList;
		} mViews;
};

/* ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ */
#pragma mark -
#pragma mark ₯₯ Application ₯₯

#pragma mark CMyApplication
class CMyApplication : public CApplication
{
	friend class CMyConnectTask;
	friend class CMySynchTask;
	
	public:
		// construction
		CMyApplication();
		virtual ~CMyApplication();
		void StartUp();
		
		void DoRefresh();
		void DoAddSynch();
		void DoSynch();
		bool DoOptions(const SMySynchInfo *inSynchInfo = nil);
		void DoStopTask();
		
		bool UseOldNewsFormat();
		Uint8 *GetEmailAddress();
		Uint8 *GetOrganization();
		void *GetAddArticleText(Uint32& outSize);
		Uint32 GetServerNameBundle(TFSRefObj* inBundleRef, Uint8 *outServerName, Uint32 inMaxSize);
		Uint32 GetServerNameGroup(TFSRefObj* inGroupFile, Uint8 *outServerName, Uint32 inMaxSize);
		
		// server news
		void DoSelectServer1();
		void DoListGroups1();
		bool DoListGroups1(Uint32& inGroupIndex, const Uint8 *inServerAddr, TNntpTransact inNntpTransact);
		void DoListArticles1();
		bool DoListArticles1(Uint32& inArticleIndex, const Uint8 *inGroupName, TNntpTransact inNntpTransact, CPtrList<SMyArticleInfo1>& inTempArticleList, bool inFinish);
		void DoListArticles1(CPtrList<SMyArticleInfo1>& inTempArticleList, CPtrList<SMyArticleInfo1>& inSearchArticleList);
		void DoGetArticle1();
		void DoGetArticle1(Uint32 inArticleNumber, TNntpTransact inNntpTransact);

		// hotline news
		void DoSelectServer2();
		void SetServerName2();
		void DoSelectBundle2();
		void DoListGroups2();
		void DoListArticles2();
		void DoOpenArticle2();

		// synchronization
		void DoSynchGroup1(const Uint8 *inGroupName1, const Uint8 *inGroupName2);
		void DoSynchArticle1(Uint32 inArticleID2, const Uint8 *inArticleName2, CPtrList<SMyDataInfo2>& inDataList2, CPtrList<Uint8>& inParentIDs1);
		void DoSynchGroup2(const Uint8 *inGroupName1, const Uint8 *inGroupName2);
		void DoSynchArticle2(Uint32 inArticleID1, const Uint8 *inArticleName1, Uint32 inParentID2);

		void MakeSynch(SMySynchInfo& inSynchInfo);

		// logs
		Uint32 MakeLog(const Uint8 *inLogText);
		void AddLog(Uint32 inLogID, const Uint8 *inLogText);
		void SelectLog(Uint32 inLogID);
		void LogToFile(const Uint8 *inData);
		void LogToFile(const void *inData, Uint32 inSize);
		
		// tasks
		void AddTask(CMyTask *inTask, const Uint8 *inDesc);
		void RemoveTask(CMyTask *inTask);
		void FinishTask(CMyTask *inTask);
		void SetTaskProgress(CMyTask *inTask, Uint32 inVal, Uint32 inMax, const Uint8 *inDesc);
		void SelectTask(Uint32 inLogID);

		// quit
		virtual void UserQuit();
		
		// message handlers
		virtual void WindowHit(CWindow *inWindow, const SHitMsgData& inInfo);
		virtual void HandleMessage(void *inObject, Uint32 inMsg, const void *inData, Uint32 inDataSize);
		virtual void Timer(TTimer inTimer);

	protected:
		SMyOptions mOptions;
		CMySynchWin *mSynchWin;
		
		CPtrList<SMyServerInfo> mServerList1;
		CPtrList<SMyServerBundleInfo> mServerList2;
		CPtrList<SMyOffSynchInfo> mSynchList;
		TTimer mSynchTimer;

		SMyServerInfo mServerInfo;

		SNewsGroupInfo mGroupInfo;
		TFSRefObj* mBundleRef;

		CPtrList<CMyTask> mTaskList;
		TTimer mFinishTaskTimer;
		
		// automatic synchronization
		void MakeAutoSynch();
		
		void DoGroupFilter1(const SHitMsgData& inInfo);
		void DoGroupFilter2(const SHitMsgData& inInfo);
		
		void SortArticles1();
		void SortArticles2();
		void SaveAttachment1();
		void SaveAttachment2();

		// tasks
		void ProcessTasks();
		bool StopListGroupsTask(const Uint8 *inServerAddr);
		bool StopListArticlesTask(const Uint8 *inServerAddr, const Uint8 *inGroupName);
		bool IsGetArticleTask(const Uint8 *inServerAddr, const Uint8 *inGroupName, Uint32 inArticleID);
		bool IsSynchArticleTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, Uint32 inArticleID2);
		bool IsSynchArticleTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, Uint32 inArticleID1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2);
		bool IsSynchGroupTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll = true);
		bool IsSynchGroupTask1(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll = true);
		bool IsSynchGroupTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll = true);
		bool IsSynchGroupTask2(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll = true);
		bool IsSynchGroupTask21(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFile, bool inSearchAll = true);
		bool IsSynchGroupTask21(const Uint8 *inServerAddr, const Uint8 *inGroupName1, TFSRefObj* inGroupFolder, const Uint8 *inGroupName2, bool inSearchAll = true);
		void KillFinishedTasks();
		void KillAllTasks();

		TFSRefObj* GetPrefsRef();
		bool WritePrefs();
		bool ReadPrefs(SRect& outWinRect, SRect outViewsRect[14], Uint8 outTabPercent[17]);

		TFSRefObj* GetSynchRef();
		void MakeSynchBackup();
		void WriteSynchInfo(CPtrList<SMySynchInfo> *inSynchList);
		void WriteLastSynch(Uint32 inDateOffset, SCalendarDate& inLastSynch);
		void ReadSynchInfo(CPtrList<SMySynchInfo> *outSynchList);
		void ReadSynchList();
		void ClearSynchList();
		void ClearSynchList(CPtrList<SMySynchInfo> *inSynchList);
		
		bool AddServerList(const SMySynchInfo& inSynchInfo);
		void AddServerList1(const SMyServerInfo& inServerInfo);
		void AddServerList2(const Uint8 *inServerName, TFSRefObj* inBundleRef);
		Uint32 SearchServerList2(TFSRefObj* inBundleRef, Uint8 *outServerName, Uint32 inMaxSize);
		void ClearServerList1();
		void ClearServerList2();
};

