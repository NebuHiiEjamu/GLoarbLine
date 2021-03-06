
Uint32 UGUID::ToText(const SGUID &inGUID, void *outData, Uint32 inMaxSize)
{
	if (inMaxSize < 36)
		return 0;

	UMemory::Fill(outData, 36, '0');
	Uint8 *pData = (Uint8 *)outData;
	 
	Uint8 bufTemp[9];
	
	Uint32 nSize = UText::Format(bufTemp, 9, "%lX-", inGUID.time_low);
	UMemory::Copy(pData + 9 - nSize, bufTemp, nSize);
	pData += 9;
	 
	nSize = UText::Format(bufTemp, 5, "%hX-", inGUID.time_mid);
	UMemory::Copy(pData + 5 - nSize, bufTemp, nSize);
	pData += 5;

	nSize = UText::Format(bufTemp, 5, "%hX-", inGUID.time_hi_and_version);
	UMemory::Copy(pData + 5 - nSize, bufTemp, nSize);
	pData += 5;

	nSize = UText::Format(bufTemp, 2, "%X", inGUID.clock_seq_hi_and_reserved);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;

	nSize = UText::Format(bufTemp, 3, "%X-", inGUID.clock_seq_low);
	UMemory::Copy(pData + 3 - nSize, bufTemp, nSize);
	pData += 3;

	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[0]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;

	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[1]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;
	
	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[2]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;
	
	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[3]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;
	
	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[4]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;
	
	nSize = UText::Format(bufTemp, 2, "%X", inGUID.node[5]);
	UMemory::Copy(pData + 2 - nSize, bufTemp, nSize);
	pData += 2;
	
	return 36;
}

bool UGUID::FromText(SGUID &outGUID, const void *inData, Uint32 inDataSize)
{
	if (inDataSize < 36)
		return false;

	Uint8 *pData = (Uint8 *)inData;
	if (*(pData + 8) != '-' || *(pData + 13) != '-' || *(pData + 18) != '-' || *(pData + 23) != '-')
		return false;
	
	UMemory::HexToData(pData, 8, &outGUID.time_low, 4);
	UMemory::HexToData(pData + 9, 4, &outGUID.time_mid, 2);
	UMemory::HexToData(pData + 14, 4, &outGUID.time_hi_and_version, 2);
	UMemory::HexToData(pData + 19, 2, &outGUID.clock_seq_hi_and_reserved, 1);
	UMemory::HexToData(pData + 21, 2, &outGUID.clock_seq_low, 1);
	UMemory::HexToData(pData + 24, 2, &outGUID.node[0], 1);
	UMemory::HexToData(pData + 26, 2, &outGUID.node[1], 1);
	UMemory::HexToData(pData + 28, 2, &outGUID.node[2], 1);
	UMemory::HexToData(pData + 30, 2, &outGUID.node[3], 1);
	UMemory::HexToData(pData + 32, 2, &outGUID.node[4], 1);
	UMemory::HexToData(pData + 34, 2, &outGUID.node[5], 1);
	
	return true;
}

