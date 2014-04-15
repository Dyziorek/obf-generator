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

template<> std::stringstream& writeInt<float>(std::stringstream& oa, float data)
{
	float shortData = (float)data;
	oa << shortData;
	return oa;
}



template<class T> std::stringstream& readSmallInts(std::stringstream& oa, std::vector<T>& data)
{
	size_t dataSize = sizeof(T);
	T dataElem;
	char* shortData = (char*)&dataElem;
	oa.rdbuf()->pubseekpos(0);
	while(oa.rdbuf()->in_avail() > 0)
	{
		oa.rdbuf()->sgetn(&shortData[0], 1);
		oa.rdbuf()->sgetn(&shortData[1], 1);
		data.push_back(dataElem);
	}
	return oa;
}

template<class T> std::stringstream& readInts(std::stringstream& oa, std::vector<T>& data)
{
	size_t dataSize = sizeof(T);
	T dataElem;
	char* shortData = (char*)&dataElem;
	oa.rdbuf()->pubseekpos(0);
	while(oa.rdbuf()->in_avail() > 0)
	{
		oa.rdbuf()->sgetn(&shortData[0], 1);
		oa.rdbuf()->sgetn(&shortData[1], 1);
		oa.rdbuf()->sgetn(&shortData[2], 1);
		oa.rdbuf()->sgetn(&shortData[3], 1);
		data.push_back(dataElem);
	}
	return oa;
}

template<> std::stringstream& readInts<double>(std::stringstream& oa, std::vector<double>& data)
{
	float shortData;
	oa.rdbuf()->pubseekpos(0);
	while(oa.rdbuf()->in_avail() > 0)
	{
		oa >> shortData;
		data.push_back(shortData);
	}
	return oa;
}