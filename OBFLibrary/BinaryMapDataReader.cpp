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
#include <Ole2.h>
#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataObjects.h"
#include "BinaryMapDataReader.h"
#include "ArchiveIO.h"
#include <limits>
//#include "Rtree_Serialization.h"
#include "BinaryReaderUtils.h"
#include "MapStyleData.h"
#include "MapStyleValue.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"

using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;


BinaryMapDataReader::BinaryMapDataReader(void) : mapRules(new BinaryMapRules)
{
}


BinaryMapDataReader::~BinaryMapDataReader(void)
{
}


void BinaryMapDataReader::ReadMapDataSection(gio::CodedInputStream* cis, RandomAccessFileReader* outData)
{
	boxI initBox;
	boost::geometry::assign_inverse(initBox);
	pointI ptCenter;
	
	uint32_t defaultId = 1;
	while(true)
	{
		int tag = cis->ReadTag();
		int tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch (tagVal)
		{
		case 0:
			{
#ifdef _DEBUG
				std::wstringstream strmData;
				strmData << L"Sections count: " << sections.size() << std::endl;
				for(auto secIt : sections)
				{
					strmData << L"Section: minX "<<	MapUtils::get31LongitudeX(std::get<0>(secIt).min_corner().get<0>()) << L" minY "<< MapUtils::get31LatitudeY(std::get<0>(secIt).min_corner().get<1>()) << std::endl;
					strmData << L"         maxX "<<	MapUtils::get31LongitudeX(std::get<0>(secIt).max_corner().get<0>()) << L" maxY "<< MapUtils::get31LatitudeY(std::get<0>(secIt).max_corner().get<1>()) << std::endl;
					strmData << L"Children count " << 	std::get<2>(secIt)->childSections.size() << std::endl;
					if (std::get<2>(secIt)->childSections.size() > 0)
					{
						auto childSection = std::get<2>(secIt)->childSections.front();
						{
							strmData  <<  L"    " << L"Sub Children count " << 	childSection->childSections.size() << std::endl;
							for (auto childSectionSub : childSection->childSections)
							{
								strmData <<  L"    " <<  L"    " << L" minX:" << childSectionSub->geoBox.min_corner().get<0>() << L" minY:" << childSectionSub->geoBox.min_corner().get<1>() << std::endl;
								strmData <<  L"    " <<  L"    " << L" maxX:" << childSectionSub->geoBox.max_corner().get<0>() << L" maxY:" << childSectionSub->geoBox.max_corner().get<1>() << std::endl;
								if (childSectionSub->childSections.size() > 0)
								{
									strmData  <<  L"    " << L"Sub Sub Children count " << 	childSectionSub->childSections.size() << std::endl;
									for (auto childSectionSubSub : childSectionSub->childSections)
									{
										strmData <<  L"    " << L"    " <<  L"    " << L" minX:" << childSectionSubSub->geoBox.min_corner().get<0>() << L" minY:" << childSectionSubSub->geoBox.min_corner().get<1>() << std::endl;
										strmData <<  L"    " << L"    " <<  L"    " << L" maxX:" << childSectionSubSub->geoBox.max_corner().get<0>() << L" maxY:" << childSectionSubSub->geoBox.max_corner().get<1>() << std::endl;
									}
								}
							}
						}
					}
				}
				std::wstring strText = strmData.str();
				if (strText.size() > 1024)
				{
					while (strText.size() > 1024)
					{
						OutputDebugString(strText.substr(0, 1024).c_str());
						strText = strText.substr(1024, std::wstring::npos);
					}
				}
				OutputDebugString(strText.c_str());
#endif // _DEBUG
			}
			return;
		case OsmAndMapIndex::kNameFieldNumber:
			WireFormatLite::ReadString(cis, &mapName);
			break;
		case OsmAndMapIndex::kLevelsFieldNumber:
			{
			auto length = BinaryReaderUtils::readBigEndianInt(cis);
            auto offset = cis->CurrentPosition();
            auto oldLimit = cis->PushLimit(length);
			std::shared_ptr<BinaryMapSection> section(new BinaryMapSection());
			section->rules = mapRules;
			section->offset = offset;
            readMapLevelHeader(cis, section, offset, initBox);
			sections.push_back(std::make_tuple(section->rootBox, section->zoomLevels, section));
            cis->PopLimit(oldLimit);

			}
			break;
		case OsmAndMapIndex::kRulesFieldNumber:
			readMapEncodingRules(cis, defaultId++);
			break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}

	}

	//boost::promise<int> nextData;

	//boost::shared_future<int> dataLoad;
	//boost::async(
	//dataLoad.get();
	
}


void BinaryMapDataReader::readMapEncodingRules(gio::CodedInputStream* cis, uint32_t defRuleId)
 {
	 uint32_t rlength = 0;
	 cis->ReadVarint32(&rlength);

	 uint32_t  oldLimit = cis->PushLimit(rlength);

	 uint32_t ruleID = defRuleId;
	 uint32_t ruleType = 0;
	 std::string name;
	 std::string value;

	 bool visited = false;

	for(;;)
    {
		  const auto tag = cis->ReadTag();
		  const auto tagID = wfl::WireFormatLite::GetTagFieldNumber(tag);
		  switch(tagID)
		  {
			case 0:
				mapRules->createRule(ruleType, ruleID, name, value);
				cis->PopLimit(oldLimit);
			  return;
			case OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
				cis->ReadVarint32(&ruleID);
				 break;
			case OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
				wfl::WireFormatLite::ReadString(cis, &name);
				break;
			case OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber:
				cis->ReadVarint32(&ruleType);
				break;
			case OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
				wfl::WireFormatLite::ReadString(cis, &value);
				break;
			default:
				BinaryReaderUtils::skipUnknownField(cis, tag);
				break;
		  }
	}
 }


 void BinaryMapDataReader::readMapLevelHeader(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection> section, int parentoffset, boxI& region)
 {
	
	for(;;)
    {
        const auto tagPos = cis->CurrentPosition();
        const auto tag = cis->ReadTag();
		const auto tagVal = wfl::WireFormatLite::GetTagFieldNumber(tag);
		gp::uint32 BoxVal;
        switch(tagVal)
        {
        case 0:
			section->geoBox = translateBox(section->rootBox);
            return;
        case OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&section->zoomLevels.first));
            break;
        case OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&section->zoomLevels.second));
            break;
        case OsmAndMapIndex_MapRootLevel::kLeftFieldNumber:
			cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.min_corner().set<0>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kRightFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.max_corner().set<0>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kTopFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.min_corner().set<1>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kBottomFieldNumber:
            cis->ReadVarint32(reinterpret_cast<gp::uint32*>(&BoxVal));
			section->rootBox.max_corner().set<1>(BoxVal);
            break;
        case OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber:
            {
                // Save boxes offset
                section->offset = tagPos - parentoffset;
				loadTreeNodes(cis, section, region);
				getBoxesReferences(section);
                // Skip reading boxes and surely, following blocks
                //cis->Skip(cis->BytesUntilLimit());
            }
            break;
		case OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber:
			{
				auto areaVal = fabs(bg::area(region));
			  if (areaVal >= 1)
			  {
				  auto posBlock = cis->CurrentPosition();
				  if (mapDataReferences.find(posBlock) != mapDataReferences.end())
				  {
					  loadMapDataObjects(cis, mapDataReferences[cis->CurrentPosition()], region);
				  }
				  else
				  {
					  BinaryReaderUtils::skipUnknownField(cis, tag);
				  }
			  }
			  else
			  {
			  BinaryReaderUtils::skipUnknownField(cis, tag);
			  }
              break;
			}
        default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }
 }

 
void BinaryMapDataReader::loadTreeNodes(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section, boxI& area)
{
	gp::uint32 BoxLength;
	uint32_t currOffset = cis->CurrentPosition();
	BoxLength = BinaryReaderUtils::readBigEndianInt(cis);
	gp::uint32 oldVal = cis->PushLimit(BoxLength);
	std::shared_ptr<BinaryMapSection> childSection(new BinaryMapSection);
	childSection->offset = currOffset;
	for (;;)
	{

		uint32_t tag = cis->ReadTag();
		uint32_t tagVal = wfl::WireFormatLite::GetTagFieldNumber(tag);
		int32_t value;
		switch (tagVal)
		{
		case 0:
			cis->PopLimit(oldVal);

			section->childSections.push_back(childSection);
			childSection->geoBox = translateBox(childSection->rootBox);
			return;
		case OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kRightFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<0>(section->rootBox.max_corner().get<0>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kTopFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<1>(section->rootBox.min_corner().get<1>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<1>(section->rootBox.max_corner().get<1>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
			{
				pointI ptCenter;
				boxI bxInit;
				boxI bxCalc;
				bg::convert(section->rootBox, bxCalc);
				bg::centroid(bxCalc, ptCenter);
				bg::assign_inverse(bxInit);
				bxInit.min_corner().set<0>(section->rootBox.min_corner().get<0>());
				bxInit.min_corner().set<1>(section->rootBox.min_corner().get<1>());
				bxInit.max_corner().set<0>(ptCenter.get<0>());
				bxInit.max_corner().set<1>(ptCenter.get<1>());
				if (bg::covered_by(bxInit, section->rootBox ))
				{
					loadChildTreeNode(cis, childSection, bxInit);
}
				else
				{
					BinaryReaderUtils::skipUnknownField(cis, tag);
				}
			}
			//BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		case OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
			childSection->dataOffset = BinaryReaderUtils::readBigEndianInt(cis);
			childSection->offset = cis->CurrentPosition();
			break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}

	}

}

void BinaryMapDataReader::loadChildTreeNode(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section, boxI& region)
{
	gp::uint32 BoxLength;
	BoxLength = BinaryReaderUtils::readBigEndianInt(cis);
	gp::uint32 oldVal = cis->PushLimit(BoxLength);
	std::shared_ptr<BinaryMapSection> childSection(new BinaryMapSection);
	childSection->offset = cis->CurrentPosition();
	for (;;)
	{

		uint32_t tag = cis->ReadTag();
		uint32_t tagVal = wfl::WireFormatLite::GetTagFieldNumber(tag);
		int32_t value;
		switch (tagVal)
		{
		case 0:
			cis->PopLimit(oldVal);
			section->childSections.push_back(childSection);
			childSection->geoBox = translateBox(childSection->rootBox);
			return;
		case OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kRightFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<0>(section->rootBox.max_corner().get<0>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kTopFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<1>(section->rootBox.min_corner().get<1>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
			BinaryReaderUtils::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<1>(section->rootBox.max_corner().get<1>() + value);
			break;
		case OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
			{
				auto areaVal = bg::area(region);
				if (/*fabs(areaVal) > 1 && bg::covered_by(region, childSection->rootBox)*/true)
				{
					loadChildTreeNode(cis, childSection, region);
				}
				else
				{
					BinaryReaderUtils::skipUnknownField(cis, tag);
				}
			}
			break;
		case OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
			childSection->dataOffset = BinaryReaderUtils::readBigEndianInt(cis);
			break;
		default:
			BinaryReaderUtils::skipUnknownField(cis, tag);
			break;
		}

	}
}


void BinaryMapDataReader::PaintSections()
{
	if (sections.size() > 0)
	{
		for (int indexSect = 0; indexSect < sections.size(); indexSect++)
		{
			auto sectionInfo = std::get<2>(sections[indexSect]);
			if (sectionInfo->childSections.size() > 0)
			{
				SkImage::Info info = {
				1600, 1000, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
				};
				SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
				SkCanvas* painter = imageRender->getCanvas();
				
				painter->drawColor(SK_ColorWHITE);
				SkRect limits;
				painter->getClipBounds(&limits);
				SkScalar w = limits.width();
				SkScalar h = limits.height();
				SkPaint paint;
				paint.setColor(SK_ColorBLACK);
				paint.setStyle(SkPaint::Style::kStroke_Style);
				paint.setStrokeWidth(0);
				SkPaint paintSub;
				paintSub.setColor(SK_ColorBLUE);
				paintSub.setStyle(SkPaint::Style::kStroke_Style);
				int maxY = -1000, maxX = -1000;
				int minY = 1000, minX = 1000;
				auto rootBoxData = sectionInfo->childSections.front();
				boxI wholeMapBox;
				boxI lockerBox;
				minX = std::min(rootBoxData->rootBox.min_corner().get<0>(), 	rootBoxData->rootBox.max_corner().get<0>());
				maxX = std::max(rootBoxData->rootBox.min_corner().get<0>(), 	rootBoxData->rootBox.max_corner().get<0>());
				minY = std::min(rootBoxData->rootBox.min_corner().get<1>(), 	rootBoxData->rootBox.max_corner().get<1>());
				maxY = std::max(rootBoxData->rootBox.min_corner().get<1>(), 	rootBoxData->rootBox.max_corner().get<1>());
				int widthStep = (maxX - minX)/2;
				int heightStep = (maxY - minY)/2;
				if (rootBoxData->rootBox.min_corner().get<0>() > rootBoxData->rootBox.max_corner().get<0>())
				{
					widthStep = -widthStep;
				}
				if (rootBoxData->rootBox.min_corner().get<1>() >	rootBoxData->rootBox.max_corner().get<1>())
				{
					heightStep = -heightStep;
				}
				for (int idx = 0; idx < 4; idx++)
				{
					int YStep = abs(heightStep);
					int XStep = abs(widthStep);
					if (heightStep < 0)
					{
						wholeMapBox.max_corner().set<1>(minY + (YStep * ((idx/2) % 2)));
						wholeMapBox.min_corner().set<1>(minY + YStep * (((idx/2) % 2)+1));
					}
					else
					{
						wholeMapBox.max_corner().set<1>(minY + YStep * (((idx/2) % 2)+1));
						wholeMapBox.min_corner().set<1>(minY + YStep * ((idx/2) % 2));
					}
					if (widthStep < 0)
					{
						wholeMapBox.min_corner().set<0>(minX + XStep * (idx % 2));
						wholeMapBox.max_corner().set<0>(minX + XStep * ((idx % 2)+1));
					}
					else
					{
						wholeMapBox.min_corner().set<0>(minX + XStep * (idx % 2));
						wholeMapBox.max_corner().set<0>(minX + XStep * ((idx % 2)+1));
					}
					boxD lockerBox = translateBox(wholeMapBox);
					double minXW = std::min(lockerBox.min_corner().get<0>(), 	lockerBox.max_corner().get<0>());
					double maxXW = std::max(lockerBox.min_corner().get<0>(), 	lockerBox.max_corner().get<0>());
					double minYW = std::min(lockerBox.min_corner().get<1>(), 	lockerBox.max_corner().get<1>());
					double maxYW = std::max(lockerBox.min_corner().get<1>(), 	lockerBox.max_corner().get<1>());
					double scale = 1.0;
					bool painted = false;
					if (maxXW - minXW > w || maxYW - minYW > h)
					{
						if ((maxXW - minXW - w) > (maxYW - minYW - h))
						{
							scale = w / (maxXW - minXW);
						}
						else
						{
							scale = w / (maxYW - minYW);
						}
					}
					else if (maxXW - minXW < w && maxYW - minYW < h)
					{
						scale = std::min<SkScalar>(w / (maxXW - minXW)   , h / (maxYW - minYW));
					}
					bool isCoveredSegment = isCovered(sectionInfo, wholeMapBox);
					for( auto subChilds : sectionInfo->childSections.front()->childSections)
					{
						
						if (isCoveredSegment && isCovered(subChilds, wholeMapBox) /*|| bg::covered_by(tileMapBox, subChilds->rootBox )  || bg::intersects(tileMapBox,  subChilds->rootBox)*/)
						{
							double maxYC = -1000, maxXC = -1000;
							double minYC = 1000, minXC = 1000;
							double scaleC = 1.0;
							painted = true;
							minXC = std::min(subChilds->geoBox.min_corner().get<0>(), 	subChilds->geoBox.max_corner().get<0>());
							maxXC = std::max(subChilds->geoBox.min_corner().get<0>(), 	subChilds->geoBox.max_corner().get<0>());
							minYC = std::min(subChilds->geoBox.min_corner().get<1>(), 	subChilds->geoBox.max_corner().get<1>());
							maxYC = std::max(subChilds->geoBox.min_corner().get<1>(), 	subChilds->geoBox.max_corner().get<1>());
							if (maxXC - minXC < w && maxYC - minYC < h)
							{
								scaleC = std::min<SkScalar>(w / (maxXC - minXC)   , h / (maxYC - minYC));
							}
							if (subChilds->childSections.size() > 0)
							{
								for (auto subChildsPop : subChilds->childSections)
								{
									paintSection(subChildsPop, wholeMapBox, minXW, minYW, scale, painter);
								}
							}
							{
								painter->drawRectCoords((minXC - minX) * scale, h - (minYC- minY)*scale, (maxXC - minX)*scale, h - (maxYC - minY)*scale, paint);
							}
						}
						
					}

				
					if (painted)
					{
						SkAutoTUnref<SkImage> image(imageRender->newImageSnapshot());
				
						SkAutoDataUnref data(image->encode());
						if (NULL == data.get()) {
							return ;
						}
						std::string buff;
						buff = boost::lexical_cast<std::string,int>(indexSect);
						buff += "_";
						buff += boost::lexical_cast<std::string,int>(idx);
						std::string pathImage = "D:\\osmData\\resultImageBox" + buff + std::string(".png");
						SkFILEWStream stream(pathImage.c_str());

						stream.write(data->data(), data->size());
					}
				}
			}
		}
	}

}

bool BinaryMapDataReader::isCovered(std::shared_ptr<BinaryMapSection>& subChildsPop, boxI& cover)
{
	if (bg::covered_by( subChildsPop->rootBox, cover))
	{
		return true;
	}
	for (auto subChilds : subChildsPop->childSections)
	{
		if (isCovered(subChilds, cover))
			return true;
	}
	return false;
}

void BinaryMapDataReader::paintSection(std::shared_ptr<BinaryMapSection>& subChildsPop, boxI& cover, double  minX,double minY, double scale, void* painter)
{
	if (subChildsPop->sectionData.size() > 0 && bg::covered_by(subChildsPop->rootBox, cover))
	{
		paintSectionData(subChildsPop->sectionData, minX, minY, scale, painter);
	}
	if (subChildsPop->childSections.size() > 0)
	{
		for (auto subChilds : subChildsPop->childSections)
		{
			paintSection(subChilds, cover, minX, minY, scale, painter);
		}
	}
}

void BinaryMapDataReader::paintSectionData(std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>>& sectionData, double minX, double minY,double scale, void* painterPass)
{
	SkCanvas* painter = (SkCanvas*)painterPass;
	SkPaint paintSubPrimary;
	paintSubPrimary.setColor(SK_ColorRED);
	paintSubPrimary.setStyle(SkPaint::Style::kStroke_Style);
	paintSubPrimary.setStrokeWidth(2);
	SkPaint paintSub;
	paintSub.setColor(SK_ColorGREEN);
	paintSub.setStyle(SkPaint::Style::kStroke_Style);
	SkPaint paintOther;
	paintOther.setColor(SK_ColorGRAY);
	paintOther.setStyle(SkPaint::Style::kStroke_Style);
	SkRect bounds;
	painter->getClipBounds(&bounds);
	for (auto sectionElem : sectionData)
	{
		for (int typeId : sectionElem.second->type)
		{
			MapDecodingRule rule = mapRules->getRuleInfo(typeId);
			if (rule.tag == "highway" || rule.tag == "boundary")
			{
				
				SkPath pather;
				std::vector<SkPoint> pathPoints;
				for (pointI ptI : sectionElem.second->points)
				{
					pointD mapPoint;
					mapPoint.set<0>(MapUtils::get31LongitudeX(ptI.get<0>()));
					mapPoint.set<1>(MapUtils::get31LatitudeY(ptI.get<1>()));
					SkPoint ptData;
					ptData.fX = (mapPoint.get<0>() - minX) * scale;
					ptData.fY = bounds.height() - (mapPoint.get<1>() - minY) * scale;
					pathPoints.push_back(ptData);
				}
				pather.addPoly(pathPoints.data(), pathPoints.size(), false);
				if (rule.value == "primary")
				{
					painter->drawPath(pather, paintSubPrimary);
				}
				else if (rule.value == "residential")
				{
					painter->drawPath(pather, paintSub);
				}
				else if (rule.tag == "boundary")
				{
					painter->drawPath(pather, paintOther);
				}
			}
		}
	}
}

void BinaryMapDataReader::loadMapDataObjects(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section, boxI& area)
{
	uint32_t limitSize;
	cis->ReadVarint32(&limitSize);
	uint32_t oldLimit = cis->PushLimit(limitSize);
	
	std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>> objects;
	std::vector<std::string> stringTable;
	bool continueWhile = true;

	uint64_t baseId;

	while (continueWhile)
	{
		uint32_t tag = cis->ReadTag();
		uint32_t tagVal = wfl::WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
			case 0:
			{
				continueWhile = false;
			}
			break;

			case MapDataBlock::kBaseIdFieldNumber:
				cis->ReadVarint64(&baseId);
				break;
			case MapDataBlock::kDataObjectsFieldNumber:
				{
				uint32_t	 len;
				cis->ReadVarint32(&len);
				uint32_t localimit = cis->PushLimit(len);
				readMapObject(cis, section, baseId, objects); 
				cis->PopLimit(localimit);
				}
				break;
			case MapDataBlock::kStringTableFieldNumber:
				{
					uint32_t lenStr;
					cis->ReadVarint32(&lenStr);
					auto oldStrLimit = cis->PushLimit(lenStr);
					if(objects.empty())
					{
						cis->Skip(lenStr);
						cis->PopLimit(oldStrLimit);
						break;
					}
					BinaryReaderUtils::readStringTable(cis, stringTable);
					assert(cis->BytesUntilLimit() == 0);
					cis->PopLimit(oldStrLimit);
				}
				break;
			default:
				BinaryReaderUtils::skipUnknownField(cis, tag);
				break;
		}
	}
	
	MergeStringsToObjects(objects, stringTable);
	section->sectionData = std::move(objects);
	cis->PopLimit(oldLimit);
}



void BinaryMapDataReader::getBoxesReferences(std::shared_ptr<BinaryMapSection>& section)
{
	if (section->dataOffset > 0)
	{
		mapDataReferences.insert(std::make_pair(section->offset + section->dataOffset, section));
	}
	for(auto childData : section->childSections)
	{
		getBoxesReferences(childData);
	}
}

void BinaryMapDataReader::readMapObject(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section, uint64_t baseid,std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>>& objects)
{
	bool continueWhile = true;
	 __int64 idDef;
	 MapObjectData* localMapObj = new MapObjectData;
	 localMapObj->section = section;
	while (continueWhile)
	{
		uint32_t tag = cis->ReadTag();
		uint32_t tagVal = wfl::WireFormatLite::GetTagFieldNumber(tag);
		switch(tagVal)
		{
			case 0:
			{
				continueWhile = false;
			}
			break;
			case MapData::kIdFieldNumber:
				{
				 idDef = BinaryReaderUtils::readSInt64(cis);
				 objects.insert(std::make_pair(baseid+idDef, std::shared_ptr<MapObjectData>(localMapObj)));
    			 localMapObj->localId = baseid+idDef;
				}
				break;
			case MapData::kAreaCoordinatesFieldNumber:
			case MapData::kCoordinatesFieldNumber:
				{
					uint32_t limitVtx;
					cis->ReadVarint32(&limitVtx);
					uint32_t oldLimitVtx = cis->PushLimit(limitVtx);
					
					pointI ptRef;
					ptRef.set<0>(section->rootBox.min_corner().get<0>() & MaskToRead);
					ptRef.set<1>(section->rootBox.min_corner().get<1>() & MaskToRead);

					std::vector<pointI> ptDataVec(cis->BytesUntilLimit()/2);
					auto verticesCount = 0;
					auto pointerData = ptDataVec.data();
					while(cis->BytesUntilLimit() > 0)
					{
						int32_t x = 0;
						int32_t y = 0;
						pointI ptData;
						BinaryReaderUtils::readSInt32(cis, x);
						BinaryReaderUtils::readSInt32(cis, y);
						ptRef.set<0>(ptRef.get<0>() + (x << ShiftCoordinates));
						ptRef.set<1>(ptRef.get<1>() + (y << ShiftCoordinates));
						(*pointerData++) = ptRef;
						verticesCount++;
					}
					cis->PopLimit(oldLimitVtx);
					ptDataVec.resize(verticesCount);
					if (localMapObj != nullptr)
					{
						
						localMapObj->points = ptDataVec;
						localMapObj->isArea = MapData::kAreaCoordinatesFieldNumber == tagVal;
#ifdef _DEBUG 
						for(pointI ptVec : ptDataVec)
						{
							pointD geoPt;
							geoPt.set<0>(MapUtils::get31LongitudeX(ptVec.get<0>()));
							geoPt.set<1>(MapUtils::get31LatitudeY(ptVec.get<1>()));
							localMapObj->geoPoints.push_back(geoPt);	
						}

#endif
						polyArea polyData;
						polyData.outer().insert(polyData.outer().end(), localMapObj->points.begin(), localMapObj->points.end());
						bg::envelope(polyData, localMapObj->bbox);
#ifdef _DEBUG
						localMapObj->geoBBox = translateBox(localMapObj->bbox);
#endif
					}
				}
				break;
			case MapData::kPolygonInnerCoordinatesFieldNumber:
				{
					uint32_t limitVtx;
					cis->ReadVarint32(&limitVtx);
					uint32_t oldLimitVtx = cis->PushLimit(limitVtx);
					
					pointI ptRef;
					ptRef.set<0>(section->rootBox.min_corner().get<0>() & MaskToRead);
					ptRef.set<1>(section->rootBox.min_corner().get<1>() & MaskToRead);

					

					std::vector<pointI> ptDataVec(cis->BytesUntilLimit()/2);
					auto verticesCount = 0;
					auto pointerData = ptDataVec.data();
					while(cis->BytesUntilLimit() > 0)
					{
						int32_t x = 0;
						int32_t y = 0;
						pointI ptData;
						BinaryReaderUtils::readSInt32(cis, x);
						BinaryReaderUtils::readSInt32(cis, y);
						ptRef.set<0>(ptRef.get<0>() + (x << ShiftCoordinates));
						ptRef.set<1>(ptRef.get<1>() + (y << ShiftCoordinates));
						(*pointerData++) = ptRef;
						verticesCount++;
					}
					cis->PopLimit(oldLimitVtx);
					ptDataVec.resize(verticesCount);
					if (localMapObj != nullptr)
					{
						localMapObj->innerpolypoints.push_back(ptDataVec);
					}
				}
				break;
			case MapData::kTypesFieldNumber:
			case MapData::kAdditionalTypesFieldNumber:
				{
					uint32_t limitTypes;
					cis->ReadVarint32(&limitTypes);
					uint32_t oldLimitTypes = cis->PushLimit(limitTypes);
					std::list<int>& typeIds = tagVal == MapData::kTypesFieldNumber ? localMapObj->type : localMapObj->addtype;
					while(cis->BytesUntilLimit() > 0)
					{
						uint32_t ruleId;
						cis->ReadVarint32(&ruleId);
						typeIds.push_back(ruleId);
					}
					cis->PopLimit(oldLimitTypes);
				}
				break;
			case MapData::kStringNamesFieldNumber:
				{
					uint32_t limitNames;
					cis->ReadVarint32(&limitNames);
					uint32_t oldLimitNames = cis->PushLimit(limitNames);
					while(cis->BytesUntilLimit() > 0)
					{
						uint32_t ruleId, ruleValId;
						cis->ReadVarint32(&ruleId);
						cis->ReadVarint32(&ruleValId);
						localMapObj->nameTypeString.push_back(std::make_tuple(ruleId, ruleValId, ""));
					}
					cis->PopLimit(oldLimitNames);
				}				
				break;
			default:
				BinaryReaderUtils::skipUnknownField(cis, tag);
				break;
		}
	}

	// few assertions
#ifdef _DEBUG
	polyArea polyData;
	polyData.outer().insert(polyData.outer().end(), localMapObj->points.begin(), localMapObj->points.end());
	AreaI locBox;
	bg::envelope(polyData, locBox);
	localMapObj->correctBBox = bg::covered_by(locBox, section->rootBox);
	if (localMapObj->correctBBox == true)
	{
		AreaD translated = translateBox(locBox);
		AreaD translatedRoot = translateBox(section->rootBox);
		bool interData = bg::intersects(translated, translatedRoot );
		if (interData == false)
		{
			bg::expand(translated, translatedRoot);
		}
		interData = bg::intersects(translated, translatedRoot );
	}
#endif

}

void BinaryMapDataReader::MergeStringsToObjects(std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>>& objects, std::vector<std::string>& stringList)
{
	bool uncorectbbox = false;
	for (auto mapDataItem : objects)
	{
		for (std::list<std::tuple<int,int,std::string>>::iterator itemString = mapDataItem.second->nameTypeString.begin(); itemString != mapDataItem.second->nameTypeString.end(); itemString++)
		{
			assert(std::get<1>(*itemString) >= 0 && std::get<1>(*itemString) < stringList.size());
			std::string value = stringList[std::get<1>(*itemString)];
			std::get<2>(*itemString) = value;
			value = std::get<2>(*itemString);
		}
#ifdef _DEBUG
		if (mapDataItem.second->correctBBox == false)
			uncorectbbox = true;
#endif
	}
	if (uncorectbbox)
	{
		std::wstringstream strmData;
		strmData << L"Sections objects : " << objects.size() << L" has incrrect bbxo" << std::endl;
		OutputDebugString(strmData.str().c_str());
	}
}


void BinaryMapDataReader::evaluate(std::shared_ptr<MapStyleInfo>& infoDump)
{
	std::shared_ptr<MapStyleInfo> styleData(infoDump);
	MapStyleEval evaluator(styleData, 0.10f);

	evaluator.setIntValue(evaluator._builtInDefValues->id_INPUT_MINZOOM, 15);
	evaluator.setIntValue(evaluator._builtInDefValues->id_INPUT_MAXZOOM, 15);


	for (auto sampleSection : sections)
	{
		auto sectData = std::get<2>(sampleSection);
		if(!sectData->sectionData.empty())
		{
			for (auto mapData : sectData->sectionData)
			{
				MapStyleResult* ressultPlacer = new MapStyleResult(); 
				evaluator.evaluate(mapData.second, rulesetType::order, ressultPlacer);
			}
		}
		else
		{
			for (auto sampleSection : sectData->childSections)
			{
				if(!sampleSection->sectionData.empty())
				{
					for (auto mapData : sampleSection->sectionData)
					{
						MapStyleResult* ressultPlacer = new MapStyleResult(); 
						evaluator.evaluate(mapData.second, rulesetType::order, ressultPlacer);
					}
				}
				else
				{
					for (auto sampleSubSection : sampleSection->childSections)
					{
						if(!sampleSubSection->sectionData.empty())
						{
							for (auto mapData : sampleSubSection->sectionData)
							{
								MapStyleResult* ressultPlacer = new MapStyleResult(); 
								evaluator.evaluate(mapData.second, rulesetType::order, ressultPlacer);
							}
						}
						else
						{
							for (auto childSection : sampleSubSection->childSections)
							{
								if(!childSection->sectionData.empty())
								{
									for (auto mapData : childSection->sectionData)
									{
										for (auto typeID : mapData.second->type)
										{
											MapStyleResult* ressultPlacer = new MapStyleResult(); 
											auto decodedType = mapData.second->section->rules->getRuleInfo(typeID);
											evaluator.setStringValue(evaluator._builtInDefValues->id_INPUT_TAG, decodedType.tag);
											evaluator.setStringValue(evaluator._builtInDefValues->id_INPUT_VALUE, decodedType.value);
											evaluator.setIntValue(evaluator._builtInDefValues->id_INPUT_LAYER, mapData.second->getSimpleLayerValue());
											evaluator.setBoolValue(evaluator._builtInDefValues->id_INPUT_AREA,  mapData.second->isArea);
											evaluator.setBoolValue(evaluator._builtInDefValues->id_INPUT_POINT,  mapData.second->points.size() == 1);
											evaluator.setBoolValue(evaluator._builtInDefValues->id_INPUT_CYCLE, mapData.second->isClosedFigure());

											evaluator.evaluate(mapData.second, rulesetType::order, ressultPlacer);
										}
									}
								}
							}
						}
					}

				}
			}
		}
	}

	
}