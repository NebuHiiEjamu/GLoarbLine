/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#pragma once
#include "typedefs.h"

class UMime
{
	public:
		// conversion between file extension and MIME format
		static const Int8 *ConvertExtension_Mime(const Int8 *inExtension);
		static const Int8 *ConvertMime_Extension(const Int8 *inMime);

		// conversion from file name with extension to MIME format
		static const Int8 *ConvertNameExtension_Mime(const Uint8 *inName);

		// conversion between type code and MIME format
		static const Int8 *ConvertTypeCode_Mime(Uint32 inTypeCode);
		static Uint32 ConvertMime_TypeCode(const Int8 *inMime); 
};
