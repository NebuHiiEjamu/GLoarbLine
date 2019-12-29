/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */



#pragma once

#include "typedefs.h"

#include "CWindow.h"

#include "CButtonView.h"

#include "CContainerView.h"





class CWizard : protected CWindow

{

	public:

		CWizard();

		~CWizard();

		

		void SetTitle(const Uint8 *inTitle)			{	CWindow::SetTitle(inTitle);		}

		void AddPane(CView *inPaneView);

		

		bool Process();	// false = canceled, true = finished



	protected:

		CContainerView *mContainerView;

		CButtonView *mCancelBtn, *mBackBtn, *mNextBtn, *mFinishBtn;

		

		struct SMyViewLink

		{

			CView *view;

			SMyViewLink *prev;

			SMyViewLink *next;

		};

		

		SMyViewLink *mHeadPane;

		SMyViewLink *mCurrentPane;



		void GoNext();

		void GoBack();

		

		void Finish();

		void Reset();

};

