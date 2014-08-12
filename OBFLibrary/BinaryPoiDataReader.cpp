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
#include "BinaryReaderUtils.h"
#include "BinaryPoiDataReader.h"


using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;


BinaryPoiDataReader::BinaryPoiDataReader(void)
{
}


BinaryPoiDataReader::~BinaryPoiDataReader(void)
{
}


void BinaryPoiDataReader::ReadPoiDataInfo(gio::CodedInputStream* cis, RandomAccessFileReader* outData)
{
	_offset = cis->CurrentPosition();
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
		case 0:
			return;
		case OsmAndPoiIndex::kNameFieldNumber:
			BinaryReaderUtils::readString(cis, poiName);
			break;
		case OsmAndPoiIndex::kBoundariesFieldNumber:
			{
			uint32_t bboxlen;
			cis->ReadVarint32(&bboxlen);
			uint32_t oldLimit = cis->PushLimit(bboxlen);
			ReadBoundsBox(cis);
			cis->PopLimit(oldLimit);
			break;
			}
		case OsmAndPoiIndex::kCategoriesTableFieldNumber:
			{
				uint32_t catLen;
				cis->ReadVarint32(&catLen);
				uint32_t oldLimit = cis->PushLimit(catLen);
				ReadCategories(cis);
				cis->PopLimit(oldLimit);
			}
			break;
		case OsmAndPoiIndex::kNameIndexFieldNumber:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}
	}
}

void BinaryPoiDataReader::ReadBoundsBox(gio::CodedInputStream* cis)
{
	short numReads = 0;
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
		case 0:
			if(numReads != 4)
			{
#ifdef _DEBUG
				std::wstringstream strmData;
				strmData << L"Incomplete read BBox - read count: " << numReads << std::endl;
				OutputDebugString(strmData.str().data());
#endif
			}
			return;
		case OsmAndTileBox::kBottomFieldNumber:
			{
				uint32_t boxVal;
				cis->ReadVarint32(&boxVal);
				poiBBox.min_corner().set<1>(boxVal);
				numReads++;
			}
			break;
		case OsmAndTileBox::kTopFieldNumber:
			{
				uint32_t boxVal;
				cis->ReadVarint32(&boxVal);
				poiBBox.max_corner().set<1>(boxVal);
				numReads++;
			}
			break;
		case OsmAndTileBox::kLeftFieldNumber:
			{
				uint32_t boxVal;
				cis->ReadVarint32(&boxVal);
				poiBBox.min_corner().set<0>(boxVal);
				numReads++;
			}
			break;
		case OsmAndTileBox::kRightFieldNumber:
			{
				uint32_t boxVal;
				cis->ReadVarint32(&boxVal);
				poiBBox.max_corner().set<0>(boxVal);
				numReads++;
			}
			break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}
	}
}

void BinaryPoiDataReader::ReadCategories(gio::CodedInputStream* cis)
{
	AmenityCategory amCat;
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
		case OsmAndCategoryTable::kCategoryFieldNumber:
			{
				BinaryReaderUtils::readString(cis, amCat.name);
			}
			break;
		case OsmAndCategoryTable::kSubcategoriesFieldNumber:
			{
				std::string subName;
				BinaryReaderUtils::readString(cis, subName);
				amCat.subNames.push_back(subName);
			}
			break;
		case 0:
			categories.push_back(amCat);
			return;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}
	}
}

void BinaryPoiDataReader::ReadAmenities(gio::CodedInputStream* cis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth)
{
	uint32_t catLen;
	cis->ReadVarint32(&catLen);
	uint32_t oldLimit = cis->PushLimit(catLen);
	ReadAmenity(cis, amCom, bbox, neededCategories, zoom, zoomDepth);
	cis->PopLimit(oldLimit);
}

void BinaryPoiDataReader::ReadAmenity(gio::CodedInputStream* cis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth)
{
	std::vector<std::shared_ptr<AmenityPoint>> loadedPoi;
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
		case OsmAndPoiIndex::kBoxesFieldNumber:
			{
			uint32_t catLen;
			cis->ReadVarint32(&catLen);
			uint32_t oldLimit = cis->PushLimit(catLen);
			ReadTileBox(cis, bbox, neededCategories, zoom, zoomDepth, loadedPoi, nullptr, nullptr);
			cis->PopLimit(oldLimit);
			}
			break;
		case OsmAndPoiIndex::kPoiDataFieldNumber:
			{
				std::sort(loadedPoi.begin(), loadedPoi.end(), [](const std::shared_ptr<AmenityPoint>& r, const std::shared_ptr<AmenityPoint>l) ->
				bool { return r->offset < l->offset; }
				);
				for (auto itPoi = loadedPoi.begin(); itPoi != loadedPoi.end(); itPoi++)
				{
					const std::shared_ptr<AmenityPoint> workData = *itPoi;
					cis->Skip(_offset + workData->offset);
					ReadAmenitiesData(cis, amCom, bbox, neededCategories, zoom, zoomDepth);
				}
			}
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}

	}
}

bool BinaryPoiDataReader::ReadTileBox(gio::CodedInputStream* cis, std::shared_ptr<AreaI>& bbox31, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth, std::vector<std::shared_ptr<AmenityPoint>>& poiPoints, AmenityPoint* parentPoi, std::set<uint64_t>* skippedPoi)
{
	std::set<uint64_t> skipPoi;
	if (parentPoi == nullptr && skippedPoi == nullptr)
	{
		skippedPoi = &skipPoi;
	}
	uint32_t zoomSkip = zoom + zoomDepth;
	google::protobuf::uint32 lzoom;
	std::shared_ptr<AmenityPoint> poiPt;
	for(;;)
    {
        auto tag = cis->ReadTag();
        switch(WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            poiPoints.push_back(std::move(poiPt));
            return true;
        case OsmAndPoiBox::kZoomFieldNumber:
			{
                cis->ReadVarint32(&lzoom);
                poiPt->zoom = lzoom;
                if(parentPoi)
                    poiPt->zoom += parentPoi->zoom;
            }
			break;
		case OsmAndPoiBox::kLeftFieldNumber:
			{
				int32_t s;
				BinaryReaderUtils::readSInt32(cis, s);
				if (parentPoi != nullptr)
				{
					poiPt->x = s + (parentPoi->x << lzoom);
				}
				else
				{
					poiPt->x = s;
				}
			}
			break;
		case OsmAndPoiBox::kTopFieldNumber:
			{
				int32_t s;
				BinaryReaderUtils::readSInt32(cis, s);
				if (parentPoi != nullptr)
				{
					poiPt->y = s + (parentPoi->y << lzoom);
				}
				else
				{
					poiPt->y = s;
				}
				if (bbox31)
				{
					pointI ptTest;
					ptTest.set<0>(poiPt->x << (31 - poiPt->zoom));
					ptTest.set<1>(poiPt->y << (31 - poiPt->zoom));
					if (!bg::within(ptTest, *bbox31))
					{
						cis->Skip(cis->BytesUntilLimit());
						return false;
					}
				}
			}
			break;
		case OsmAndPoiBox::kCategoriesFieldNumber:
			{
				gp::uint32 length;
                cis->ReadLittleEndian32(&length);
                auto oldLimit = cis->PushLimit(length);
				bool allowed = checkCategories(cis, neededCategories);
				cis->PopLimit(oldLimit);
				if (!allowed)
				{
					cis->Skip(cis->BytesUntilLimit());
						return false;
				}
			}
			break;
		case OsmAndPoiBox::kSubBoxesFieldNumber:
			{
				gp::uint32 length;
                cis->ReadLittleEndian32(&length);
                auto oldLimit = cis->PushLimit(length);
				auto toSkip = ReadTileBox(cis, bbox31, neededCategories, zoom, zoomDepth, poiPoints, parentPoi, skippedPoi);
				cis->PopLimit(oldLimit);
				if(skippedPoi && poiPt->zoom >= zoomSkip && toSkip)
                {
                    auto skipHash = (static_cast<uint64_t>(poiPt->x) >> (poiPt->zoom - zoomSkip)) << zoomSkip;
                    skipHash |= static_cast<uint64_t>(poiPt->y) >> (poiPt->zoom - zoomSkip);
                    if(skippedPoi->find(skipHash) != skippedPoi->end())
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return true;
                    }
                }
			}
			break;
		case OsmAndPoiBox::kShiftToDataFieldNumber:
			{
                poiPt->offset = BinaryReaderUtils::readBigEndianInt(cis);
                poiPt->hash  = static_cast<uint64_t>(poiPt->x) << poiPt->zoom;
                poiPt->hash |= static_cast<uint64_t>(poiPt->y);
                poiPt->hash |= poiPt->zoom;

                // skipTiles - these tiles are going to be ignored, since we need only 1 POI object (x;y)@zoom
                if(skippedPoi && poiPt->zoom >= zoomSkip)
                {
                    auto skipHash = (static_cast<uint64_t>(poiPt->x) >> (poiPt->zoom - zoomSkip)) << zoomSkip;
                    skipHash |= static_cast<uint64_t>(poiPt->y) >> (poiPt->zoom - zoomSkip);
                    skippedPoi->insert(skipHash);
                }
            }
            break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}
	}
}

bool BinaryPoiDataReader::checkCategories(gio::CodedInputStream* cis, std::set<uint64_t>& neededCategories)
{
	for(;;)
    {
        auto tag = cis->ReadTag();
        switch(WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return false;
        case OsmAndPoiCategories::kCategoriesFieldNumber:
            {
                gp::uint32 binaryMixedId;
                cis->ReadVarint32(&binaryMixedId);
                const auto catId = binaryMixedId & CategoryIdMask;
                const auto subId = binaryMixedId >> SubcategoryIdShift;

                const uint32_t allSubsId = (catId << 16) | 0xFFFF;
                const uint32_t mixedId = (catId << 16) | subId;
                if(neededCategories.find(allSubsId) !=  neededCategories.end() || neededCategories.find(mixedId) !=  neededCategories.end())
                {
                    cis->Skip(cis->BytesUntilLimit());
                    return true;
                }
            }
            break;
        default:
            BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }
	return false;
}

void BinaryPoiDataReader::ReadAmenitiesData(gio::CodedInputStream* cis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories, uint32_t zoom, uint32_t zoomDepth)
{
	const auto zoomToSkip = zoom + zoomDepth;
    std::set< uint64_t > amenitiesSkip;

    pointI pt;
    uint32_t zoomSk = 0;

    for(;;)
    {

        auto tag = cis->ReadTag();
        switch(WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            return;
        case OsmAndPoiBoxData::kZoomFieldNumber:
            {
                gp::uint32 value;
                cis->ReadVarint32(&value);
                zoom = value;
            }
            break;
        case OsmAndPoiBoxData::kXFieldNumber:
            {
                gp::uint32 value;
                cis->ReadVarint32(&value);
				pt.set<0>(value);
            }
            break;
        case OsmAndPoiBoxData::kYFieldNumber:
            {
                gp::uint32 value;
                cis->ReadVarint32(&value);
                pt.set<1>(value);
            }
            break;
        case OsmAndPoiBoxData::kPoiDataFieldNumber:
            {
                gp::uint32 length;
                cis->ReadVarint32(&length);
                auto oldLimit = cis->PushLimit(length);

                std::shared_ptr<AmenityPoi> amenity;
                ReadAmenity(cis, zoom, amenity, neededCategories, bbox, pt);

                cis->PopLimit(oldLimit);

                if(!amenity)
                    break;
				const auto xp = amenity->point31.get<0>() >> (31 - zoomToSkip);
                const auto yp = amenity->point31.get<1>() >> (31 - zoomToSkip);
                const auto hash = (static_cast<uint64_t>(xp) << zoomToSkip) | static_cast<uint64_t>(yp);
                if(amenitiesSkip.find(hash)==amenitiesSkip.end())
                {
                      amenitiesSkip.insert(hash);
                      amCom.push_back(std::move(amenity));
                }
            }
            break;
        default:
            BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }
}

void BinaryPoiDataReader::ReadAmenity(gio::CodedInputStream* cis,uint32_t zoom, std::shared_ptr<AmenityPoi>& amenity, std::set<uint64_t>& neededCategories,std::shared_ptr<AreaI>& bbox, pointI& parentPT)
{

    pointI point;
    uint32_t catId;
    uint32_t subId;
	int32_t ptValue;
    for(;;)
    {

        auto tag = cis->ReadTag();
        switch(WireFormatLite::GetTagFieldNumber(tag))
        {
        case 0:
            if(amenity->latinName.empty())
                amenity->latinName = amenity->name;
            amenity->point31 = point;
            amenity->categoryId = catId;
            amenity->subcategoryId = subId;
            return;
        case OsmAndPoiBoxDataAtom::kDxFieldNumber:
			BinaryReaderUtils::readSInt32(cis, ptValue);
			point.set<0>((ptValue + (parentPT.get<0>() << (24 - zoom))) << 7);
            break;
        case OsmAndPoiBoxDataAtom::kDyFieldNumber:
			BinaryReaderUtils::readSInt32(cis, ptValue);
			point.set<1>((ptValue + (parentPT.get<1>() << (24 - zoom))) << 7);
			if(bbox && !bg::covered_by(point, *bbox))
            {
                cis->Skip(cis->BytesUntilLimit());
                return;
            }
            break;
        case OsmAndPoiBoxDataAtom::kCategoriesFieldNumber:
            {
                gp::uint32 value;
                cis->ReadVarint32(&value);
                catId = value & CategoryIdMask;
                subId = value >> SubcategoryIdShift;

                if(!neededCategories.empty())
                {
                    const uint32_t allSubsId = (catId << 16) | 0xFFFF;
                    const uint32_t mixedId = (catId << 16) | subId;
                    if(neededCategories.find(allSubsId) != neededCategories.end() || neededCategories.find(mixedId) != neededCategories.end())
                    {
                        cis->Skip(cis->BytesUntilLimit());
                        return;
                    }
                }
                amenity.reset(new AmenityPoi());
            }
            break;
        case OsmAndPoiBoxDataAtom::kIdFieldNumber:
            {
                cis->ReadVarint64(reinterpret_cast<gp::uint64*>(&amenity->id));
            }
            break;
        case OsmAndPoiBoxDataAtom::kNameFieldNumber:
			BinaryReaderUtils::readString(cis, amenity->name);
            break;
        case OsmAndPoiBoxDataAtom::kNameEnFieldNumber:
            BinaryReaderUtils::readString(cis, amenity->latinName);
            break;
        case OsmAndPoiBoxDataAtom::kOpeningHoursFieldNumber:
            BinaryReaderUtils::readString(cis, amenity->openingHours);
            break;
        case OsmAndPoiBoxDataAtom::kSiteFieldNumber:
            BinaryReaderUtils::readString(cis, amenity->site);
            break;
        case OsmAndPoiBoxDataAtom::kPhoneFieldNumber:
            BinaryReaderUtils::readString(cis, amenity->phone);
            break;
        case OsmAndPoiBoxDataAtom::kNoteFieldNumber:
            BinaryReaderUtils::readString(cis, amenity->description);
            break;
        default:
            BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }

}