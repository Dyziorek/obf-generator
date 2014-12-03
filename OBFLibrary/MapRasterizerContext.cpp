#include "stdafx.h"
#include "SkCanvas.h"
#include "MapObjectData.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"
#include "MapRasterizer.h"
#include "MapRasterizerContext.h"


MapRasterizerContext::MapRasterizerContext(void)
{
}


MapRasterizerContext::~MapRasterizerContext(void)
{
}

void MapRasterizerContext::sortGraphicElements()
{
	const auto funcSort = [](const std::shared_ptr<GraphicElement>& firstPair, const std::shared_ptr<GraphicElement>& secondPair){
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

	for(std::shared_ptr<GraphicElementGroup> groupElem : _graphicElements)
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
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

    uint64_t osmId = 0;
    QVector< PointI > linePoints31;
    for(auto itCoastline = coastlines.cbegin(); itCoastline != coastlines.cend(); ++itCoastline)
    {
        const auto& coastline = *itCoastline;

        if(coastline->_points31.size() < 2)
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Warning,
                "Map object #%" PRIu64 " (%" PRIi64 ") is polygonized as coastline, but has %d vertices",
                coastline->id >> 1, static_cast<int64_t>(coastline->id) / 2,
                coastline->_points31.size());
            continue;
        }

        osmId = coastline->id >> 1;
        linePoints31.clear();
        auto itPoint = coastline->_points31.cbegin();
        auto pp = *itPoint;
        auto cp = pp;
        auto prevInside = alignedArea31.contains(cp);
        if(prevInside)
            linePoints31.push_back(cp);
        for(++itPoint; itPoint != coastline->_points31.cend(); ++itPoint)
        {
            cp = *itPoint;

            const auto inside = alignedArea31.contains(cp);
            const auto lineEnded = buildCoastlinePolygonSegment(env, context, inside, cp, prevInside, pp, linePoints31);
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

    if (closedPolygons.isEmpty() && coastlinePolylines.isEmpty())
        return false;

    // Draw coastlines
    for(auto itPolyline = coastlinePolylines.cbegin(); itPolyline != coastlinePolylines.cend(); ++itPolyline)
    {
        const auto& polyline = *itPolyline;

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polyline;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    const bool coastlineCrossesBounds = !coastlinePolylines.isEmpty();
    if(!coastlinePolylines.isEmpty())
    {
        // Add complete water tile with holes
        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.bottom)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.bottom)));
        mapObject->_points31.push_back(mapObject->_points31.first());
        convertCoastlinePolylinesToPolygons(env, context, coastlinePolylines, mapObject->_innerPolygonsPoints31, osmId);

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        assert(mapObject->isClosedFigure(true));
        outVectorized.push_back(qMove(mapObject));
    }

    if(!coastlinePolylines.isEmpty())
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Invalid polylines found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);
    }

    if (includeBrokenCoastlines)
    {
        for(auto itPolygon = coastlinePolylines.cbegin(); itPolygon != coastlinePolylines.cend(); ++itPolygon)
        {
            const auto& polygon = *itPolygon;

            const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
            mapObject->_isArea = false;
            mapObject->_points31 = polygon;
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineBroken_encodingRuleId);

            outVectorized.push_back(qMove(mapObject));
        }
    }

    // Draw coastlines
    for(auto itPolygon = closedPolygons.cbegin(); itPolygon != closedPolygons.cend(); ++itPolygon)
    {
        const auto& polygon = *itPolygon;

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_isArea = false;
        mapObject->_points31 = polygon;
        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastlineLine_encodingRuleId);

        outVectorized.push_back(qMove(mapObject));
    }

    if (abortIfBrokenCoastlinesExist && !coastlinePolylines.isEmpty())
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

        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31 = qMove(polygon);
        if(clockwise)
        {
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
            fullWaterObjects++;
        }
        else
        {
            mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalLand_encodingRuleId);
            fullLandObjects++;
        }
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
    }

    if(fullWaterObjects == 0u && !coastlineCrossesBounds)
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "Isolated islands found during polygonization of coastlines in area [%d, %d, %d, %d]@%d",
            context._area31.top,
            context._area31.left,
            context._area31.bottom,
            context._area31.right,
            context._zoom);

        // Add complete water tile
        const std::shared_ptr<Model::MapObject> mapObject(new Model::MapObject(env.dummyMapSection, nullptr));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.top)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.right, context._area31.bottom)));
        mapObject->_points31.push_back(qMove(PointI(context._area31.left, context._area31.bottom)));
        mapObject->_points31.push_back(mapObject->_points31.first());

        mapObject->_typesRuleIds.push_back(mapObject->section->encodingDecodingRules->naturalCoastline_encodingRuleId);
        mapObject->_id = osmId;
        mapObject->_isArea = true;

        assert(mapObject->isClosedFigure());
        outVectorized.push_back(qMove(mapObject));
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
    auto alignedArea31 = context._area31;
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

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
    const auto& px = p0.x;
    const auto& py = p0.y;
    const auto& x = p1.x;
    const auto& y = p1.y;
    const auto& leftX = bbox.left;
    const auto& rightX = bbox.right;
    const auto& topY = bbox.top;
    const auto& bottomY = bbox.bottom;

    // firstly try to search if the line goes in
    if (py < topY && y >= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py > bottomY && y <= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px < leftX && x >= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px > rightX && x <= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;//b.first = rightX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }

    // try to search if point goes out
    if (py > topY && y <= topY) {
        int tx = (int) (px + ((double) (x - px) * (topY - py)) / (y - py));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = topY;//b.second = topY;
            return true;
        }
    }
    if (py < bottomY && y >= bottomY) {
        int tx = (int) (px + ((double) (x - px) * (py - bottomY)) / (py - y));
        if (leftX <= tx && tx <= rightX) {
            pX.x = tx;//b.first = tx;
            pX.y = bottomY;//b.second = bottomY;
            return true;
        }
    }
    if (px > leftX && x <= leftX) {
        int ty = (int) (py + ((double) (y - py) * (leftX - px)) / (x - px));
        if (ty >= topY && ty <= bottomY) {
            pX.x = leftX;//b.first = leftX;
            pX.y = ty;//b.second = ty;
            return true;
        }

    }
    if (px < rightX && x >= rightX) {
        int ty = (int) (py + ((double) (y - py) * (px - rightX)) / (px - x));
        if (ty >= topY && ty <= bottomY) {
            pX.x = rightX;//b.first = rightX;
            pX.y = ty;//b.second = ty;
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
    if(polyline.isEmpty())
        return;

    if(polyline.first() == polyline.last())
    {
        closedPolygons.push_back(polyline);
        return;
    }

    bool add = true;

    for(auto itPolygon = coastlinePolylines.begin(); itPolygon != coastlinePolylines.end();)
    {
        auto& polygon = *itPolygon;

        bool remove = false;

        if(polyline.first() == polygon.last())
        {
            polygon.reserve(polygon.size() + polyline.size() - 1);
            polygon.pop_back();
            polygon += polyline;
            remove = true;
            polyline = polygon;
        }
        else if(polyline.last() == polygon.first())
        {
            polyline.reserve(polyline.size() + polygon.size() - 1);
            polyline.pop_back();
            polyline += polygon;
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

        if (polyline.first() == polyline.last())
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

void MapRasterizerContext::convertCoastlinePolylinesToPolygons(
    const MapRasterizerProvider& env,
    std::list< std::vector< pointI > >& coastlinePolylines, std::list< std::vector< pointI > >& coastlinePolygons, uint64_t osmId )
{
    // Align area to 32: this fixes coastlines and specifically Antarctica
    auto alignedArea31 = _area31;
    alignedArea31.top &= ~((1u << 5) - 1);
    alignedArea31.left &= ~((1u << 5) - 1);
    alignedArea31.bottom &= ~((1u << 5) - 1);
    alignedArea31.right &= ~((1u << 5) - 1);

    std::list< std::vector< pointI > > validPolylines;

    // Check if polylines has been cut by rasterization viewport
    QMutableListIterator< QVector< PointI > > itPolyline(coastlinePolylines);
    while(itPolyline.hasNext())
    {
        const auto& polyline = itPolyline.next();
        assert(!polyline.isEmpty());

        const auto& head = polyline.first();
        const auto& tail = polyline.last();

        // This curve has not been cut by rasterization viewport, so it's
        // impossible to fix it
        if (!alignedArea31.isOnEdge(head) || !alignedArea31.isOnEdge(tail))
            continue;

        validPolylines.push_back(polyline);
        itPolyline.remove();
    }

    std::set< QList< QVector< PointI > >::iterator > processedPolylines;
    while(processedPolylines.size() != validPolylines.size())
    {
        for(auto itPolyline = validPolylines.begin(); itPolyline != validPolylines.end(); ++itPolyline)
        {
            // If this polyline was already processed, skip it
            if(processedPolylines.find(itPolyline) != processedPolylines.end())
                continue;

            // Start from tail of the polyline and search for it's continuation in CCV order
            auto& polyline = *itPolyline;
            const auto& tail = polyline.last();
            auto tailEdge = AreaI::Edge::Invalid;
            alignedArea31.isOnEdge(tail, &tailEdge);
            auto itNearestPolyline = validPolylines.end();
            auto firstIteration = true;
            for(int idx = static_cast<int>(tailEdge) + 4; (idx >= static_cast<int>(tailEdge)) && (itNearestPolyline == validPolylines.end()); idx--, firstIteration = false)
            {
                const auto currentEdge = static_cast<AreaI::Edge>(idx % 4);

                for(auto itOtherPolyline = validPolylines.begin(); itOtherPolyline != validPolylines.end(); ++itOtherPolyline)
                {
                    // If this polyline was already processed, skip it
                    if(processedPolylines.find(itOtherPolyline) != processedPolylines.end())
                        continue;

                    // Skip polylines that are on other edges
                    const auto& otherHead = itOtherPolyline->first();
                    auto otherHeadEdge = AreaI::Edge::Invalid;
                    alignedArea31.isOnEdge(otherHead, &otherHeadEdge);
                    if(otherHeadEdge != currentEdge)
                        continue;

                    // Skip polyline that is not next in CCV order
                    if(firstIteration)
                    {
                        bool isNextByCCV = false;
                        if(currentEdge == AreaI::Edge::Top)
                            isNextByCCV = (otherHead.x <= tail.x);
                        else if(currentEdge == AreaI::Edge::Right)
                            isNextByCCV = (otherHead.y <= tail.y);
                        else if(currentEdge == AreaI::Edge::Bottom)
                            isNextByCCV = (tail.x <= otherHead.x);
                        else if(currentEdge == AreaI::Edge::Left)
                            isNextByCCV = (tail.y <= otherHead.y);
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
                    const auto& previouslySelectedHead = itNearestPolyline->first();
                    bool isCloserByCCV = false;
                    if(currentEdge == AreaI::Edge::Top)
                        isCloserByCCV = (otherHead.x > previouslySelectedHead.x);
                    else if(currentEdge == AreaI::Edge::Right)
                        isCloserByCCV = (otherHead.y > previouslySelectedHead.y);
                    else if(currentEdge == AreaI::Edge::Bottom)
                        isCloserByCCV = (otherHead.x < previouslySelectedHead.x);
                    else if(currentEdge == AreaI::Edge::Left)
                        isCloserByCCV = (otherHead.y < previouslySelectedHead.y);

                    // If closer-by-CCV, then select this
                    if(isCloserByCCV)
                        itNearestPolyline = itOtherPolyline;
                }
            }
            assert(itNearestPolyline != validPolylines.cend());

            // Get edge of nearest-by-CCV head
            auto nearestHeadEdge = AreaI::Edge::Invalid;
            const auto& nearestHead = itNearestPolyline->first();
            alignedArea31.isOnEdge(nearestHead, &nearestHeadEdge);

            // Fill by edges of area, if required
            int loopShift = 0;
            if( static_cast<int>(tailEdge) - static_cast<int>(nearestHeadEdge) < 0 )
                loopShift = 4;
            else if(tailEdge == nearestHeadEdge)
            {
                bool skipAddingSides = false;
                if(tailEdge == AreaI::Edge::Top)
                    skipAddingSides = (tail.x >= nearestHead.x);
                else if(tailEdge == AreaI::Edge::Right)
                    skipAddingSides = (tail.y >= nearestHead.y);
                else if(tailEdge == AreaI::Edge::Bottom)
                    skipAddingSides = (tail.x <= nearestHead.x);
                else if(tailEdge == AreaI::Edge::Left)
                    skipAddingSides = (tail.y <= nearestHead.y);

                if(!skipAddingSides)
                    loopShift = 4;
            }
            for(int idx = static_cast<int>(tailEdge) + loopShift; idx > static_cast<int>(nearestHeadEdge); idx--)
            {
                const auto side = static_cast<AreaI::Edge>(idx % 4);
                PointI p;

                if(side == AreaI::Edge::Top)
                {
                    p.y = alignedArea31.top;
                    p.x = alignedArea31.left;
                }
                else if(side == AreaI::Edge::Right)
                {
                    p.y = alignedArea31.top;
                    p.x = alignedArea31.right;
                }
                else if(side == AreaI::Edge::Bottom)
                {
                    p.y = alignedArea31.bottom;
                    p.x = alignedArea31.right;
                }
                else if(side == AreaI::Edge::Left)
                {
                    p.y = alignedArea31.bottom;
                    p.x = alignedArea31.left;
                }

                polyline.push_back(qMove(p));
            }

            // If nearest-by-CCV is head of current polyline, cap it and add to polygons, ...
            if(itNearestPolyline == itPolyline)
            {
                polyline.push_back(polyline.first());
                coastlinePolygons.push_back(polyline);
            }
            // ... otherwise join them. Joined will never be visited, and current will remain unmarked as processed
            else
            {
                const auto& otherPolyline = *itNearestPolyline;
                polyline << otherPolyline;
            }

            // After we've selected nearest-by-CCV polyline, mark it as processed
            processedPolylines.insert(itNearestPolyline);
        }
    }
}

bool MapRasterizerContext::isClockwiseCoastlinePolygon( const std::vector< pointI > & polygon )
{
    if(polygon.isEmpty())
        return true;

    // calculate middle Y
    int64_t middleY = 0;
    for(auto itVertex = polygon.cbegin(); itVertex != polygon.cend(); ++itVertex)
        middleY += itVertex->y;
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
        if(!Utilities::rayIntersectX(vertex0, vertex1, middleY, rX))
            continue;

        bool skipSameSide = (vertex1.y <= middleY) == (vertex0.y <= middleY);
        if (skipSameSide)
            continue;

        bool directionUp = vertex0.y >= middleY;
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
            clockwiseSum += qAbs(previousX - firstX);
        } else {
            clockwiseSum -= qAbs(previousX - firstX);
        }
    }

    return clockwiseSum >= 0;
}
