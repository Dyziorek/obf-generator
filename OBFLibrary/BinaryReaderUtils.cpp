#include "stdafx.h"
#include <google\protobuf\descriptor.h>
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include "BinaryReaderUtils.h"
#include "ArchiveIO.h"
#include "..\..\..\..\core\protos\OBF.pb.h"

using namespace google::protobuf;


BinaryReaderUtils::BinaryReaderUtils(void)
{
}


BinaryReaderUtils::~BinaryReaderUtils(void)
{
}

void BinaryReaderUtils::skipUnknownField( gio::CodedInputStream* cis, int tag )
{
    auto wireType = wfl::WireFormatLite::GetTagWireType(tag);
    if(wireType == wfl::WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED)
    {
        auto length = readBigEndianInt(cis);
        cis->Skip(length);
    }
    else
        wfl::WireFormatLite::SkipField(cis, tag);
}

uint32 BinaryReaderUtils::readBigEndianInt(gio::CodedInputStream* cis )
{
    uint32 be;
    cis->ReadRaw(&be, sizeof(be));
 #ifndef BOOST_BIG_ENDIAN
	reverse_bytes(sizeof(be),(char*)&be);
 #endif
	return be;
}

bool BinaryReaderUtils::readSInt32( gio::CodedInputStream* cis, int32_t& output )
{
	uint32_t newVal;
	bool returnData;
	returnData = cis->ReadVarint32(&newVal);
	output = wfl::WireFormatLite::ZigZagDecode32(newVal);
	return returnData;
}

int64_t BinaryReaderUtils::readSInt64( gio::CodedInputStream* cis )
{
	uint64_t newVal;
	int64_t result = -1LL;
	if(cis->ReadVarint64(&newVal))
		result = wfl::WireFormatLite::ZigZagDecode64(newVal);
	return result;
}


bool BinaryReaderUtils::readString( gio::CodedInputStream* cis, std::string& output )
{
    std::string value;
    if(!internal::WireFormatLite::ReadString(cis, &value))
        return false;

    output = value;
    return true;
}

void BinaryReaderUtils::readStringTable( gio::CodedInputStream* cis, std::vector<std::string>& stringTableOut )
{
    for(;;)
    {
        auto tag = cis->ReadTag();
        switch(internal::WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
		case OsmAnd::OBF::StringTable::kSFieldNumber:
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

