#pragma once
class MapRasterizerProvider
{
public:
	MapRasterizerProvider(void);
	~MapRasterizerProvider(void);
	void obtainMaps(const char* path);
	boxI getWholeBox();
	std::list<std::shared_ptr<MapObjectData>> obtainMapData(boxI& areaI, int zoom);

private:
	std::shared_ptr<MapStyleInfo> workingStyle;
	std::list<std::shared_ptr<BinaryIndexDataReader>> mapProviders;
	
};

