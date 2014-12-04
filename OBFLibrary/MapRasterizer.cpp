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
	_source.obtainMapPrimitives(mapDatum, workZoom, _context);
	_context->sortGraphicElements();

}



void MapRasterizer::DrawMap(SkCanvas& canvas)
{
	

}

