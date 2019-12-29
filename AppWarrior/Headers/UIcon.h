/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

typedef class TIconObj *TIcon;

class UIcon
{
	public:
		static void Init();
		static void Purge();
		static TIcon Load(Int32 inID);
		static void Release(TIcon inIcon);
		static void Draw(TIcon inIcon, TImage inImage, const SRect& inBounds, Uint16 inAlign = align_Center, Uint16 inTransform = transform_None);
		static void Draw(Int32 inID, TImage inImage, const SRect& inBounds, Uint16 inAlign = align_Center, Uint16 inTransform = transform_None);
		static Uint16 GetHeight(TIcon inIcon);
		static Uint16 GetWidth(TIcon inIcon);
		static Int32 GetID(TIcon inIcon);
};

class TIconObj
{
	public:
		void Draw(TImage inImage, const SRect& inBounds, Uint16 inAlign = align_Center, Uint16 inTransform = transform_None)	{	UIcon::Draw(this, inImage, inBounds, inAlign, inTransform);		}
		Uint16 GetHeight()																										{	return UIcon::GetHeight(this);									}
		Uint16 GetWidth()																										{	return UIcon::GetWidth(this);									}
		Int32 GetID()																											{	return UIcon::GetID(this);										}
		
		void operator delete(void *p)																							{	UIcon::Release((TIcon)p);										}
	protected:
		TIconObj() {}				// force creation via UIcon
};


