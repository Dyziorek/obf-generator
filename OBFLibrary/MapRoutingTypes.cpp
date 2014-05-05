#include "stdafx.h"
#include "MapRoutingTypes.h"


char MapRoutingTypes::TAG_DELIMETER = '/'; //$NON-NLS-1$

std::set<std::string> MapRoutingTypes::TAGS_TO_SAVE;
std::set<std::string> MapRoutingTypes::TAGS_TO_ACCEPT;
boost::unordered_map<std::string, std::string> MapRoutingTypes::TAGS_TO_REPLACE;
std::set<std::string> MapRoutingTypes::TAGS_RELATION_TO_ACCEPT;
std::set<std::string> MapRoutingTypes::TAGS_TEXT;
std::set<std::string> MapRoutingTypes::BASE_TAGS_TEXT;
std::set<std::string> MapRoutingTypes::BASE_TAGS_TO_SAVE;
boost::unordered_map<std::string, std::string> MapRoutingTypes::BASE_TAGS_TO_REPLACE;

MapRoutingTypes::MapRoutingTypes(void)
{
}


MapRoutingTypes::~MapRoutingTypes(void)
{
}

    bool MapRoutingTypes::contains(std::set<std::string> s, std::string tag, std::string value) {
		if(s.find(tag) != s.end() || s.find(tag + TAG_DELIMETER + value) != s.end()){
			return true;
		}
		return false;
	}
	
	std::string MapRoutingTypes::getMap(boost::unordered_map<std::string, std::string>& s, std::string tag, std::string value) {
		//std::string r = s.find(tag);
		if (s.find(tag) != s.end()) {
			return s.find(tag)->second;
		}
		if (s.find(tag + TAG_DELIMETER + value) != s.end())
		{
			return s.find(tag + TAG_DELIMETER + value)->second;
		}
		return "";
	}
	
	
	
	bool MapRoutingTypes::startsWith(std::set<std::string>& s, std::string tag, std::string value) {
		for(std::string st : s) {
			if(fa::starts_with(tag, st)) {
				return true;
			}
		}
		return false;
	}

	MapRouteType MapRoutingTypes::registerRule(std::string tag, std::string val) {
		std::string id = constructRuleKey(tag, val);
		if(types.find(id) == types.end()) {
			MapRouteType rt;
			// first one is always 1
			rt.id = types.size() + 1;
			rt.tag = tag;
			rt.value = val;
			types.insert(std::make_pair(id, rt));
			listTypes.push_back(rt);
			if(tag == "ref"){
				refRuleType = rt;
			}
			if(tag == "name" ){
				nameRuleType = rt;
			}
		}
		MapRouteType type = types.find(id)->second;
		type.freq ++;
		return type;
	}
	
	std::string MapRoutingTypes::converBooleanValue(std::string value){
		if(value == "true") {
			return "yes";
		} else if(value == "false") {
			return "no";
		}
		return value;
	}


	std::string MapRoutingTypes::getTagKey(std::string tagValue) {
		int i = tagValue.find_first_of(TAG_DELIMETER);
		if(i >= 0){
			return tagValue.substr(0, i);
		}
		return tagValue;
	}
	
	std::string MapRoutingTypes::getValueKey(std::string tagValue) {
		int i = tagValue.find_first_of(TAG_DELIMETER);
		if(i >= 0){
			return tagValue.substr(i + 1);
		}
		return "";
	}

	MapRoutingTypes::MapRoutingTypes(OBFRenderingTypes baseTypes) {
		for(MapRouteTag tg :  baseTypes.getRouteTags() ) {
			std::string t = tg.tag;
			if(tg.value != "") {
				t += TAG_DELIMETER + tg.value;
			}
			if(tg._register) {
				if(tg.relation) {
					TAGS_RELATION_TO_ACCEPT.insert(t);
				}
				TAGS_TO_ACCEPT.insert(t);
			} else if (tg.replace) {
				std::string t2 = tg.tag2;
				if (tg.value2 != "") {
					t2 += TAG_DELIMETER + tg.value2;
				}
				if (tg.base) {
					BASE_TAGS_TO_REPLACE.insert(std::make_pair(t, t2));
				}
				TAGS_TO_REPLACE.insert(std::make_pair(t, t2));
			} else if(tg.text) {
				if(tg.base) {
					BASE_TAGS_TEXT.insert(t);
				}
				TAGS_TEXT.insert(t);
			} else if(tg.amend) {
				if(tg.base) {
					BASE_TAGS_TO_SAVE.insert(t);
				}
				TAGS_TO_SAVE.insert(t);
			}
		}
	}
	
	std::string MapRoutingTypes::constructRuleKey(std::string tag, std::string val) {
		if(val == ""){
			return tag;
		}
		return tag + TAG_DELIMETER + val;
	}
	

	
	MapRouteType MapRoutingTypes::getRefRuleType() {
		return refRuleType;
	}
	
	
	MapRouteType MapRoutingTypes::getNameRuleType() {
		return nameRuleType;
	}
	
	boost::unordered_map<std::string, std::string> MapRoutingTypes::getRouteRelationPropogatedTags(EntityBase& e) {
		boost::unordered_map<std::string, std::string> propogated; 
		bool foundProp = false;
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = converBooleanValue(es.second);
			if(contains(TAGS_RELATION_TO_ACCEPT, tag, value)) {
				foundProp = true;
				break;
			}
		}
		if(!foundProp) {
			return propogated;
		}
		
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = converBooleanValue(es.second);
			if( TAGS_TEXT.find(tag) != TAGS_TEXT.end()) {
				propogated.insert(std::make_pair(tag, value));
			}
			if(contains(TAGS_TO_ACCEPT, tag, value) ||
					startsWith(TAGS_TO_SAVE, tag, value)) {
				propogated.insert(std::make_pair(tag, value));
			}
		}
		return propogated;
	}
	

	
	
	bool MapRoutingTypes::encodeEntity(EntityWay& et, std::vector<int>& outTypes, std::map<MapRouteType, std::string>& names){
		EntityWay e = et;
		bool init = false;
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = es.second;
			if (contains(TAGS_TO_ACCEPT, tag, value)) {
				init = true;
				break;
			}
		}
		if(!init) {
			return false;
		}
		outTypes.clear();
		names.clear();
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = converBooleanValue(es.second);
			std::string tvl = getMap(TAGS_TO_REPLACE, tag, value);
			if(tvl != "") {
				int i = tvl.find_first_of(TAG_DELIMETER);
				tag = tvl.substr(0, i);
				value = tvl.substr(i + 1);
			}
			if(TAGS_TEXT.find(tag) != TAGS_TEXT.end()) {
                names.insert(std::make_pair(registerRule(tag, ""), value));
            } else if(contains(TAGS_TO_ACCEPT, tag, value) || startsWith(TAGS_TO_SAVE, tag, value) || getMap(TAGS_TO_REPLACE, tag, value) != "") {
				outTypes.push_back(registerRule(tag, value).id);
			}
		}
		return true;
	}
	
	bool MapRoutingTypes::encodeBaseEntity(EntityWay& et, std::vector<int>& outTypes, std::map<MapRouteType, std::string>& names){
		EntityWay e = et;
		bool init = false;
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = es.second;
			if (contains(TAGS_TO_ACCEPT, tag, value)) {
				if(fa::starts_with(value, "trunk") || fa::starts_with(value, "motorway")
						|| fa::starts_with(value, "primary") || fa::starts_with(value, "secondary")
						|| fa::starts_with(value, "tertiary")
						||  fa::starts_with(value, "ferry")
						) {
					init = true;
					break;
				}
			}
		}
		if(!init) {
			return false;
		}
		outTypes.clear();
		names.clear();
		for(std::pair<std::string, std::string> es : e.tags) {
			std::string tag = es.first;
			std::string value = converBooleanValue(es.second);
			std::string tvl = getMap(BASE_TAGS_TO_REPLACE, tag, value);
			if(tvl != "") {
				int i = tvl.find_first_of(TAG_DELIMETER);
				tag = tvl.substr(0, i);
				value = tvl.substr(i + 1);
			}
			if(BASE_TAGS_TEXT.find(tag) != BASE_TAGS_TEXT.end()) {
				names.insert(std::make_pair(registerRule(tag, ""), value));
			}
			if(contains(TAGS_TO_ACCEPT, tag, value) ||
					startsWith(BASE_TAGS_TO_SAVE, tag, value)) {
						outTypes.push_back(registerRule(tag, value).id);
			}
		}
		return true;
	}
	

	
	void MapRoutingTypes::encodePointTypes(EntityWay& e, std::map<__int64, std::vector<int>>& pointTypes){
		pointTypes.clear();
		for(std::shared_ptr<EntityNode> nd : e.nodes ) {
			if (nd.get() != nullptr) {
				for (std::pair<std::string, std::string> es : nd->tags) {
					std::string tag = es.first;
					std::string value = converBooleanValue(es.second);
					std::string tvl = getMap(TAGS_TO_REPLACE, tag, value);
					if(tvl != "") {
						int i = tvl.find_first_of(TAG_DELIMETER);
						tag = tvl.substr(0, i);
						value = tvl.substr(i + 1);
					}
					if (contains(TAGS_TO_ACCEPT, tag, value) || startsWith(TAGS_TO_SAVE, tag, value)) {
						if (pointTypes.find(nd->id) == pointTypes.end()) {
							pointTypes.insert(std::make_pair(nd->id, std::vector<int>()));
						}
						pointTypes.find(nd->id)->second.push_back(registerRule(tag, value).id);
					}
				}
			}
		}
	}
	
	MapRouteType MapRoutingTypes::getTypeByInternalId(int id) {
		std::list<MapRouteType>::iterator itVal = listTypes.begin();
		std::advance(itVal, id - 1);
		return *itVal;
	}
	
	std::list<MapRouteType> MapRoutingTypes::getEncodingRuleTypes() {
		return listTypes;
	}
	

