/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "CImage.h"

HL_Begin_Namespace_BigRedH


// ---------------------------------------------------------------------
//  NotifyChanged											 [protected]
// ---------------------------------------------------------------------
// Helper function for derived classes (it sends the notification that 
// this image has changed).
// Called when another chunk of the image available to be drawn on screen

void
CImage::NotifyChanged(const CRect& inBounds)
{
	BroadcastMessage(ImageChanged(eChanged,inBounds,*this));
}


// ---------------------------------------------------------------------
//  NotifyImageComplete		                                 [protected]
// ---------------------------------------------------------------------
// Called when the incoming image is completely downloaded
//?? does nothing for the moment

void
CImage::NotifyImageComplete()
{
}

HL_End_Namespace_BigRedH
