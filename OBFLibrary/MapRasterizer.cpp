#include "stdafx.h"

#include "SkCanvas.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"

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

RasterSymbol::RasterSymbol()
{
}

RasterSymbol::~RasterSymbol()
{
}

RasterSymbolonPath::RasterSymbolonPath()
{
}

RasterSymbolonPath::~RasterSymbolonPath()
{
}

RasterSymbolPin::RasterSymbolPin()
{
}
RasterSymbolPin::~RasterSymbolPin()
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
	std::list< std::shared_ptr<const MapObjectData> > detailedmapMapObjects, detailedmapCoastlineObjects;
    std::list< std::shared_ptr<const MapObjectData> > polygonizedCoastlineObjects;
	for (const std::shared_ptr<const MapObjectData>& mapItem : mapDatum)
	{
		if (mapItem->containsType(mapItem->section->rules->naturalCoastlineLine_encodingRuleId))
		{
			detailedmapCoastlineObjects.push_back(mapItem);
		}
		else
		{
			detailedmapMapObjects.push_back(mapItem);
		}
	}
	bool fillEntireArea;
	if(!detailedmapCoastlineObjects.empty())
    {
        const bool coastlinesWereAdded = _context->polygonizeCoastlines(_source,  detailedmapCoastlineObjects,
            polygonizedCoastlineObjects, false, true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
    }

	if(fillEntireArea)
    {
        //assert(foundation != MapFoundationType::Undefined);
		
        const std::shared_ptr<MapObjectData> bgMapObject(new MapObjectData());
        bgMapObject->isArea = true;
        bgMapObject->points.push_back(std::move(pointI(workArea.max_corner().get<0>(), workArea.max_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.min_corner().get<0>(), workArea.max_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.min_corner().get<0>(), workArea.min_corner().get<1>())));
        bgMapObject->points.push_back(std::move(pointI(workArea.max_corner().get<0>(), workArea.min_corner().get<1>())));
        bgMapObject->points.push_back(bgMapObject->points.front());
       /* if(foundation == MapFoundationType::FullWater)
            bgMapObject->_typesRuleIds.push_back(bgMapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        else if(foundation == MapFoundationType::FullLand || foundation == MapFoundationType::Mixed)
            bgMapObject->type.push_back(bgMapObject->section->rules->naturalLand_encodingRuleId);
        else*/
        {
            bgMapObject->isArea = false;
            bgMapObject->type.push_back(bgMapObject->section->rules->naturalCoastlineBroken_encodingRuleId);
        }
        bgMapObject->addtype.push_back(bgMapObject->section->rules->layerLowest_encodingRuleId);

        assert(bgMapObject->isClosedFigure());
        polygonizedCoastlineObjects.push_back(std::move(bgMapObject));
    }

	_context = std::shared_ptr<MapRasterizerContext>(new MapRasterizerContext());
	_context->_tileScale = MapUtils::getPowZoom(workZoom);

	_source.obtainMapPrimitives(mapDatum, workZoom, _context);
	_source.obtainMapPrimitives(polygonizedCoastlineObjects, workZoom, _context);
	_context->sortGraphicElements();

}



void MapRasterizer::DrawMap(SkCanvas& canvas)
{
	if (!_context)
		return;

	if(/*fillBackground*/ false)
    {
        if(/*destinationArea*/ false)
        {
            // If destination area is specified, fill only it with background
            SkPaint bgPaint;
            bgPaint.setColor(_source.);
            bgPaint.setStyle(SkPaint::kFill_Style);
            //canvas.drawRectCoords(destinationArea->top, destinationArea->left, destinationArea->right, destinationArea->bottom, bgPaint);
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
	_destinationArea.max_corner().set<0>(0);
	_destinationArea.max_corner().set<1>(0);
	_destinationArea.min_corner().set<1>(sizer.height());
	_destinationArea.min_corner().set<0>(sizer.width());
	_context->_pixelScaleXY.set<0>(_context->_tileScale / static_cast<double>(sizer.width()));
	_context->_pixelScaleXY.set<1>(_context->_tileScale / static_cast<double>(sizer.height()));

	_mapPaint = _source.mapPaint;
	rasterizeMapElements(&_destinationArea, canvas, _context->_polygons, GraphElementType::Polygons);
	
}

void MapRasterizer::rasterizeMapElements(const AreaI* const destinationArea,SkCanvas& canvas, const std::vector< std::shared_ptr< GraphicElement> >& primitives, GraphElementType type)
{
	const auto polygonMinSizeToDisplay31 = _source.polygonMinSizeToDisplay * (_context->_pixelScaleXY.get<0>() * _context->_pixelScaleXY.get<1>());
    const auto polygonSizeThreshold = 1.0 / polygonMinSizeToDisplay31;

    for(auto itPrimitive = primitives.cbegin(); itPrimitive != primitives.cend(); ++itPrimitive)
    {

        const auto& primitive = *itPrimitive;

        if(type == Polygons)
        {
            if(primitive->zOrder > polygonSizeThreshold + static_cast<int>(primitive->zOrder))
                continue;

            rasterizePolygon(destinationArea, canvas, primitive);
        }
        else if(type == Polylines || type == Polylines_ShadowOnly)
        {
            rasterizePolyline(destinationArea, canvas, primitive, type == Polylines_ShadowOnly);
        }
    }
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
            ok = env.obtainPathEffect(encodedPathEffect, pathEffect);

            if(ok && pathEffect)
                _mapPaint.setPathEffect(pathEffect);
        }
    }

    SkColor color;
    ok = evalResult.getIntegerValue(valueDefId_color, color);
    if(!ok || !color)
        return false;
    _mapPaint.setColor(color);

    if (valueSetSelector == PaintValuesSet::Set_0)
    {
        std::string shader;
        ok = evalResult.getStringValue(builtinStyleDefs->id_OUTPUT_SHADER, shader);
        if(ok && !shader.empty())
        {
            SkBitmapProcShader* shaderObj = nullptr;
            if(env.obtainBitmapShader(shader, shaderObj) && shaderObj)
            {
                _mapPaint.setShader(static_cast<SkShader*>(shaderObj));
                shaderObj->unref();
            }
        }
    }

    // do not check shadow color here
    if (context._shadowRenderingMode == 1 && valueSetSelector == PaintValuesSet::Set_0)
    {
        int shadowColor;
        ok = evalResult.getIntegerValue(builtinStyleDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
        int shadowRadius;
        evalResult.getIntegerValue(builtinStyleDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
        if(!ok || shadowColor == 0)
            shadowColor = context._shadowRenderingColor;
        if(shadowColor == 0)
            shadowRadius = 0;

        if(shadowRadius > 0)
            _mapPaint.setLooper(new SkBlurDrawLooper(static_cast<SkScalar>(shadowRadius), 0, 0, shadowColor))->unref();
    }

    return true;
}


void MapRasterizer::rasterizePolygon(
    const AreaI* const destinationArea,
    SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive)
{
    assert(primitive->mapObject->points31.size() > 2);
    assert(primitive->mapObject->isClosedFigure());
    assert(primitive->mapObject->isClosedFigure(true));

    if(!updatePaint(*primitive->styleResult, PaintValuesSet::Set_0, true))
        return;

    SkPath path;
    bool containsAtLeastOnePoint = false;
    int pointIdx = 0;
    PointF vertex;
    int bounds = 0;
    QVector< PointF > outsideBounds;
    const auto pointsCount = primitive->mapObject->points31.size();
    auto pPoint = primitive->mapObject->points31.constData();
    for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            path.lineTo(vertex.x, vertex.y);
        }

        if(destinationArea && !containsAtLeastOnePoint)
        {
            if(destinationArea->contains(vertex))
            {
                containsAtLeastOnePoint = true;
            }
            else
            {
                outsideBounds.push_back(qMove(vertex));
            }
            bounds |= (vertex.x < destinationArea->left ? 1 : 0);
            bounds |= (vertex.x > destinationArea->right ? 2 : 0);
            bounds |= (vertex.y < destinationArea->top ? 4 : 0);
            bounds |= (vertex.y > destinationArea->bottom ? 8 : 0);
        }

    }

    if(destinationArea && !containsAtLeastOnePoint)
    {
        // fast check for polygons
        if((bounds & 3) != 3 || (bounds >> 2) != 3)
            return;

        bool ok = true;
        ok = ok || contains(outsideBounds, destinationArea->topLeft);
        ok = ok || contains(outsideBounds, destinationArea->bottomRight);
        ok = ok || contains(outsideBounds, PointF(0, destinationArea->bottom));
        ok = ok || contains(outsideBounds, PointF(destinationArea->right, 0));
        if(!ok)
            return;
    }

    if(!primitive->mapObject->innerPolygonsPoints31.empty())
    {
        path.setFillType(SkPath::kEvenOdd_FillType);
        for(auto itPolygon = primitive->mapObject->innerPolygonsPoints31.cbegin(); itPolygon != primitive->mapObject->innerPolygonsPoints31.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            pointIdx = 0;
            for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex, pointIdx++)
            {
                const auto& point = *itVertex;
                calculateVertex(point, vertex);

                if(pointIdx == 0)
                {
                    path.moveTo(vertex.x, vertex.y);
                }
                else
                {
                    path.lineTo(vertex.x, vertex.y);
                }
            }
        }
    }

    canvas.drawPath(path, _mapPaint);
    if(updatePaint(*primitive->styleResult, PaintValuesSet::Set_1, false))
        canvas.drawPath(path, _mapPaint);
}

void MapRasterizer::rasterizePolyline(    const AreaI* const destinationArea,    SkCanvas& canvas, const std::shared_ptr< GraphicElement>& primitive, bool drawOnlyShadow)
{
    /*assert(primitive->mapObject->points31.size() >= 2);*/
	auto builtinStyleDefs =  _source.getDefaultStyles();

    if(!updatePaint(*primitive->styleResult, PaintValuesSet::Set_0, false))
        return;

    bool ok;

    int shadowColor;
    ok = primitive->styleResult->getIntegerValue(builtinStyleDefs->id_OUTPUT_SHADOW_COLOR, shadowColor);
    if(!ok || shadowColor == 0)
        shadowColor = context._shadowRenderingColor;

    int shadowRadius;
    ok = primitive->styleResult->getIntegerValue(builtinStyleDefs->id_OUTPUT_SHADOW_RADIUS, shadowRadius);
    if(drawOnlyShadow && (!ok || shadowRadius == 0))
        return;

    const auto typeRuleId = primitive->mapObject->_typesRuleIds[primitive->typeRuleIdIndex];

    int oneway = 0;
    if(context._zoom >= ZoomLevel16 && typeRuleId == primitive->mapObject->section->encodingDecodingRules->highway_encodingRuleId)
    {
        if(primitive->mapObject->containsType(primitive->mapObject->section->encodingDecodingRules->oneway_encodingRuleId, true))
            oneway = 1;
        else if(primitive->mapObject->containsType(primitive->mapObject->section->encodingDecodingRules->onewayReverse_encodingRuleId, true))
            oneway = -1;
    }

    SkPath path;
    int pointIdx = 0;
    bool intersect = false;
    int prevCross = 0;
    PointF vertex, middleVertex;
    const auto pointsCount = primitive->mapObject->points31.size();
    const auto middleIdx = pointsCount / 2;
    auto pPoint = primitive->mapObject->points31.constData();
    for(pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        calculateVertex(*pPoint, vertex);

        if(pointIdx == 0)
        {
            path.moveTo(vertex.x, vertex.y);
        }
        else
        {
            if(pointIdx == middleIdx)
                middleVertex = vertex;

            path.lineTo(vertex.x, vertex.y);
        }

        if(destinationArea && !intersect)
        {
            if(destinationArea->contains(vertex))
            {
                intersect = true;
            }
            else
            {
                int cross = 0;
                cross |= (vertex.x < destinationArea->left ? 1 : 0);
                cross |= (vertex.x > destinationArea->right ? 2 : 0);
                cross |= (vertex.y < destinationArea->top ? 4 : 0);
                cross |= (vertex.y > destinationArea->bottom ? 8 : 0);
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
        return;

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
}

void MapRasterizer::rasterizeLineShadow(
    SkCanvas& canvas, const SkPath& path, uint32_t shadowColor, int shadowRadius )
{
    // blurred shadows
    if (context._shadowRenderingMode == 2 && shadowRadius > 0)
    {
        // simply draw shadow? difference from option 3 ?
        // paint->setColor(0xffffffff);
        _mapPaint.setLooper(new SkBlurDrawLooper(shadowRadius, 0, 0, shadowColor))->unref();
        canvas.drawPath(path, _mapPaint);
    }

    // option shadow = 3 with solid border
    if (context._shadowRenderingMode == 3 && shadowRadius > 0)
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
        for(auto itPaint = env.oneWayPaints.cbegin(); itPaint != env.oneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
    else
    {
        for(auto itPaint = env.reverseOneWayPaints.cbegin(); itPaint != env.reverseOneWayPaints.cend(); ++itPaint)
        {
            canvas.drawPath(path, *itPaint);
        }
    }
}

void MapRasterizer::calculateVertex( const pointI& point31, pointF& vertex )
{
    vertex.x = static_cast<float>(point31.x - context._area31.left) / _31toPixelDivisor.x;
    vertex.y = static_cast<float>(point31.y - context._area31.top) / _31toPixelDivisor.y;
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

        if(Utilities::rayIntersect(vertex0, vertex1, other))
            intersections++;
    }

    // special handling, also count first and last, might not be closed, but
    // we want this!
    const auto& vertex0 = vertices.first();
    const auto& vertex1 = vertices.last();
    if(Utilities::rayIntersect(vertex0, vertex1, other))
        intersections++;

    return intersections % 2 == 1;
}
