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

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryAddressDataReader.h"
#include "BinaryReaderUtils.h"

using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;

BinaryAddressDataReader::BinaryAddressDataReader(void)
{
}


BinaryAddressDataReader::~BinaryAddressDataReader(void)
{
}

void BinaryAddressDataReader::ReadMapAddresses(gio::CodedInputStream* cis)
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
			BinaryReaderUtils::readString(cis, name);
			break;
		case OsmAndAddressIndex::kNameEnFieldNumber:
			BinaryReaderUtils::readString(cis, enName);
			break;
		case OsmAndAddressIndex::kCitiesFieldNumber:
			{
				auto currPos = cis->CurrentPosition();
				uint32_t cityLen = BinaryReaderUtils::readBigEndianInt(cis);
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
            auto length = BinaryReaderUtils::readBigEndianInt(cis);
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
            BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }
}