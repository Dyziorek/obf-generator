#include "stdafx.h"
#include <google\protobuf\descriptor.h>
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "Amenity.h"
#include "Street.h"
#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryIndexDataReader.h"
#include "ArchiveIO.h"



using namespace google::protobuf;
using namespace obf;

BinaryIndexDataReader::BinaryIndexDataReader(RandomAccessFileReader* outData) :strmData(outData)
{
	rad = outData;
	 
	uint32 versionID;
	uint32 confirmVersionID;
	uint64 dateTime;
	uint32 tagCode;
	for(;;)
	{
		tagCode = strmData.ReadTag();
		switch (wfl::WireFormatLite::GetTagFieldNumber(tagCode))
		{
		case OsmAndStructure::kVersionFieldNumber:
			strmData.ReadVarint32(&versionID);
			break;
		case OsmAndStructure::kDateCreatedFieldNumber:
			strmData.ReadVarint64(&dateTime);
			break;
		case OsmAndStructure::kVersionConfirmFieldNumber:
			strmData.ReadVarint32(&confirmVersionID);
			break;
		case OsmAndStructure::kMapIndexFieldNumber:
			ReadMapData(&strmData);
			break;
		default:
			skipUnknownField(&strmData, tagCode);
			break;
		case 0:
			return;
		}
	}
}


BinaryIndexDataReader::~BinaryIndexDataReader(void)
{
}


void BinaryIndexDataReader::skipUnknownField( google::protobuf::io::CodedInputStream* cis, int tag )
{
    auto wireType = internal::WireFormatLite::GetTagWireType(tag);
    if(wireType == internal::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED)
    {
        auto length = readBigEndianInt(cis);
        cis->Skip(length);
    }
    else
        internal::WireFormatLite::SkipField(cis, tag);
}


int BinaryIndexDataReader::readBigEndianInt( google::protobuf::io::CodedInputStream* cis)
{
	google::protobuf::uint32 be;
    cis->ReadRaw(&be, sizeof(be));
	#ifndef BOOST_BIG_ENDIAN
		reverse_bytes(sizeof(be), (char*)&be);
	#endif
    return be;
}

void BinaryIndexDataReader::ReadMapData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	reader.ReadMapDataSection(cis);

	cis->PopLimit(oldLimit);
	

}

bool BinaryIndexDataReader::readString( gio::CodedInputStream* cis, std::string& output )
{
    std::string value;
    if(!internal::WireFormatLite::ReadString(cis, &value))
        return false;

    output = value;
    return true;
}

void BinaryIndexDataReader::readStringTable( gio::CodedInputStream* cis, std::list<std::string>& stringTableOut )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
		case StringTable::kSFieldNumber:
            {
                std::string value;
                if(readString(cis, value))
                    stringTableOut.push_back(std::move(value));
            }
            break;
        default:
            skipUnknownField(cis, tag);
            break;
        }
    }
}
