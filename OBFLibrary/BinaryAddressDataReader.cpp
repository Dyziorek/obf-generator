#include "stdafx.h"
#include "OBFRenderingTypes.h"
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

#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"


#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryAddressDataReader.h"
#include "BinaryMapDataReader.h"
#include "BinaryIndexDataReader.h"

using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;

BinaryAddressDataReader::BinaryAddressDataReader(void)
{
}


BinaryAddressDataReader::~BinaryAddressDataReader(void)
{
}

void BinaryAddressDataReader::ReadMapAddresses(gio::CodedInputStream* cis, RandomAccessFileReader* outData)
{
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);

		switch(tagVal)
		{
		case 0:
			return;
		case OsmAndAddressIndex::kNameFieldNumber:
			BinaryIndexDataReader::readString(cis, name);
			break;
		case OsmAndAddressIndex::kNameEnFieldNumber:
			BinaryIndexDataReader::readString(cis, enName);
			break;
		case OsmAndAddressIndex::kCitiesFieldNumber:
			{
				auto currPos = cis->CurrentPosition();
				uint32_t cityLen = BinaryIndexDataReader::readBigEndianInt(cis);
				std::shared_ptr<MapAddresBlock> block = std::shared_ptr<MapAddresBlock>(new MapAddresBlock);
				block->_length = cityLen;
				block->_offset = currPos;
				blocksInfo.push_back(block);
				readCityInfo(cis, block);
				cis->Seek(block->_offset + block->_length);
			}
			break;
		case OsmAndAddressIndex::kNameIndexFieldNumber:
			auto indexNameOffset = cis->CurrentPosition();
            auto length = BinaryIndexDataReader::readBigEndianInt(cis);
            cis->Seek(indexNameOffset + length + 4);
			break;
		}
	}
}

void BinaryAddressDataReader::readCityInfo(gio::CodedInputStream* cis, std::shared_ptr<MapAddresBlock>& block)
{
	for(;;)
    {
        auto tag = cis->ReadTag();
        switch(WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndAddressIndex_CitiesIndex::kTypeFieldNumber:
            cis->ReadVarint32(reinterpret_cast<uint32_t*>(&block->_type));
            return;
        default:
            BinaryIndexDataReader::skipUnknownField(cis, tag);
            break;
        }
    }
}