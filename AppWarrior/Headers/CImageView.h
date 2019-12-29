/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "CLaunchUrlView.h"
#include "CDecompressImage.h"


class CImageView : public CLaunchUrlView
{
	public:
		// construction
		CImageView(CViewHandler *inHandler, const SRect& inBounds, CDecompressImage *inDecompressImage);
		virtual ~CImageView();
		
		// drawing
		virtual void Draw(TImage inImage, const SRect& inUpdateRect, Uint32 inDepth);

		// play
		virtual void PlayImage(){};
		virtual void StopImage(){};

		// set 
		virtual bool SetImage(Uint32 inWidth=0, Uint32 inHeight=0);
		virtual bool FinishImage();

		// get 
		bool GetImageSize(Uint32& outWidth, Uint32& outHeight);
		bool IsComplete();	

	protected:
		TImage mImage;
		CDecompressImage *mDecompressImage;

		Uint32 mWidth;
		Uint32 mHeight;

		bool mIsComplete;
};
