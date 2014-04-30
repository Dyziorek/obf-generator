#pragma once
#include <boost/algorithm/string.hpp>
#include "OBFRenderingTypes.h"
#include "EntityNode.h"

namespace fa = boost::algorithm;

class MapRouteType {
public: 
		int freq;
		int id;
		int targetId;
		std::string tag;
		std::string value;
		
		int getInternalId() {
			return id;
		}
		
		int getFreq() {
			return freq;
		}
		
		int getTargetId() {
			return targetId;
		}
		
		std::string getTag() {
			return tag;
		}
		
		std::string getValue() {
			return value;
		}
		
		void setTargetId(int targetId) { 
			this->targetId = targetId;
		}
		

		std::string toString() {
			if (value == "") {
				return "'" + tag + "'";
			}
			return tag + "='" + value + "'";
		}
		MapRouteType(){ freq = 0;}

		bool operator<(const MapRouteType& op2) const
		{
			return id < op2.id;
		}
};



class MapRoutingTypes
{

public:
	MapRoutingTypes(void);
	~MapRoutingTypes(void);

private:
	static std::set<std::string> TAGS_TO_SAVE;
	static std::set<std::string> TAGS_TO_ACCEPT;
	static boost::unordered_map<std::string, std::string> TAGS_TO_REPLACE;
	static std::set<std::string> TAGS_RELATION_TO_ACCEPT;
	static std::set<std::string> TAGS_TEXT;
	static std::set<std::string> BASE_TAGS_TEXT;
	static std::set<std::string> BASE_TAGS_TO_SAVE;
	static boost::unordered_map<std::string, std::string> BASE_TAGS_TO_REPLACE;
	static char TAG_DELIMETER; //$NON-NLS-1$
	
	std::map<std::string, MapRouteType> types;
	std::list<MapRouteType> listTypes;
	MapRouteType refRuleType;
	MapRouteType nameRuleType; 
	
    bool contains(std::set<std::string> s, std::string tag, std::string value);
	
	std::string getMap(boost::unordered_map<std::string, std::string>& s, std::string tag, std::string value);
	
	
	
	bool startsWith(std::set<std::string>& s, std::string tag, std::string value);

	MapRouteType registerRule(std::string tag, std::string val);
	
	std::string converBooleanValue(std::string value);

protected:
	static std::string getTagKey(std::string tagValue) ;
	
	static std::string getValueKey(std::string tagValue) ;
public:
	MapRoutingTypes(OBFRenderingTypes baseTypes) ;

	static std::string constructRuleKey(std::string tag, std::string val) ;
	

	
	MapRouteType getRefRuleType() ;
	
	
	MapRouteType getNameRuleType() ;
	
	boost::unordered_map<std::string, std::string> getRouteRelationPropogatedTags(EntityBase& e) ;

	bool encodeEntity(EntityWay& et, std::vector<int>& outTypes, std::map<MapRouteType, std::string>& names);
	
	
	bool encodeBaseEntity(EntityWay& et, std::vector<int> &outTypes, std::map<MapRouteType, std::string>& names);

	
	void encodePointTypes(EntityWay& e, std::map<__int64, std::vector<int>>& pointTypes);
	
	MapRouteType getTypeByInternalId(int id) ;
	
	std::list<MapRouteType> getEncodingRuleTypes() ;
	
	
};
