/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */



#include "UCheckBox.h"



void UCheckBox_Draw(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);

void UCheckBox_DrawFocused(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);

void UCheckBox_DrawHilited(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);

void UCheckBox_DrawDisabled(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo);



void _SetVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin);

void _GetVirtualOrigin(TImage inImage, SPoint& outVirtualOrigin);

void _ResetVirtualOrigin(TImage inImage);



/* -------------------------------------------------------------------------- */



void UCheckBox::Draw(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UCheckBox_Draw(inImage, inBounds, inInfo);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UCheckBox_Draw(inImage, stBounds, inInfo);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UCheckBox::DrawFocused(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UCheckBox_DrawFocused(inImage, inBounds, inInfo);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UCheckBox_DrawFocused(inImage, stBounds, inInfo);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UCheckBox::DrawHilited(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UCheckBox_DrawHilited(inImage, inBounds, inInfo);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UCheckBox_DrawHilited(inImage, stBounds, inInfo);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UCheckBox::DrawDisabled(TImage inImage, const SRect& inBounds, const SCheckBoxInfo& inInfo)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UCheckBox_DrawDisabled(inImage, inBounds, inInfo);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UCheckBox_DrawDisabled(inImage, stBounds, inInfo);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}

