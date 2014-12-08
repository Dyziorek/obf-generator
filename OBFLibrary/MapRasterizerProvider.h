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
	void applyStyle(std::shared_ptr<MapStyleEval>& evalData);
	void initializeOneWayPaint( SkPaint& paint );
	void initialize();
	std::shared_ptr<DefaultMapStyleValue> getDefaultStyles()
	{
		return workingStyle->getDefaultValueDefinitions();
	}

	std::vector<uint8_t> obtainResourceByName(const std::string& name) const;
	bool obtainBitmapShader( const std::string& name, SkBitmapProcShader* &outShader ) const;
	bool obtainPathEffect( const std::string& encodedPathEffect, SkPathEffect* &outPathEffect ) const;
	bool obtainMapIcon( const std::string& name, std::shared_ptr<const SkBitmap>& outIcon ) const;
	bool obtainTextShield( const std::string& name, std::shared_ptr<const SkBitmap>& outTextShield ) const;

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
private:
	std::shared_ptr<MapStyleInfo> workingStyle;
	std::list<std::shared_ptr<BinaryIndexDataReader>> mapProviders;
	
	std::shared_ptr<MapStyleEval> _orderEval;
	std::shared_ptr<MapStyleEval> _pointEval;
	std::shared_ptr<MapStyleEval> _lineEval;
	std::shared_ptr<MapStyleEval> _textEval;
	std::shared_ptr<MapStyleEval> _polyEval;

	std::shared_ptr<const MapStyleRule>  _attributeRule_defaultColor;
    std::shared_ptr<const MapStyleRule> _attributeRule_shadowRendering;
	std::shared_ptr<const MapStyleRule> _attributeRule_polygonMinSizeToDisplay;
    std::shared_ptr<const MapStyleRule> _attributeRule_roadDensityZoomTile;
    std::shared_ptr<const MapStyleRule> _attributeRule_roadsDensityLimitPerTile;

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


};

