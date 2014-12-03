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
	for (const std::shared_ptr<MapObjectData>& mapItem : mapDatum)
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

	if(!detailedmapCoastlineObjects.empty())
    {
        const bool coastlinesWereAdded = polygonizeCoastlines(env, context,
            detailedmapCoastlineObjects,
            polygonizedCoastlineObjects,
            !basemapCoastlineObjects.isEmpty(),
            true);
        fillEntireArea = !coastlinesWereAdded && fillEntireArea;
        addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || zoom <= static_cast<ZoomLevel>(BasemapZoom);
    }

	_context = std::shared_ptr<MapRasterizerContext>(new MapRasterizerContext());
	_source.obtainMapPrimitives(mapDatum, workZoom, _context);
	_context->sortGraphicElements();

}



void MapRasterizer::DrawMap(SkCanvas& canvas)
{
	

}

