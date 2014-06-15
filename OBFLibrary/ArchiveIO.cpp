#include "stdafx.h"

#include "ArchiveIO.h"

int parseSmallIntFromBytes(const void* plData, int position)
{
	int val = 0;
	char valData = 0;
	char* pData = (char*)plData;
	memcpy(&valData, pData + position + 1, 1);
	val = (0xff & valData) << 8;
	memcpy(&valData, pData + position, 1);
	val |= (0xff & valData);
	return val;
}


int parseIntFromBytes(const void* byteData, int position)
{
	int val = 0;
	char valData = 0;
	char* pData = (char*)byteData;
	memcpy(&valData, pData + position + 3, 1);
	val = (0xff & valData) << 24;
	memcpy(&valData, pData + position + 2, 1);
	val |= (0xff & valData) << 16;
	memcpy(&valData, pData + position + 1, 1);
	val |= (0xff & valData) << 8;
	memcpy(&valData, pData + position, 1);
	val |= (0xff & valData);
	return val;
}