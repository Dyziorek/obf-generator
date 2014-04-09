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
	oa.save_binary((const void*)&shortData[1], 1);
	oa.save_binary((const void*)&shortData[0], 1);
	return oa;
}

template<class T> portable_binary_oarchive& writeInt(portable_binary_oarchive& oa, T data)
{
	char* shortData = (char*)&data;
	oa.save_binary((const void*)&shortData[3], 1);
	oa.save_binary((const void*)&shortData[2], 1);
	oa.save_binary((const void*)&shortData[1], 1);
	oa.save_binary((const void*)&shortData[0], 1);
	return oa;
}

template<> portable_binary_oarchive& writeInt<double>(portable_binary_oarchive& oa, double data)
{
	float shortData = (float)data;
	oa << shortData;
	return oa;
}