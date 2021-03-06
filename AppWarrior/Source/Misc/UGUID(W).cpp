#if WIN32


void UGUID::Generate(SGUID &outGUID)
{
	if(::CoCreateGuid((GUID *)(&outGUID)) != S_OK)
		Fail(errorType_Misc, error_Unknown);
}

bool UGUID::IsEqual(const SGUID &inGUIDA, const SGUID &inGUIDB)
{
	return UMemory::Equal(&inGUIDA, &inGUIDB, sizeof(SGUID));
}

bool SGUID::operator<(const SGUID &inGUID) const
{
	if(time_low < inGUID.time_low)
		return true;
	
	if(time_low == inGUID.time_low)
	{
		if(time_mid < inGUID.time_mid)
			return true;
		
		if(time_mid == inGUID.time_mid)
		{
			if(time_hi_and_version < inGUID.time_hi_and_version)
				return true;
			
			if(time_hi_and_version == inGUID.time_hi_and_version)
				return UMemory::Compare(this + 8, &inGUID + 8, 8) == -1;
		}
	}
	
	return false;
	
}

bool SGUID::operator>(const SGUID &inGUID) const
{
	if(time_low > inGUID.time_low)
		return true;
	
	if(time_low == inGUID.time_low)
	{
		if(time_mid > inGUID.time_mid)
			return true;
		
		if(time_mid == inGUID.time_mid)
		{
			if(time_hi_and_version > inGUID.time_hi_and_version)
				return true;
			
			if(time_hi_and_version == inGUID.time_hi_and_version)
				return UMemory::Compare(this + 8, &inGUID + 8, 8) == 1;
		}
	}
	
	return false;
	
}

Uint32 UGUID::Flatten(const SGUID &inGUID, void *outData)
{
	// first 8 bytes
	((SGUID *)outData)->time_low = TB(inGUID.time_low);
	((SGUID *)outData)->time_mid = TB(inGUID.time_mid);
	((SGUID *)outData)->time_hi_and_version = TB(inGUID.time_hi_and_version);

	// final 8 are chars - don't need byte swapping
	((Uint32 *)outData)[2] = ((Uint32 *)&inGUID)[2];
	((Uint32 *)outData)[3] = ((Uint32 *)&inGUID)[3];
	
	return sizeof(SGUID);
}

void UGUID::Unflatten(SGUID &outGUID, const void *inData)
{
	// first 8 bytes
	outGUID.time_low = FB(((SGUID *)inData)->time_low);
	outGUID.time_mid = FB(((SGUID *)inData)->time_mid);
	outGUID.time_hi_and_version = FB(((SGUID *)inData)->time_hi_and_version);

	// final 8 are chars - don't need byte swapping
	((Uint32 *)&outGUID)[2] = ((Uint32 *)inData)[2];
	((Uint32 *)&outGUID)[3] = ((Uint32 *)inData)[3];
}


#endif
