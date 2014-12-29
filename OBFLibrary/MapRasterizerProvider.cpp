#include "stdafx.h"
#include "SkCanvas.h"
#include "SkDashPathEffect.h"
#include "SkBitmapProcShader.h"
#include "SkStream.h"
#include "SkImageDecoder.h"
#include <google\protobuf\io\coded_stream.h>
#include <boost/filesystem.hpp>
#include "MapObjectData.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"
#include "MapRasterizer.h"
#include "MapRasterizerContext.h"
#include "MapRasterizerProvider.h"
#include "EmbeddedResources.h"
#include "Tools.h"

#include <locale>
#include <codecvt>

namespace bf = boost::filesystem;

MapRasterizerProvider::MapRasterizerProvider(void) :
	defaultBgColor(_defaultBgColor), shadowLevelMin(_shadowLevelMin), shadowLevelMax(_shadowLevelMax),
	polygonMinSizeToDisplay(_polygonMinSizeToDisplay), roadDensityZoomTile(_roadDensityZoomTile),
	roadsDensityLimitPerTile(_roadsDensityLimitPerTile), shadowRenderingMode(_shadowRenderingMode), shadowRenderingColor(_shadowRenderingColor),
	mapPaint(_mapPaint), textPaint(_textPaint), oneWayPaints(_oneWayPaints), reverseOneWayPaints(_reverseOneWayPaints)/*,
	attributeRule_defaultColor(_attributeRule_defaultColor),attributeRule_shadowRendering(_attributeRule_shadowRendering),
	attributeRule_polygonMinSizeToDisplay(_attributeRule_polygonMinSizeToDisplay), attributeRule_roadDensityZoomTile(_attributeRule_roadDensityZoomTile),
    attributeRule_roadsDensityLimitPerTile(_attributeRule_roadsDensityLimitPerTile)*/
{
	workingStyle.reset(new MapStyleInfo());
	workingStyle->loadRenderStyles(nullptr);

#ifdef _DEBUG
	//workingStyle->dump(rulesetType::line);
#endif

	dummySectionData.reset(new BinaryMapSection());
	dummySectionData->rules.reset(new BinaryMapRules());
	dummySectionData->rules->createMissingRules();

	initialize();
}


MapRasterizerProvider::~MapRasterizerProvider(void)
{
}


void MapRasterizerProvider::initializeOneWayPaint( SkPaint& paint )
{
    paint.setAntiAlias(true);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(0xff6c70d5);
}

void MapRasterizerProvider::initialize()
{
	_mapPaint.setAntiAlias(true);

    _textPaint.setAntiAlias(true);
    _textPaint.setLCDRenderText(true);
    _textPaint.setTextEncoding(SkPaint::kUTF16_TextEncoding);
    /*_textPaint.setStyle(SkPaint::kFill_Style);
    _textPaint.setStrokeWidth(1);
    _textPaint.setColor(SK_ColorBLACK);
    _textPaint.setTextAlign(SkPaint::kCenter_Align);*/
    //static_assert(sizeof(QChar) == 2, "If QChar is not 2 bytes, then encoding is not kUTF16_TextEncoding");

    _shadowLevelMin = 0;
    _shadowLevelMax = 256;
    _roadDensityZoomTile = 0;
    _roadsDensityLimitPerTile = 0;
    _shadowRenderingMode = 0;
    _shadowRenderingColor = 0xff969696;
    _polygonMinSizeToDisplay = 0.0;
    _defaultBgColor = 0xfff1eee8;

	
    workingStyle->resolveAttribute("defaultColor", attributeRule_defaultColor);
    workingStyle->resolveAttribute("shadowRendering", attributeRule_shadowRendering);
    workingStyle->resolveAttribute("polygonMinSizeToDisplay", attributeRule_polygonMinSizeToDisplay);
    workingStyle->resolveAttribute("roadDensityZoomTile", attributeRule_roadDensityZoomTile);
    workingStyle->resolveAttribute("roadsDensityLimitPerTile", attributeRule_roadsDensityLimitPerTile);

    {
        const float intervals_oneway[4][4] =
        {
            {0, 12, 10, 152},
            {0, 12, 9, 153},
            {0, 18, 2, 154},
            {0, 18, 1, 155}
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_oneway[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_oneway[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_oneway[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_oneway[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _oneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _oneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _oneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _oneWayPaints.push_back(std::move(paint));
        }
    }
    
    {
        const float intervals_reverse[4][4] =
        {
            {0, 12, 10, 152},
            {0, 13, 9, 152},
            {0, 14, 2, 158},
            {0, 15, 1, 158}
        };
        SkPathEffect* arrowDashEffect1 = new SkDashPathEffect(intervals_reverse[0], 4, 0);
        SkPathEffect* arrowDashEffect2 = new SkDashPathEffect(intervals_reverse[1], 4, 1);
        SkPathEffect* arrowDashEffect3 = new SkDashPathEffect(intervals_reverse[2], 4, 1);
        SkPathEffect* arrowDashEffect4 = new SkDashPathEffect(intervals_reverse[3], 4, 1);

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(1.0f);
            paint.setPathEffect(arrowDashEffect1)->unref();
            _reverseOneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(2.0f);
            paint.setPathEffect(arrowDashEffect2)->unref();
            _reverseOneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(3.0f);
            paint.setPathEffect(arrowDashEffect3)->unref();
            _reverseOneWayPaints.push_back(std::move(paint));
        }

        {
            SkPaint paint;
            initializeOneWayPaint(paint);
            paint.setStrokeWidth(4.0f);
            paint.setPathEffect(arrowDashEffect4)->unref();
            _reverseOneWayPaints.push_back(std::move(paint));
        }
    }


	MapStyleResult evalResult;

    if(attributeRule_polygonMinSizeToDisplay)
    {
        MapStyleEval evaluator(getStyleInfo());
        
        evaluator.setIntValue(getDefaultStyles()->id_INPUT_MINZOOM, ZoomLevel0);

        evalResult.clear();
        if(evaluator.evaluateRule(attributeRule_polygonMinSizeToDisplay, &evalResult))
        {
            int polygonMinSizeToDisplay;
			if(evalResult.getIntVal(getDefaultStyles()->id_OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
                _polygonMinSizeToDisplay = polygonMinSizeToDisplay;
        }
    }
	if(attributeRule_roadDensityZoomTile)
    {
        MapStyleEval evaluator(getStyleInfo());
        
        evaluator.setIntValue(getDefaultStyles()->id_INPUT_MINZOOM, ZoomLevel0);

        evalResult.clear();
        if(evaluator.evaluateRule(attributeRule_roadDensityZoomTile, &evalResult))
        {
            int roadDensityZoomTile;
			if(evalResult.getIntVal(getDefaultStyles()->id_OUTPUT_ATTR_INT_VALUE, roadDensityZoomTile))
                _roadDensityZoomTile = roadDensityZoomTile;
        }
    }
	if(attributeRule_roadsDensityLimitPerTile)
    {
        MapStyleEval evaluator(getStyleInfo());
        
        evaluator.setIntValue(getDefaultStyles()->id_INPUT_MINZOOM, ZoomLevel0);

        evalResult.clear();
        if(evaluator.evaluateRule(attributeRule_roadsDensityLimitPerTile, &evalResult))
        {
            int roadsDensityLimitPerTile;
			if(evalResult.getIntVal(getDefaultStyles()->id_OUTPUT_ATTR_INT_VALUE, roadsDensityLimitPerTile))
                _roadsDensityLimitPerTile = roadsDensityLimitPerTile;
        }
    }
}

void MapRasterizerProvider::obtainMaps(const char* path)
{
	bf::path& fileInfo = bf::path(std::string(path));

	if ( bf::is_regular_file(fileInfo))
	{
		mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(fileInfo)));
	}
	else if (bf::is_directory(fileInfo))
	{
		bf::directory_iterator dirEntry = bf::directory_iterator(fileInfo);
		std::vector<bf::directory_entry> entries;
		std::for_each(dirEntry, bf::directory_iterator(), [&entries](bf::directory_entry input)
		{
			auto checker = input;
			if (checker.path().extension().generic_string() == std::string("pbf"))
			{
				entries.push_back(input);
			}
		});
		for (auto mapPath : entries)
		{
			auto& pathData = (bf::path&)mapPath.path();
			mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(pathData)));
		}
		
	}

}

std::list<std::shared_ptr<const MapObjectData>> MapRasterizerProvider::obtainMapData(boxI& areaI, int zoom)
{
	std::list<std::shared_ptr<const MapObjectData>> outListData;

	

	for (auto& mapData : mapProviders)
	{
		mapData->getMapObjects(areaI, zoom, outListData);

	}


	return outListData;
}


boxI MapRasterizerProvider::getWholeBox()
{
	boxI baseBox;
	bg::assign_inverse(baseBox);

	for (auto& mapDataItem : mapProviders)
	{
		for (auto& sectData : mapDataItem->GetReader().getSections())
		{
			bg::expand(baseBox, std::get<0>(sectData));
		}
	}

	return baseBox;
}

void MapRasterizerProvider::applyStyle(std::shared_ptr<MapStyleEval>& evalData)
{
	// no initial settings later on.
}

bool MapRasterizerProvider::obtainMapPrimitives(std::list<std::shared_ptr<const MapObjectData>>& mapData, int zoom,  std::shared_ptr<MapRasterizerContext>& _context)
{
	auto& builtinDef = workingStyle->getDefaultValueDefinitions();

	MapStyleEval orderEval(workingStyle, 0.30f);
	MapStyleEval pointEval(workingStyle, 0.30f);;
	MapStyleEval lineEval(workingStyle, 0.30f);;
	MapStyleEval textEval(workingStyle, 0.30f);;
	MapStyleEval polyEval(workingStyle, 0.30f);;
	//applyStyle(_orderEval);

	orderEval.setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	orderEval.setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	pointEval.setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	pointEval.setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	lineEval.setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	lineEval.setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	polyEval.setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	polyEval.setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	
	for (std::shared_ptr<const MapObjectData> cobjectMapData: mapData)
	{
		std::shared_ptr<MapObjectData> objectMapData(std::const_pointer_cast<MapObjectData>(cobjectMapData));

		std::shared_ptr<MapRasterizer::GraphicElementGroup> graphGroup(new MapRasterizer::GraphicElementGroup());
		graphGroup->_mapObject = objectMapData;
		int typeRuleIDIndex = 0;
		for(int typeRuleId : objectMapData->typeIds)
		{
			if (objectMapData->section.expired())
				continue;
			auto sectionData = objectMapData->section.lock();
			auto& mapDecoder = sectionData->rules->getRuleInfo(typeRuleId);
			
			orderEval.setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
			orderEval.setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
			orderEval.setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
			orderEval.setBoolValue(builtinDef->id_INPUT_AREA, objectMapData->isArea);
			orderEval.setBoolValue(builtinDef->id_INPUT_CYCLE, objectMapData->isClosedFigure());
			orderEval.setBoolValue(builtinDef->id_INPUT_POINT, objectMapData->points.size() == 1);

			MapStyleResult orderResults;
			bool bOK = orderEval.evaluate(objectMapData, rulesetType::order, &orderResults);
			if (!bOK)
				continue;

			int graphicType = -1;
			bOK = orderResults.getIntVal(builtinDef->id_OUTPUT_OBJECT_TYPE, graphicType);
			if (!bOK)
				continue;
			int zOrder = -1;
			bOK = orderResults.getIntVal(builtinDef->id_OUTPUT_ORDER, zOrder);
			if (!bOK)
				continue;

			std::shared_ptr<MapRasterizer::GraphicElement> graphicElement(new MapRasterizer::GraphicElement(graphGroup, objectMapData, static_cast<MapRasterizer::GraphElementType>(graphicType), typeRuleIDIndex));
			graphicElement->zOrder = zOrder;

			if(graphicElement->_type == MapRasterizer::GraphElementType::Polygon)
			{
				if (objectMapData->points.size() <=2)
				{
					continue;
				}

				if (!objectMapData->isClosedFigure())
				{
					// not a correct polygon cannot be filled figure
					continue;
				}
				if (!objectMapData->isClosedFigure(true))
				{
					// not a correct polygon cannot be filled figure withour proper inner data
					continue;
				}

				polyEval.setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				polyEval.setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);

				std::shared_ptr<MapStyleResult> polyResults(new MapStyleResult());
				bOK = polyEval.evaluate(objectMapData, rulesetType::polygon, polyResults.get());
				if (!bOK)
					continue;
				
				graphicElement->styleResult = polyResults;

				auto areaData = Tools::polygonArea(objectMapData->points);
				if (areaData > MapRasterizer::PolygonAreaCutoffLowerThreshold)
				{
					// polygon large enough to show as polygon in any zoom plus convert to point in low zooms if neccessary
					graphicElement->zOrder += 1.0 / areaData;
					std::shared_ptr<MapRasterizer::GraphicElement> pointElement(new MapRasterizer::GraphicElement(graphGroup, objectMapData, MapRasterizer::GraphElementType::Point, typeRuleIDIndex));
					pointElement->zOrder = graphicElement->zOrder;
					graphGroup->_polygons.push_back(std::move(graphicElement));

					pointEval.setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
					pointEval.setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
					std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
					bOK = pointEval.evaluate(objectMapData, rulesetType::point, pointResults.get());
					if (bOK)
					{
						// indicates it has icon
						pointElement->styleResult = pointResults;
					}
					if (typeRuleIDIndex == 0 || objectMapData->nameTypeString.empty())
					{
						graphGroup->_points.push_back(std::move(pointElement));
					}
				}

			}
			else if (graphicElement->_type == MapRasterizer::GraphElementType::Polyline)
			{
				if (objectMapData->points.size() < 2)
				{
					continue;
				}
				lineEval.setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				lineEval.setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				lineEval.setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
				std::shared_ptr<MapStyleResult> lineResults(new MapStyleResult());
				bOK = lineEval.evaluate(objectMapData, rulesetType::line, lineResults.get());
				if (!bOK)
					continue;

				graphicElement->styleResult = lineResults;

				graphGroup->_polyLines.push_back(std::move(graphicElement));
			}
			else if(graphicElement->_type == MapRasterizer::GraphElementType::Point)
			{
				if (objectMapData->points.size() < 1)
				{
					// no point defined
					continue;
				}
				pointEval.setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				pointEval.setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
				bOK = pointEval.evaluate(objectMapData, rulesetType::point, pointResults.get());
				if (bOK)
				{
					graphicElement->styleResult = pointResults;
				}
				if (typeRuleIDIndex != 0 || (objectMapData->nameTypeString.size() == 0 && !bOK))
				{
					continue;
				}
				graphGroup->_points.push_back(std::move(graphicElement));
			}
			else
			{
				// !!
				assert(false);
				continue;
			}
			typeRuleIDIndex++;
		}
		if (graphGroup)
		{
			for (auto apolygon : graphGroup->_polygons)
			{
				_context->_polygons.push_back(apolygon);
			}
			for (auto apolyline : graphGroup->_polyLines)
			{
				_context->_polyLines.push_back(apolyline);
			}
			for (auto apoint : graphGroup->_points)
			{
				_context->_points.push_back(apoint);
			}
		}

		_context->_graphicElements.push_back(std::move(graphGroup));

	}

	return _context->_graphicElements.size() > 0;
}

bool MapRasterizerProvider::obtainBitmapShader( const std::string& name, SkBitmapProcShader* &outShader ) const
{
	std::lock_guard<std::mutex> scopedLock(_shadersBitmapsMutex);

    auto itShaderBitmap = _shadersBitmaps.find(name);
    if(itShaderBitmap == _shadersBitmaps.cend())
    {
        const auto shaderBitmapPath = boost::format("map/shaders/%1%.png") % name;

        // Get data from embedded resources
        const auto data = obtainResourceByName(shaderBitmapPath.str());

        // Decode bitmap for a shader
        auto shaderBitmap = new SkBitmap();
        SkMemoryStream dataStream(data.data(), data.size(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, shaderBitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;
        auto itShaderBitmapRet = _shadersBitmaps.insert(std::make_pair(name, std::shared_ptr<SkBitmap>(shaderBitmap)));
		itShaderBitmap = itShaderBitmapRet.first;
    }

    // Create shader from that bitmap
	outShader = new SkBitmapProcShader(*itShaderBitmap->second.get(), SkShader::kRepeat_TileMode, SkShader::kRepeat_TileMode);
    return true;
}

bool MapRasterizerProvider::obtainPathEffect( const std::string& encodedPathEffect, SkPathEffect* &outPathEffect ) const
{
    std::lock_guard<std::mutex> scopedLock(_pathEffectsMutex);

    auto itPathEffects = _pathEffects.find(encodedPathEffect);
    if(itPathEffects == _pathEffects.cend())
    {
        std::vector<std::string> strIntervals;
		boost::split(strIntervals, encodedPathEffect, boost::is_any_of("_"), boost::algorithm::token_compress_on);

        const auto intervals = new SkScalar[strIntervals.size()];
        auto interval = intervals;
        for(auto itInterval = strIntervals.cbegin(); itInterval != strIntervals.cend(); ++itInterval, interval++)
            *interval = boost::lexical_cast<float>(*itInterval);

        SkPathEffect* pathEffect = new SkDashPathEffect(intervals, strIntervals.size(), 0);
        delete[] intervals;

        auto itPathEffectsRet = _pathEffects.insert(std::make_pair(encodedPathEffect, pathEffect));
		itPathEffects = itPathEffectsRet.first;
    }

	outPathEffect = itPathEffects->second;
    return true;
}

bool MapRasterizerProvider::obtainMapIcon( const std::string& name, std::shared_ptr<const SkBitmap>& outIcon ) const
{
    std::lock_guard<std::mutex> scopedLock(_mapIconsMutex);

    auto itIcon = _mapIcons.find(name);
    if(itIcon == _mapIcons.cend())
    {
        const auto bitmapPath =  boost::format("map/map_icons/%1%.png") % name;

        // Get data from embedded resources
        auto data = obtainResourceByName(bitmapPath.str());

        // Decode data
        auto bitmap = new SkBitmap();
        SkMemoryStream dataStream(data.data(), data.size(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        auto itIconRet = _mapIcons.insert(std::make_pair(name, std::shared_ptr<const SkBitmap>(bitmap)));
		itIcon = itIconRet.first;
    }

	outIcon = itIcon->second;
    return true;
}

bool MapRasterizerProvider::obtainTextShield( const std::string& name, std::shared_ptr<const SkBitmap>& outTextShield ) const
{
    std::lock_guard<std::mutex> scopedLock(_textShieldsMutex);

    auto itTextShield = _textShields.find(name);
    if(itTextShield == _textShields.cend())
    {
        const auto bitmapPath =  boost::format("map/shields/%1%.png") % name;

        // Get data from embedded resources
        auto data = obtainResourceByName(bitmapPath.str());

        // Decode data
        auto bitmap = new SkBitmap();
        SkMemoryStream dataStream(data.data(), data.size(), false);
        if(!SkImageDecoder::DecodeStream(&dataStream, bitmap, SkBitmap::Config::kNo_Config, SkImageDecoder::kDecodePixels_Mode))
            return false;

        auto itTextShieldRet = _textShields.insert(std::make_pair(name, std::shared_ptr<const SkBitmap>(bitmap)));
		itTextShield = itTextShieldRet.first;
    }

	outTextShield = itTextShield->second;
    return true;
}

std::vector<uint8_t> MapRasterizerProvider::obtainResourceByName( const std::string& name ) const
{
	std::vector<char> resultData = EmbeddedResources::getDataFromResource(name);
	return std::vector<uint8_t>(resultData.begin(), resultData.end());

}

bool MapRasterizerProvider::obtainPrimitivesSymbols(std::shared_ptr<MapRasterizerContext>& _context)
{
	for (auto& graphicElement : _context->_graphicElements)
	{
		auto rasterSymbolGroup = std::shared_ptr<MapRasterizer::RasterSymbolGroup>(new MapRasterizer::RasterSymbolGroup);
		collectSymbolsFromPrimitives(_context, graphicElement->_polygons, MapRasterizer::GraphElementType::Polygon, rasterSymbolGroup->_symbols);
		collectSymbolsFromPrimitives(_context, graphicElement->_polyLines, MapRasterizer::GraphElementType::Polyline, rasterSymbolGroup->_symbols);
		collectSymbolsFromPrimitives(_context, graphicElement->_points, MapRasterizer::GraphElementType::Point, rasterSymbolGroup->_symbols);
		rasterSymbolGroup->_mapObject = graphicElement->_mapObject;
		_context->_symbols.push_back(std::move(rasterSymbolGroup));
	}
	return true;
}


void MapRasterizerProvider::collectSymbolsFromPrimitives(std::shared_ptr<MapRasterizerContext>& _context, 
	std::vector< std::shared_ptr<MapRasterizer::GraphicElement> >& elements, const MapRasterizer::GraphElementType type,
    std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	for (auto& graphicElement : elements)
	{
		switch(type)
		{
		case MapRasterizer::GraphElementType::Polygon:
			uploadSymbolsForPolygon(_context, graphicElement, outSymbols);
			break;
		case MapRasterizer::GraphElementType::Polyline:
			uploadSymbolsForPolyline(_context, graphicElement, outSymbols);
			break;
		case MapRasterizer::GraphElementType::Point:
			uploadSymbolsForPoint(_context, graphicElement, outSymbols);
			break;

		default:
			assert(false);
		}

	}
}


void MapRasterizerProvider::uploadSymbolsForPolygon(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	bgm::ring<pointI> ringCheck(graphicElement->_mapData->points.begin(), graphicElement->_mapData->points.end());
	pointI ptCenter;
	bg::centroid(ringCheck, ptCenter);

	uploadSymbolTextForElement(_context, graphicElement, Tools::normalizeCoordinates(ptCenter, ZoomLevel31), outSymbols);
}

void MapRasterizerProvider::uploadSymbolsForPolyline(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	pointI ptLoc = graphicElement->_mapData->points[graphicElement->_mapData->points.size() >> 1];
	uploadSymbolTextForElement(_context, graphicElement, ptLoc, outSymbols);
}

void MapRasterizerProvider::uploadSymbolsForPoint(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	pointI ptLoc;
	if (graphicElement->_mapData->points.size() == 1)
	{
		ptLoc = graphicElement->_mapData->points[0];
	}
	else
	{
		bgm::ring<pointI> ringCheck(graphicElement->_mapData->points.begin(), graphicElement->_mapData->points.end());
		bg::centroid(ringCheck, ptLoc);
		ptLoc = Tools::normalizeCoordinates(ptLoc, ZoomLevel31);
	}

	uploadSymbolIconForElement(_context, graphicElement, ptLoc, outSymbols);

	uploadSymbolTextForElement(_context, graphicElement, ptLoc, outSymbols);
}

void MapRasterizerProvider::uploadSymbolTextForElement(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement,const pointI& ptSymbolLoc, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	const auto typeRuleId = graphicElement->_mapData->typeIds[graphicElement->_typeIdIndex];
	const auto& section =  graphicElement->_mapData->section.lock();
    const auto& decodedType = section->rules->mapRules[typeRuleId];

	MapStyleResult textEvalResult;

    MapStyleEval textEvaluator(workingStyle, 0.1f);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> coder;

    bool ok;
	for(auto itName = graphicElement->_mapData->nameTypeString.cbegin(); itName != graphicElement->_mapData->nameTypeString.cend(); ++itName)
    {
        const auto& name = std::get<2>(*itName);

        // Skip empty names
        if(name.empty())
            continue;

        // Evaluate style to obtain text parameters
        textEvaluator.setStringValue(getDefaultStyles()->id_INPUT_TAG, decodedType.tag);
        textEvaluator.setStringValue(getDefaultStyles()->id_INPUT_VALUE, decodedType.value);
		textEvaluator.setIntValue(getDefaultStyles()->id_INPUT_MINZOOM, _context->zoom);
        textEvaluator.setIntValue(getDefaultStyles()->id_INPUT_MAXZOOM, _context->zoom);
        textEvaluator.setIntValue(getDefaultStyles()->id_INPUT_TEXT_LENGTH, name.length());

        std::string nameTag;
        if(std::get<0>(*itName) != section->rules->name_encodingRuleId)
			nameTag = section->rules->mapRules[std::get<0>(*itName)].tag;

        textEvaluator.setStringValue(getDefaultStyles()->id_INPUT_NAME_TAG, nameTag);

        textEvalResult.clear();
		if(!textEvaluator.evaluate(graphicElement->_mapData, rulesetType::text, &textEvalResult))
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
		ok = textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_SIZE, textSize);
        if(!ok || textSize == 0)
            continue;

        // Create primitive
		const auto text = new MapRasterizer::RasterSymbolonPath();
        text->graph = graphicElement;
		text->location = ptSymbolLoc;
        text->value = coder.from_bytes(name);

        text->drawOnPath = false;
		textEvalResult.getBoolVal(getDefaultStyles()->id_OUTPUT_TEXT_ON_PATH, text->drawOnPath);

		textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_ORDER, text->zOrder);

        text->verticalOffset = 0;
        textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_DY, text->verticalOffset);

        ok = textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_COLOR, text->color);
        if(!ok || !text->color)
            text->color = SK_ColorBLACK;

        text->size = textSize;

        text->shadowRadius = 0;
        textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);

		ok = textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_HALO_COLOR, text->shadowColor);
        if(!ok || !text->shadowColor)
            text->shadowColor = SK_ColorWHITE;

        text->wrapWidth = 0;
        textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_WRAP_WIDTH, text->wrapWidth);

        text->isBold = false;
        textEvalResult.getBoolVal(getDefaultStyles()->id_OUTPUT_TEXT_BOLD, text->isBold);

        text->minDistance = 0;
        textEvalResult.getIntVal(getDefaultStyles()->id_OUTPUT_TEXT_MIN_DISTANCE, text->minDistance);

        textEvalResult.getStringVal(getDefaultStyles()->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);

        outSymbols.push_back(std::move(std::shared_ptr<MapRasterizer::RasterSymbol>(text)));
    }
}

void MapRasterizerProvider::uploadSymbolIconForElement(std::shared_ptr<MapRasterizerContext>& _context, std::shared_ptr<MapRasterizer::GraphicElement>& graphicElement,const pointI& ptSymbolLoc, std::vector< std::shared_ptr<MapRasterizer::RasterSymbol> >& outSymbols)
{
	if(!graphicElement->styleResult)
        return;

    bool ok;

    std::string iconResourceName;
    ok = graphicElement->styleResult->getStringVal(getDefaultStyles()->id_OUTPUT_ICON, iconResourceName);

    if(ok && !iconResourceName.empty())
    {
		const auto icon = new MapRasterizer::RasterSymbolPin();
        icon->graph = graphicElement;
		icon->location = ptSymbolLoc;

        icon->resourceName = std::move(iconResourceName);

        icon->zOrder = 100;
        graphicElement->styleResult->getIntVal(getDefaultStyles()->id_OUTPUT_ICON, icon->zOrder);
        //NOTE: a magic shifting of icon order. This is needed to keep icons less important than anything else
        icon->zOrder += 100000;

        outSymbols.push_back(std::move(std::shared_ptr<MapRasterizer::RasterSymbol>(icon)));
    }
}
