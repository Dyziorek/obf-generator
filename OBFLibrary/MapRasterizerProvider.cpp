#include "stdafx.h"
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


namespace bf = boost::filesystem;

MapRasterizerProvider::MapRasterizerProvider(void)
{
	workingStyle.reset(new MapStyleInfo());
	workingStyle->loadRenderStyles(nullptr);

}


MapRasterizerProvider::~MapRasterizerProvider(void)
{
}

void MapRasterizerProvider::obtainMaps(const char* path)
{
	bf::path& fileInfo = bf::path(std::string(path));

	if ( bf::is_regular_file(fileInfo))
	{
		mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(fileInfo)));
	}
	else if (bf::is_directory(fileInfo))
	{
		bf::directory_iterator dirEntry = bf::directory_iterator(fileInfo);
		std::vector<bf::directory_entry> entries;
		std::for_each(dirEntry, bf::directory_iterator(), [&entries](bf::directory_entry input)
		{
			auto checker = input;
			if (checker.path().extension().generic_string() == std::string("pbf"))
			{
				entries.push_back(input);
			}
		});
		for (auto mapPath : entries)
		{
			auto& pathData = (bf::path&)mapPath.path();
			mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(pathData)));
		}
		
	}

}

std::list<std::shared_ptr<MapObjectData>> MapRasterizerProvider::obtainMapData(boxI& areaI, int zoom)
{
	std::list<std::shared_ptr<MapObjectData>> outListData;

	for (auto& mapData : mapProviders)
	{
		mapData->getMapObjects(areaI, zoom, outListData);

	}

	return outListData;
}


boxI MapRasterizerProvider::getWholeBox()
{
	boxI baseBox;
	bg::assign_inverse(baseBox);

	for (auto& mapDataItem : mapProviders)
	{
		for (auto& sectData : mapDataItem->GetReader().getSections())
		{
			bg::expand(baseBox, std::get<0>(sectData));
		}
	}

	return baseBox;
}