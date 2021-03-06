/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"
#include "CView.h"
#include "CPtrList.h"

class CViewContainer : public CViewHandler
{
	public:
		virtual bool GetNextView(CView*& ioView, Uint32& ioIndex) const = 0;
		CView *GetViewByID(Uint32 inID) const;
		CView *GetDeepViewByID(Uint32 inID) const;
};

class CSingleViewContainer : public CViewContainer
{
	public:
		CSingleViewContainer();
		virtual ~CSingleViewContainer();
		
		virtual void SetView(CView *inView);
		virtual CView *DetachView();
		CView *GetView() const;
		
		virtual bool GetNextView(CView*& ioView, Uint32& ioIndex) const;

	protected:
		CView *mView;
		
		virtual void HandleInstall(CView *inView);
		virtual void HandleRemove(CView *inView);
};

class CMultiViewContainer : public CViewContainer
{	
	public:
		CMultiViewContainer();
		virtual ~CMultiViewContainer();
	
		void InstallView(CView *inView);
		void RemoveView(CView *inView);
	
		Uint32 SetViewOrder(CView *inView, Uint32 inIndex);
		bool InvertViewOrder(CView *inView1, CView *inView2);

		virtual bool GetNextView(CView*& ioView, Uint32& ioIndex) const;
	
	protected:
		CPtrList<CView> mViewList;
		Uint16 mIsDestructing : 1;
		
		virtual void HandleInstall(CView *inView);
		virtual void HandleRemove(CView *inView);
};



