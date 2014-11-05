#include "stdafx.h"
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

#include "RandomAccessFileReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryReaderUtils.h"

using namespace google::protobuf::internal;
using namespace OsmAnd::OBF;


BinaryRouteDataReader::BinaryRouteDataReader(void)
{
}


BinaryRouteDataReader::~BinaryRouteDataReader(void)
{
}


void BinaryRouteDataReader::ReadRouteInfo(gio::CodedInputStream* cis)
{
	uint32_t routeEncodingRuleId = 1;
	for (;;)
	{
		auto tag = cis->ReadTag();
		auto tagVal = WireFormatLite::GetTagFieldNumber(tag);

		switch(tagVal)
		{
		case 0:
			return;
		case  OsmAndRoutingIndex::kNameFieldNumber:
			BinaryReaderUtils::readString(cis, name);
			break;
		case OsmAndRoutingIndex::kRulesFieldNumber:
			{
				std::shared_ptr<RouteEncodingRule> ruleInfo = ReadRulesInfo(cis, routeEncodingRuleId++);
				while (routeRules.size() < routeEncodingRuleId)
				{
					routeRules.push_back(std::shared_ptr<RouteEncodingRule>(new RouteEncodingRule()));
				}
				routeRules.push_back(ruleInfo);
			}
			break;
		case OsmAndRoutingIndex::kRootBoxesFieldNumber:
			{
				auto currPos = cis->CurrentPosition();
				uint32_t cityLen = BinaryReaderUtils::readBigEndianInt(cis);
				
			}
			break;
		case OsmAndAddressIndex::kNameIndexFieldNumber:
			auto indexNameOffset = cis->CurrentPosition();
            auto length = BinaryReaderUtils::readBigEndianInt(cis);
            cis->Seek(indexNameOffset + length + 4);
			break;
		}
	}

}

std::shared_ptr<RouteEncodingRule>  BinaryRouteDataReader::ReadRulesInfo(gio::CodedInputStream* cis, uint32_t routeEncodingRuleId)
{
	gp::uint32 length;
    cis->ReadVarint32(&length);
    auto oldLimit = cis->PushLimit(length);

    std::shared_ptr<RouteEncodingRule> encodingRule(new RouteEncodingRule());

    encodingRule->_id = routeEncodingRuleId;
    bool readRule = true;

	while(readRule)
    {
        auto tag = cis->ReadTag();
		auto ttVal = WireFormatLite::GetTagFieldNumber(tag);
        switch(ttVal)
        {
        case 0:
            {
				if( boost::iequals(encodingRule->_value ,"true"))
                    encodingRule->_value = "yes";
                if(boost::iequals(encodingRule->_value,"false"))
                    encodingRule->_value = "no";

                if(boost::iequals(encodingRule->_tag,"oneway"))
                {
                    encodingRule->_type = RouteEncodingRule::OneWay;
                    if(encodingRule->_value == "-1" || encodingRule->_value == "reverse")
                        encodingRule->_parsedValue.asSignedInt = -1;
                    else if(encodingRule->_value == "1" || encodingRule->_value == "yes")
                        encodingRule->_parsedValue.asSignedInt = 1;
                    else
                        encodingRule->_parsedValue.asSignedInt = 0;
                }
                else if(boost::iequals(encodingRule->_tag, "highway") && encodingRule->_value == "traffic_signals")
                {
                    encodingRule->_type = RouteEncodingRule::TrafficSignals;
                }
                else if(boost::iequals(encodingRule->_tag, "railway") && encodingRule->_value == "crossing" || encodingRule->_value == "level_crossing")
                {
                    encodingRule->_type = RouteEncodingRule::RailwayCrossing;
                }
                else if(boost::iequals(encodingRule->_tag,"roundabout") && !encodingRule->_value.empty())
                {
                    encodingRule->_type = RouteEncodingRule::Roundabout;
                }
                else if(boost::iequals(encodingRule->_tag, "junction") && boost::iequals(encodingRule->_value, "roundabout"))
                {
                    encodingRule->_type = RouteEncodingRule::Roundabout;
                }
                else if(boost::iequals(encodingRule->_tag, "highway") && !encodingRule->_value.empty())
                {
                    encodingRule->_type = RouteEncodingRule::Highway;
                }
				else if(boost::starts_with(encodingRule->_tag, "access") && !encodingRule->_value.empty())
                {
                    encodingRule->_type = RouteEncodingRule::Access;
                }
                else if(boost::iequals(encodingRule->_tag, "maxspeed") && !encodingRule->_value.empty())
                {
                    encodingRule->_type = RouteEncodingRule::Maxspeed;
					try
					{
						encodingRule->_parsedValue.asFloat = boost::lexical_cast<float, std::string>(encodingRule->_value);
					}
					catch (boost::bad_lexical_cast lexx)
					{
						encodingRule->_parsedValue.asFloat = -1.0;
					}
                }
                else if (boost::iequals(encodingRule->_tag, "lanes") && !encodingRule->_value.empty())
                {
                    encodingRule->_type = RouteEncodingRule::Lanes;
                    try
					{
						encodingRule->_parsedValue.asSignedInt = boost::lexical_cast<int, std::string>(encodingRule->_value);
					}
					catch (boost::bad_lexical_cast lexx)
					{
						encodingRule->_parsedValue.asSignedInt = -1;
					}
                }

            }
            readRule = false;
        case OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber:
            BinaryReaderUtils::readString(cis, encodingRule->_tag);
            break;
        case OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber:
            BinaryReaderUtils::readString(cis, encodingRule->_value);
            break;
        case OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber:
            {
                gp::uint32 id;
                cis->ReadVarint32(&id);
                encodingRule->_id = id;
            }
            break;
        default:
            BinaryReaderUtils::skipUnknownField(cis, tag);
            break;
        }
    }

    cis->PopLimit(oldLimit);

	return encodingRule;
}