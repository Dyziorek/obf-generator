#include "stdafx.h"

#include "SkCanvas.h"
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
	_context = std::shared_ptr<MapRasterizerContext>(new MapRasterizerContext());
	
	
}

void MapRasterizer::DrawMap(SkCanvas& canvas)
{

}

