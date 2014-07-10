#include "stdafx.h"
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
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryIndexDataReader.h"
#include "ArchiveIO.h"
#include <limits>
//#include "Rtree_Serialization.h"

using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;


#pragma push_macro("max")
#undef max

 BinaryMapRules::BinaryMapRules() :  name_encodingRuleId(0), 
	ref_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastline_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalLand_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastlineBroken_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastlineLine_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    highway_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    oneway_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    onewayReverse_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    tunnel_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    bridge_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    layerLowest_encodingRuleId(std::numeric_limits<uint32_t>::max())
 {

 }

#pragma pop_macro("max")

BinaryMapRules::~BinaryMapRules()
{
}

void BinaryMapRules::createMissingRules()
{
	
}

void BinaryMapRules::createRule(uint32_t ruleType, uint32_t id, std::string name, std::string value)
{
	std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>::iterator itMapRule = mapRuleIdNames.find(name);
	if (itMapRule == mapRuleIdNames.end())
	{
		itMapRule = mapRuleIdNames.insert(std::make_pair(name, std::unordered_map<std::string, uint32_t>())).first;
	}
	(*itMapRule).second.insert(std::make_pair(value, id));
	//mapRuleIdNames.insert(itMapRule, std::make_pair(name, id));
	if (mapRules.find(id) == mapRules.end())
	{
		MapDecodingRule ruleData;
		ruleData.type = ruleType;
		ruleData.tag = name;
		ruleData.value = value;
		mapRules.insert(std::make_pair(id, ruleData));
	}
	if(name == "name")
        name_encodingRuleId = id;
    else if(name == "ref")
        ref_encodingRuleId = id;
    else if(name == "natural" && value == "coastline")
        naturalCoastline_encodingRuleId = id;
    else if(name == "natural" && value == "land")
        naturalLand_encodingRuleId = id;
    else if(name == "natural" && value == "coastline_broken")
        naturalCoastlineBroken_encodingRuleId = id;
    else if(name == "natural" && value == "coastline_line")
        naturalCoastlineLine_encodingRuleId = id;
    else if(name == "oneway" && value == "yes")
        oneway_encodingRuleId = id;
    else if(name == "oneway" && value == "-1")
        onewayReverse_encodingRuleId = id;
    else if(name == "tunnel" && value == "yes")
    {
        tunnel_encodingRuleId = id;
        negativeLayers_encodingRuleIds.insert(id);
    }
    else if(name == "bridge"  && value == "yes")
    {
        bridge_encodingRuleId = id;
        positiveLayers_encodingRuleIds.insert(id);
    }
    else if(name == "layer")
    {
        if(!value.empty() && value != "0")
        {
            if(value[0] == '-')
                negativeLayers_encodingRuleIds.insert(id);
            else if(value[0] == '0')
                zeroLayers_encodingRuleIds.insert(id);
            else
                positiveLayers_encodingRuleIds.insert(id);
        }
    }
}

BinaryMapDataReader::BinaryMapDataReader(void) : mapRules(new BinaryMapRules)
{
}


BinaryMapDataReader::~BinaryMapDataReader(void)
{
}


void BinaryMapDataReader::ReadMapDataSection(gio::CodedInputStream* cis)
{
	uint32_t defaultId = 1;
	while(true)
	{
		int tag = cis->ReadTag();
		int tagVal = WireFormatLite::GetTagFieldNumber(tag);
		switch (tagVal)
		{
		case 0:
			{
				std::wstringstream strmData;
				strmData << L"Sections count: " << sections.size() << std::endl;
				for(auto secIt : sections)
				{
					strmData << L"Section: minX "<<	MapUtils::get31LongitudeX(std::get<0>(secIt).min_corner().get<0>()) << L" minY "<< MapUtils::get31LatitudeY(std::get<0>(secIt).min_corner().get<1>()) << std::endl;
					strmData << L"Section: maxX "<<	MapUtils::get31LongitudeX(std::get<0>(secIt).max_corner().get<0>()) << L" maxY "<< MapUtils::get31LatitudeY(std::get<0>(secIt).max_corner().get<1>()) << std::endl;
					strmData << L"Children count " << 	std::get<2>(secIt)->childSections.size() << std::endl;
					for (auto childSection : std::get<2>(secIt)->childSections)
					{
						strmData <<  L"    " << L" minX:" << childSection->geoBox.min_corner().get<0>() << L" minY:" << childSection->geoBox.min_corner().get<1>() << std::endl;
						strmData <<  L"    " << L" maxX:" << childSection->geoBox.max_corner().get<0>() << L" maxY:" << childSection->geoBox.max_corner().get<1>() << std::endl;
						strmData  <<  L"    " << L"Sub Children count " << 	childSection->childSections.size() << std::endl;
						for (auto childSectionSub : childSection->childSections)
						{
							strmData <<  L"    " <<  L"    " << L" minX:" << childSectionSub->geoBox.min_corner().get<0>() << L" minY:" << childSectionSub->geoBox.min_corner().get<1>() << std::endl;
							strmData <<  L"    " <<  L"    " << L" maxX:" << childSectionSub->geoBox.max_corner().get<0>() << L" maxY:" << childSectionSub->geoBox.max_corner().get<1>() << std::endl;
						}

					}

				}
				OutputDebugString(strmData.str().c_str());
			}
			return;
		case OsmAndMapIndex::kNameFieldNumber:
			WireFormatLite::ReadString(cis, &mapName);
			break;
		case OsmAndMapIndex::kLevelsFieldNumber:
			{
			auto length = BinaryIndexDataReader::readBigEndianInt(cis);
            auto offset = cis->CurrentPosition();
            auto oldLimit = cis->PushLimit(length);
			std::shared_ptr<BinaryMapSection> section(new BinaryMapSection());
            readMapLevelHeader(cis, section, offset);
			sections.push_back(std::make_tuple(section->rootBox, section->zoomLevels, section));
            cis->PopLimit(oldLimit);

			}
			break;
		case OsmAndMapIndex::kRulesFieldNumber:
			readMapEncodingRules(cis, defaultId++);
			break;
		default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		}

	}



	//boost::promise<int> nextData;

	//boost::shared_future<int> dataLoad;
	//boost::async(
	//dataLoad.get();
	
}

 void BinaryMapDataReader::readMapLevelHeader(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection> section, int parentoffset)
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
			section->translateBox();
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
				loadTreeNodes(cis, section);
                // Skip reading boxes and surely, following blocks
                //cis->Skip(cis->BytesUntilLimit());
            }
            break;
		case OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber:
			{
			  BinaryIndexDataReader::skipUnknownField(cis, tag);
              break;
			}
        default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
            break;
        }
    }
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
			case obf::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber:
				cis->ReadVarint32(&ruleID);
				 break;
			case obf::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber:
				wfl::WireFormatLite::ReadString(cis, &name);
				break;
			case obf::OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber:
				cis->ReadVarint32(&ruleType);
				break;
			case obf::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber:
				wfl::WireFormatLite::ReadString(cis, &value);
				break;
			default:
				BinaryIndexDataReader::skipUnknownField(cis, tag);
				break;
		  }
	}
 }




void BinaryMapDataReader::loadTreeNodes(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section)
{
	gp::uint32 BoxLength;
	BoxLength = BinaryIndexDataReader::readBigEndianInt(cis);
	gp::uint32 oldVal = cis->PushLimit(BoxLength);
	std::shared_ptr<BinaryMapSection> childSection(new BinaryMapSection);
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
			childSection->translateBox();
			return;
		case obf::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<1>(section->rootBox.min_corner().get<1>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<1>(section->rootBox.max_corner().get<1>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
			loadChildTreeNode(cis, childSection);
			//BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
			childSection->dataOffset = BinaryIndexDataReader::readBigEndianInt(cis);
			break;
		default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		}

	}

}

void BinaryMapDataReader::loadChildTreeNode(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section)
{
	gp::uint32 BoxLength;
	BoxLength = BinaryIndexDataReader::readBigEndianInt(cis);
	gp::uint32 oldVal = cis->PushLimit(BoxLength);
	std::shared_ptr<BinaryMapSection> childSection(new BinaryMapSection);
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
			childSection->translateBox();
			return;
		case obf::OsmAndMapIndex_MapDataBox::kLeftFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kRightFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<0>(section->rootBox.min_corner().get<0>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kTopFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.min_corner().set<1>(section->rootBox.min_corner().get<1>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kBottomFieldNumber:
			BinaryIndexDataReader::readSInt32(cis, value);
			childSection->rootBox.max_corner().set<1>(section->rootBox.max_corner().get<1>() + value);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber:
			loadChildTreeNode(cis, childSection);
			//BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		case obf::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber:
			childSection->dataOffset = BinaryIndexDataReader::readBigEndianInt(cis);
			break;
		default:
			BinaryIndexDataReader::skipUnknownField(cis, tag);
			break;
		}

	}
}