#pragma once
class ArchiveIO
{
public:
	ArchiveIO(void);
	~ArchiveIO(void);

};



template<class T> portable_binary_oarchive& writeSmallInt(portable_binary_oarchive& oa, T data)
{
	char* shortData = (char*)&data;
	for (int i = sizeof(data)-1; i > sizeof(data)-3; i--)
	{
		oa.save_binary((const void*)&shortData[i], 1);
	}
	return oa;
}

template<class T> portable_binary_oarchive& writeInt(portable_binary_oarchive& oa, T data)
{
	char* shortData = (char*)&data;
	for (int i = sizeof(data)-1; i > sizeof(data)-5; i--)
	{
		oa.save_binary((const void*)&shortData[i], 1);
	}
	return oa;
}

template<> portable_binary_oarchive& writeInt<double>(portable_binary_oarchive& oa, double data)
{
	float shortData = (float)data;
	oa << shortData;
	return oa;
}