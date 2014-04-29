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

