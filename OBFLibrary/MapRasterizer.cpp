#include "stdafx.h"

#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#include "SkDashPathEffect.h"
#include "SkBlurDrawLooper.h"
#include "SkBitmapProcShader.h"
#include "SkImageDecoder.h"
#include "SkColorFilter.h"
#include "SkBitmapDevice.h"

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

#include <locale>
#include <codecvt>

MapRasterizer::RasterSymbol::RasterSymbol()
{
}

MapRasterizer::RasterSymbol::~RasterSymbol()
{
}

MapRasterizer::RasterSymbolonPath::RasterSymbolonPath()
{
}

MapRasterizer::RasterSymbolonPath::~RasterSymbolonPath()
{
}

MapRasterizer::RasterSymbolPin::RasterSymbolPin()
{
}
MapRasterizer::RasterSymbolPin::~RasterSymbolPin()
{
}



MapRasterizer::MapRasterizer(MapRasterizerProvider& dataSrc) : _source(dataSrc)
{
}



MapRasterizer::~MapRasterizer(void)
{
}

void MapRasterizer::createContextData(boxI& workArea, int workZoom )
{
	
	auto& mapDatum = _source.obtainMapData(workArea, workZoom);
	_context = std::shared_ptr<MapRasterizerContext>(new MapRasterizerContext());
	_context->initialize(_source, workZoom);
	_context->_tileScale = MapUtils::getPowZoom(31 - workZoom);
	_context->zoom = workZoom;
	_context->_area31 = workArea;
	std::list< std::shared_ptr<const MapObjectData> > detailedmapMapObjects, detailedmapCoastlineObjects;
    std::list< std::shared_ptr<const MapObjectData> > polygonizedCoastlineObjects;
	for (const std::shared_ptr<const MapObjectData>& mapItem : mapDatum)
	{
		if (mapItem->section.expired())
			continue;
		auto sectionData = mapItem->section.lock();
		if (mapItem->containsType(sectionData->rules->naturalCoastline_encodingRuleId))
		{
			detailedmapCoastlineObjects.push_back(mapItem);
		}
		else
		{
			detailedmapMapObjects.push_back(mapItem);
		}
	}
	bool fillEntireArea = true;
	if(!detailedmapCoastlineObjects.empty())
    {
        const bool coastlinesWereAdded = _context->polygonizeCoastlines(_source,  detailedmapCoastlineObjects,
            polygonizedCoastlineObjects, false, true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

	if(fillEntireArea)
    {
        //assert(foundation != MapFoundationType::Undefined);
		
        const std::shared_ptr<MapObjectData> bgMapObject(new MapObjectData(_source.dummySectionData));
        bgMapObject->isArea = true;
        bgMapObject->points.push_back(std::move(pointI(workArea.min_corner().get<0>(), workArea.min_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.max_corner().get<0>(), workArea.min_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.max_corner().get<0>(), workArea.max_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.min_corner().get<0>(), workArea.max_corner().get<1>())));
        bgMapObject->points.push_back(bgMapObject->points.front());
       /* if(foundation == MapFoundationType::FullWater)
            bgMapObject->_typesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        else if(foundation == MapFoundationType::FullLand || foundation == MapFoundationType::Mixed)
            bgMapObject->type.push_back(bgMapObject->section->rules->naturalLand_encodingRuleId);
        else*/
        {
            bgMapObject->isArea = false;
            bgMapObject->typeIds.push_back(_source.dummySectionData->rules->naturalCoastlineBroken_encodingRuleId);
#ifdef _DEBUG
			auto tpData = _source.dummySectionData->rules->getRuleInfo(_source.dummySectionData->rules->naturalCoastlineBroken_encodingRuleId);
			bgMapObject->typeNames.push_back(tpData.tag+", "+tpData.value);
#endif
        }
        bgMapObject->addtypeIds.push_back(_source.dummySectionData->rules->layerLowest_encodingRuleId);
#ifdef _DEBUG
		auto tagVal = _source.dummySectionData->rules->getRuleInfo(_source.dummySectionData->rules->layerLowest_encodingRuleId);
		bgMapObject->addtypeNames.push_back(tagVal.tag+", "+tagVal.value);
#endif
		assert(bgMapObject->isClosedFigure());
        polygonizedCoastlineObjects.push_back(std::move(bgMapObject));
    }

	
	_source.obtainMapPrimitives(mapDatum, workZoom, _context);
	_source.obtainMapPrimitives(polygonizedCoastlineObjects, workZoom, _context);
	_context->removeHighwaysBasedOnDensity(_source);
	_context->sortGraphicElements();

	_source.obtainPrimitivesSymbols(_context);

}

void MapRasterizer::DrawMap(std::string pathFile)
{
	if (pathFile.size() == 0)
	{
		return;
	}

	SkImage::Info info = {
		512, 512, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
		};
	SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
	SkCanvas* painter = imageRender->getCanvas();
	bool painted = DrawMap(*painter);
	if (painted)
	{
		SkAutoTUnref<SkImage> imageSrc(imageRender->newImageSnapshot());
		SkAutoDataUnref data(imageSrc->encode());
		SkFILEWStream stream(pathFile.c_str());
		stream.write(data->data(), data->size());
	}
}

bool MapRasterizer::DrawSymbols(SkCanvas& canvas)
{
	if (!_context)
		return false;

	bool drawn = false;
	bool painted = false;

	auto sizer = canvas.getDeviceSize();
	
	AreaI _destinationArea;
	_destinationArea.min_corner().set<0>(0);
	_destinationArea.min_corner().set<1>(0);
	_destinationArea.max_corner().set<1>(sizer.height());
	_destinationArea.max_corner().set<0>(sizer.width());

	
	std::vector<const std::shared_ptr<const RenderSymbolGroup>> symbols;

	rasterizeSymbols(symbols);

	std::vector<RenderSymbolGroup> symbolsOnSurface;

	uint64_t totalArea = 0;

	for (auto symbolInfo : symbols)
	{
		for (auto SymbolData : symbolInfo->_symbols)
		{
			uint32_t sizePix =  SymbolData->bitmap->getSize();
			totalArea += Tools::getNextPowerOfTwo(sizePix);
		}
	}

	if (totalArea > 1024*1024)
	{
		return false;
	}

	return true;

}

bool MapRasterizer::DrawMap(SkCanvas& canvas)
{
	if (!_context)
		return false;

	bool drawn = false;
	bool painted = false;
	if(/*fillBackground*/ false)
    {
        if(/*destinationArea*/ false)
        {
            // If destination area is specified, fill only it with background
            SkPaint bgPaint;
            bgPaint.setColor(_source.defaultBgColor);
            bgPaint.setStyle(SkPaint::kFill_Style);
            //canvas.drawRectCoords(destinationArea->top, destinationArea->min_corner()->get<0>(), destinationArea->right, destinationArea->bottom, bgPaint);
        }
        else
        {
            // Since destination area is not specified, erase whole canvas with specified color
            canvas.clear(_source.defaultBgColor);
        }
    }

	//const SkPaint& mapPaint = _source.mapPaint;
	
	auto sizer = canvas.getDeviceSize();
	AreaI _destinationArea;
	_destinationArea.min_corner().set<0>(0);
	_destinationArea.min_corner().set<1>(0);
	_destinationArea.max_corner().set<1>(sizer.height());
	_destinationArea.max_corner().set<0>(sizer.width());
	_context->_pixelScaleXY.set<0>(_context->_tileScale / static_cast<double>(sizer.width()));
	_context->_pixelScaleXY.set<1>(_context->_tileScale / static_cast<double>(sizer.height()));

	_mapPaint = _source.mapPaint;

	drawn = rasterizeMapElements(&_destinationArea, canvas, _context->_polygons, GraphElementsType::Polygons);
	if (!painted && drawn)
		painted = true;
	drawn = rasterizeMapElements(&_destinationArea, canvas, _context->_polyLines, GraphElementsType::Polylines);
	if (!painted && drawn)
		painted = true;

	return painted;
}

bool MapRasterizer::rasterizeMapElements(const AreaI* const destinationArea,SkCanvas& canvas, const std::vector< std::shared_ptr< GraphicElement> >& primitives, GraphElementsType type)
{
	bool drawn = false;
	bool painted = false;
	const auto polygonMinSizeToDisplay31 = _source.polygonMinSizeToDisplay * (_context->_pixelScaleXY.get<0>() * _context->_pixelScaleXY.get<1>());
    const auto polygonSizeThreshold = 1.0 / polygonMinSizeToDisplay31;

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if(primitive->zOrder > polygonSizeThreshold + static_cast<int>(primitive->zOrder))
                continue;

            drawn = rasterizePolygon(destinationArea, canvas, primitive);
			if (!painted && drawn)
				painted = true;
        }
        else if(type == Polylines || type == Polylines_ShadowOnly)
        {
            drawn = rasterizePolyline(destinationArea, canvas, primitive, type == Polylines_ShadowOnly);
			if (!painted && drawn)
				painted = true;

        }
    }

	return painted;
}


bool MapRasterizer::rasterizeSymbols(std::vector<const std::shared_ptr<const RenderSymbolGroup>>& renderedSymbols)
{
	for(auto itSymbolsEntry = _context->_symbols.cbegin(); itSymbolsEntry != _context->_symbols.cend(); ++itSymbolsEntry)
    {
        // Create group
		const auto constructedGroup = new RenderSymbolGroup((*itSymbolsEntry)->_mapObject);
        std::shared_ptr<const RenderSymbolGroup> group(constructedGroup);
		auto symbolVal = *itSymbolsEntry;
        // Total offset allows several texts to stack into column
        pointI totalOffset;
		totalOffset.set<0>(0);
		totalOffset.set<1>(0);

        for(auto itPrimitiveSymbol = symbolVal->_symbols.cbegin(); itPrimitiveSymbol != symbolVal->_symbols.cend(); ++itPrimitiveSymbol)
        {
            

            const auto& symbol = *itPrimitiveSymbol;

			if(const auto textSymbol = std::dynamic_pointer_cast<const RasterSymbolonPath>(symbol))
            {
                //TODO: reshape name with icu4c, since skia doesn't know how to do that
                const std::string text = textSymbol->value;
				std::wstring_convert<std::codecvt_utf8<wchar_t>> coder;
				std::wstring cvt = coder.from_bytes(text);

                // Obtain shield for text if such exists
                std::shared_ptr<const SkBitmap> textShieldBitmap;
                if(!textSymbol->shieldResourceName.empty())
                    _source.obtainTextShield(textSymbol->shieldResourceName, textShieldBitmap);

                // Configure paint for text
                SkPaint textPaint = _source.textPaint;
                textPaint.setTextSize(textSymbol->size);
                textPaint.setFakeBoldText(textSymbol->isBold);
                textPaint.setColor(textSymbol->color);

                // Measure text
                SkRect textBounds;
                auto totalWidth = textPaint.measureText(cvt.data(), cvt.length()*sizeof(wchar_t), &textBounds);
                SkRect textBBox = textBounds;
                std::vector<float> glyphsWidth;
				std::vector<float> rotations;
                if(textSymbol->drawOnPath)
                {
                    int glyphsCount = textPaint.countText(cvt.data(), cvt.length()*sizeof(wchar_t));
                    glyphsWidth.resize(glyphsCount);
					
                    textPaint.getTextWidths(cvt.data(), cvt.length()*sizeof(wchar_t), glyphsWidth.data());
					auto graphElem = symbol->graph;
					if (graphElem->_type == GraphElementType::Polyline)
					{
						const auto pointVec = graphElem->_mapData->points;
						float px = 0;
						float py = 0;
						for (int i = 1; i < pointVec.size(); i++) {
							px +=  pointVec[i].get<0>() - pointVec[i - 1].get<0>();
							py +=  pointVec[i].get<1>() - pointVec[i - 1].get<1>();
						}
						float rotation = 0.0;
						if (px != 0 || py != 0) {
							rotation = (float) (-std::atan2(px, py) + boost::math::constants::pi<float>() / 2);
							rotations.insert(rotations.end(), glyphsCount, rotation);
						}
					}
                }

                // Process shadow (if such is enabled)
                SkPaint textShadowPaint;
                SkRect shadowBounds;
                if(textSymbol->shadowRadius > 0)
                {
                    textShadowPaint = textPaint;
                    textShadowPaint.setStyle(SkPaint::kStroke_Style);
                    textShadowPaint.setColor(textSymbol->shadowColor);
                    textShadowPaint.setStrokeWidth(textSymbol->shadowRadius);

                    totalWidth = textShadowPaint.measureText(cvt.data(), cvt.length()*sizeof(wchar_t), &shadowBounds);
                    textBBox.join(shadowBounds);

                    if(textSymbol->drawOnPath)
                        textShadowPaint.getTextWidths(cvt.data(), cvt.length()*sizeof(wchar_t), glyphsWidth.data());
                }

                // Calculate bitmap size and text area
                auto textArea = textBBox;
                textArea.offset(-2.0f*textBBox.left(), -2.0f*textBBox.top());
                if(!glyphsWidth.empty())
                    glyphsWidth[0] += -textBBox.left();
                auto bitmapWidth = textArea.width();
                auto bitmapHeight = textArea.height();
                if(textShieldBitmap)
                {
                    // Enlarge bitmap if shield is larger than text
					bitmapWidth = max(bitmapWidth, static_cast<float>(textShieldBitmap->width()));
                    bitmapHeight = max(bitmapHeight, static_cast<float>(textShieldBitmap->height()));
                    // Shift text area to proper position in a larger
                    textArea.offset(
                        (bitmapWidth - textArea.width()) / 2.0f,
                        (bitmapHeight - textArea.height()) / 2.0f);
                }

                // Create a bitmap that will be hold text
                auto bitmap = new SkBitmap();
                bitmap->setConfig(SkBitmap::kARGB_8888_Config, bitmapWidth, bitmapHeight);
                bitmap->allocPixels();
                bitmap->eraseColor(SK_ColorTRANSPARENT);
                SkBitmapDevice target(*bitmap);
                SkCanvas canvas(&target);

                // If there is shield for this text, rasterize it also
                if(textShieldBitmap)
                {
                    canvas.drawBitmap(*textShieldBitmap,
                        (bitmapWidth - textShieldBitmap->width()) / 2.0f,
                        (bitmapHeight - textShieldBitmap->height()) / 2.0f,
                        nullptr);
                }

                // Rasterize text
                if(textSymbol->shadowRadius > 0)
                    canvas.drawText(cvt.data(), cvt.length()*sizeof(wchar_t), textArea.left(), textArea.top(), textShadowPaint);
                canvas.drawText(cvt.data(), cvt.length()*sizeof(wchar_t), textArea.left(), textArea.top(), textPaint);

                //////////////////////////////////////////////////////////////////////////
                //std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                //std::string path;
                //path.sprintf("D:\\texts\\%p.png", bitmap);
                //encoder->encodeFile(path.toLocal8Bit(), *bitmap, 100);
                //////////////////////////////////////////////////////////////////////////

                if(textSymbol->drawOnPath)
                {
                    // Publish new rasterized symbol
					std::vector<std::pair<float, float>> glyphMods;
					glyphMods.resize(glyphsWidth.size());
					if (glyphsWidth.size() == rotations.size())
					{
					for(int glyphIdx = 0; glyphIdx < glyphsWidth.size(); glyphIdx++)
					{
						glyphMods[glyphIdx] = std::make_pair(glyphsWidth[glyphIdx], rotations[glyphIdx]);
					}
					}
					else
					{
						for(int glyphIdx = 0; glyphIdx < glyphsWidth.size(); glyphIdx++)
						{
							glyphMods[glyphIdx] = std::make_pair(glyphsWidth[glyphIdx], 0.0f);
						}
					}
					
					const auto rasterizedSymbol = new RenderSymbol_Path(
                        group,
                        constructedGroup->_mapObject,
                        std::move(std::shared_ptr<const SkBitmap>(bitmap)),
                        symbol->zOrder,
                        glyphMods);
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
					constructedGroup->_symbols.push_back(std::move(std::shared_ptr<RenderSymbol>(rasterizedSymbol)));
                }
                else
                {
                    // Calculate local offset
                    pointI localOffset;
					localOffset.set<0>(0);
                    localOffset.set<1>((bitmap->height() / 2) + textSymbol->verticalOffset);

                    // Increment total offset
					bg::add_point(totalOffset,localOffset);

                    // Publish new rasterized symbol
                    const auto rasterizedSymbol = new RenderSymbol_Pin(
                        group,
                        constructedGroup->_mapObject,
                        std::move(std::shared_ptr<const SkBitmap>(bitmap)),
                        symbol->zOrder,
						symbol->location,
                        (constructedGroup->_symbols.empty() ? pointI() : totalOffset));
                    assert(static_cast<bool>(rasterizedSymbol->bitmap));
                    constructedGroup->_symbols.push_back(std::move(std::shared_ptr<RenderSymbol>(rasterizedSymbol)));
                }
            }
			else if(const auto iconSymbol = std::dynamic_pointer_cast<const RasterSymbolPin>(symbol))
            {
                std::shared_ptr<const SkBitmap> bitmap;
                if(!_source.obtainMapIcon(iconSymbol->resourceName, bitmap) || !bitmap)
                    continue;

                //////////////////////////////////////////////////////////////////////////
                //std::unique_ptr<SkImageEncoder> encoder(CreatePNGImageEncoder());
                //std::string path;
                //path.sprintf("D:\\icons\\%p.png", bitmap);
                //encoder->encodeFile(path.toLocal8Bit(), *bitmap, 100);
                //////////////////////////////////////////////////////////////////////////

                // Calculate local offset
                pointI localOffset;
				localOffset.set<0>(0);
                localOffset.set<1>((bitmap->height() / 2));

                // Increment total offset
                bg::add_point(totalOffset,localOffset);

                // Publish new rasterized symbol
                const auto rasterizedSymbol = new RenderSymbol_Pin(
                    group,
                    constructedGroup->_mapObject,
                    std::move(bitmap),
                    symbol->zOrder,
                    symbol->location,
                    (constructedGroup->_symbols.empty() ? pointI() : totalOffset));
                assert(static_cast<bool>(rasterizedSymbol->bitmap));
                constructedGroup->_symbols.push_back(std::move(std::shared_ptr<RenderSymbol>(rasterizedSymbol)));
            }
        }

        // Add group to output
        renderedSymbols.push_back(std::move(group));
    }

	return true;
}

bool MapRasterizer::updatePaint(const MapStyleResult& evalResult, const PaintValuesSet valueSetSelector, const bool isArea )
{
    bool ok = true;

	auto builtinStyleDefs =  _source.getDefaultStyles();
    int valueDefId_color = -1;
    int valueDefId_strokeWidth = -1;
    int valueDefId_cap = -1;
    int valueDefId_pathEffect = -1;
    switch(valueSetSelector)
    {
    case PaintValuesSet::Set_0:
        valueDefId_color =builtinStyleDefs->id_OUTPUT_COLOR;
        valueDefId_strokeWidth = builtinStyleDefs->id_OUTPUT_STROKE_WIDTH;
        valueDefId_cap = builtinStyleDefs->id_OUTPUT_CAP;
        valueDefId_pathEffect = builtinStyleDefs->id_OUTPUT_PATH_EFFECT;
        break;
    case PaintValuesSet::Set_1:
        valueDefId_color = builtinStyleDefs->id_OUTPUT_COLOR_2;
        valueDefId_strokeWidth = builtinStyleDefs->id_OUTPUT_STROKE_WIDTH_2;
        valueDefId_cap = builtinStyleDefs->id_OUTPUT_CAP_2;
        valueDefId_pathEffect = builtinStyleDefs->id_OUTPUT_PATH_EFFECT_2;
        break;
    case PaintValuesSet::Set_minus1:
        valueDefId_color = builtinStyleDefs->id_OUTPUT_COLOR_0;
        valueDefId_strokeWidth = builtinStyleDefs->id_OUTPUT_STROKE_WIDTH_0;
        valueDefId_cap = builtinStyleDefs->id_OUTPUT_CAP_0;
        valueDefId_pathEffect = builtinStyleDefs->id_OUTPUT_PATH_EFFECT_0;
        break;
    case PaintValuesSet::Set_minus2:
        valueDefId_color = builtinStyleDefs->id_OUTPUT_COLOR__1;
        valueDefId_strokeWidth = builtinStyleDefs->id_OUTPUT_STROKE_WIDTH__1;
        valueDefId_cap = builtinStyleDefs->id_OUTPUT_CAP__1;
        valueDefId_pathEffect = builtinStyleDefs->id_OUTPUT_PATH_EFFECT__1;
        break;
    case PaintValuesSet::Set_3:
        valueDefId_color = builtinStyleDefs->id_OUTPUT_COLOR_3;
        valueDefId_strokeWidth = builtinStyleDefs->id_OUTPUT_STROKE_WIDTH_3;
        valueDefId_cap = builtinStyleDefs->id_OUTPUT_CAP_3;
        valueDefId_pathEffect = builtinStyleDefs->id_OUTPUT_PATH_EFFECT_3;
        break;
    }

    if(isArea)
    {
        _mapPaint.setColorFilter(nullptr);
        _mapPaint.setShader(nullptr);
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStyle(SkPaint::kStrokeAndFill_Style);
        _mapPaint.setStrokeWidth(0);
    }
    else
    {
        float stroke;
		ok = evalResult.getFloatVal(valueDefId_strokeWidth, stroke);
        if(!ok || stroke <= 0.0f)
            return false;

        _mapPaint.setColorFilter(nullptr);
        _mapPaint.setShader(nullptr);
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStyle(SkPaint::kStroke_Style);
        _mapPaint.setStrokeWidth(stroke);

        std::string cap;
        ok = evalResult.getStringVal(valueDefId_cap, cap);
        if (!ok || cap.empty() || cap == "BUTT")
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);
        else if (cap == "ROUND")
            _mapPaint.setStrokeCap(SkPaint::kRound_Cap);
        else if (cap == "SQUARE")
            _mapPaint.setStrokeCap(SkPaint::kSquare_Cap);
        else
            _mapPaint.setStrokeCap(SkPaint::kButt_Cap);

        std::string encodedPathEffect;
        ok = evalResult.getStringVal(valueDefId_pathEffect, encodedPathEffect);
        if(!ok || encodedPathEffect.empty())
        {
            _mapPaint.setPathEffect(nullptr);
        }
        else
        {
            SkPathEffect* pathEffect = nullptr;
			ok = _source.obtainPathEffect(encodedPathEffect, pathEffect);

            if(ok && pathEffect)
                _mapPaint.setPathEffect(pathEffect);
        }
    }

    SkColor color;
    ok = evalResult.getIntVal(valueDefId_color, color);
    if(!ok || !color)
        return false;
    _mapPaint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        std::string shader;
        ok = evalResult.getStringVal(builtinStyleDefs->id_OUTPUT_SHADER, shader);
        if(ok && !shader.empty())
        {
            SkBitmapProcShader* shaderObj = nullptr;
            if(_source.obtainBitmapShader(shader, shaderObj) && shaderObj)
            {
                _mapPaint.setShader(static_cast<SkShader*>(shaderObj));
                shaderObj->unref();
            }
        }
    }

    // do not check shadow color here
    if (_source.shadowRenderingMode == 1 && valueSetSelector == PaintValuesSet::Set_0)
    {
        int shadowColor;
        ok = evalResult.getIntVal(builtinStyleDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
        int shadowRadius;
        evalResult.getIntVal(builtinStyleDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
        if(!ok || shadowColor == 0)
            shadowColor = _source.shadowRenderingColor;
        if(shadowColor == 0)
            shadowRadius = 0;

        if(shadowRadius > 0)
            _mapPaint.setLooper(new SkBlurDrawLooper(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}


bool MapRasterizer::rasterizePolygon(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive)
{
    /*assert(primitive->_mapData->points.size() > 2);
    assert(primitive->_mapData->isClosedFigure());
    assert(primitive->_mapData->isClosedFigure(true));
*/
    if(!updatePaint(*primitive->styleResult, PaintValuesSet::Set_0, true))
        return false;

    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    pointF vertex;
    int bounds = 0;
    std::vector< pointF > outsideBounds;
    const auto pointsCount = primitive->_mapData->points.size();
    auto pPoint = primitive->_mapData->points.data();
    for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.get<0>(), vertex.get<1>());
        }
        else
        {
            path.lineTo(vertex.get<0>(), vertex.get<1>());
        }

        if(destinationArea && !containsAtLeastOnePoint)
        {
            if(bg::covered_by(vertex, destinationArea))
            {
                containsAtLeastOnePoint = true;
            }
            else
            {
                outsideBounds.push_back(std::move(vertex));
            }
			bounds |= (vertex.get<0>() < destinationArea->min_corner().get<0>() ? 1 : 0);
            bounds |= (vertex.get<0>() > destinationArea->max_corner().get<0>() ? 2 : 0);
            bounds |= (vertex.get<1>() < destinationArea->min_corner().get<1>() ? 4 : 0);
            bounds |= (vertex.get<1>() > destinationArea->max_corner().get<1>() ? 8 : 0);
        }

    }

    if(destinationArea && !containsAtLeastOnePoint)
    {
        // fast check for polygons
        if((bounds & 3) != 3 || (bounds >> 2) != 3)
            return false;

        bool ok = true;
        ok = ok || contains(outsideBounds, pointF(destinationArea->min_corner().get<0>(),destinationArea->min_corner().get<1>()));
        ok = ok || contains(outsideBounds, pointF(destinationArea->max_corner().get<0>(),destinationArea->max_corner().get<1>()));
        ok = ok || contains(outsideBounds, pointF(0, destinationArea->max_corner().get<1>()));
        ok = ok || contains(outsideBounds, pointF(destinationArea->max_corner().get<0>(), 0));
        if(!ok)
            return false;
    }

	if(!primitive->_mapData->innerpolypoints.empty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itPolygon = primitive->_mapData->innerpolypoints.cbegin(); itPolygon != primitive->_mapData->innerpolypoints.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            pointIdx = 0;
            for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(point, vertex);

                if(pointIdx == 0)
                {
                    path.moveTo(vertex.get<0>(), vertex.get<1>());
                }
                else
                {
                    path.lineTo(vertex.get<0>(), vertex.get<1>());
                }
            }
        }
    }

    canvas.drawPath(path, _mapPaint);
    if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_1, false))
        canvas.drawPath(path, _mapPaint);

	return true;
}

bool MapRasterizer::rasterizePolyline(    const AreaI* const destinationArea,    SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive, bool drawOnlyShadow)
{
    /*assert(primitive->_mapData->points.size() >= 2);*/
	auto builtinStyleDefs =  _source.getDefaultStyles();

    if(!updatePaint(*primitive->styleResult, PaintValuesSet::Set_0, false))
        return false;

	if (primitive->_mapData->section.expired())
		return false;

	auto mapsSectionData = primitive->_mapData->section.lock();

    bool ok;

    int shadowColor;
    ok = primitive->styleResult->getIntVal(builtinStyleDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
    if(!ok || shadowColor == 0)
        shadowColor = _source.shadowRenderingColor;

    int shadowRadius;
    ok = primitive->styleResult->getIntVal(builtinStyleDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
    if(drawOnlyShadow && (!ok || shadowRadius == 0))
        return false;

	const auto typeRuleId = primitive->_mapData->typeIds[primitive->_typeIdIndex];

    int oneway = 0;
	if(_context->zoom >= ZoomLevel16 && typeRuleId == mapsSectionData->rules->highway_encodingRuleId)
    {
        if(primitive->_mapData->containsType(mapsSectionData->rules->oneway_encodingRuleId, true))
            oneway = 1;
        else if(primitive->_mapData->containsType(mapsSectionData->rules->onewayReverse_encodingRuleId, true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    bool intersect = false;
    int prevCross = 0;
    pointF vertex, middleVertex;
    const auto pointsCount = primitive->_mapData->points.size();
    const auto middleIdx = pointsCount / 2;
    auto pPoint = primitive->_mapData->points.data();
    for(pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.get<0>(), vertex.get<1>());
        }
        else
        {
            if(pointIdx == middleIdx)
                middleVertex = vertex;

            path.lineTo(vertex.get<0>(), vertex.get<1>());
        }

        if(destinationArea && !intersect)
        {
            if(bg::covered_by(vertex, destinationArea))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (vertex.get<0>() < destinationArea->min_corner().get<0>() ? 1 : 0);
                cross |= (vertex.get<0>() > destinationArea->max_corner().get<0>() ? 2 : 0);
                cross |= (vertex.get<1>() < destinationArea->min_corner().get<1>() ? 4 : 0);
                cross |= (vertex.get<1>() > destinationArea->max_corner().get<0>() ? 8 : 0);
                if(pointIdx > 0)
                {
                    if((prevCross & cross) == 0)
                    {
                        intersect = true;
                    }
                }
                prevCross = cross;
            }
        }
    }

    if (destinationArea && !intersect)
        return false;

    if (pointIdx > 0)
    {
        if (drawOnlyShadow)
        {
            rasterizeLineShadow(canvas, path, shadowColor, shadowRadius);
        }
        else
        {
            if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_minus2, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_minus1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_0, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            canvas.drawPath(path, _mapPaint);
            if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_1, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_3, false))
            {
                canvas.drawPath(path, _mapPaint);
            }
            if (oneway && !drawOnlyShadow)
            {
                rasterizeLine_OneWay(canvas, path, oneway);
            }
        }
    }
	if (primitive->_mapData->localId == 0)
	{
		//this is auto generated lines which does not count
		return false;
	}
	return true;
}

void MapRasterizer::rasterizeLineShadow(
    SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius )
{
    // blurred shadows
    if (_source.shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        _mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, _mapPaint);
    }

    // option shadow = 3 with solid border
    if (_source.shadowRenderingMode == 3 && shadowRadius > 0)
    {
        _mapPaint.setLooper(nullptr);
        _mapPaint.setStrokeWidth(_mapPaint.getStrokeWidth() + shadowRadius*2);
        //		paint->setColor(0xffbababa);
        _mapPaint.setColorFilter(SkColorFilter::CreateModeFilter(shadowColor, SkXfermode::kSrcIn_Mode))->unref();
        //		paint->setColor(shadowColor);
        canvas.drawPath(path, _mapPaint);
    }
}

void MapRasterizer::rasterizeLine_OneWay(
    SkCanvas& canvas, const SkPath& path, int oneway )
{
    if (oneway > 0)
    {
        for(auto itPaint = _source.oneWayPaints.cbegin(); itPaint != _source.oneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
    else
    {
        for(auto itPaint = _source.reverseOneWayPaints.cbegin(); itPaint != _source.reverseOneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
}

void MapRasterizer::calculateVertex( const pointI& point31, pointF& vertex )
{
	auto xVal = static_cast<float>(point31.get<0>());
	auto xArea = static_cast<float>(_context->_area31.min_corner().get<0>());
	auto yArea = static_cast<float>(_context->_area31.min_corner().get<1>());
	auto yVal = static_cast<float>(point31.get<1>());
	vertex.set<0>((xVal - xArea) / _context->_pixelScaleXY.get<0>());
	vertex.set<1>((yVal - yArea)/ _context->_pixelScaleXY.get<1>());
}

bool MapRasterizer::contains( const std::vector< pointF >& vertices, const pointF& other )
{
    uint32_t intersections = 0;

    auto itPrevVertex = vertices.cbegin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != vertices.cend(); itPrevVertex = itVertex, ++itVertex)
    {
        const auto& vertex0 = *itPrevVertex;
        const auto& vertex1 = *itVertex;

        if(Tools::rayIntersect(vertex0, vertex1, other))
            intersections++;
    }

    // special handling, also count first and last, might not be closed, but
    // we want this!
    const auto& vertex0 = vertices.front();
    const auto& vertex1 = vertices.back();
    if(Tools::rayIntersect(vertex0, vertex1, other))
        intersections++;

    return intersections % 2 == 1;
}
