#pragma once
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
		for (int iCol = 7; iCol >= dataSize; iCol--)
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

extern int parseSmallIntFromBytes(const void* plData, int position);



extern int parseIntFromBytes(const void* byteData, int position);
