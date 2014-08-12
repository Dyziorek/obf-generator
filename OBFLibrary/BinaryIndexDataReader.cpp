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
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"


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
		case OsmAndStructure::kRoutingIndexFieldNumber:
			ReadRouteData(&strmData);
			break;
		case OsmAndStructure::kAddressIndexFieldNumber:
			ReadAddresIndex(&strmData);
			break;
		default:
			BinaryReaderUtils::skipUnknownField(&strmData, tagCode);
			break;
		}
	}
}


BinaryIndexDataReader::~BinaryIndexDataReader(void)
{
}





void BinaryIndexDataReader::ReadMapData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	reader.ReadMapDataSection(cis, rad);
	reader.PaintSections();
	cis->PopLimit(oldLimit);
	

}

void BinaryIndexDataReader::ReadAddresIndex(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	addresser.ReadMapAddresses(cis, rad);
	cis->PopLimit(oldLimit);
	cis->Seek(offset + limitValue);
}

void BinaryIndexDataReader::ReadRouteData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	router.ReadRouteInfo(cis, rad);
	cis->PopLimit(oldLimit);
	cis->Seek(offset + limitValue);
}



