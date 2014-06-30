#pragma once

#include "RTree.h"
#include "MapObjectData.h"
namespace gp = google::protobuf;
namespace gio = google::protobuf::io;

typedef RTree<std::pair<__int64, std::vector<std::shared_ptr<MapObjectData>>>> treeMap;

struct BinaryMapSection
{
	std::pair<gp::uint32, gp::uint32> zoomLevels;
	treeMap::box rootBox;
	gp::uint32 offset;
	treeMap mapDataTree;
};


class BinaryMapDataReader
{
public:


	BinaryMapDataReader(void);
	~BinaryMapDataReader(void);

	
	void ReadMapDataSection(gio::CodedInputStream* cis);
	void readMapLevelHeader(gio::CodedInputStream* cis,  std::shared_ptr<BinaryMapSection> section, int offset);

	

private:
	std::vector<std::tuple<treeMap::box, std::pair<gp::uint32, gp::uint32> ,std::shared_ptr<BinaryMapSection>>> sections;
	std::string mapName;

	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BinaryMapDataReader);
};

