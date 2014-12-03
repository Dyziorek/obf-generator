#pragma once

#include <future>

class MapRasterizerContext
{
public:
	MapRasterizerContext(void);
	~MapRasterizerContext(void);

	std::vector<std::shared_ptr<GraphicElementGroup>> _graphicElements;
	std::vector<std::shared_ptr<RasterSymbolGroup>> symbols;
	std::vector<std::shared_ptr<GraphicElement>> _polygons, _polyLines, _points;
	void sortGraphicElements();
	bool polygonizeCoastlines( const MapRasterizerProvider& env, const std::list< std::shared_ptr<const MapObjectData> >& coastlines,
	std::list< std::shared_ptr<const MapObjectData> >& outVectorized,   bool abortIfBrokenCoastlinesExist,  bool includeBrokenCoastlines );
	bool buildCoastlinePolygonSegment(  const MapRasterizerProvider& env,   bool currentInside,   const pointI& currentPoint31,   bool prevInside,
    const pointI& previousPoint31,   std::vector< pointI >& segmentPoints );
	bool calculateIntersection( const pointI& p1, const pointI& p0, const AreaI& bbox, pointI& pX );
	void appendCoastlinePolygons( std::list< std::vector< pointI > >& closedPolygons, std::list< std::vector< pointI > >& coastlinePolylines, std::vector< pointI > & polyline );
	
void convertCoastlinePolylinesToPolygons(
    const MapRasterizerProvider& env,
    std::list< std::vector< pointI > >& coastlinePolylines, std::list< std::vector< pointI > >& coastlinePolygons, uint64_t osmId );
	bool isClockwiseCoastlinePolygon( const std::vector< pointI > & polygon );

};

