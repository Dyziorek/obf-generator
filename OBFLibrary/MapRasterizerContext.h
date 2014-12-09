#pragma once

#include <future>


namespace std
{
	template<>
	struct hash< std::list< std::vector< pointI > >::iterator >
	{
		std::size_t operator()(const std::list< std::vector< pointI > >::iterator& hashWork) const
		{
			size_t seed;
			std::list< std::vector< pointI > >::iterator& objCon = const_cast<std::list< std::vector< pointI > >::iterator&>(hashWork);
			boost::hash_combine(seed, objCon->size());
			for (pointI pt : *objCon)
			{
				boost::hash_combine(seed, pt.get<0>());
				boost::hash_combine(seed, pt.get<1>());
			}
			return seed;
		}
	};
}

class MapRasterizerContext
{
public:
	MapRasterizerContext(void);
	~MapRasterizerContext(void);

	std::vector<std::shared_ptr<MapRasterizer::GraphicElementGroup>> _graphicElements;
	std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>> symbols;
	std::vector<std::shared_ptr<MapRasterizer::GraphicElement>> _polygons, _polyLines, _points;
	AreaI _area31;
	double _tileScale;
	pointD _pixelScaleXY;
	int zoom;
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

