/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "UIconButton.h"

void UIconButton_Draw(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo);
void UIconButton_DrawFocused(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo);
void UIconButton_DrawHilited(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo);
void UIconButton_DrawDisabled(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo);

void _SetVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin);
void _GetVirtualOrigin(TImage inImage, SPoint& outVirtualOrigin);
void _ResetVirtualOrigin(TImage inImage);

/* -------------------------------------------------------------------------- */

void UIconButton::Draw(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UIconButton_Draw(inImage, inBounds, inInfo);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UIconButton_Draw(inImage, stBounds, inInfo);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

void UIconButton::DrawFocused(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UIconButton_DrawFocused(inImage, inBounds, inInfo);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UIconButton_DrawFocused(inImage, stBounds, inInfo);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

void UIconButton::DrawHilited(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UIconButton_DrawHilited(inImage, inBounds, inInfo);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UIconButton_DrawHilited(inImage, stBounds, inInfo);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

void UIconButton::DrawDisabled(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo)
{
	// get virtual origin
	SPoint stVirtualOrigin;
	_GetVirtualOrigin(inImage, stVirtualOrigin);

	if (stVirtualOrigin.IsNull())
		UIconButton_DrawDisabled(inImage, inBounds, inInfo);
	else
	{
		// reset virtual origin
		_ResetVirtualOrigin(inImage);

		// recalculate rect
		SRect stBounds = inBounds;
		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);
	
		// draw
		UIconButton_DrawDisabled(inImage, stBounds, inInfo);

		// restore virtual origin
		_SetVirtualOrigin(inImage, stVirtualOrigin);
	}
}

/* -------------------------------------------------------------------------- */
#pragma mark -

void UIconButton::CalcRects(TImage inImage, const SRect& inBounds, const SIconButtonInfo& inInfo, SRect *outButtonRect, SRect *outTextRect, SRect *outIconRect)
{
	Uint32 textRectHeight;

	if (inInfo.options & iconBtn_TitleLeft)
	{
		// get size of icon
		Uint32 iconWidth;
		if (inInfo.icon == nil)
			iconWidth = 16;
		else
		{
#if MACINTOSH
			iconWidth = inInfo.icon->GetWidth();
#else
			Uint32 iconHeight;
			UPixmap::GetLayerSize(inInfo.icon, inInfo.iconLayer, iconWidth, iconHeight);
#endif
		}
		
		// calc button rect
		if (outButtonRect) *outButtonRect = inBounds;
		
		// calc text rect
		if (outTextRect) outTextRect->Set(inBounds.left + iconWidth + 6 + 4, inBounds.top, inBounds.right, inBounds.bottom);

		// calc icon rect
		if (outIconRect) outIconRect->Set(inBounds.left + 6, inBounds.top + 4, inBounds.left + 6 + iconWidth, inBounds.bottom - 4);
	}
	else if (inInfo.options & iconBtn_TitleOutside)
	{
		// calc text rect
		if (outTextRect)
		{
			if (inInfo.title == 0 || inInfo.titleSize == 0)
			{
				outTextRect->SetEmpty();
				textRectHeight = 0;
			}
			else
			{
				textRectHeight = inImage->GetFontHeight();
				outTextRect->Set(inBounds.left, inBounds.bottom - textRectHeight, inBounds.right, inBounds.bottom);
			}
		}
		else
		{
			textRectHeight = (inInfo.title == 0 || inInfo.titleSize == 0) ? 0 : inImage->GetFontHeight();
		}
		
		// calc button/icon rect
		if (outButtonRect || outIconRect)
		{
			SRect r = inBounds;
			
			r.right = r.left + 44;
			Int32 rw = r.GetWidth();
			
			if (rw >= inBounds.GetWidth())
				r.right = inBounds.right;
			else
			{
				// center horiz
				r.left += (inBounds.GetWidth() - rw) / 2;
				r.right = r.left + rw;
			}
			
			r.bottom = r.top + 44;
			Int32 rh = r.GetHeight();
			
			if (rh >= (inBounds.GetHeight() - textRectHeight))
				r.bottom = inBounds.bottom - textRectHeight;
			else
			{
				// center vert
				r.top += (inBounds.GetHeight() - rh - textRectHeight) / 2;
				r.bottom = r.top + rh;
			}
			
			if (outIconRect) *outIconRect = r;
			if (outButtonRect) *outButtonRect = r;
		}
	}
	else
	{
		if (outButtonRect) *outButtonRect = inBounds;
		
		// calc text rect
		if (outTextRect)
		{
			if (inInfo.title == 0 || inInfo.titleSize == 0)
			{
				outTextRect->SetEmpty();
				textRectHeight = 0;
			}
			else
			{
				textRectHeight = inImage->GetFontHeight();
				outTextRect->Set(inBounds.left, inBounds.bottom - textRectHeight - 4, inBounds.right, inBounds.bottom - 4);
			}
		}
		else
		{
			textRectHeight = (inInfo.title == 0 || inInfo.titleSize == 0) ? 0 : inImage->GetFontHeight();
		}
		
		// calc icon rect
		if (outIconRect) outIconRect->Set(inBounds.left, inBounds.top + 4, inBounds.right, inBounds.bottom - textRectHeight - 4);
	}
}

