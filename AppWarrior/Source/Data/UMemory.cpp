/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
/*
Functions for working with memory blocks.  UMemory works with three types of
pointers.

-- void * / const void * --

Functions that accept pointers of type "void *" or "const void *" work with
any pointer.  For example, the address of a stack variable, a TPtr, or the
pointer from a locked THdl.

-- TPtr --

When a function expects a TPtr, you MUST pass a real TPtr. That is, as
allocated via UMemory::New() or similiar.  For example, typecasting the
address of a stack variable to a TPtr will probably cause a crash.

Note that TPtr types cannot be resized.  Use a THdl if you need to resize the
allocation.  Alternatively, Reallocate() can be used to resize a TPtr, but it
may change the address/value of the TPtr.  Furthermore, you cannot obtain the
size of a TPtr.  If you need the size, either store it yourself, or use a
THdl.

-- THdl --

When a function expects a THdl, you MUST pass a real THdl.  That is, as
allocated via UMemory::NewHandle() or similiar.  Note that unlike TPtr's,
you CANNOT typecast the THdl to access the data.  You must use the Lock()
function.

-- Debugging --

When compiled for debugging, UMemory will verify TPtr's and THdl's before using 
them.  If a TPtr or THdl fails the test, the function will drop into the 
debugger with a warning message.

When a block is allocated, it is filled with the plus ('+') character. This
helps you detect use of uninitialized memory.  Just before a block is released,
it is filled with 0xF3.  This helps you detect use of released memory.

-- Errors --

100		An unknown memory related error has occured.
101		Not enough memory.  Try closing windows and saving documents.
102		Memory block is not valid.
*/

#include "UMemory.h"

static bool _IsDelim(Uint8 inChar, const Int8 *inDelim);

/* -------------------------------------------------------------------------- */

void UMemory::Fill(void *inData, Uint32 n, Uint8 inByte)
{
	unsigned char *	cpd	= ((unsigned char *) inData);
	unsigned long *	lpd	= ((unsigned long *) inData);

	unsigned long			v = (unsigned char) inByte;
	unsigned long			i;
	
#if USE_PRE_INCREMENT
	cpd = ((unsigned char *) inData) - 1;
#endif
	
	if (n >= 32)
	{
		i = (- (unsigned long) inData) & 3;
		
		if (i)
		{
			n -= i;
			
			do
#if USE_PRE_INCREMENT
				*++cpd = v;
#else
				*cpd++ = v;
#endif
			while (--i);
		}
	
		if (v)
			v |= v << 24 | v << 16 | v <<  8;
		
#if USE_PRE_INCREMENT
		lpd = ((unsigned long *) (cpd + 1)) - 1;
#else
		lpd = ((unsigned long *) (cpd));
#endif
		
		i = n >> 5;
		
		if (i)
			do
			{
#if USE_PRE_INCREMENT
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
				*++lpd = v;
#else
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
				*lpd++ = v;
#endif
			}
			while (--i);
		
		i = (n & 31) >> 2;
		
		if (i)
			do
#if USE_PRE_INCREMENT
				*++lpd = v;
#else
				*lpd++ = v;
#endif
			while (--i);
		
#if USE_PRE_INCREMENT
		cpd = ((unsigned char *) (lpd + 1)) - 1;
#else
		cpd = ((unsigned char *) (lpd));
#endif
		
		n &= 3;
	}

	if (n)
		do
#if USE_PRE_INCREMENT
			*++cpd = v;
#else
			*cpd++ = v;
#endif
		while (--n);

	return;
}

void UMemory::Clear(void *inData, Uint32 inSize)
{
	UMemory::Fill(inData, inSize, 0);
}

bool UMemory::Equal(const void *inDataA, const void *inDataB, Uint32 inSize)
{
	const Uint8 *pa = (Uint8 *)inDataA;
	const Uint8 *pb = (Uint8 *)inDataB;

#if USE_PRE_INCREMENT
	pa--;
	pb--;
#endif

	while (inSize--)
	{
#if USE_PRE_INCREMENT
		if (*++pa != *++pb)
			return false;
#else
		if (*pa++ != *pb++)
			return false;
#endif
	}
	
	return true;
}

bool UMemory::EqualPString(const Uint8 *inStrA, const Uint8 *inStrB)
{
	Uint32 n = (Uint32)*inStrA + 1;

#if USE_PRE_INCREMENT
	inStrA--;
	inStrB--;
#endif

	while (n--)
	{
#if USE_PRE_INCREMENT
		if (*++inStrA != *++inStrB)
			return false;
#else
		if (*inStrA++ != *inStrB++)
			return false;
#endif
	}
	
	return true;
}

// ioDest should point to 256 bytes.  Truncates the string if necessary
void UMemory::AppendPString(Uint8 *d, const Uint8 *s)
{
	register int i, j;

	if (((j = s[0]) + d[0]) > 255)
		j = 255 - d[0];

	for (i = 0; i < j;)
		d[++d[0]] = s[++i];
}

// returns ptr to first occurance of <inSearchData> (of <inSearchSize> bytes in length) within <inData>
Uint8 *UMemory::Search(const void *inSearchData, Uint32 inSearchSize, const void *inData, Uint32 inDataSize)
{
	Uint8 *p, *tp, *sp;
	Uint32 sn;
	Uint8 firstc;
		
	if (inSearchSize == 1)
	{
		firstc = *(Uint8 *)inSearchData;
		p = (Uint8 *)inData;
		
		while (inDataSize--)
		{
			if (*p++ == firstc)
				return p-1;
		}
	}
	else if (inSearchSize)
	{
		firstc = *(Uint8 *)inSearchData;
		p = (Uint8 *)inData;
		
		while (inDataSize--)
		{
			if (*p == firstc)
			{
				sp = ((Uint8 *)inSearchData) + 1;
				sn = inSearchSize - 1;
				tp = p + 1;
				if (sn > inDataSize) return nil;
				while (sn--)
				{
					if (*sp++ != *tp++) goto keepgoing;
				}
				
				return p;
			}
			
			keepgoing:
			p++;
		}
	}
	
	// not found
	return nil;
}

Uint8 *UMemory::SearchByte(Uint8 inByte, const void *inData, Uint32 n)
{
	const unsigned char *p;
	
#if USE_PRE_INCREMENT

	unsigned long v = inByte;
	
	for (p = (unsigned char *)inData - 1, n++; --n;)
		if (*++p == v)
			return (Uint8 *)p;
	
#else
	
	unsigned char v = inByte;
	
	for (p = (unsigned char *)inData, n++; --n;)
		if (*p++ == v)
			return (Uint8 *)p-1;

#endif
	
	return nil;
}

// search in reverse
Uint8 *UMemory::SearchByteBackwards(Uint8 inByte, const void *inData, Uint32 n)
{
	const unsigned char *p;
	
#if USE_PRE_INCREMENT

	unsigned long v = inByte;
	
	for (p = (unsigned char *)inData + n, n++; --n;)
		if (*--p == v)
			return (Uint8 *)p;

	
#else
	
	unsigned char v = inByte;
	
	for (p = (unsigned char *)inData + n, n++; --n;)
		if (*--p == v)
			return (Uint8 *)p;

#endif
	
	return nil;
}

Uint32 *UMemory::SearchLongArray(Uint32 inLong, const Uint32 *inArray, Uint32 inCount)
{
	if (inArray)
	{
#if USE_PRE_INCREMENT
		inArray--;
#endif

		while (inCount--)
		{
#if USE_PRE_INCREMENT
			if (*++inArray == inLong)
				return (Uint32 *)inArray;
#else
			if (*inArray++ == inLong)
				return (Uint32 *)inArray-1;
#endif
		}
	}
	
	return nil;
}

// returns number of times the target data was replaced, inReplaceSize can be 0 to search-and-destroy
Uint32 UMemory::SearchAndReplaceAll(THdl inHdl, Uint32 inOffset, const void *inSearchData, Uint32 inSearchSize, const void *inReplaceData, Uint32 inReplaceSize)
{
	Uint8 *sp, *fp;
	Uint32 bytesLeft = UMemory::GetSize(inHdl);
	Uint32 count = 0;
	
	if (inOffset >= bytesLeft) return 0;
	bytesLeft -= inOffset;
	
	if (inSearchSize == inReplaceSize)
	{
		if (inSearchSize == 1 && inReplaceSize == 1)	// if searching and replacing single bytes
		{
			ASSERT(inSearchData && inReplaceData);
			Uint8 sch = *(Uint8 *)inSearchData;
			Uint8 rch = *(Uint8 *)inReplaceData;
			sp = UMemory::Lock(inHdl) + inOffset;
			while (bytesLeft--)
			{
				if (*sp == sch)
				{
					*sp = rch;
					count++;
				}
				
				sp++;
			}
			UMemory::Unlock(inHdl);
		}
		else	// search and replace sizes are the same
		{
			sp = UMemory::Lock(inHdl) + inOffset;
			fp = sp + bytesLeft;
			
			for(;;)
			{
				sp = UMemory::Search(inSearchData, inSearchSize, sp, fp - sp);
				if (sp == nil) break;
				
				UMemory::Copy(sp, inReplaceData, inReplaceSize);
				count++;
				
				sp += inReplaceSize;
			}
			
			UMemory::Unlock(inHdl);
		}
	}
	else
	{
		for(;;)
		{
			sp = UMemory::Lock(inHdl);
			
			fp = (Uint8 *)UMemory::Search(inSearchData, inSearchSize, sp + inOffset, bytesLeft);
			
			if (fp == nil)
			{
				UMemory::Unlock(inHdl);
				break;
			}
			
			Uint32 replaceOffset = fp - sp;		// calculate this BEFORE unlocking
			
			UMemory::Unlock(inHdl);

			UMemory::Replace(inHdl, replaceOffset, inSearchSize, inReplaceData, inReplaceSize);
			
			Uint32 newOffset = replaceOffset + inReplaceSize;
			bytesLeft -= (replaceOffset + inSearchSize) - inOffset;
			inOffset = newOffset;
			
			if (bytesLeft == 0) break;
			count++;
		}
	}
	
	return count;
}

Int16 UMemory::Compare(const void *inDataA, const void *inDataB, Uint32 n)
{
	const unsigned char *p1;
	const unsigned char *p2;

#if USE_PRE_INCREMENT
	
	for (p1 = (const unsigned char *)inDataA - 1, p2 = (const unsigned char *)inDataB - 1, n++; --n;)
		if (*++p1 != *++p2)
			return ((*p1 < *p2) ? -1 : +1);
	
#else
	
	for (p1 = (const unsigned char *)inDataA, p2 = (const unsigned char *)inDataB, n++; --n;)
		if (*p1++ != *p2++)
			return ((*--p1 < *--p2) ? -1 : +1);

#endif
	
	return 0;
}

Int16 UMemory::Compare(const void *inDataA, Uint32 inSizeA, const void *inDataB, Uint32 inSizeB)
{
	if (inSizeA < inSizeB)
		return -1;
	else if (inSizeA > inSizeB)
		return 1;
	else
		return Compare(inDataA, inDataB, inSizeA);
}

// generates a simple checksum on the specified data
Uint32 UMemory::Checksum(const void *inData, Uint32 inDataSize, Uint32 inInit)
{
	Uint8 *p = (Uint8 *)inData;
	Uint32 n = inInit;

#if USE_PRE_INCREMENT
	p--;
#endif

	while (inDataSize--)
	{
#if USE_PRE_INCREMENT
		n += *++p;
#else
		n += *p++;
#endif
	}
	
	return n;
}

// CCITT32 CRC table
static const unsigned long ccitt32_crctab[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
    0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
    0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
    0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
    0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
    0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
    0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
    0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
    0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
    0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
    0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
    0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
    0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
    0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
    0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
    0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
    0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
    0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
    0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
    0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
    0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
    0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
};

// calculates CCITT32 cylindrical-redundancy-check on the specified data
Uint32 UMemory::CRC(const void *inData, Uint32 inDataSize, Uint32 inInit)
{
	register Uint8 *p = (Uint8 *)inData;
	register Uint32 crc = inInit;

#if USE_PRE_INCREMENT
	p--;
#endif

    while (inDataSize--)
    {
#if USE_PRE_INCREMENT
		crc = ((crc << 8) & 0xffffff00) ^ ccitt32_crctab[((crc >> 24) & 0xff) ^ *++p];
#else
		crc = ((crc << 8) & 0xffffff00) ^ ccitt32_crctab[((crc >> 24) & 0xff) ^ *p++];
#endif
    }

    return crc;
}

Uint32 UMemory::AdlerSum(const void *inData, Uint32 inDataSize, Uint32 inInit)
{
	register Uint8 *p = (Uint8 *)inData;
	register Uint32 s1 = inInit & 0xFFFF;
	register Uint32 s2 = (inInit >> 16) & 0xFFFF;
	register int k;

	while (inDataSize > 0)
	{
		k = inDataSize < 5552 ? inDataSize : 5552;	// 5552 is the largest n such that 255n(n+1)/2 + (n+1)(65521-1) <= 2^32-1 
		inDataSize -= k;
		
		while (k >= 16)
		{
			s1 += p[0];	s2 += s1;
			s1 += p[1];	s2 += s1;
			s1 += p[2];	s2 += s1;
			s1 += p[3];	s2 += s1;
			s1 += p[4];	s2 += s1;
			s1 += p[5];	s2 += s1;
			s1 += p[6];	s2 += s1;
			s1 += p[7];	s2 += s1;
			s1 += p[8];	s2 += s1;
			s1 += p[9];	s2 += s1;
			s1 += p[10];	s2 += s1;
			s1 += p[11];	s2 += s1;
			s1 += p[12];	s2 += s1;
			s1 += p[13];	s2 += s1;
			s1 += p[14];	s2 += s1;
			s1 += p[15];	s2 += s1;
			
			p += 16;
			k -= 16;
		}
		
		if (k != 0)
			do {
				s1 += *p++;
				s2 += s1;
			} while (--k);
		
		s1 %= 65521L;					// 65521 is largest prime smaller than 65536
		s2 %= 65521L;
	}
	
	return (s2 << 16) | s1;
}

void UMemory::Swap(void *inDataA, void *inDataB, Uint32 inSize)
{
	Uint8 *p, *q;

#if USE_PRE_INCREMENT

	unsigned long tmp;

	for (p = (Uint8 *)inDataA - 1, q = (Uint8 *)inDataB - 1, inSize++; --inSize;)
	{
		tmp = *++q;
		*q = *++p;
		*p = tmp;
	}

#else

	unsigned char tmp;

	for (p = (Uint8 *)inDataA, q = (Uint8 *)inDataB, inSize++; --inSize;)
	{
		tmp = *q;
		*q++ = *p;
		*p++ = tmp;
	}

#endif
}

void UMemory::Swap(void *a, Uint32 as, void *b, Uint32 bs)
{
	// check if same size
	if (as == bs)
	{
		Swap(a, b, as);
		return;
	}

	// make a less than b
	if (a > b)
	{
		void *t = a;
		Uint32 ts = as;
		a = b;
		as = bs;
		b = t;
		bs = ts;
	}
	
	// copy item into buf
	TPtr buf;
	if (as > bs)
	{
		buf = New(as);
		Copy(buf, a, as);
	}
	else
	{
		buf = New(bs);
		Copy(buf, b, bs);
	}
	
	// move the data in between
	Uint32 n = BPTR(b) - (BPTR(a) + as);
	Copy(BPTR(a)+bs, BPTR(a)+as, n);
	
	// swap the items
	if (as > bs)
	{
		Copy(a, b, bs);
		Copy(BPTR(a)+bs+n, buf, as);
	}
	else
	{
		Copy(BPTR(a)+bs+n, a, as);
		Copy(a, buf, bs);
	}
	
	// done
	Dispose(buf);
}

void UMemory::Move(void * /* ioDest */, Uint32 /* inSize */, Int32 /* inDelta */)
{
	DebugBreak("UMemory - Move() is not yet implemented!");
	Fail(errorType_Misc, error_Unimplemented);
}

/*
Match - see if a string matches a particular pattern

The following special characters are used in the pattern:
 
*	match any substring
?	match any single character
[	followed by a list of characters that are acceptable (eg, "[ajk]"), or by two
 	characters separated by "-" to indicate a range (eg, "[a-z]").
\	forces exact matching on the following character. For when you want to 
 	include one of the above symbols without matching (eg, "\?" or "\\")
*/
bool UMemory::Match(const void *inData, Uint32 inDataSize, const Int8 *inPattern)
{
	Uint8 *string = (Uint8 *)inData;
	Uint8 *ep = string + inDataSize;
	char c2;

	for (;;)
	{
		/*
		 * See if we're at the end of both the pattern and the
		 * string. If, we succeeded.  If we're at the end of the
		 * pattern but not at the end of the string, we failed.
		 */
		if (*inPattern == 0)
			return string == ep;
		if (string == ep && *inPattern != '*')
			return false;
		
		/*
		 * Check for a "*" as the next pattern character.  It matches
		 * any substring.  We handle this by calling ourselves
		 * recursively for each postfix of string, until either we
		 * match or we reach the end of the string.
		 */
		if (*inPattern == '*')
		{
			inPattern += 1;
			if (*inPattern == 0)
				return true;
			while (string != ep)
			{
				if (Match(string, ep - string, inPattern))
					return true;
				++string;
			}
			return false;
		}
		
		/*
		 * Check for a "?" as the next pattern character.  It matches
		 * any single character.
		 */
		if (*inPattern == '?')
			goto thisCharOK;
		/*
		 * Check for a "[" as the next pattern character.  It is
		 * followed by a list of characters that are acceptable, or
		 * by a range (two characters separated by "-").
		 */
		if (*inPattern == '[')
		{
			++inPattern;
			for (;;)
			{
				if ((*inPattern == ']') || (*inPattern == 0))
					return false;
				if (*inPattern == *string)
					break;
				if (inPattern[1] == '-')
				{
					c2 = inPattern[2];
					if (c2 == 0)
						return false;
					if ((*inPattern <= *string) &&
						(c2 >= *string))
						break;
					if ((*inPattern >= *string) &&
						(c2 <= *string))
						break;
					inPattern += 2;
				}
				++inPattern;
			}
			while ((*inPattern != ']') && (*inPattern != 0))
				++inPattern;
			goto thisCharOK;
		}
		/*
		 * If the next pattern character is '\', just strip off the
		 * '\' so we do exact matching on the character that follows.
		 */
		if (*inPattern == '\\')
		{
			++inPattern;
			if (*inPattern == 0)
				return false;
		}
		/*
		 * There's no special character.  Just make sure that the
		 * next characters of each string match.
		 */
		if (*inPattern != *string)
			return false;
thisCharOK:
		++inPattern;
		++string;
	}
}

// make sure you set ioTokenOffset and ioTokenSize to 0 the first time you call this func
bool UMemory::Token(const void *inData, Uint32 inDataSize, const Int8 *inDelims, Uint32& ioTokenOffset, Uint32& ioTokenSize)
{
	// ******** see strtok() in ANSI.cp for a cool way of optimizing this
	// ******** it optimizes the _IsDelim part - to check if a char is a delim, strtok checks the corresponding bit in an array of bits

	const Uint8 *p = ((const Uint8 *)inData) + ioTokenOffset + ioTokenSize;
	const Uint8 *ep = ((const Uint8 *)inData) + inDataSize;
	
	// skip delims at start
	while (p < ep && _IsDelim(*p, inDelims))
		p++;
	
	ioTokenOffset = p - (Uint8 *)inData;
	
	// find end of token
	while (p < ep && !_IsDelim(*p, inDelims))
		p++;

	ioTokenSize = (p - (Uint8 *)inData) - ioTokenOffset;
	return (ioTokenSize != 0);
}

Uint32 UMemory::HexDumpLine(Uint32 inOffset, Uint32 inLineBytes, const void *inData, Uint32 inDataSize, void *outData, Uint32 inMaxSize)
{
	const Int8 hexDigits[] = "0123456789ABCDEF";
	Uint8 *dp, *sp, *ep, *p;
	Uint32 sizeLeft, i, n;
	
	// make sure line bytes is valid
	inLineBytes &= ~1;
	if (inLineBytes < 2)
		inLineBytes = 2;
	if (inDataSize > inLineBytes) inDataSize = inLineBytes;

	// begin
	dp = (Uint8 *)outData;
	sizeLeft = inMaxSize;

	// write decimal offset
	if (sizeLeft < 8) goto done;
	sizeLeft -= 8;
	dp += 8;
	n = inOffset;
	p = dp;
	for (i = 0; n; n /= 10, i++) *--p = n % 10 + '0';
	for (; i < 8; i++) *--p = '0';
	
	// write divider
	if (sizeLeft < 3) goto done;
	sizeLeft -= 3;
	*dp++ = ' ';
	*dp++ = '|';
	*dp++ = ' ';
	
	// write hex words
	sp = (Uint8 *)inData;
	ep = ((Uint8 *)inData) + inLineBytes;
	p = ((Uint8 *)inData) + inDataSize;
	while (sp < ep)
	{
		if (sizeLeft < 2) goto done;
		sizeLeft -= 2;
		
		if (sp < p)
		{
			n = *sp++;
			*dp++ = hexDigits[(n / 16) % 16];
			*dp++ = hexDigits[n % 16];
		}
		else
		{
			sp++;
			*dp++ = ' ';
			*dp++ = ' ';
		}
		
		if (sizeLeft < 3) goto done;
		sizeLeft -= 3;

		if (sp < p)
		{
			n = *sp++;
			*dp++ = hexDigits[(n / 16) % 16];
			*dp++ = hexDigits[n % 16];
		}
		else
		{
			sp++;
			*dp++ = ' ';
			*dp++ = ' ';
		}
		*dp++ = ' ';
	}
		
	// write divider
	if (sizeLeft < 2) goto done;
	sizeLeft -= 2;
	*dp++ = '|';
	*dp++ = ' ';

	// write text turning unprintables into a period
	if (sizeLeft < inDataSize) goto done;
	sizeLeft -= inDataSize;
	sp = (Uint8 *)inData;
	ep = ((Uint8 *)inData) + inDataSize;
	while (sp < ep)
	{
		n = *sp++;
		if (n < 32 || n > 216)
			*dp++ = '.';
		else
			*dp++ = n;
	}

	// return number of bytes written
	done:
	return inMaxSize - sizeLeft;
}

THdl UMemory::HexDump(const void *inData, Uint32 inDataSize, Uint32 inLineBytes)
{
	Uint32 s, ss, sourceLeft, maxPerLine, n;
	THdl h;
	Uint8 *p, *sp;
	
	maxPerLine = (((inLineBytes * 7) / 2) + 24);	// same as (lb+(lb*2)+(lb/2)+8+16)
	s = maxPerLine * ((inDataSize / inLineBytes) + 1);
	s = RoundUp64(s);
	
	h = UMemory::NewHandle(s);
	try
	{
		p = UMemory::Lock(h);
		s = 0;
		sp = (Uint8 *)inData;
		sourceLeft = inDataSize;
		
		while (sourceLeft)
		{
			ss = sourceLeft;
			if (ss > inLineBytes) ss = inLineBytes;
			
			n = UMemory::HexDumpLine(inDataSize - sourceLeft, inLineBytes, sp, ss, p, maxPerLine);
			p += n;
			*p++ = '\r';
			s += n + 1;

			sourceLeft -= ss;
			sp += ss;
		}
		
		UMemory::Unlock(h);
		
		UMemory::SetSize(h, s);
	}
	catch(...)
	{
		UMemory::Dispose(h);
		throw;
	}
	
	return h;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
Possible extension to Flatten() - if size in SDataPtr is max_Uint32, data is 
a callback proc.  Call it and it writes to the specified buffer.  Return
number of bytes written.  For example, callback could read from file.
If inMaxSize is 0, could return total number of bytes (for GetFlattenSize()).
Pass index to callback func (useful if wants to store something on per-item
basis).
*/

Uint32 UMemory::Flatten(SFlattenInfo& ioInfo, void *outData, Uint32 inMaxSize)
{
	const SDataPtr *item, *items;
	Uint32 s, bytesWritten, index, count, offset, itemSize;
	bool align;
	
	bytesWritten = 0;
	index = ioInfo.index;
	offset = ioInfo.offset;
	count = ioInfo.count;
	items = ioInfo.items;
	align = (ioInfo.options & 1) != 0;
	
	while (index < count)
	{
		item = items + index;
		
		itemSize = item->size;
		if (align)
		{
			itemSize += 3;
			itemSize &= ~3;
		}
		
		s = itemSize - offset;
		if (s > inMaxSize) s = inMaxSize;
		
		// with align, may copy a few bytes beyond end of data.  Hopefully doesn't cause access exception
		UMemory::Copy(outData, BPTR(item->data) + offset, s);
		offset += s;
		
		if (offset >= itemSize)
		{
			index++;
			offset = 0;
		}

		bytesWritten += s;
		
		inMaxSize -= s;
		if (inMaxSize == 0) break;
		outData = BPTR(outData) + s;
	}
	
	ioInfo.index = index;
	ioInfo.offset = offset;
	
	return bytesWritten;
}

THdl UMemory::FlattenToHandle(const SFlattenInfo& inInfo)
{
	THdl h;
	Uint8 *p;
	Uint32 n, s;
	const SDataPtr *item;
	
	item = inInfo.items;
	n = inInfo.count;
	h = UMemory::NewHandle(UMemory::GetFlattenSize(inInfo));
	
	try
	{
		StHandleLocker lock(h, (void*&)p);
		
		if (inInfo.options & 1)		// if align
		{
			while (n--)
			{
				s = item->size;
				UMemory::Copy(p, item->data, s);
				s += 3;
				s &= ~3;
				p += s;
				item++;
			}
		}
		else
		{
			while (n--)
			{
				p += UMemory::Copy(p, item->data, item->size);
				item++;
			}
		}
	}
	catch(...)
	{
		UMemory::Dispose(h);
		throw;
	}
	
	return h;
}

Uint32 UMemory::GetFlattenSize(const SFlattenInfo& inInfo)
{
	Uint32 s = 0;
	Uint32 n = inInfo.count;
	const SDataPtr *item = inInfo.items;
	
	if (inInfo.options & 1)		// if align
	{
		while (n--)
		{
			s += (item->size + 3) & ~3;
			item++;
		}
	}
	else
	{
		while (n--)
		{
			s += item->size;
			item++;
		}
	}
	
	return s;
}

/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 * PackIntegers() packs <inIntCount> integers from <inIntList> into a compact format and writes
 * the packed data to <outData> (which must be of <inMaxSize> bytes) and returns the number of
 * bytes written.  Works best for integers with small, positive values.
 */
Uint32 UMemory::PackIntegers(const Uint32 *inIntList, Uint32 inIntCount, void *outData, Uint32 inMaxSize)
{
	const Uint32 *intp;
	Uint32 sizeBytes, worstSize, n;
	Uint8 *sp, *dp;
	Uint8 sc, ss;
	
	// calculate number of bytes needed to store integer-size-data
	sizeBytes = inIntCount / 4;
	if (sizeBytes*4 != sizeBytes) sizeBytes++;
	
	// ensure output buffer is big enough for worst case
	worstSize = sizeof(Uint32) + sizeBytes + (inIntCount * sizeof(Uint32));
	if (worstSize > inMaxSize) Fail(errorType_Misc, error_Param);
	
	// get ptrs to start of data areas
	intp = inIntList;
	sp = (Uint8 *)outData;
	*((Uint32 *)sp)++ = inIntCount;
	dp = sp + sizeBytes;
	ss = 6;
	
	// clear integer-size-data
	UMemory::Clear(sp, sizeBytes);
	
	// pack each integer
	while (inIntCount--)
	{
		n = *intp;
		
		if (n & 0xFFFF0000)			// if needs 3 or 4 bytes
		{
			sc = 3;
			*((Uint32 *)dp)++ = n;
		}
		else if (n & 0x0000FF00)	// if needs 2 bytes
		{
			sc = 2;
			*((Uint16 *)dp)++ = n;
		}
		else if (n)					// if needs 1 byte
		{
			sc = 1;
			*((Uint8 *)dp)++ = n;
		}
		else						// needs 0 bytes
		{
			sc = 0;
		}
		
		*sp |= sc << ss;
		if (ss == 0)
		{
			ss = 6;
			sp++;
		}
		else
			ss -= 2;

		intp++;
	}
	
	// return size of packed data
	return dp - (Uint8 *)outData;
}

/*
 * UnpackIntegers() unpacks integer data that was compacted with PackIntegers().
 * <inData> is a pointer to the compacted data and is of <inDataSize> bytes in
 * length.  Up to <inMaxCount> integers are written to <outIntList>.   Returns
 * the number of integers actually written.
 */
Uint32 UMemory::UnpackIntegers(const void *inData, Uint32 inDataSize, Uint32 *outIntList, Uint32 inMaxCount)
{
	Uint32 intCount, sizeBytes, i;
	Uint32 *intp;
	Uint8 *sp, *dp;
	Uint8 sc, ss;
	
	// extract number of integers
	if (inDataSize < sizeof(Uint32)) Fail(errorType_Misc, error_Param);
	intCount = *(Uint32 *)inData;

	// calculate number of bytes needed to store integer-size-data
	sizeBytes = intCount / 4;
	if (sizeBytes*4 != sizeBytes) sizeBytes++;
	
	// make sure we have the minimum amount of data
	if (inDataSize < (sizeof(Uint32) + sizeBytes))
		Fail(errorType_Misc, error_Param);

	// get ptrs to start of data areas
	if (intCount > inMaxCount) intCount = inMaxCount;
	i = intCount;
	intp = outIntList;
	sp = BPTR(inData) + sizeof(Uint32);
	dp = sp + sizeBytes;
	ss = 6;
	
	// unpack each integer
	while (i--)
	{
		// extract size code
		sc = *sp & (0x03 << ss);
		sc >>= ss;
		if (ss == 0)
		{
			ss = 6;
			sp++;
		}
		else
			ss -= 2;

		// extract integer
		switch (sc)
		{
			case 1:
				*intp = *((Uint8 *)dp)++;
				break;
			case 2:
				*intp = *((Uint16 *)dp)++;
				break;
			case 3:
				*intp = *((Uint32 *)dp)++;
				break;
			default:
				*intp = 0;
				break;
		}

		// next integer
		intp++;
	}
	
	// return number of integers that were unpacked
	return intCount;
}
/* -------------------------------------------------------------------------- */
#pragma mark -

/*
 *		Specialized routines to copy memory by transferring longs and using
 *		unrolled loops whenever possible.
 *		
 *		There are two pairs of routines.
 *		
 *		One pair handles the cases where both the source and destination are
 *		mutually aligned, in which case it suffices to transfer bytes until they
 *		are long-aligned, then transfer longs, then finish up transferring any
 *		excess bytes.
 *		
 *		The other pair handles the cases where the source and destination are not
 *		mutually aligned. The handling is similar but requires repeatedly
 *		assembling a destination long from two source longs.
 *		
 *		Each pair consists of one routine to transfer bytes from first byte to last
 *		and another to transfer bytes from last byte to first. The latter should be
 *		used whenever the address of the source is less than the address of the
 *		destination in case they overlap each other.
 *		
 *		There are also two routines here that effectively duplicate memcpy and
 *		memmove, so that these routines may be used without bringing in the entire
 *		StdLib.
 *
 *		Oh, and let's not forget __fill_mem, which fills memory a long at a time
 *		and in an unrolled loop whenever possible. 
 *
 *
 */

#pragma options align=native

void	__copy_mem                (void * dst, const void * src, unsigned long n);
void	__move_mem                (void * dst, const void * src, unsigned long n);
void	__copy_longs_aligned      (void * dst, const void * src, unsigned long n);
void	__copy_longs_rev_aligned  (void * dst, const void * src, unsigned long n);
void	__copy_longs_unaligned    (void * dst, const void * src, unsigned long n);
void	__copy_longs_rev_unaligned(void * dst, const void * src, unsigned long n);

void	__fill_mem(void * dst, unsigned long n, int val);

#pragma options align=reset

#define __min_bytes_for_long_copy	32		/* NEVER let this be < 16 */

#pragma ANSI_strict off

#if USE_PRE_INCREMENT
#define deref_auto_inc(p)	*++(p)
#else
#define deref_auto_inc(p)	*(p)++
#endif

void __copy_mem(void * dst, const void * src, unsigned long n)
{
	// **** still keep this code even once the optimized version is working!!!  (for reference purposes)
	const char *from = (char *)src;
	char *to = (char *)dst;
	
	while (n--)
		*to++ = *from++;


// following doesn't work (at least not on x86)
#if 0

	const	char * p;
				char * q;
	
	if (n >= __min_bytes_for_long_copy)
	{
		if ((((int) dst ^ (int) src)) & 3)
			__copy_longs_unaligned(dst, src, n);
		else
			__copy_longs_aligned(dst, src, n);
		
		return;
	}

#if USE_PRE_INCREMENT
	
	for (p = (const char *) src - 1, q = (char *) dst - 1, n++; --n;)
		*++q = *++p;
	
#else
	
	for (p = (const char *) src, q = (char *) dst, n++; --n;)
		*q++ = *p++;

#endif


#endif
}

void __move_mem(void * dst, const void * src, unsigned long n)
{
	// **** still keep this code even once the optimized version is working!!!  (for reference purposes)
	const char *from = (char *)src;
	char *to = (char *)dst;

	if (n) {
		if (to > from) {
			to += n;
			from += n;
			while (n--) {
				*--to = *--from;
			}
		}
		else {
			while (n--) {
				*to++ = *from++;
			}
		}
	}

// following doesn't work (at least not on x86)
#if 0
	const	char * p;
				char * q;
				int		 rev = ((unsigned long) src < (unsigned long) dst);
	
	if (n >= __min_bytes_for_long_copy)
	{
		if ((((int) dst ^ (int) src)) & 3)
			if (!rev)
				__copy_longs_unaligned(dst, src, n);
			else
				__copy_longs_rev_unaligned(dst, src, n);
		else
			if (!rev)
				__copy_longs_aligned(dst, src, n);
			else
				__copy_longs_rev_aligned(dst, src, n);
		
		return;
	}

	if (!rev)
	{
		
#if USE_PRE_INCREMENT
		
		for (p = (const char *) src - 1, q = (char *) dst - 1, n++; --n;)
			*++q = *++p;
	
#else
		
		for (p = (const char *) src, q = (char *) dst, n++; --n;)
			*q++ = *p++;

#endif
		
	}
	else
	{
		for (p = (const char *) src + n, q = (char *) dst + n, n++; --n;)
			*--q = *--p;
	}



#endif
}

void __copy_longs_aligned(void * dst, const void * src, unsigned long n)
{
	unsigned char *	cps	= ((unsigned char *) src);
	unsigned char *	cpd	= ((unsigned char *) dst);
	unsigned long *	lps	= ((unsigned long *) src);
	unsigned long *	lpd	= ((unsigned long *) dst);

	unsigned long	i;
	
	i = (- (unsigned long) dst) & 3;
	
#if USE_PRE_INCREMENT
	cps = ((unsigned char *) src) - 1;
	cpd = ((unsigned char *) dst) - 1;
#endif
	
	if (i)
	{
		n -= i;
		
		do
			deref_auto_inc(cpd) = deref_auto_inc(cps);
		while (--i);
	}
	
#if USE_PRE_INCREMENT
	lps = ((unsigned long *) (cps + 1)) - 1;
	lpd = ((unsigned long *) (cpd + 1)) - 1;
#endif
	
	i = n >> 5;
	
	if (i)
		do
		{
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
			deref_auto_inc(lpd) = deref_auto_inc(lps);
		}
		while (--i);
	
	i = (n & 31) >> 2;
	
	if (i)
		do
			deref_auto_inc(lpd) = deref_auto_inc(lps);
		while (--i);
	
#if USE_PRE_INCREMENT
	cps = ((unsigned char *) (lps + 1)) - 1;
	cpd = ((unsigned char *) (lpd + 1)) - 1;
#endif
	
	n &= 3;
	
	if (n)
		do
			deref_auto_inc(cpd) = deref_auto_inc(cps);
		while (--n);
	
	return;
}

void __copy_longs_rev_aligned(void * dst, const void * src, unsigned long n)
{
	unsigned char *	cps	= ((unsigned char *) src) + n;
	unsigned char *	cpd	= ((unsigned char *) dst) + n;
	unsigned long *	lps	= ((unsigned long *) cps);
	unsigned long *	lpd	= ((unsigned long *) cpd);

	unsigned long i;
		
	i = ((unsigned long) cpd) & 3;
	
	if (i)
	{
		n -= i;
		
		do
			*--cpd = *--cps;
		while (--i);
	}
	
	i = n >> 5;
	
	if (i)
		do
		{
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
			*--lpd = *--lps;
		}
		while (--i);
	
	i = (n & 31) >> 2;
	
	if (i)
		do
			*--lpd = *--lps;
		while (--i);
	
	n &= 3;
	
	if (n)
		do
			*--cpd = *--cps;
		while (--n);
	
	return;
}

void __copy_longs_unaligned(void * dst, const void * src, unsigned long n)
{
	unsigned char *	cps	= ((unsigned char *) src);
	unsigned char *	cpd	= ((unsigned char *) dst);
	unsigned long *	lps	= ((unsigned long *) src);
	unsigned long *	lpd	= ((unsigned long *) dst);

	unsigned long	i, v1, v2;
	unsigned int	src_offset, left_shift, right_shift;
	
	i = (- (unsigned long) dst) & 3;
	
#if USE_PRE_INCREMENT
	cps = ((unsigned char *) src) - 1;
	cpd = ((unsigned char *) dst) - 1;
#endif
	
	if (i)
	{
		n -= i;
		
		do
			deref_auto_inc(cpd) = deref_auto_inc(cps);
		while (--i);
	}

#if USE_PRE_INCREMENT
	src_offset = ((unsigned int) (cps + 1)) & 3;
#else
	src_offset = ((unsigned int) cps) & 3;
#endif
	
	left_shift  = src_offset << 3;
	right_shift = 32 - left_shift;
	
	cps -= src_offset;
	
#if USE_PRE_INCREMENT
	lps = ((unsigned long *) (cps + 1)) - 1;
	lpd = ((unsigned long *) (cpd + 1)) - 1;
#endif
	
	i = n >> 3;
	
	v1 = deref_auto_inc(lps);
	
	do
	{
		v2                  = deref_auto_inc(lps);
		deref_auto_inc(lpd) = (v1 << left_shift) | (v2 >> right_shift);
		v1                  = deref_auto_inc(lps);
		deref_auto_inc(lpd) = (v2 << left_shift) | (v1 >> right_shift);
	}
	while (--i);
	
	if (n & 4)
	{
		v2                  = deref_auto_inc(lps);
		deref_auto_inc(lpd) = (v1 << left_shift) | (v2 >> right_shift);
	}
	
#if USE_PRE_INCREMENT
	cps = ((unsigned char *) (lps + 1)) - 1;
	cpd = ((unsigned char *) (lpd + 1)) - 1;
#endif
	
	n &= 3;
	
	if (n)
	{
		cps -= 4 - src_offset;
		do
			deref_auto_inc(cpd) = deref_auto_inc(cps);
		while (--n);
	}
	
	return;
}

void __copy_longs_rev_unaligned(void * dst, const void * src, unsigned long n)
{
	unsigned char *	cps	= ((unsigned char *) src) + n;
	unsigned char *	cpd	= ((unsigned char *) dst) + n;
	unsigned long *	lps	= ((unsigned long *) cps);
	unsigned long *	lpd	= ((unsigned long *) cpd);

	unsigned long	i, v1, v2;
	unsigned int	src_offset, left_shift, right_shift;
		
	i = ((unsigned long) cpd) & 3;
	
	if (i)
	{
		n -= i;
		
		do
			*--cpd = *--cps;
		while (--i);
	}
	
	src_offset = ((unsigned int) cps) & 3;
	
	left_shift  = src_offset << 3;
	right_shift = 32 - left_shift;
	
	cps += 4 - src_offset;
	
	i = n >> 3;
	
	v1 = *--lps;
	
	do
	{
		v2     = *--lps;
		*--lpd = (v2 << left_shift) | (v1 >> right_shift);
		v1     = *--lps;
		*--lpd = (v1 << left_shift) | (v2 >> right_shift);
	}
	while (--i);
	
	if (n & 4)
	{
		v2     = *--lps;
		*--lpd = (v2 << left_shift) | (v1 >> right_shift);
	}
	
	n &= 3;
	
	if (n)
	{
		cps += src_offset;
		do
			*--cpd = *--cps;
		while (--n);
	}
	
	return;
}

void __fill_mem(void * dst, unsigned long n, int val)
{
/*	unsigned char *	cps	= ((unsigned char *) src);	*/
	unsigned char *	cpd	= ((unsigned char *) dst);
/*	unsigned long *	lps	= ((unsigned long *) src);	*/
	unsigned long *	lpd	= ((unsigned long *) dst);

	unsigned long			v = (unsigned char) val;
	unsigned long			i;
	
#if USE_PRE_INCREMENT
	cpd = ((unsigned char *) dst) - 1;
#endif
	
	if (n >= 32)
	{
		i = (- (unsigned long) dst) & 3;
		
		if (i)
		{
			n -= i;
			
			do
				deref_auto_inc(cpd) = v;
			while (--i);
		}
	
		if (v)
			v |= v << 24 | v << 16 | v <<  8;
		
#if USE_PRE_INCREMENT
		lpd = ((unsigned long *) (cpd + 1)) - 1;
#endif
		
		i = n >> 5;
		
		if (i)
			do
			{
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
				deref_auto_inc(lpd) = v;
			}
			while (--i);
		
		i = (n & 31) >> 2;
		
		if (i)
			do
				deref_auto_inc(lpd) = v;
			while (--i);
		
#if USE_PRE_INCREMENT
		cpd = ((unsigned char *) (lpd + 1)) - 1;
#endif
		
		n &= 3;
	}
	
	if (n)
		do
			deref_auto_inc(cpd) = v;
		while (--n);
	
	return;
}

static bool _IsDelim(Uint8 inChar, const Int8 *inDelim)
{
	for(;;)
	{
		if (*inDelim == 0)
			return false;
		
		if (inChar == *inDelim)
			return true;
		
		inDelim++;
	}
}

void *operator new(operator_new_size_t s)
{
	return UMemory::New(s);
}

void *operator new(operator_new_size_t, operator_new_size_t s)
{
	return UMemory::New(s);
}

void operator delete(void *p)
{
	UMemory::Dispose((TPtr)p);
}




