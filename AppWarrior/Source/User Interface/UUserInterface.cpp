/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */


#include "UUserInterface.h"



void UUserInterface_DrawStandardBox(TImage inImage, const SRect& inBounds, const SColor *inFillColor, bool inIsDisabled, bool inCanFocus, bool inIsFocus);

void UUserInterface_DrawEtchedBox(TImage inImage, const SRect& inBounds, const void *inTitle, Uint32 inTitleSize);

void UUserInterface_DrawSunkenBox(TImage inImage, const SRect& inBounds, const SColor *inColor);

void UUserInterface_DrawRaisedBox(TImage inImage, const SRect& inBounds, const SColor *inColor);

void UUserInterface_DrawBarBox(TImage inImage, const SRect& inBounds);



void _SetVirtualOrigin(TImage inImage, const SPoint& inVirtualOrigin);

void _GetVirtualOrigin(TImage inImage, SPoint& outVirtualOrigin);

void _ResetVirtualOrigin(TImage inImage);



/* -------------------------------------------------------------------------- */



void UUserInterface::DrawStandardBox(TImage inImage, const SRect& inBounds, const SColor *inFillColor, bool inIsDisabled, bool inCanFocus, bool inIsFocus)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UUserInterface_DrawStandardBox(inImage, inBounds, inFillColor, inIsDisabled, inCanFocus, inIsFocus);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UUserInterface_DrawStandardBox(inImage, stBounds, inFillColor, inIsDisabled, inCanFocus, inIsFocus);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UUserInterface::DrawEtchedBox(TImage inImage, const SRect& inBounds, const void *inTitle, Uint32 inTitleSize)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UUserInterface_DrawEtchedBox(inImage, inBounds, inTitle, inTitleSize);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UUserInterface_DrawEtchedBox(inImage, stBounds, inTitle, inTitleSize);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UUserInterface::DrawSunkenBox(TImage inImage, const SRect& inBounds, const SColor *inColor)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UUserInterface_DrawSunkenBox(inImage, inBounds, inColor);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UUserInterface_DrawSunkenBox(inImage, stBounds, inColor);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UUserInterface::DrawRaisedBox(TImage inImage, const SRect& inBounds, const SColor *inColor)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UUserInterface_DrawRaisedBox(inImage, inBounds, inColor);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UUserInterface_DrawRaisedBox(inImage, stBounds, inColor);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



void UUserInterface::DrawBarBox(TImage inImage, const SRect& inBounds)

{

	// get virtual origin

	SPoint stVirtualOrigin;

	_GetVirtualOrigin(inImage, stVirtualOrigin);



	if (stVirtualOrigin.IsNull())

		UUserInterface_DrawBarBox(inImage, inBounds);

	else

	{

		// reset virtual origin

		_ResetVirtualOrigin(inImage);



		// recalculate rect

		SRect stBounds = inBounds;

		stBounds.MoveBack(stVirtualOrigin.x, stVirtualOrigin.y);

	

		// draw

		UUserInterface_DrawBarBox(inImage, stBounds);



		// restore virtual origin

		_SetVirtualOrigin(inImage, stVirtualOrigin);

	}

}



