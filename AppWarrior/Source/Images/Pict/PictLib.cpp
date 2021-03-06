/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "PictLib.h"

#define HASH_SIZE 20023


colorhist_vector ppm_computecolorhist(Uint32 **pixels, Uint16 cols, Uint16 rows, Uint16 maxcolors, Uint16 *colorsP)
{
    colorhash_table cht;
    colorhist_vector chv;

    cht = ppm_computecolorhash( pixels, cols, rows, maxcolors, colorsP);
    if( cht == (colorhash_table) 0)
		return (colorhist_vector) 0;

    chv = ppm_colorhashtocolorhist(cht, maxcolors);
    ppm_freecolorhash(cht);

    return chv;
}

void ppm_addtocolorhist(colorhist_vector chv, Uint16 *colorsP, Uint16 maxcolors, Uint32 *colorP, Uint16 value, Uint16 position)
{
    int i, j;

    // Search colorhist for the color
    for( i = 0; i < *colorsP; ++i)
	if(PPM_EQUAL( chv[i].color, *colorP))
	{
	    // Found it - move to new slot
	    if(position > i)
		{
			for (j = i; j < position; ++j)
		    	chv[j] = chv[j + 1];
		}
	    else if(position < i)
		{
			for(j = i; j > position; --j)
			    chv[j] = chv[j - 1];
		}
	
	    chv[position].color = *colorP;
	    chv[position].value = value;
	
	    return;
	}
    
    if(*colorsP < maxcolors)
	{
		// Didn't find it, but there's room to add it; so do so
		for ( i = *colorsP; i > position; --i )
	    	chv[i] = chv[i - 1];

		chv[position].color = *colorP;
		chv[position].value = value;
		++(*colorsP);
	}
}

colorhash_table ppm_computecolorhash(Uint32 **pixels, Uint16 cols, Uint16 rows, Uint16 maxcolors, Uint16* colorsP)
{
    colorhash_table cht;
    Uint32 *pP;
    colorhist_list chl;
    Uint16 col, row, hash;

    cht = ppm_alloccolorhash( );
    *colorsP = 0;

    // Go through the entire image, building a hash table of colors. 
    for(row = 0; row < rows; ++row)
		for(col = 0, pP = pixels[row]; col < cols; ++col, ++pP)
		{
	    	hash = ppm_hashpixel( *pP );
		    for(chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next)
			if(PPM_EQUAL( chl->ch.color, *pP ))
			    break;
	
		    if(chl != (colorhist_list) 0)
				++(chl->ch.value);
		    else
			{
				if(++(*colorsP) > maxcolors)
		    	{
			    	ppm_freecolorhash(cht);
			    	return (colorhash_table) 0;
			    }
		    
				try
				{
					chl = (colorhist_list) UMemory::NewClear(sizeof(struct colorhist_list_item));
				}
				catch(...)
				{
					Fail(errorType_Image, imgError_InsufficientMemory);
				}
		
				chl->ch.color = *pP;
				chl->ch.value = 1;
				chl->next = cht[hash];
				cht[hash] = chl;
			}
		}
    
    return cht;
}

colorhash_table ppm_alloccolorhash()
{
    colorhash_table cht;

    try
    {
    	cht = (colorhash_table) UMemory::NewClear( HASH_SIZE * sizeof(colorhist_list) );
    }
    catch(...)
    {
		Fail(errorType_Image, imgError_InsufficientMemory);
	}

    return cht;
}

Int16 ppm_addtocolorhash(colorhash_table cht, Uint32* colorP, Int16 value)
{
    Uint16 hash;
    colorhist_list chl;

    try
    {
    	chl = (colorhist_list) UMemory::NewClear(sizeof(struct colorhist_list_item));
    }
    catch(...)
    {
		// don't throw
		return -1;
	}
		
    hash = ppm_hashpixel( *colorP );
    chl->ch.color = *colorP;
    chl->ch.value = value;
    chl->next = cht[hash];
    cht[hash] = chl;

    return 0;
}

colorhist_vector ppm_colorhashtocolorhist(colorhash_table cht, Uint16 maxcolors)
{
    colorhist_vector chv;
    colorhist_list chl;
    Uint16 i, j;

    // Now collate the hash table into a simple colorhist array
    try
    {
	    chv = (colorhist_vector) UMemory::NewClear(maxcolors * sizeof(struct colorhist_item));
    	// (Leave room for expansion by caller) 
    }
    catch(...)
    {
		Fail(errorType_Image, imgError_InsufficientMemory);
	}

    // Loop through the hash table. 
    j = 0;
    for(i = 0; i < HASH_SIZE; ++i)
		for(chl = cht[i]; chl != (colorhist_list) 0; chl = chl->next)
    	{
	    	// Add the new entry
		    chv[j] = chl->ch;
		    ++j;
	    }

    // All done
    return chv;
}

colorhash_table ppm_colorhisttocolorhash(colorhist_vector chv, Uint16 colors)
{
    colorhash_table cht;
    Uint16 hash;
    Uint32 color;
    colorhist_list chl;

    cht = ppm_alloccolorhash( );

    for(Uint16 i = 0; i < colors; ++i)
	{
		color = chv[i].color;
		hash = ppm_hashpixel( color );
		for(chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next)
		    if(PPM_EQUAL( chl->ch.color, color ))
				Fail(errorType_Image, imgError_ColorFoundTwice);

		try
		{
			chl = (colorhist_list) UMemory::NewClear( sizeof(struct colorhist_list_item) );
		}
		catch(...)
		{
			Fail(errorType_Image, imgError_InsufficientMemory);
		}
	    	
		chl->ch.color = color;
		chl->ch.value = i;
		chl->next = cht[hash];
		cht[hash] = chl;
	}

    return cht;
}

Int16 ppm_lookupcolor(colorhash_table cht,Uint32 *colorP)
{
    Uint16 hash;
    colorhist_list chl;

    hash = ppm_hashpixel( *colorP );
    for(chl = cht[hash]; chl != (colorhist_list) 0; chl = chl->next)
		if(PPM_EQUAL( chl->ch.color, *colorP))
		    return chl->ch.value;

    return -1;
}

void ppm_freecolorhist(colorhist_vector chv)
{
    UMemory::Dispose((TPtr)chv);
}

void ppm_freecolorhash(colorhash_table cht)
{
    Uint16 i;
    colorhist_list chl, chlnext;

    for(i = 0; i < HASH_SIZE; ++i )
		for(chl = cht[i]; chl != (colorhist_list) 0; chl = chlnext)
	    {
		    chlnext = chl->next;
		    UMemory::Dispose((TPtr)chl);
	    }
	    	
    UMemory::Dispose((TPtr)cht);
}
