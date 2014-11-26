#pragma once


class MapRasterizerProvider
{
public:
	MapRasterizerProvider(void);
	~MapRasterizerProvider(void);
	void obtainMaps(const char* path);
	boxI getWholeBox();
	std::list<std::shared_ptr<MapObjectData>> obtainMapData(boxI& areaI, int zoom);
	bool obtainMapPrimitives(std::list<std::shared_ptr<MapObjectData>>& mapData, int zoom, std::shared_ptr<MapRasterizerContext>& _context);
	void applyStyle(std::shared_ptr<MapStyleEval>& evalData);
private:
	std::shared_ptr<MapStyleInfo> workingStyle;
	std::list<std::shared_ptr<BinaryIndexDataReader>> mapProviders;
	
	std::shared_ptr<MapStyleEval> _orderEval;
	std::shared_ptr<MapStyleEval> _pointEval;
	std::shared_ptr<MapStyleEval> _lineEval;
	std::shared_ptr<MapStyleEval> _textEval;
	std::shared_ptr<MapStyleEval> _polyEval;
};

