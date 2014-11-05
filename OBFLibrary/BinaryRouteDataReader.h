#pragma once

namespace gp = google::protobuf;
namespace gio = google::protobuf::io;

struct RouteInfoDataBlock;

enum RoadDirection : uint32_t
    {
        OneWayForward = -1,
        TwoWay = 0,
        OneWayReverse = 1
    };

enum RoadRestriction: int32_t
{
    Special_ReverseWayOnly = -1,

    Invalid = 0,

    NoRightTurn = 1,
    NoLeftTurn = 2,
    NoUTurn = 3,
    NoStraightOn = 4,
    OnlyRightTurn = 5,
    OnlyLeftTurn = 6,
    OnlyStraightOn = 7,
};

struct RouteEncodingRule
{
    enum Type : uint32_t
    {
        Access = 1,
        OneWay = 2,
        Highway = 3,
        Maxspeed = 4,
        Roundabout = 5,
        TrafficSignals = 6,
        RailwayCrossing = 7,
        Lanes = 8,
    };

    uint32_t _id;
    std::string _tag;
    std::string _value;
    Type _type;
    union
    {
        int32_t asSignedInt;
        uint32_t asUnsignedInt;
        float asFloat;
    } _parsedValue;

    bool isRoundabout() const;
    RoadDirection getDirection() const;
};

class BinaryRouteDataReader
{
public:
	BinaryRouteDataReader(void);
	~BinaryRouteDataReader(void);
	void ReadRouteInfo(gio::CodedInputStream* cis);
	std::shared_ptr<RouteEncodingRule>  ReadRulesInfo(gio::CodedInputStream* cis, uint32_t routeEncodingRuleId);
	std::string name;
	std::vector<std::shared_ptr<RouteEncodingRule>> routeRules;

	
};

struct RouteInfoDataBlock
{

};
