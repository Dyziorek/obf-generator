#include "stdafx.h"
#include "stdafx.h"
#include <google\protobuf\descriptor.h>
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "Amenity.h"
#include "Street.h"
#include <boost\thread.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <ios>
#include <sstream>
#include <sys/stat.h>
#include "RTree.h"
#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryIndexDataReader.h"
#include "ArchiveIO.h"


using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;


BinaryMapDataReader::BinaryMapDataReader(void)
{
}


BinaryMapDataReader::~BinaryMapDataReader(void)
{
}


void BinaryMapDataReader::ReadMapDataSection(gio::CodedInputStream* cis)
{
	while(true)
	{
		int tag = cis->ReadTag();
		switch (WireFormatLite::GetTagFieldNumber(tag))
		{
		case 0:
			return;
		case OsmAndMapIndex::kNameFieldNumber:
			WireFormatLite::ReadString(cis, &mapName);
			break;
		case OsmAndMapIndex::kLevelsFieldNumber:
			{
			auto length = BinaryIndexDataReader::readBigEndianInt(cis);
            auto offset = cis->CurrentPosition();
            auto oldLimit = cis->PushLimit(length);
			std::shared_ptr<BinaryMapSection> section(new BinaryMapSection());
            readMapLevelHeader(cis, section, offset);
			sections.push_back(std::make_tuple(section->rootBox, section->zoomLevels, section));
            cis->PopLimit(oldLimit);

			}
			break;
		case OsmAndMapIndex::kRulesFieldNumber:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		}

	}

	//boost::promise<int> nextData;

	//boost::shared_future<int> dataLoad;
	//boost::async(
	//dataLoad.get();
}

 void BinaryMapDataReader::readMapLevelHeader(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection> section, int parentoffset)
 {
	
	for(;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
		gp::uint32 BoxVal;
        switch(wfl::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&section->zoomLevels.first));
            break;
        case OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&section->zoomLevels.second));
            break;
        case OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.min_corner().set<0>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.max_corner().set<0>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.min_corner().set<1>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.max_corner().set<1>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                // Save boxes offset
                section->offset = tagPos - parentoffset;

                // Skip reading boxes and surely, following blocks
                cis->Skip(cis->BytesUntilLimit());
            }
            return;
        default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
            break;
        }
    }
 }

