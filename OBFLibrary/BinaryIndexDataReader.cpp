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

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
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
		case OsmAndStructure::kMapIndexFieldNumber:
			ReadMapData(&strmData);
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

void BinaryIndexDataReader::ReadMapData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	reader.ReadMapDataSection(cis);

	reader.PaintSections();
	cis->PopLimit(oldLimit);
	

}

bool BinaryIndexDataReader::readSInt32( gio::CodedInputStream* cis, int32_t& output )
{
	uint32_t newVal;
	bool returnData;
	returnData = cis->ReadVarint32(&newVal);
	output = wfl::WireFormatLite::ZigZagDecode32(newVal);
	return returnData;
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

