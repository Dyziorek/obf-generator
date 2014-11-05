#include "stdafx.h"

#include "SkCanvas.h"
#include <google\protobuf\io\coded_stream.h>
#include <boost/filesystem.hpp>
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"
#include "MapRasterizerProvider.h"
#include "MapRasterizer.h"

MapRasterizer::MapRasterizer(MapRasterizerProvider& context) : _context(context)
{
}



MapRasterizer::~MapRasterizer(void)
{
}


void MapRasterizer::DrawMap(SkCanvas& canvas)
{

}