/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CViewContainer.h"
#include "CPtrList.h"

/* -------------------------------------------------------------------------- */

CView *CViewContainer::GetViewByID(Uint32 inID) const
{
	Uint32 i = 0;
	CView *v;
	
	while (GetNextView(v, i))
	{
		if (v->GetID() == inID)
			return v;
	}
	
	return nil;
}

CView *CViewContainer::GetDeepViewByID(Uint32 inID) const
{
	CView *v = GetViewByID(inID);	// we want to find the shallowest view with that ID

	if (v == nil)
	{
		CViewContainer *vc;
		CView *iv;
		Uint32 i = 0;
		
		while (GetNextView(iv, i))
		{
			vc = dynamic_cast<CViewContainer*>(iv);		// don't you just love RTTI?
			
			if (vc)
			{
				v = vc->GetDeepViewByID(inID);			// and recursive functions too
				if (v) break;
			}
		}
	}
	
	return v;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

CSingleViewContainer::CSingleViewContainer()
{
	mView = nil;
}

CSingleViewContainer::~CSingleViewContainer()
{
	if (mView)
	{
		CView *v = mView;
		mView = nil;
		delete v;
	}
}

void CSingleViewContainer::SetView(CView *inView)
{
	if (inView)
		inView->SetHandler(this);
	else
		HandleInstall(nil);
}

CView *CSingleViewContainer::DetachView()
{
	if (mView)
	{
		CView *v = mView;
		v->SetHandler(nil);
		return v;
	}
	return nil;
}

CView *CSingleViewContainer::GetView() const
{
	return mView;
}

bool CSingleViewContainer::GetNextView(CView*& ioView, Uint32& ioIndex) const
{
	ioView = (ioIndex == 0 ? mView : nil);
	return (ioView != nil);
}

void CSingleViewContainer::HandleInstall(CView *inView)
{
	if (mView != inView)
	{
		if (mView)
		{
			CView *v = mView;
			mView = nil;
			delete v;
		}
		mView = inView;
	}
}

void CSingleViewContainer::HandleRemove(CView *inView)
{
	if (mView == inView)
		mView = nil;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

CMultiViewContainer::CMultiViewContainer()
{
	mIsDestructing = false;
}

CMultiViewContainer::~CMultiViewContainer()
{
	Uint32 i = 0;
	CView *v;

	// make sure HandleRemove() doesn't touch mViewList
	mIsDestructing = true;

	// delete all the views
	while (mViewList.GetNext(v, i))
	{
		try { delete v; } catch(...) {}
	}
}

void CMultiViewContainer::InstallView(CView *inView)
{
	if (inView)
		inView->SetHandler(this);
}

void CMultiViewContainer::RemoveView(CView *inView)
{
	if (inView && inView->GetHandler() == this)
		inView->SetHandler(nil);
}

Uint32 CMultiViewContainer::SetViewOrder(CView *inView, Uint32 inIndex)
{
	if (!inView || !inIndex)
		return 0;
	
	Uint32 i = 0;
	CView *pView;

	while (mViewList.GetNext(pView, i))
	{
		if (pView == inView)
		{
			mViewList.RemoveItem(i);
			
			if (inIndex > mViewList.GetItemCount())
				return mViewList.AddItem(inView);
			else
				return mViewList.InsertItem(inIndex, inView);
		}
	}
	
	return 0;
}

bool CMultiViewContainer::InvertViewOrder(CView *inView1, CView *inView2)
{
	if (!inView1 || !inView2)
		return false;

	Uint32 nIndex1 = 0;
	Uint32 nIndex2 = 0;
	
	Uint32 i = 0;
	CView *pView;

	while (mViewList.GetNext(pView, i))
	{
		if (pView == inView1)
		{
			nIndex1 = i;
			if (nIndex2) break;
		}
		else if (pView == inView2)
		{
			nIndex2 = i;
			if (nIndex1) break;
		}
	}
	
	if (!nIndex1 || !nIndex2)
		return false;
		
	mViewList.SetItem(nIndex1, inView2);
	mViewList.SetItem(nIndex2, inView1);

	return true;
}

bool CMultiViewContainer::GetNextView(CView*& ioView, Uint32& ioIndex) const
{
	return mViewList.GetNext(ioView, ioIndex);
}

void CMultiViewContainer::HandleInstall(CView *inView)
{
	mViewList.AddItem(inView);
}

void CMultiViewContainer::HandleRemove(CView *inView)
{
	/*
	 * If we're destructing, then this function was called because we're
	 * deleting all views (CView::~CView() calls mHandler->HandleRemove()).
	 * We don't want to alter the view list at this stage, or it will stuff
	 * our destructor up.
	 */
	if (!mIsDestructing)
		mViewList.RemoveItem(inView);
}



