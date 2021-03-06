#pragma once


class MapRasterizerProvider
{
public:
	MapRasterizerProvider(void);
	~MapRasterizerProvider(void);
	void obtainMaps(const char* path);
	boxI getWholeBox();
	std::list<std::shared_ptr<const MapObjectData>> obtainMapData(boxI& areaI, int zoom);
	bool obtainMapPrimitives(std::list<std::shared_ptr<const MapObjectData>>& mapData, int zoom, std::shared_ptr<MapRasterizerContext>& _context);
	bool obtainPrimitivesSymbols(std::shared_ptr<MapRasterizerContext>& _context);
	void applyStyle(std::shared_ptr<MapStyleEval>& evalData);
	void initializeOneWayPaint( SkPaint& paint );
	void initialize();
	bool obtainBitmapShader( const std::string& name, SkBitmapProcShader* &outShader ) const;
	bool obtainPathEffect( const std::string& encodedPathEffect, SkPathEffect* &outPathEffect ) const;
	bool obtainMapIcon( const std::string& name, std::shared_ptr<const SkBitmap>& outIcon ) const;
	bool obtainTextShield( const std::string& name, std::shared_ptr<const SkBitmap>& outTextShield ) const;
	std::vector<uint8_t> obtainResourceByName(const std::string& name) const;

	std::shared_ptr<DefaultMapStyleValue> getDefaultStyles()
	{
		return workingStyle->getDefaultValueDefinitions();
	}

	std::shared_ptr<MapStyleInfo> getStyleInfo()
	{
		return workingStyle;
	}

	const SkPaint& mapPaint;
    const SkPaint& textPaint;

    const SkColor& defaultBgColor;
    const uint32_t& shadowLevelMin;
    const uint32_t& shadowLevelMax;
    const double& polygonMinSizeToDisplay;
    const uint32_t& roadDensityZoomTile;
    const uint32_t& roadsDensityLimitPerTile;
    const int& shadowRenderingMode;
    const SkColor& shadowRenderingColor;
	const std::vector< SkPaint >& oneWayPaints;
    const std::vector< SkPaint >& reverseOneWayPaints;

	std::shared_ptr<const MapStyleRule> attributeRule_defaultColor;
    std::shared_ptr<const MapStyleRule> attributeRule_shadowRendering;
	std::shared_ptr<const MapStyleRule> attributeRule_polygonMinSizeToDisplay;
    std::shared_ptr<const MapStyleRule> attributeRule_roadDensityZoomTile;
    std::shared_ptr<const MapStyleRule> attributeRule_roadsDensityLimitPerTile;

	std::shared_ptr<BinaryMapSection> dummySectionData;
private:
	std::shared_ptr<MapStyleInfo> workingStyle;
	std::list<std::shared_ptr<BinaryIndexDataReader>> mapProviders;
	



    SkPaint _mapPaint;
    SkPaint _textPaint;

	std::vector< SkPaint > _oneWayPaints;
    std::vector< SkPaint > _reverseOneWayPaints;
    SkColor _defaultBgColor;
    uint32_t _shadowLevelMin;
    uint32_t _shadowLevelMax;
    double _polygonMinSizeToDisplay;
    uint32_t _roadDensityZoomTile;
    uint32_t _roadsDensityLimitPerTile;
    int _shadowRenderingMode;
    SkColor _shadowRenderingColor;


	mutable std::mutex _shadersBitmapsMutex;
	mutable std::unordered_map< std::string, std::shared_ptr<SkBitmap> > _shadersBitmaps;

	mutable std::mutex _pathEffectsMutex;
	mutable std::unordered_map< std::string, SkPathEffect* > _pathEffects;

	mutable std::mutex _mapIconsMutex;
	mutable std::unordered_map< std::string, std::shared_ptr<const SkBitmap> > _mapIcons;

	mutable std::mutex _textShieldsMutex;
	mutable std::unordered_map< std::string, std::shared_ptr<const SkBitmap> > _textShields;

	
	void collectSymbolsFromPrimitives(std::shared_ptr<MapRasterizerContext>& _context, 
	std::vector< std::shared_ptr<MapRasterizer::GraphicElement> >& elements, const MapRasterizer::GraphElementType type,
    std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);
	void uploadSymbolsForPolygon(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);
	void uploadSymbolsForPolyline(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);
	void uploadSymbolsForPoint(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);
	void uploadSymbolTextForElement(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement,const pointI& ptSymbolLoc, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);
	void uploadSymbolIconForElement(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement,const pointI& ptSymbolLoc, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols);

};

