#pragma once

enum GraphElementType
        {
            Polygons,
            Polylines,
            Polylines_ShadowOnly,
            Points,
        };

struct GraphicElement;
struct GraphicElementGroup
{
	std::shared_ptr<MapObjectData> _mapObject;
	std::vector<std::shared_ptr<GraphicElement>> _polygons, _polyLines, _points;
};

struct GraphicElement
{
	GraphicElement(const std::shared_ptr<GraphicElementGroup> group, const std::shared_ptr<MapObjectData>& mapData,
		const GraphElementType type, const uint32_t typeIdIndex) : _group(group), _mapData(mapData), _type(type), _typeIdIndex(typeIdIndex)
	{
	}
	const std::weak_ptr<GraphicElementGroup> _group;
	const std::shared_ptr<MapObjectData> _mapData;
	const GraphElementType _type;
	const uint32_t _typeIdIndex;


	uint32_t zOrder;
	std::shared_ptr<MapStyleResult> styleResult;
};

struct RasterSymbol;
struct RasterSymbolGroup
{
	std::shared_ptr<MapObjectData> _mapObject;
	std::vector<std::shared_ptr<RasterSymbol>> _symbols;
};

struct RasterSymbol
{
	RasterSymbol();
	virtual ~RasterSymbol();
	pointI location;
	uint32_t zOrder;
};

struct RasterSymbolonPath : public RasterSymbol
{

	RasterSymbolonPath();
	virtual ~RasterSymbolonPath();

	std::string value;
    bool drawOnPath;
    int verticalOffset;
    uint32_t color;
    int size;
    int shadowRadius;
    uint32_t shadowColor;
    int wrapWidth;
    bool isBold;
    int minDistance;
    std::string shieldResourceName;
};

struct RasterSymbolPin : public RasterSymbol
{

	RasterSymbolPin();
	virtual ~RasterSymbolPin();

	std::string resourceName;
};

class MapRasterizerProvider;

class MapRasterizerContext;

class MapRasterizer
{
public:
	MapRasterizer(MapRasterizerProvider& dataSrc);

	~MapRasterizer(void);

	void createContextData(boxI& workArea, int workZoom);

	void DrawMap(SkCanvas& canvas);


	enum {
            PolygonAreaCutoffLowerThreshold = 75,
            BasemapZoom = 11,
            DetailedLandDataZoom = 14,
        };

private:

	MapRasterizerProvider& _source;
	std::shared_ptr<MapRasterizerContext> _context;
};

