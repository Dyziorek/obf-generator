#include "stdafx.h"
#include "SkCanvas.h"
#include "SkDashPathEffect.h"
#include "SkBitmapProcShader.h"
#include "MapObjectData.h"
#include "BinaryMapDataObjects.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"
#include "MapRasterizer.h"

#include <google\protobuf\io\coded_stream.h>
#include <boost/filesystem.hpp>
#include "RandomAccessFileReader.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"
#include "MapRasterizerProvider.h"
#include "MapRasterizerContext.h"
#include "Tools.h"

MapRasterizerContext::MapRasterizerContext(void)
{
}


MapRasterizerContext::~MapRasterizerContext(void)
{
}


void MapRasterizerContext::initialize(MapRasterizerProvider& envData, int zoom)
{
	MapStyleResult evalResult;

 //   defaultBgColor = envData.defaultBgColor;
 //   if(envData.attributeRule_defaultColor)
 //   {
 //       MapStyleEvaluator evaluator(envData.owner->style, envData.owner->displayDensityFactor);
 //       //envData.applyTo(evaluator);
 //       evaluator.setIntegerValue(envData.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

 //       evalResult.clear();
 //       if(evaluator.evaluate(envData.attributeRule_defaultColor, &evalResult))
 //           evalResult.getIntegerValue(envData.styleBuiltinValueDefs->id_OUTPUT_ATTR_COLOR_VALUE, context._defaultBgColor);
 //   }

 //   _shadowRenderingMode = envData.shadowRenderingMode;
 //   _shadowRenderingColor = envData.shadowRenderingColor;
 //   if(envData.attributeRule_shadowRendering)
 //   {
 //       MapStyleEvaluator evaluator(envData.owner->style, envData.owner->displayDensityFactor);
 //       envData.applyTo(evaluator);
 //       evaluator.setIntegerValue(envData.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);

 //       evalResult.clear();
 //       if(evaluator.evaluate(envData.attributeRule_shadowRendering, &evalResult))
 //       {
 //           evalResult.getIntegerValue(envData.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._shadowRenderingMode);
 //           evalResult.getIntegerValue(envData.styleBuiltinValueDefs->id_OUTPUT_SHADOW_COLOR, context._shadowRenderingColor);
 //       }
 //   }

    //polygonMinSizeToDisplay = envData.polygonMinSizeToDisplay;
   // if(envData.attributeRule_polygonMinSizeToDisplay)
   // {
   //     MapStyleEval evaluator(envData.getStyleInfo());
   //     
   //     evaluator.setIntValue(envData.getDefaultStyles()->id_INPUT_MINZOOM, zoom);

   //     evalResult.clear();
   //     if(evaluator.evaluate(envData.attributeRule_polygonMinSizeToDisplay, &evalResult))
   //     {
   //         int polygonMinSizeToDisplay;
			//if(evalResult.getIntVal(envData.getDefaultStyles()->id_OUTPUT_ATTR_INT_VALUE, polygonMinSizeToDisplay))
   //             envData.polygonMinSizeToDisplay = polygonMinSizeToDisplay;
   //     }
   // }

 //   context._roadDensityZoomTile = envData.roadDensityZoomTile;
 //   if(envData.attributeRule_roadDensityZoomTile)
 //   {
 //       MapStyleEvaluator evaluator(envData.owner->style, envData.owner->displayDensityFactor);
 //       envData.applyTo(evaluator);
 //       evaluator.setIntegerValue(envData.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
 //       
 //       evalResult.clear();
 //       if(evaluator.evaluate(envData.attributeRule_roadDensityZoomTile, &evalResult))
 //           evalResult.getIntegerValue(envData.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._roadDensityZoomTile);
 //   }

 //   context._roadsDensityLimitPerTile = envData.roadsDensityLimitPerTile;
 //   if(envData.attributeRule_roadsDensityLimitPerTile)
 //   {
 //       MapStyleEvaluator evaluator(envData.owner->style, envData.owner->displayDensityFactor);
 //       envData.applyTo(evaluator);
 //       evaluator.setIntegerValue(envData.styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
 //       
 //       evalResult.clear();
 //       if(evaluator.evaluate(envData.attributeRule_roadsDensityLimitPerTile, &evalResult))
 //           evalResult.getIntegerValue(envData.styleBuiltinValueDefs->id_OUTPUT_ATTR_INT_VALUE, context._roadsDensityLimitPerTile);
 //   }

 //   context._shadowLevelMin = envData.shadowLevelMin;
 //   context._shadowLevelMax = envData.shadowLevelMax;
}

void MapRasterizerContext::sortGraphicElements()
{
	const auto funcSort = [](const std::shared_ptr<MapRasterizer::GraphicElement>& firstPair, const std::shared_ptr<MapRasterizer::GraphicElement>& secondPair){
		if (MapUtils::fuzzyCompare(firstPair->zOrder, secondPair->zOrder))
		{
			if (firstPair->_typeIdIndex == secondPair->_typeIdIndex)
			{
				return firstPair->_mapData->points.size() < secondPair->_mapData->points.size();
			}
			return firstPair->_typeIdIndex < secondPair->_typeIdIndex;
		}
		return firstPair->zOrder < secondPair->zOrder;
	};

	for(std::shared_ptr<MapRasterizer::GraphicElementGroup> groupElem : _graphicElements)
	{
		_points.insert(_points.end(), groupElem->_points.begin(), groupElem->_points.end());
		_polygons.insert(_polygons.end(), groupElem->_polygons.begin(), groupElem->_polygons.end());
		_polyLines.insert(_polyLines.end(), groupElem->_polyLines.begin(), groupElem->_polyLines.end());
	}

	std::sort(_points.begin(), _points.end(), funcSort);
	std::sort(_polygons.begin(), _polygons.end(), funcSort);
	std::sort(_polyLines.begin(), _polyLines.end(), funcSort);
}

bool MapRasterizerContext::polygonizeCoastlines(
    const MapRasterizerProvider& env, const std::list< std::shared_ptr<const MapObjectData> >& coastlines,
	std::list< std::shared_ptr<const MapObjectData> >& outVectorized,
    bool abortIfBrokenCoastlinesExist,
    bool includeBrokenCoastlines )
{
    std::list< std::vector< pointI > > closedPolygons;
    std::list< std::vector< pointI > > coastlinePolylines; // Broken == not closed in this case

    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = _area31;
	alignedArea31.min_corner().set<1>(alignedArea31.min_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.min_corner().set<0>(alignedArea31.min_corner().get<0>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<1>(alignedArea31.max_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<0>(alignedArea31.max_corner().get<0>() & ~((1u << 5) - 1));

    uint64_t osmId = 0;
    std::vector< pointI > linePoints31;
    for(auto itCoastline = coastlines.cbegin(); itCoastline != coastlines.cend(); ++itCoastline)
    {
        const auto& coastline = *itCoastline;

        if(coastline->points.size() < 2)
        {
            /*OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                "Map object #%" PRIu64 " (%" PRIi64 ") is polygonized as coastline, but has %d vertices",
                coastline->id >> 1, static_cast<int64_t>(coastline->id) / 2,
                coastline->points.size());*/
            continue;
        }

		osmId = coastline->localId >> 1;
        linePoints31.clear();
        auto itPoint = coastline->points.cbegin();
        auto pp = *itPoint;
        auto cp = pp;
		auto prevInside = bg::covered_by(cp, alignedArea31);// alignedArea31.contains(cp);
        if(prevInside)
            linePoints31.push_back(cp);
        for(++itPoint; itPoint != coastline->points.cend(); ++itPoint)
        {
            cp = *itPoint;

            const auto inside = bg::covered_by(cp, alignedArea31);
            const auto lineEnded = buildCoastlinePolygonSegment(env,  inside, cp, prevInside, pp, linePoints31);
            if (lineEnded)
            {
                appendCoastlinePolygons(closedPolygons, coastlinePolylines, linePoints31);

                // Create new line if it goes outside
                linePoints31.clear();
            }

            pp = cp;
            prevInside = inside;
        }

        appendCoastlinePolygons(closedPolygons, coastlinePolylines, linePoints31);
    }

    if (closedPolygons.empty() && coastlinePolylines.empty())
        return false;

    // Draw coastlines
    for(auto itPolyline = coastlinePolylines.cbegin(); itPolyline != coastlinePolylines.cend(); ++itPolyline)
    {
        const auto& polyline = *itPolyline;

        const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
        mapObject->isArea = false;
        mapObject->points = polyline;
		mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
#ifdef _DEBUG
		auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
		mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif
        outVectorized.push_back(std::move(mapObject));
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.empty();
    if(!coastlinePolylines.empty())
    {
        // Add complete water tile with holes
        const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
        mapObject->points.push_back(std::move(pointI(_area31.min_corner().get<0>(), _area31.min_corner().get<1>())));
        mapObject->points.push_back(std::move(pointI(_area31.max_corner().get<0>(), _area31.min_corner().get<1>())));
        mapObject->points.push_back(std::move(pointI(_area31.max_corner().get<0>(),_area31.max_corner().get<1>())));
        mapObject->points.push_back(std::move(pointI(_area31.min_corner().get<0>(), _area31.max_corner().get<1>())));
        mapObject->points.push_back(mapObject->points.front());
        convertCoastlinePolylinesToPolygons(env, coastlinePolylines, mapObject->innerpolypoints, osmId);

        mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
#ifdef _DEBUG
		auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
		mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif
		mapObject->localId = osmId;
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(std::move(mapObject));
    }

    if(!coastlinePolylines.empty())
    {
      /*  OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Invalid polylines found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            _area31.top,
            _area31.left,
            _area31.bottom,
            _area31.right,
            _zoom);*/
    }

    if (includeBrokenCoastlines)
    {
        for(auto itPolygon = coastlinePolylines.cbegin(); itPolygon != coastlinePolylines.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
			mapObject->isArea = false;
            mapObject->points = polygon;
	        mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
#ifdef _DEBUG
			auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
			mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif
            outVectorized.push_back(std::move(mapObject));
        }
    }

    // Draw coastlines
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
        mapObject->isArea = false;
        mapObject->points = polygon;
        mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
#ifdef _DEBUG
		auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
		mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif
        outVectorized.push_back(std::move(mapObject));
    }

    if (abortIfBrokenCoastlinesExist && !coastlinePolylines.empty())
        return false;

    auto fullWaterObjects = 0u;
    auto fullLandObjects = 0u;
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        // If polygon has less than 4 points, it's invalid
        if(polygon.size() < 4)
            continue;

        bool clockwise = isClockwiseCoastlinePolygon(polygon);

        const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
        mapObject->points = std::move(polygon);
        if(clockwise)
        {
        mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
		#ifdef _DEBUG
				auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
				mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
		#endif
            fullWaterObjects++;
        }
        else
        {
			mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
#ifdef _DEBUG
			auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
			mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif  
			fullLandObjects++;
        }
		mapObject->localId = osmId;
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(std::move(mapObject));
    }

    if(fullWaterObjects == 0u && !coastlineCrossesBounds)
    {
        /*OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Isolated islands found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            _area31.top,
            _area31.left,
            _area31.bottom,
            _area31.right,
            _zoom);*/

        // Add complete water tile
        const std::shared_ptr<MapObjectData> mapObject(new MapObjectData(env.dummySectionData));
		areaInt checkArea(_area31);
        mapObject->points.push_back(std::move(pointI(checkArea.Left(), checkArea.Top())));
        mapObject->points.push_back(std::move(pointI(checkArea.Right(), checkArea.Top())));
        mapObject->points.push_back(std::move(pointI(checkArea.Right(), checkArea.Bottom())));
		mapObject->points.push_back(std::move(pointI(checkArea.Left(), checkArea.Bottom())));
        mapObject->points.push_back(mapObject->points.front());

        mapObject->typeIds.push_back(env.dummySectionData->rules->naturalCoastline_encodingRuleId);
#ifdef _DEBUG
		auto typeData = env.dummySectionData->rules->getRuleInfo(env.dummySectionData->rules->naturalCoastlineLine_encodingRuleId);
		mapObject->typeNames.push_back(typeData.tag+", "+typeData.value);
#endif
		mapObject->localId = osmId;
        mapObject->isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(std::move(mapObject));
    }

    return true;
}

bool MapRasterizerContext::buildCoastlinePolygonSegment(
    const MapRasterizerProvider& env,
    bool currentInside,
    const pointI& currentPoint31,
    bool prevInside,
    const pointI& previousPoint31,
    std::vector< pointI >& segmentPoints )
{
    bool lineEnded = false;

    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = _area31;

	alignedArea31.min_corner().set<1>(alignedArea31.min_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.min_corner().set<0>(alignedArea31.min_corner().get<0>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<1>(alignedArea31.max_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<0>(alignedArea31.max_corner().get<0>() & ~((1u << 5) - 1));

    auto point = currentPoint31;
    if (prevInside)
    {
        if (!currentInside)
        {
            bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, alignedArea31, point);
            if (!hasIntersection)
                point = previousPoint31;
            segmentPoints.push_back(point);
            lineEnded = true;
        }
        else
        {
            segmentPoints.push_back(point);
        }
    }
    else
    {
        bool hasIntersection = calculateIntersection(currentPoint31, previousPoint31, alignedArea31, point);
        if (currentInside)
        {
            assert(hasIntersection);
            segmentPoints.push_back(point);
            segmentPoints.push_back(currentPoint31);
        }
        else if (hasIntersection)
        {
            segmentPoints.push_back(point);
            calculateIntersection(currentPoint31, point, alignedArea31, point);
            segmentPoints.push_back(point);
            lineEnded = true;
        }
    }

    return lineEnded;
}

// Calculates intersection between line and bbox in clockwise manner.
bool MapRasterizerContext::calculateIntersection( const pointI& p1, const pointI& p0, const AreaI& bbox, pointI& pX )
{
    // Well, since Victor said not to touch this thing, I will replace only output writing,
    // and will even retain old variable names.
	const auto& px = p0.get<0>();
    const auto& py = p0.get<1>();
    const auto& x = p1.get<0>();
    const auto& y = p1.get<1>();
	const auto& leftX = bbox.min_corner().get<0>();
    const auto& rightX = bbox.max_corner().get<0>();
    const auto& topY = bbox.min_corner().get<1>();
    const auto& bottomY = bbox.max_corner().get<1>();

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.set<0>(tx);//b.first = tx;
            pX.set<1>(topY);//b.second = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.set<0>(tx);//b.first = tx;
            pX.set<1>(bottomY);//b.second = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.set<0>(leftX);//b.first = leftX;
            pX.set<1>(ty);//b.second = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.set<0>(rightX);//b.first = rightX;
            pX.set<1>(ty);//b.second = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.set<0>(tx);//b.first = tx;
            pX.set<1>(topY);//b.second = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.set<0>(tx);//b.first = tx;
            pX.set<1>(bottomY);//b.second = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.set<0>(leftX);//b.first = leftX;
            pX.set<1>(ty);//b.second = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.set<0>(rightX);//b.first = rightX;
            pX.set<1>(ty);//b.second = ty;
            return true;
        }

    }

    if (px == rightX || px == leftX || py == topY || py == bottomY) {
        pX = p0;//b.first = px; b.second = py;
        //		return true;
        // Is it right? to not return anything?
    }
    return false;
}

void MapRasterizerContext::appendCoastlinePolygons( std::list< std::vector< pointI > >& closedPolygons, std::list< std::vector< pointI > >& coastlinePolylines, std::vector< pointI > & polyline )
{
    if(polyline.empty())
        return;

    if(bg::equals(polyline.front(), polyline.back()))
    {
        closedPolygons.push_back(polyline);
        return;
    }

    bool add = true;

    for(auto itPolygon = coastlinePolylines.begin(); itPolygon != coastlinePolylines.end();)
    {
        auto& polygon = *itPolygon;

        bool remove = false;

        if(bg::equals(polyline.front(), polygon.back()))
        {
            polygon.reserve(polygon.size() + polyline.size() - 1);
            polygon.pop_back();
			polygon.insert(polygon.end(), polyline.begin(), polyline.end());
            remove = true;
            polyline = polygon;
        }
        else if(bg::equals(polyline.back(), polygon.front()))
        {
            polyline.reserve(polyline.size() + polygon.size() - 1);
            polyline.pop_back();
			polyline.insert(polyline.end(), polygon.begin(), polygon.end());
            remove = true;
        }

        if (remove)
        {
            itPolygon = coastlinePolylines.erase(itPolygon);
        }
        else
        {
            ++itPolygon;
        }

        if (bg::equals(polyline.front(), polyline.back()))
        {
            closedPolygons.push_back(polyline);
            add = false;
            break;
        }
    }

    if (add)
    {
        coastlinePolylines.push_back(polyline);
    }
}

struct unref_iterator_less
{
	template <class IterRef>  bool operator()(const IterRef& obj1, const IterRef& obj2)
	{
		const void* refAddr1 = (void* )&(obj1);
		const void* refAddr2 = (void* )&(obj2);
		return refAddr1 < refAddr2;
	}
};

void MapRasterizerContext::convertCoastlinePolylinesToPolygons(
    const MapRasterizerProvider& env,
    std::list< std::vector< pointI > >& coastlinePolylines, std::list< std::vector< pointI > >& coastlinePolygons, uint64_t osmId )
{
    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = _area31;
	alignedArea31.min_corner().set<1>(alignedArea31.min_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.min_corner().set<0>(alignedArea31.min_corner().get<0>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<1>(alignedArea31.max_corner().get<1>() & ~((1u << 5) - 1));
    alignedArea31.max_corner().set<0>(alignedArea31.max_corner().get<0>() & ~((1u << 5) - 1));

	areaInt checkerArea(alignedArea31);

    std::list< std::vector< pointI > > validPolylines;

    // Check if polylines has been cut by rasterization viewport
	std::list< std::vector< pointI > >::iterator itPolyline(coastlinePolylines.begin());
	while(itPolyline != coastlinePolylines.end())
    {
		auto itRemoving = itPolyline;
        const auto& polyline = *(itPolyline++);
        //assert(!polyline.empty());

        const auto& head = polyline.front();
        const auto& tail = polyline.back();

        // This curve has not been cut by rasterization viewport, so it's
        // impossible to fix it
		if ( checkerArea.onEdge(head) == areaInt::EdgeBox::invalid || checkerArea.onEdge(tail) == areaInt::EdgeBox::invalid)
            continue;

        validPolylines.push_back(polyline);
		coastlinePolylines.erase(itRemoving);
    }

    std::set< std::list< std::vector< pointI > >::iterator, unref_iterator_less > processedPolylines;
    while(processedPolylines.size() != validPolylines.size())
    {
        for(auto itPolyline = validPolylines.begin(); itPolyline != validPolylines.end(); ++itPolyline)
        {
            // If this polyline was already processed, skip it
            if(processedPolylines.find(itPolyline) != processedPolylines.end())
                continue;

            // Start from tail of the polyline and search for it's continuation in CCV order
            auto& polyline = *itPolyline;
            const auto& tail = polyline.back();
            auto tailEdge = areaInt::EdgeBox::invalid;
			areaInt checkArea(alignedArea31);
			tailEdge = checkArea.onEdge(tail);
            auto itNearestPolyline = validPolylines.end();
            auto firstIteration = true;
            for(int idx = static_cast<int>(tailEdge) + 4; (idx >= static_cast<int>(tailEdge)) && (itNearestPolyline == validPolylines.end()); idx--, firstIteration = false)
            {
                const auto currentEdge = static_cast<areaInt::EdgeBox>(idx % 4);

                for(auto itOtherPolyline = validPolylines.begin(); itOtherPolyline != validPolylines.end(); ++itOtherPolyline)
                {
                    // If this polyline was already processed, skip it
                    if(processedPolylines.find(itOtherPolyline) != processedPolylines.end())
                        continue;

                    // Skip polylines that are on other edges
                    const auto& otherHead = itOtherPolyline->front();
                    auto otherHeadEdge = areaInt::EdgeBox::invalid;
                    otherHeadEdge = checkArea.onEdge(otherHead);
                    if(otherHeadEdge != currentEdge)
                        continue;

                    // Skip polyline that is not next in CCV order
                    if(firstIteration)
                    {
                        bool isNextByCCV = false;
                        if(currentEdge == areaInt::EdgeBox::top)
                            isNextByCCV = (otherHead.get<0>() <= tail.get<0>());
                        else if(currentEdge == areaInt::EdgeBox::right)
                            isNextByCCV = (otherHead.get<1>() <= tail.get<1>());
                        else if(currentEdge == areaInt::EdgeBox::bottom)
                            isNextByCCV = (tail.get<0>() <= otherHead.get<0>());
                        else if(currentEdge == areaInt::EdgeBox::left)
                            isNextByCCV = (tail.get<1>() <= otherHead.get<1>());
                        if(!isNextByCCV)
                            continue;
                    }

                    // If nearest was not yet set, set this
                    if(itNearestPolyline == validPolylines.cend())
                    {
                        itNearestPolyline = itOtherPolyline;
                        continue;
                    }

                    // Check if current polyline's head is closer (by CCV) that previously selected
                    const auto& previouslySelectedHead = itNearestPolyline->front();
                    bool isCloserByCCV = false;
                    if(currentEdge == areaInt::EdgeBox::top)
                        isCloserByCCV = (otherHead.get<0>() > previouslySelectedHead.get<0>());
                    else if(currentEdge == areaInt::EdgeBox::right)
                        isCloserByCCV = (otherHead.get<1>() > previouslySelectedHead.get<1>());
                    else if(currentEdge == areaInt::EdgeBox::bottom)
                        isCloserByCCV = (otherHead.get<0>() < previouslySelectedHead.get<0>());
                    else if(currentEdge == areaInt::EdgeBox::left)
                        isCloserByCCV = (otherHead.get<1>() < previouslySelectedHead.get<1>());

                    // If closer-by-CCV, then select this
                    if(isCloserByCCV)
                        itNearestPolyline = itOtherPolyline;
                }
            }
            assert(itNearestPolyline != validPolylines.cend());

            // Get edge of nearest-by-CCV head
			auto nearestHeadEdge = areaInt::EdgeBox::invalid;
            const auto& nearestHead = itNearestPolyline->front();
			nearestHeadEdge = checkArea.onEdge(nearestHead);

            // Fill by edges of area, if required
            int loopShift = 0;
            if( static_cast<int>(tailEdge) - static_cast<int>(nearestHeadEdge) < 0 )
                loopShift = 4;
            else if(tailEdge == nearestHeadEdge)
            {
                bool skipAddingSides = false;
                if(tailEdge == areaInt::EdgeBox::top)
                    skipAddingSides = (tail.get<0>() >= nearestHead.get<0>());
                else if(tailEdge == areaInt::EdgeBox::right)
                    skipAddingSides = (tail.get<1>() >= nearestHead.get<1>());
                else if(tailEdge == areaInt::EdgeBox::bottom)
                    skipAddingSides = (tail.get<0>() <= nearestHead.get<0>());
                else if(tailEdge == areaInt::EdgeBox::left)
                    skipAddingSides = (tail.get<1>() <= nearestHead.get<1>());

                if(!skipAddingSides)
                    loopShift = 4;
            }
            for(int idx = static_cast<int>(tailEdge) + loopShift; idx > static_cast<int>(nearestHeadEdge); idx--)
            {
                const auto side = static_cast<areaInt::EdgeBox>(idx % 4);
                pointI p;

                if(side == areaInt::EdgeBox::top)
                {
					p.set<1>(checkArea.Top());
                    p.set<0>(checkArea.Left());
                }
                else if(side == areaInt::EdgeBox::right)
                {
                    p.set<1>(checkArea.Top());
                    p.set<0>(checkArea.Right());
                }
                else if(side == areaInt::EdgeBox::bottom)
                {
                    p.set<1>(checkArea.Bottom());
                    p.set<0>(checkArea.Right());
                }
                else if(side == areaInt::EdgeBox::left)
                {
                    p.set<1>(checkArea.Bottom());
                    p.set<0>(checkArea.Left());
                }

                polyline.push_back(std::move(p));
            }

            // If nearest-by-CCV is head of current polyline, cap it and add to polygons, ...
            if(itNearestPolyline == itPolyline)
            {
                polyline.push_back(polyline.front());
                coastlinePolygons.push_back(polyline);
            }
            // ... otherwise join them. Joined will never be visited, and current will remain unmarked as processed
            else
            {
                const auto& otherPolyline = *itNearestPolyline;
				polyline.insert(polyline.end(), otherPolyline.begin(), otherPolyline.end());
            }

            // After we've selected nearest-by-CCV polyline, mark it as processed
            processedPolylines.insert(itNearestPolyline);
        }
    }
}

bool MapRasterizerContext::isClockwiseCoastlinePolygon( const std::vector< pointI > & polygon )
{
    if(polygon.empty())
        return true;

    // calculate middle Y
    int64_t middleY = 0;
    for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex)
        middleY += itVertex->get<1>();
    middleY /= polygon.size();

    double clockwiseSum = 0;

    bool firstDirectionUp = false;
    int previousX = INT_MIN;
    int firstX = INT_MIN;

    auto itPrevVertex = polygon.cbegin();
    auto itVertex = itPrevVertex + 1;
    for(; itVertex != polygon.cend(); itPrevVertex = itVertex, ++itVertex)
    {
        const auto& vertex0 = *itPrevVertex;
        const auto& vertex1 = *itVertex;

        int32_t rX;
        if(!Tools::rayIntersectX(vertex0, vertex1, middleY, rX))
            continue;

        bool skipSameSide = (vertex1.get<1>() <= middleY) == (vertex0.get<1>() <= middleY);
        if (skipSameSide)
            continue;

        bool directionUp = vertex0.get<1>() >= middleY;
        if (firstX == INT_MIN) {
            firstDirectionUp = directionUp;
            firstX = rX;
        } else {
            bool clockwise = (!directionUp) == (previousX < rX);
            if (clockwise) {
                clockwiseSum += abs(previousX - rX);
            } else {
                clockwiseSum -= abs(previousX - rX);
            }
        }
        previousX = rX;
    }

    if (firstX != INT_MIN) {
        bool clockwise = (!firstDirectionUp) == (previousX < firstX);
        if (clockwise) {
			clockwiseSum += std::abs(previousX - firstX);
        } else {
            clockwiseSum -= std::abs(previousX - firstX);
        }
    }

    return clockwiseSum >= 0;
}


void MapRasterizerContext::removeHighwaysBasedOnDensity(const MapRasterizerProvider& source)
{
    // Check if any filtering needed
    if(source.roadDensityZoomTile == 0 || source.roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = zoom + source.roadDensityZoomTile;
    std::unordered_map< uint64_t, std::pair<uint32_t, double> > densityMap;
    

	std::vector<std::shared_ptr<MapRasterizer::GraphicElement>>::iterator itLine = _polyLines.end();
    std::vector<std::shared_ptr<MapRasterizer::GraphicElement>>::iterator itLineCheck = _polyLines.end();
	itLineCheck = itLine - 1;
    while(itLine != _polyLines.begin())
    {
		const auto& line = *(--itLine);

        auto accept = true;
		const std::shared_ptr<BinaryMapSection> sectionInfo = line->_mapData->section.lock();
		if(line->_mapData->typeIds[line->_typeIdIndex] == sectionInfo->rules->highway_encodingRuleId)
        {
            accept = false;

            uint64_t prevId = 0;
            const auto pointsCount = line->_mapData->points.size();
            auto pPoint = line->_mapData->points.data();
            for(auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
            {
                auto x = pPoint->get<0>() >> (31 - dZ);
                auto y = pPoint->get<1>() >> (31 - dZ);
                uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
                if(prevId != id)
                {
                    prevId = id;

                    auto& mapEntry = densityMap[id];
                    if(mapEntry.first < source.roadDensityZoomTile /*&& p.second > o */)
                    {
                        accept = true;
                        mapEntry.first += 1;
                        mapEntry.second = line->zOrder;
                    }
                }
            }
        }

        if(!accept)
             _polyLines.erase(itLine);
		itLineCheck = itLine;
    }
}