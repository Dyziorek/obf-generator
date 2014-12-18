#pragma once

class MapRasterizerProvider;

class MapRasterizerContext;

struct RenderSymbol;
struct RenderSymbolGroup
{
	RenderSymbolGroup(const std::shared_ptr<MapObjectData>& mapObject): _mapObject(mapObject)
	{
	}
	virtual ~RenderSymbolGroup() {}
	const std::shared_ptr<MapObjectData> _mapObject;
	std::vector<std::shared_ptr<RenderSymbol>> _symbols;
};

struct RenderSymbol
{
	RenderSymbol(std::shared_ptr<const RenderSymbolGroup> _group, std::shared_ptr<MapObjectData> mapObject, std::shared_ptr<const SkBitmap> surface, uint32_t order)
		:group(_group), _mapObject(mapObject), bitmap(surface), zOrder(order) { assert(_mapObject);}

	virtual ~RenderSymbol() {}
	boxI offRect;
	uint32_t zOrder;
	const std::weak_ptr<const RenderSymbolGroup> group;
	const std::shared_ptr<MapObjectData> _mapObject;
	const std::shared_ptr<const SkBitmap> bitmap;
};

struct RenderSymbol_Path : public RenderSymbol
{

	RenderSymbol_Path(std::shared_ptr<const RenderSymbolGroup> _group, std::shared_ptr<MapObjectData> mapObject, std::shared_ptr<const SkBitmap> surface, uint32_t zOrder, std::vector<std::pair<float, float>> glyphData)
		: RenderSymbol(_group, mapObject, surface, zOrder), glyphs(glyphData)
	{
		assert(_mapObject);
	}
	virtual ~RenderSymbol_Path() {}
	
	const std::vector<std::pair<float, float>> glyphs;

};


struct RenderSymbol_Pin : public RenderSymbol
{

	RenderSymbol_Pin(std::shared_ptr<const RenderSymbolGroup> _group, std::shared_ptr<MapObjectData> mapObject, std::shared_ptr<const SkBitmap> surface, uint32_t zOrder, const pointI& loc, const pointI& off )
		: RenderSymbol(_group, mapObject, surface, zOrder), location(loc), offset(off)
	{
		assert(_mapObject);
	}
	virtual ~RenderSymbol_Pin() {}
	const pointI location;
	const pointI offset;
	

};


class MapRasterizer
{
public:
	enum GraphElementType : uint32_t
{
    Point = 1,
    Polyline = 2,
    Polygon = 3,
};


enum GraphElementsType
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


	double zOrder;
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
	std::shared_ptr<GraphicElement> graph;
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


public:
	MapRasterizer(MapRasterizerProvider& dataSrc);

	~MapRasterizer(void);

	void createContextData(boxI& workArea, int workZoom);

	void DrawMap(std::string pathFile);
	bool DrawMap(SkCanvas& canvas);
	bool DrawSymbols(SkCanvas& canvas);
	

	enum {
            PolygonAreaCutoffLowerThreshold = 75,
            BasemapZoom = 11,
            DetailedLandDataZoom = 14,
        };
	
	enum PaintValuesSet
        {
            Set_0,
            Set_1,
            Set_minus1,
            Set_minus2,
            Set_3,
        };
private:

	SkPaint _mapPaint;

	MapRasterizerProvider& _source;
	std::shared_ptr<MapRasterizerContext> _context;
	bool rasterizeMapElements(const AreaI* const destinationArea,SkCanvas& canvas, const std::vector< std::shared_ptr<GraphicElement> >& primitives, GraphElementsType type);
	bool rasterizePolygon( const AreaI* const destinationArea, SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive);
	bool rasterizePolyline(const AreaI* const destinationArea, SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive, bool drawOnlyShadow);
	bool updatePaint(const MapStyleResult& evalResult, const PaintValuesSet valueSetSelector, const bool isArea );
	void rasterizeLineShadow(  SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius );
	void rasterizeLine_OneWay( SkCanvas& canvas, const SkPath& path, int oneway );
	void calculateVertex( const pointI& point31, pointF& vertex );
	bool contains( const std::vector< pointF >& vertices, const pointF& other );
	bool rasterizeSymbols(std::vector<const std::shared_ptr<const RenderSymbolGroup>>& renderedSymbols);
};

