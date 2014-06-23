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
#include "BinaryIndexDataReader.h"

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
		default:
			skipUnknownField(&strmData, tagCode);
			break;
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
       // auto length = readBigEndianInt(cis);
       // cis->Skip(length);
    }
    else
        internal::WireFormatLite::SkipField(cis, tag);
}