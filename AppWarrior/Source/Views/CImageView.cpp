/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "CImageView.h"


CImageView::CImageView(CViewHandler *inHandler, const SRect& inBounds, CDecompressImage *inDecompressImage)
	: CLaunchUrlView(inHandler, inBounds)
{
	mImage = nil;
	mDecompressImage = inDecompressImage;

	mWidth = 0;
	mHeight = 0;

	mIsComplete = false;
}
 
CImageView::~CImageView()
{
	if (mImage)
		UGraphics::DisposeImage(mImage);
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void CImageView::Draw(TImage inImage, const SRect& inUpdateRect, Uint32 /* inDepth */)
{
    if (mWidth && mHeight)
    {
	    SRect rViewRect;
		GetBounds(rViewRect);

		SPoint ptImage(rViewRect.left, rViewRect.top); 		
		SRect rImage(ptImage.x, ptImage.y, ptImage.x + mWidth, ptImage.y + mHeight);
	
	 	SRect rUpdateRect;
		inUpdateRect.GetIntersection(rImage, rUpdateRect);
    
    	if (rUpdateRect.IsNotEmpty() && rUpdateRect.IsValid())
    	{
    		if(mDecompressImage)
    			mDecompressImage->Draw(inImage, rUpdateRect.TopLeft(), SPoint(rUpdateRect.left-ptImage.x, rUpdateRect.top-ptImage.y), rUpdateRect.GetWidth(), rUpdateRect.GetHeight());
		    else if(mImage)
    		{
	    		StImageLocker lockImage(mImage);
	    		UGraphics::CopyPixels(inImage, rUpdateRect.TopLeft(), mImage, SPoint(rUpdateRect.left-ptImage.x, rUpdateRect.top-ptImage.y), rUpdateRect.GetWidth(), rUpdateRect.GetHeight());	
    		}
    	}	
	}    
}

/* -------------------------------------------------------------------------- */
#pragma mark -

bool CImageView::GetImageSize(Uint32& outWidth, Uint32& outHeight)
{
	if (!mWidth || !mHeight)
		return false;

	outWidth = mWidth;
	outHeight = mHeight;
		
	return true;
}

bool CImageView::IsComplete()
{
	return mIsComplete;
}

bool CImageView::SetImage(Uint32 inWidth, Uint32 inHeight)
{
   	Uint32 nWidth, nHeight;
   	if (!inWidth || !inHeight)
   	{
		if(!mDecompressImage || !mDecompressImage->GetSize(mWidth, mHeight))
			return false;
		
		nWidth = mWidth;
    	nHeight = mHeight;
    }
    else
    {
    	nWidth = inWidth;
    	nHeight = inHeight;
    }
    
	SRect rImageView;
	GetBounds(rImageView);

	if (rImageView.GetWidth() != nWidth || rImageView.GetHeight() != nHeight)
	{
		rImageView.right = rImageView.left + nWidth;
		rImageView.bottom = rImageView.top + nHeight;

		SetBounds(rImageView);	

		Refresh();	
		return true;
	}

	Refresh();
	return false;
}

bool CImageView::FinishImage()
{
	if (!mDecompressImage || !mDecompressImage->IsComplete())
		return false;
	
	mImage = mDecompressImage->DetachImagePtr();
	mDecompressImage = nil;

	mIsComplete = true;

	Refresh();
	return true;
}

