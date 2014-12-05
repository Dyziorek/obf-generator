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
	void rasterizeMapElements(const AreaI* const destinationArea,SkCanvas& canvas, const std::vector< std::shared_ptr<GraphicElement> >& primitives, GraphElementType type);

	void rasterizePolygon( const AreaI* const destinationArea, SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive);
	void rasterizePolyline(const AreaI* const destinationArea, SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive, bool drawOnlyShadow);
	bool updatePaint(const MapStyleResult& evalResult, const PaintValuesSet valueSetSelector, const bool isArea );
	void rasterizeLineShadow(  SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius );
	void rasterizeLine_OneWay( SkCanvas& canvas, const SkPath& path, int oneway );
	void calculateVertex( const pointI& point31, pointF& vertex );
	bool contains( const std::vector< pointF >& vertices, const pointF& other );
};

