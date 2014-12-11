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
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"

namespace wfl = google::protobuf::internal;
using namespace google::protobuf;
using namespace OsmAnd::OBF;


BinaryIndexDataReader::BinaryIndexDataReader(boost::filesystem::path& pathInfo) 
{
	fileReader.reset(new RandomAccessFileReader(pathInfo));
	strmData.reset(new gio::CodedInputStream(fileReader.get()));

	 bool loadedCorrectly = false;
	uint32 versionID;
	uint32 confirmVersionID;
	uint64 dateTime;
	uint32 tagCode;
	for(;;)
	{
		tagCode = strmData->ReadTag();
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
			strmData->ReadVarint32(&versionID);
			break;
		case OsmAndStructure::kDateCreatedFieldNumber:
			strmData->ReadVarint64(&dateTime);
			break;
		case OsmAndStructure::kVersionConfirmFieldNumber:
			strmData->ReadVarint32(&confirmVersionID);
			if (confirmVersionID == versionID)
				loadedCorrectly = true;
			break;
		case OsmAndStructure::kMapIndexFieldNumber:
			ReadMapData(strmData.get());
			break;
		case OsmAndStructure::kRoutingIndexFieldNumber:
			ReadRouteData(strmData.get());
			break;
		case OsmAndStructure::kAddressIndexFieldNumber:
			ReadAddresIndex(strmData.get());
			break;
		default:
			BinaryReaderUtils::skipUnknownField(strmData.get(), tagCode);
			break;
		}
	}
}


BinaryIndexDataReader::~BinaryIndexDataReader(void)
{
	strmData.reset();
	fileReader.reset();
}





void BinaryIndexDataReader::ReadMapData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	reader.ReadMapDataSection(cis);
	//reader.PaintSections();
	cis->PopLimit(oldLimit);
	

}

void BinaryIndexDataReader::ReadAddresIndex(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	addresser.ReadMapAddresses(cis);
	cis->PopLimit(oldLimit);
	cis->Seek(offset + limitValue);
}

void BinaryIndexDataReader::ReadRouteData(google::protobuf::io::CodedInputStream* cis)
{
	int limitValue = BinaryReaderUtils::readBigEndianInt(cis);
	int offset = cis->CurrentPosition();
	int oldLimit = cis->PushLimit(limitValue);
	router.ReadRouteInfo(cis);
	cis->PopLimit(oldLimit);
	cis->Seek(offset + limitValue);
}



void BinaryIndexDataReader::getMapObjects(boxI& areaCheck, int zoom, std::list<std::shared_ptr<const MapObjectData>>& outList)
{
	reader.loadMapDataObjects(strmData.get(), areaCheck,zoom, outList);
}