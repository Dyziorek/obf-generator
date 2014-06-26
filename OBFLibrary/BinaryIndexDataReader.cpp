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
#include "ArchiveIO.h"
#include "Street.h"
#include "RandomAccessFileReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>

using namespace google::protobuf;
using namespace obf;



BinaryIndexDataReader::BinaryIndexDataReader(RandomAccessFileReader* outData) :strmData(outData)
{

	rad = outData;
	 bool loadedCorrectly = false;
	uint32 versionID;
	uint32 confirmVersionID;
	uint64 dateTime;
	uint32 tagCode;
	for(;;)
	{
		tagCode = strmData.ReadTag();
		switch (wfl::WireFormatLite::GetTagFieldNumber(tagCode))
		{
		case 0:
			// End of file mark
			if (loadedCorrectly)
			{
				std::wstringstream strm;
			strm << L"Whole map read OK:\r\n";
			OutputDebugString(strm.str().c_str());
			}
			else
			{
				std::wstringstream strm;
				strm << L"Whole map read WRONG:\r\n";
				OutputDebugString(strm.str().c_str());
			}
			return;
		case OsmAndStructure::kVersionFieldNumber:
			strmData.ReadVarint32(&versionID);
			break;
		case OsmAndStructure::kDateCreatedFieldNumber:
			strmData.ReadVarint64(&dateTime);
			break;
		case OsmAndStructure::kVersionConfirmFieldNumber:
			strmData.ReadVarint32(&confirmVersionID);
			if (confirmVersionID == versionID)
				loadedCorrectly = true;
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


void BinaryIndexDataReader::skipUnknownField( gio::CodedInputStream* cis, int tag )
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

uint32 BinaryIndexDataReader::readBigEndianInt(gio::CodedInputStream* cis )
{
    uint32 be;
    cis->ReadRaw(&be, sizeof(be));
 #ifndef BOOST_BIG_ENDIAN
	reverse_bytes(sizeof(be),(char*)&be);
 #endif
	return be;
}
