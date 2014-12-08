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

#include "Tools.h"

#include "Sensorsapi.h"

namespace bf = boost::filesystem;

MapRasterizerProvider::MapRasterizerProvider(void) :
	defaultBgColor(_defaultBgColor), shadowLevelMin(_shadowLevelMin), shadowLevelMax(_shadowLevelMax),
	polygonMinSizeToDisplay(_polygonMinSizeToDisplay), roadDensityZoomTile(_roadDensityZoomTile),
	roadsDensityLimitPerTile(_roadsDensityLimitPerTile), shadowRenderingMode(_shadowRenderingMode), shadowRenderingColor(_shadowRenderingColor),
	mapPaint(_mapPaint), textPaint(_textPaint), oneWayPaints(_oneWayPaints), reverseOneWayPaints(_reverseOneWayPaints)
{
	workingStyle.reset(new MapStyleInfo());
	workingStyle->loadRenderStyles(nullptr);

	_orderEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	applyStyle(_orderEval);

	_pointEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_lineEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_polyEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_textEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));

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

	
    workingStyle->resolveAttribute("defaultColor", _attributeRule_defaultColor);
    workingStyle->resolveAttribute("shadowRendering", _attributeRule_shadowRendering);
    workingStyle->resolveAttribute("polygonMinSizeToDisplay", _attributeRule_polygonMinSizeToDisplay);
    workingStyle->resolveAttribute("roadDensityZoomTile", _attributeRule_roadDensityZoomTile);
    workingStyle->resolveAttribute("roadsDensityLimitPerTile", _attributeRule_roadsDensityLimitPerTile);

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

	_orderEval.reset(new MapStyleEval(workingStyle, 1.0f));
	applyStyle(_orderEval);

	_pointEval.reset(new MapStyleEval(workingStyle, 1.0f));
	_lineEval.reset(new MapStyleEval(workingStyle, 1.0f));
	_polyEval.reset(new MapStyleEval(workingStyle, 1.0f));
	_textEval.reset(new MapStyleEval(workingStyle, 1.0f));


	_orderEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_orderEval->setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	_pointEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_pointEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	_lineEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_lineEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	_polyEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_polyEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	
	for (std::shared_ptr<const MapObjectData> cobjectMapData: mapData)
	{
		std::shared_ptr<MapObjectData> objectMapData(std::const_pointer_cast<MapObjectData>(cobjectMapData));

		std::shared_ptr<MapRasterizer::GraphicElementGroup> graphGroup(new MapRasterizer::GraphicElementGroup());
		int typeRuleIDIndex = 0;
		for(int typeRuleId : objectMapData->type)
		{
			auto& mapDecoder = objectMapData->section->rules->getRuleInfo(typeRuleId);
			
			_orderEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
			_orderEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
			_orderEval->setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
			_orderEval->setBoolValue(builtinDef->id_INPUT_AREA, objectMapData->isArea);
			_orderEval->setBoolValue(builtinDef->id_INPUT_CYCLE, objectMapData->isClosedFigure());
			_orderEval->setBoolValue(builtinDef->id_INPUT_POINT, objectMapData->points.size() == 1);

			MapStyleResult orderResults;
			bool bOK = _orderEval->evaluate(objectMapData, rulesetType::order, &orderResults);
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

				_polyEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_polyEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);

				std::shared_ptr<MapStyleResult> polyResults(new MapStyleResult());
				bOK = _polyEval->evaluate(objectMapData, rulesetType::polygon, polyResults.get());
				if (!bOK)
					continue;
				
				graphicElement->styleResult = polyResults;

				auto areaData = Tools::polygonArea(objectMapData->points);
				if (areaData < /*MapRasterizer::PolygonAreaCutoffLowerThreshold*/ 75)
				{
					// polygon too small to show as polygon in low zoom so convert to point in low zooms
					graphicElement->zOrder += 1.0 / areaData;
					std::shared_ptr<MapRasterizer::GraphicElement> pointElement(new MapRasterizer::GraphicElement(graphGroup, objectMapData, MapRasterizer::GraphElementType::Point, typeRuleIDIndex));
					pointElement->zOrder = graphicElement->zOrder;
					graphGroup->_polygons.push_back(graphicElement);

					_pointEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
					_pointEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
					std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
					bOK = _pointEval->evaluate(objectMapData, rulesetType::point, pointResults.get());
					if (bOK)
					{
						// indicates it has icon
						pointElement->styleResult = pointResults;
					}
					if (typeRuleIDIndex == 0 || objectMapData->nameTypeString.empty())
					{
						graphGroup->_points.push_back(pointElement);
					}
				}

			}
			else if (graphicElement->_type == MapRasterizer::GraphElementType::Polyline)
			{
				if (objectMapData->points.size() < 2)
				{
					continue;
				}
				_lineEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_lineEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				_lineEval->setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
				std::shared_ptr<MapStyleResult> lineResults(new MapStyleResult());
				bOK = _lineEval->evaluate(objectMapData, rulesetType::line, lineResults.get());
				if (!bOK)
					continue;

				graphicElement->styleResult = lineResults;

				graphGroup->_polyLines.push_back(graphicElement);
			}
			else if(graphicElement->_type == MapRasterizer::GraphElementType::Point)
			{
				if (objectMapData->points.size() < 1)
				{
					// no point defined
					continue;
				}
				_pointEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_pointEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
				bOK = _pointEval->evaluate(objectMapData, rulesetType::point, pointResults.get());
				if (bOK)
				{
					graphicElement->styleResult = pointResults;
				}
				if (typeRuleIDIndex != 0 || (objectMapData->nameTypeString.size() == 0 && !bOK))
				{
					continue;
				}
				graphGroup->_points.push_back(graphicElement);
			}
			else
			{
				// !!
				assert(false);
				continue;
			}
			typeRuleIDIndex++;
		}
		graphGroup->_mapObject = objectMapData;
		_context->_graphicElements.push_back(graphGroup);
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

	return std::vector<uint8_t>();
    // Otherwise obtain from embedded
    //return EmbeddedResources::decompressResource(name);
}
