#pragma once

#include "iconv.h"

class ArchiveIO
{
public:
	ArchiveIO(void);
	~ArchiveIO(void);

};



template<class T> std::stringstream& writeSmallInt(std::stringstream& oa, T data)
{
	const char* shortData = (char*)&data;
	oa.rdbuf()->sputn(&shortData[0], 1);
	oa.rdbuf()->sputn(&shortData[1], 1);
	return oa;
}

template<class T> std::stringstream& writeInt(std::stringstream& oa, T data)
{
	const char* shortData = (char*)&data;
	oa.rdbuf()->sputn(&shortData[0], 1);
	oa.rdbuf()->sputn(&shortData[1], 1);
	oa.rdbuf()->sputn(&shortData[2], 1);
	oa.rdbuf()->sputn(&shortData[3], 1);
	return oa;
}


template<class T> std::stringstream& writeLongInt(std::stringstream& oa, T data)
{
	size_t dataSize = sizeof(T);
	if (dataSize >= 8)
	{
		// truncate higher part of data leaving only last 8byte part of type
		const char* shortData = (char*)&data;
		oa.rdbuf()->sputn(&shortData[0], 1);
		oa.rdbuf()->sputn(&shortData[1], 1);
		oa.rdbuf()->sputn(&shortData[2], 1);
		oa.rdbuf()->sputn(&shortData[3], 1);
		oa.rdbuf()->sputn(&shortData[4], 1);
		oa.rdbuf()->sputn(&shortData[5], 1);
		oa.rdbuf()->sputn(&shortData[6], 1);
		oa.rdbuf()->sputn(&shortData[7], 1);
		return oa;
	}
	else if (dataSize < 8)
	{
		const char* shortData = (char*)&data;
		for (size_t iCol = 0; iCol < dataSize; iCol++)
		{
			oa.rdbuf()->sputn(&shortData[iCol], 1);
		}
		const char* shortDataZero = '\0';
		for (size_t iCol = 7; iCol >= dataSize; iCol--)
		{
			oa.rdbuf()->sputn(shortDataZero, 1);
		}
		return oa;
	}
	return oa;
}

template<class T> std::stringstream& readSmallInts(std::stringstream& oa, std::vector<T>& data)
{
	size_t dataSize = sizeof(T);
	oa.rdbuf()->pubseekpos(0);
	while(oa.rdbuf()->in_avail() > 0)
	{
		T dataElem;
		ZeroMemory(&dataElem, dataSize);
		char* shortData = (char*)&dataElem;
		oa.rdbuf()->sgetn(&shortData[0], 1);
		oa.rdbuf()->sgetn(&shortData[1], 1);
		data.push_back(dataElem);
	}
	return oa;
}

template<class T> std::stringstream& readInts(std::stringstream& oa, std::vector<T>& data)
{
	size_t dataSize = sizeof(T);
	oa.rdbuf()->pubseekpos(0);
	while(oa.rdbuf()->in_avail() > 0)
	{
		T dataElem;
		ZeroMemory(&dataElem, dataSize);
		char* shortData = (char*)&dataElem;
		oa.rdbuf()->sgetn(&shortData[0], 1);
		oa.rdbuf()->sgetn(&shortData[1], 1);
		oa.rdbuf()->sgetn(&shortData[2], 1);
		oa.rdbuf()->sgetn(&shortData[3], 1);
		data.push_back(dataElem);
	}
	return oa;
}

int parseSmallIntFromBytes(const void* plData, int position);



int parseIntFromBytes(const void* byteData, int position);


inline void
reverse_bytes(char size, char *address);

struct iconverter
{
	iconverter(const char* from, const char* to) 
	{
		_i = iconv_open(to, from);
		if (_i != (iconv_t)-1)
		{
			int transVal = 1;
			iconvctl(_i, ICONV_SET_TRANSLITERATE, (void*)&transVal);
		}
	}
	~iconverter()
	{
		if (_i != (iconv_t)-1)
		{
			iconv_close(_i);
		}
	}
	std::string convert(const std::string& utf8str)
	{
		if (_i == (iconv_t)-1)
		{
			return utf8str;
		}
		std::string ret;
		ret.resize(utf8str.size());
		size_t retStr = ret.size();
		size_t inpSize = utf8str.size();
		const char* inputStr = &utf8str[0];
		char* outStr = &ret[0];
		size_t proc = iconv(_i, (const char **)&inputStr, &inpSize, &outStr, &retStr);
		if (proc == (size_t)-1 && errno == E2BIG)
		{
			const size_t delta = outStr - &ret[0];
			retStr += ret.size();
			ret.resize(ret.size() * 2);
			outStr = &ret[delta];
			proc = iconv(_i, (const char **)&inputStr, &inpSize, &outStr, &retStr);
			if (proc == (size_t)-1) {
				return utf8str;
			}

		}
		ret.resize(ret.size()-retStr);
		return ret;
	}
	
	iconv_t _i;
};
