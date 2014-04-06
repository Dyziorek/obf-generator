#include "stdafx.h"
#include "OBFRenderingTypes.h"
#include "tinyxml2.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost\algorithm\string.hpp>
#include <boost\lexical_cast.hpp>

namespace fa = boost::algorithm;

char OBFRenderingTypes::TAG_DELIMETER = '/';

std::map<std::string, AmenityType> AmenityType::amenityTypes;

byte OBFRenderingTypes::RESTRICTION_NO_RIGHT_TURN = 1;
byte OBFRenderingTypes::RESTRICTION_NO_LEFT_TURN = 2;
byte OBFRenderingTypes::RESTRICTION_NO_U_TURN = 3;
byte OBFRenderingTypes::RESTRICTION_NO_STRAIGHT_ON = 4;
byte OBFRenderingTypes::RESTRICTION_ONLY_RIGHT_TURN = 5;
byte OBFRenderingTypes::RESTRICTION_ONLY_LEFT_TURN = 6;
byte OBFRenderingTypes::RESTRICTION_ONLY_STRAIGHT_ON = 7;

std::map<AmenityType, std::map<std::string, std::string>> OBFRenderingTypes::amenityNameVal;
std::map<std::string, AmenityType> OBFRenderingTypes::namedAmenity;
std::list<MapRouteTag> OBFRenderingTypes::routeTags;
std::map<std::string, MapRulType> OBFRenderingTypes::namedRulType;
std::vector<MapRulType> OBFRenderingTypes::rules;

AmenityType EMERGENCY = AmenityType::reg("emergency", "emergency"); // [TAG] emergency services //$NON-NLS-1$ //$NON-NLS-2$
AmenityType HEALTHCARE = AmenityType::reg("healthcare", "amenity"); // hospitals, doctors, ... //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType TRANSPORTATION = AmenityType::reg("transportation", "amenity"); // trffic-stuff, parking, public transportation, ... //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType BARRIER = AmenityType::reg("barrier", "barrier"); // [TAG] barrier + traffic_calming //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType TOURISM = AmenityType::reg("tourism", "tourism"); // [TAG] tourism hotel, sights, museum .. //$NON-NLS-1$ //$NON-NLS-2$ 
	AmenityType ENTERTAINMENT = AmenityType::reg("entertainment", "amenity"); // cinema, ...  = AmenityType::reg("", +! sauna, brothel) //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType HISTORIC = AmenityType::reg("historic", "historic"); // [TAG] historic places, battlefields, ... //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType SPORT = AmenityType::reg("sport", "sport"); // [TAG] sport //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType LEISURE = AmenityType::reg("leisure", "leisure"); // [TAG] leisure //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType GEOCACHE = AmenityType::reg("geocache", "geocache");  //$NON-NLS-1$

	AmenityType OTHER = AmenityType::reg("other", "amenity"); // grave-yard, post-office, [+Internet_access] //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType FINANCE = AmenityType::reg("finance", "amenity"); // bank, atm, ... //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType OFFICE = AmenityType::reg("office", "office"); // [TAG] office //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType ADMINISTRATIVE = AmenityType::reg("administrative", "administrative"); // [TAG] administrative //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType EDUCATION = AmenityType::reg("education", "amenity"); // school, ... //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType MAN_MADE = AmenityType::reg("man_made", "man_made"); // [TAG] man_made and others //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType SEAMARK = AmenityType::reg("seamark", "seamark"); // [TAG] seamark //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType SUSTENANCE = AmenityType::reg("sustenance", "amenity"); // restaurant, cafe, ... //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType SHOP = AmenityType::reg("shop", "shop"); // [TAG] amenity convenience  = AmenityType::reg("", product); clothes,... //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType NATURAL = AmenityType::reg("natural", "natural"); // [TAG] natural places, peaks, caves, trees,... //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType LANDUSE = AmenityType::reg("landuse", "landuse"); // [TAG] landuse //$NON-NLS-1$ //$NON-NLS-2$
	AmenityType MILITARY = AmenityType::reg("military", "military"); // [TAG] military //$NON-NLS-1$ //$NON-NLS-2$

	AmenityType OSMWIKI = AmenityType::reg("osmwiki", "osmwiki");  //$NON-NLS-1$
	AmenityType USER_DEFINED = AmenityType::reg("user_defined", "user_defined");  //$NON-NLS-1$

OBFRenderingTypes::OBFRenderingTypes(void)
{

}


OBFRenderingTypes::~OBFRenderingTypes(void)
{
}


void OBFRenderingTypes::loadXmlData()
{
	tinyxml2::XMLDocument xDoc;
	
	xDoc.LoadFile("D:\\osmdata\\rendering_types.xml");
	std::string poiParentCategory, poiParentPrefix;
	for ( const tinyxml2::XMLNode* node=xDoc.FirstChildElement(); node; node=node->NextSibling() )
	{
		// type
		tinyxml2::XMLElement* elem = (tinyxml2::XMLElement*)node;
		for ( const tinyxml2::XMLNode* inNode=elem->FirstChildElement(); inNode; inNode=inNode->NextSibling() )
		{
			tinyxml2::XMLElement* inElem = (tinyxml2::XMLElement*)inNode;
			if (std::string("type") == inElem->Name())
			{
				parseBasicElement(inElem, poiParentCategory, poiParentPrefix);
			}
			if (std::string("category") == inElem->Name())
			{
				poiParentCategory = read(inElem->Attribute("poi_category"));
				poiParentPrefix = read(inElem->Attribute("poi_prefix"));
				parseCategoryElement(inElem, poiParentCategory, poiParentPrefix);
			}
		}
	}

	xDoc.Clear();


	nameRule = MapRulType::createText("name");
	registerRuleType(nameRule);
	nameEnRule = MapRulType::createText("name:en");
	registerRuleType(nameEnRule);
}

void OBFRenderingTypes::parseRouteElement(tinyxml2::XMLElement* elemData)
{
	tinyxml2::XMLElement* inElem = (tinyxml2::XMLElement*)elemData;
	if (std::string("routing_type") == inElem->Name())
	{
		MapRouteTag rtype;
		std::string mode = read(inElem->Attribute("mode")); //$NON-NLS-1$
		boost::algorithm::to_lower(mode);
		rtype.tag = read(inElem->Attribute("tag")); //$NON-NLS-1$
		rtype.value = read(inElem->Attribute("value")); //$NON-NLS-1$
		rtype.tag2 = read(inElem->Attribute("tag2")); //$NON-NLS-1$
		rtype.value2 = read(inElem->Attribute("value2")); //$NON-NLS-1$
		rtype.base = read(inElem->Attribute("base")) == "true";
		rtype.replace = mode == "replace";
		rtype._register = mode == "register";
		rtype.amend = mode == "amend";
		rtype.text = mode == "text";
		rtype.relation = read(inElem->Attribute("relation")) == "true";
		routeTags.push_back(rtype);
	}
}

void OBFRenderingTypes::parseBasicElement(tinyxml2::XMLElement* elemData, std::string poiParentCategory,std::string poiParentPrefix)
{
	std::string tag = read(elemData->Attribute("tag"));
	std::string value = read(elemData->Attribute("value"));
	if (tag == "highway")
	{
		tag = "highway";
	}
	std::string additional = read(elemData->Attribute("additional"));
	MapRulType entity = MapRulType::createMainEntity(tag, value);
	if (additional == "true")
	{
		entity = MapRulType::createAdditional(tag, value);
	}
	else if (additional == "text")
	{
		entity = MapRulType::createText(tag);
	}
	entity.onlyMap =  read(elemData->Attribute("only_map")) != "";

	std::string targetTag = read(elemData->Attribute("target_tag"));
	std::string targetTagVal = read(elemData->Attribute("target_value"));
	if (targetTag != "" || targetTagVal != "")
	{
		if (targetTag == "")
		{
			targetTag = tag;
		}
		if (targetTagVal == "")
		{
			targetTagVal = value;
		}
		entity.targetTagValue = &namedRulType.find(constructRuleKey(targetTag, targetTagVal))->second;
	}
	std::string applyTo = read(elemData->Attribute("apply_to"));
	std::string applyValue = read(elemData->Attribute("apply_value"));
		if (applyTo != "" || applyValue != "") {
			entity.applyToTagValue.insert(TagValuePattern(applyTo, applyValue));
		}
		if(!entity.onlyMap) {
			registerRuleType(entity);
		}

		if (poiParentCategory != "") {
			entity.poiCategory = AmenityType::getAndRegisterType(poiParentCategory);
			entity.poiSpecified = true;
		}
		if (poiParentPrefix != "") {
			entity.poiPrefix = poiParentPrefix;
		}

		std::string poiCategory = read(elemData->Attribute("poi_category"));
		if (poiCategory != "") {
			entity.poiSpecified = true;
			if (poiCategory.length() == 0) {
				entity.poiCategory = AmenityType();
			} else {
				entity.poiCategory = AmenityType::getAndRegisterType(poiCategory);
			}
		}
		std::string poiPrefix = read(elemData->Attribute("poi_prefix"));
		if (poiPrefix != "") {
			entity.poiPrefix = poiPrefix;
		}
		
		if (!entity.isAdditional() && !entity.isText()) {
			entity.onlyPoint = read(elemData->Attribute("point")) == "true"; //$NON-NLS-1$
			entity.relation = read(elemData->Attribute("relation")) == "true"; //$NON-NLS-1$
			entity.namePrefix = read(elemData->Attribute("namePrefix")); //$NON-NLS-1$

			std::string v = read(elemData->Attribute("nameTags"));
			if (v != "") {
				
				boost::tokenizer< boost::char_separator<char> > tokens(v, boost::char_separator<char>(","));
				std::vector<std::string> names;
				BOOST_FOREACH(const std::string token,tokens){names.push_back(token);}
				entity.names.resize(names.size());
				for (unsigned int i = 0; i < names.size(); i++) {
					std::string tagName = names[i];
					if (entity.namePrefix.length() > 0) {
						tagName = entity.namePrefix + tagName;
					}
					MapRulType mt = MapRulType::createText(tagName);
					mt.applyToTagValue.clear();
					mt.applyToTagValue.insert(entity.tagValuePattern);
					mt = registerRuleType(mt);
					entity.names[i] = mt;
				}
			}
		}


		entity.onlyPoi =  read(elemData->Attribute("only_poi")) == "true";
		if(!entity.onlyPoi) {
			std::string val = read(elemData->Attribute("minzoom")); //$NON-NLS-1$
			entity.minzoom = 15;
			if (val != "") {
				entity.minzoom = boost::lexical_cast<int>(val);
			}
			val = read(elemData->Attribute("maxzoom")); //$NON-NLS-1$
			entity.maxzoom = 31;
			if (val != "") {
				entity.maxzoom = boost::lexical_cast<int>(val);
			}
			if(entity.onlyMap) {
				registerRuleType(entity);
			}
		}
		
		
}



void OBFRenderingTypes::parseCategoryElement(tinyxml2::XMLElement* elemData, std::string poiParentCategory,std::string poiParentPrefix)
{
	std::string poi_tag = read(elemData->Attribute("poi_tag"));
		if (poi_tag != "") {
			MapRulType rtype;
			rtype.poiCategory = AmenityType::getAndRegisterType(poiParentCategory);
			rtype.poiSpecified = true;
			rtype.relation = read(elemData->Attribute("relation")) == "true";
			rtype.poiPrefix = poiParentPrefix;
			rtype.onlyPoi = true;
			rtype.tagValuePattern = TagValuePattern(poi_tag, "");
			registerRuleType(rtype);
		}

		if (!elemData->NoChildren())
		{
			for ( const tinyxml2::XMLNode* inNode=elemData->FirstChildElement(); inNode; inNode=inNode->NextSibling() )
			{
				tinyxml2::XMLElement* inElem = (tinyxml2::XMLElement*)inNode;
				if (std::string("type") == inElem->Name())
				{
					parseBasicElement(inElem, poiParentCategory, poiParentPrefix);
				}
				if (std::string("routing_type") == inElem->Name())
				{
					parseRouteElement(inElem);
				}

			}
		}
}


MapRulType OBFRenderingTypes::registerRuleType(MapRulType& rt) {
		std::string tag = rt.tagValuePattern.tag;
		std::string val = rt.tagValuePattern.value;
		std::string keyVal = constructRuleKey(tag, val);
		if(namedRulType.find(keyVal) != namedRulType.end()){
			MapRulType mapRulType = namedRulType.find(keyVal)->second;
			if(mapRulType.isAdditional() || mapRulType.isText() ) {
				rt.id = mapRulType.id;
				if(rt.applyToTagValue.size() > 0 ){
					if(mapRulType.applyToTagValue.size() == 0) {
						rt.applyToTagValue.clear();
					} else {
						rt.applyToTagValue.insert(mapRulType.applyToTagValue.begin(), mapRulType.applyToTagValue.end());
					}
				}
//				namedRulType.put(keyVal, rt);
//				typeList.set(rt.id, rt);

				if(tag == "natural" &&  val == "coastline") {
					coastlineRule = rt;
				}
				return rt;
			} else {
				throw new std::exception((std::string("Duplicate ") + keyVal).c_str());
			}
		} else {
			rt.id = namedRulType.size();
			namedRulType.insert(std::make_pair(keyVal, rt));
			rules.push_back(rt);
			if(tag == "natural" &&  val == "coastline") {
					coastlineRule = rt;
			}
			return rt;
		}
	}


std::map<MapRulType, std::string> OBFRenderingTypes::getRelationPropogatedTags(EntityRelation relation) {
		std::map<MapRulType, std::string> propogated;
		std::map<std::string, std::string> ts = relation.tags;
		std::map<std::string, std::string>::iterator its = ts.begin();
		while(its != ts.end()) {
			std::pair<std::string, std::string> ev = *its;
			MapRulType rule = getRelationalTagValue(ev.first, ev.second);
			if(!rule.isEmpty()) {
				std::string value = ev.second;
				if(rule.targetTagValue != nullptr) {
					rule = *rule.targetTagValue;
					if(rule.getValue() != std::string("")) {
						value = rule.getValue();
					}
				}
				if (rule.names.empty()) {
					for (int i = 0; i < rule.names.size(); i++) {
						std::string tag = rule.names[i].tagValuePattern.tag.substr(rule.namePrefix.size());
						if(ts.find(tag)!= ts.end()) {
							propogated.insert(std::make_pair(rule.names[i], ts.find(tag)->second));
						}
					}
				}
				propogated.insert(std::make_pair(rule, value));
			}
			addOSMCSymbolsSpecialTags(propogated, ev);
			its++;
		}
		return propogated;
	}

MapRulType OBFRenderingTypes::getMapRuleType(std::string tag, std::string val) {
		return getRuleType(tag, val, false);
	}

MapRulType OBFRenderingTypes::getRelationalTagValue(std::string tag, std::string val) {
		MapRulType rType = getRuleType(tag, val, false);
		if(!rType.isEmpty() && rType.relation) {
			return rType;
		}
		return MapRulType();
	}

	MapRulType OBFRenderingTypes::getRuleType(std::string tag, std::string val, bool poi) {
		if (namedRulType.size() == 0)
		{
			loadXmlData();
		}
		std::map<std::string, MapRulType>& types = namedRulType;
		MapRulType rType;
		if (types.find(constructRuleKey(tag, val)) != types.end())
		{
			rType = types.find(constructRuleKey(tag, val))->second;
			if (rType.isEmpty() || (!rType.isPOI() && poi) || (!rType.isMap() && !poi)) {
				if (types.find(constructRuleKey(tag, "")) != types.end())
				{
					rType = types.find(constructRuleKey(tag, ""))->second;
				}
			}
		}
			if(rType.isEmpty() || (!rType.isPOI() && poi) || (!rType.isMap() && !poi)) {
			return rType;
		} else if(rType.isAdditional() && rType.tagValuePattern.value == "") {
			MapRulType parent = rType;
			rType = MapRulType::createAdditional(tag, val);
			rType.additional = true;
			rType.applyToTagValue = parent.applyToTagValue;
			rType.onlyMap = parent.onlyMap;
			rType.onlyPoi = parent.onlyPoi;
			rType.onlyPoint = parent.onlyPoint;
			rType.poiSpecified = parent.poiSpecified;
			rType.poiCategory = parent.poiCategory;
			rType.poiPrefix = parent.poiPrefix;
			rType.namePrefix = parent.namePrefix;
			registerRuleType(rType);
		}
		return rType;
	}


 void OBFRenderingTypes::addOSMCSymbolsSpecialTags(std::map<MapRulType,std::string> propogated, std::pair<std::string,std::string> ev) {
	 if (std::string("osmc:symbol") == ev.first) {
		 
		 boost::char_separator<char> sep(":");
		 
		 boost::tokenizer< boost::char_separator<char> > tokenizer(ev.second, sep);
		 boost::tokenizer< boost::char_separator<char> >::iterator itToken = tokenizer.begin();
			std::vector<std::string> tokens;
			for (int iToken = 0; iToken < 6 && itToken != tokenizer.end(); iToken++)
			{
				tokens.push_back(itToken.current_token());
				itToken++;
			}
			if (tokenizer.begin() != tokenizer.end())
			{
				tokens.push_back(tokenizer.begin().current_token());
			}
			if (tokens.size() > 0) {
				std::string symbol_name = "osmc_symbol_" + tokens[0];
				MapRulType rt = getMapRuleType(symbol_name, "");
				if(!rt.isEmpty()) {
					propogated.insert(std::make_pair(rt, ""));
					if (tokens.size() > 2 && rt.names.size() != 0) {
						std::string symbol = "osmc_symbol_" + tokens[1] + "_" + tokens[2] + "_name";
						std::string name = "\u00A0";
						std::string token3 = tokens[3];
						boost::algorithm::trim(token3);
						if (tokens.size() > 3 && token3.length() > 0) {
							name = tokens[3];
						}
						for(int k = 0; k < rt.names.size(); k++) {
							if(rt.names[k].tagValuePattern.tag== symbol) {
								propogated.insert(std::make_pair(rt.names[k], name));
							}
						}
					}
				}
			}
		}
	 if (std::string("color") == ev.first || std::string("colour") == ev.first) {
		 std::string vl = ev.second;
		 boost::algorithm::to_lower(vl);
			if(vl == "#ffff00"){
				vl = "yellow";
			} else if(vl == "#ff0000"){
				vl = "red";
			} else if(vl == "#00ff00"){
				vl = "green";
			} else if(vl == "#0000ff"){
				vl = "blue";
			} else if(vl == "#000000"){
				vl = "black";
			}
			std::string nm = "color_"+vl;
			MapRulType rt = getMapRuleType(nm, "");
			if(!rt.isEmpty()) {
				propogated.insert(std::make_pair(rt, ""));
			}
		}
	}
	

 	AmenityType OBFRenderingTypes::getAmenityTypeForRelation(std::string tag, std::string val){
		return getAmenityType(tag, val, true);
	}
	
	AmenityType OBFRenderingTypes::getAmenityType(std::string tag, std::string val, bool relation){
		// register amenity types
		std::map<std::string, MapRulType>& rules = getRuleTypes();
		MapRulType rt;
		if (rules.find(constructRuleKey(tag, val)) != rules.end())
		{
			rt = rules.at(constructRuleKey(tag, val));
		}
		
		if(!rt.isEmpty() && rt.isPOISpecified()) {
			if((relation && !rt.relation) || rt.isAdditionalOrText()) {
				return AmenityType();
			}
			return rt.poiCategory;
		}
		if (rules.find(constructRuleKey(tag, "")) != rules.end())
		{
			rt = rules.at(constructRuleKey(tag, ""));
		}
		
		if(!rt.isEmpty() && rt.isPOISpecified()) {
			if((relation && !rt.relation) || rt.isAdditionalOrText()) {
				return AmenityType();
			}
			return rt.poiCategory;
		}
		return AmenityType();
	}

	std::list<std::map<std::string, std::string>> OBFRenderingTypes::splitTagsIntoDifferentObjects(const std::map<std::string, std::string> tags) {
		// check open sea maps tags
		boolean split = splitIsNeeded(tags);
		if(!split) {
			 std::list<std::map<std::string, std::string>> listMap;
			 listMap.push_back(tags);
			 return listMap;
		} else {
			return splitOpenSeaMapsTags(tags);
		}
	}

	 std::list<std::map<std::string, std::string>> OBFRenderingTypes::splitOpenSeaMapsTags(const std::map<std::string, std::string> tags) {
		std::map<std::string,std::map<std::string, std::string>> groupByOpenSeamaps;
		std::map<std::string, std::string> common;
		std::string ATTACHED_KEY = "seamark:attached";
		std::string type = "";
		for (auto s : tags) {
			std::string value = s.second;
			if (s.first == "seamark:type") {
				type = value;
				common.insert(std::make_pair(ATTACHED_KEY, openSeaType(value)));
			} else if (fa::starts_with(s.first,"seamark:")) {
				std::string stype = s.first.substr(std::string("seamark:").size());
				int ind = stype.find_first_of(':');
				if (ind == -1) {
					common.insert(std::make_pair(s.first, value));
				} else {
					std::string group = openSeaType(stype.substr(0, ind));
					std::string add = stype.substr(ind + 1);
					if (groupByOpenSeamaps.find(group) == groupByOpenSeamaps.end()) {
						groupByOpenSeamaps.insert(std::make_pair(group, std::map<std::string, std::string>()));
					}
					groupByOpenSeamaps.find(group)->second.insert(std::make_pair("seamark:" + add, value));
				}
			} else {
				common.insert(std::make_pair(s.first, value));
			}
		}
		std::list<std::map<std::string, std::string>> res;
		for (auto g : groupByOpenSeamaps) {
			g.second.insert(common.begin(), common.end());
			g.second.insert(make_pair("seamark", g.first));
			if (openSeaType(type)== g.first) {
				g.second.erase(ATTACHED_KEY);
				g.second.insert(make_pair("seamark", type));
				res.insert(res.begin(), g.second);
			} else {
				res.push_back(g.second);
			}
		}
		return res;
	}	
	
	
	std::string OBFRenderingTypes::openSeaType(std::string value) {
		if(value == "light_major" || value == "light_minor") {
			return "light";
		}
		return value;
	}


	std::string OBFRenderingTypes::getAmenitySubtype(std::string tag, std::string val){
		std::string prefix = getAmenitySubtypePrefix(tag, val);
		if(prefix != ""){
			return prefix + val;
		}
		return val;
	}
	
	std::string OBFRenderingTypes::getAmenitySubtypePrefix(std::string tag, std::string val){
		std::map<std::string, MapRulType>& rules = getRuleTypes();
		MapRulType rt;
		if (rules.find(constructRuleKey(tag, val)) != rules.end())
		{
			rt = rules.at(constructRuleKey(tag, val));
		}
		if(!rt.isEmpty() && rt.poiPrefix != "" && rt.isPOI()) {
			return rt.poiPrefix;
		}
		if (rules.find(constructRuleKey(tag, "")) != rules.end())
		{
			rt = rules.find(constructRuleKey(tag, ""))->second;
		}
		if(rt.isEmpty() && rt.poiPrefix != "" && rt.isPOI()) {
			return rt.poiPrefix;
		}
		return "";
	}


 std::map<std::string, std::string> OBFRenderingTypes::getAmenityAdditionalInfo(std::map<std::string, std::string> tags, AmenityType type, std::string subtype) {
		std::map<std::string, std::string> map;
		for (std::pair<std::string,std::string> tag : tags) {
			std::string val = tag.second;
			MapRulType rType = getRuleType(tag.first, val, true);
			if (!rType.isEmpty() && val.size()  > 0) {
				if(rType == nameEnRule &&  val == tags.at("name")) {
					continue;
				}
				if(rType.targetTagValue != nullptr) {
					rType = *rType.targetTagValue;
				}
				if (rType.isAdditionalOrText()) {
					boolean applied = rType.applyToTagValue.size() == 0;
					if(!applied) {
						std::set<TagValuePattern>::iterator it = rType.applyToTagValue.begin();
						while(!applied && it != rType.applyToTagValue.end()) {
							TagValuePattern nv = *it;
							applied = nv.isApplicable(tags);
						}
					}
					if (applied) {
						if (!rType.isText() && !rType.tagValuePattern.value.empty()) {
							val = rType.tagValuePattern.value;
						}
						map.insert(std::make_pair(rType.tagValuePattern.tag, val));
					}
				}
			}
		}
		return map;
	}



 bool OBFRenderingTypes::encodeEntityWithType(std::shared_ptr<EntityBase> e, int zoom, std::list<long>& outTypes, 
			std::list<long>& outAddTypes, std::map<MapRulType, std::string>& namesToEncode, std::list<MapRulType>& tempListNotUsed) {
				if(splitIsNeeded(e->tags)) {
			if(splitTagsIntoDifferentObjects(e->tags).size() > 1) {
				throw new std::bad_exception();
			}
		}
		auto inst = std::static_pointer_cast<EntityNode, EntityBase>(e);
		bool isNode = inst;
		return encodeEntityWithType(isNode, 
				e->tags, zoom, outTypes, outAddTypes, namesToEncode, tempListNotUsed);
	}
	
bool OBFRenderingTypes::encodeEntityWithType(bool isNode, std::map<std::string, std::string> tags, int zoom, std::list<long>& outTypes, 
			std::list<long>& outAddTypes, std::map<MapRulType, std::string>& namesToEncode, std::list<MapRulType>& tempListNotUsed) {
		outTypes.clear();
		outAddTypes.clear();
		namesToEncode.clear();
		boolean area = false;
		if (tags.find("area") != tags.end())
		{
			area = tags.at("area") == "yes" || tags.at("area") == "true";
		}

		for (auto tag : tags) {
			std::string val = tag.second;
			MapRulType rType = getMapRuleType(tag.first, val);
			if (!rType.isEmpty()) {
				if (rType.minzoom > zoom || rType.maxzoom < zoom) {
					continue;
				}
				if (rType.onlyPoint && !isNode) {
					continue;
				}
				std::string nameVal = "";
				if (tags.find("name") != tags.end())
				{
					nameVal = tags.at("name");
				}
				if(rType == nameEnRule && nameVal == val) {
					continue;
				}
				if(rType.targetTagValue != nullptr) {
					rType = *rType.targetTagValue;
				}
				rType.updateFreq();
				if (!rType.isAdditionalOrText()) {
					outTypes.push_back(rType.id);
				} else {
					boolean applied = rType.applyToTagValue.empty();
					if(!applied) {
						auto it = rType.applyToTagValue.begin();
						while(!applied && it != rType.applyToTagValue.end()) {
							TagValuePattern nv = *it;
							applied = nv.isApplicable(tags);
							it++;
						}
					}
					if (applied) {
						if (rType.isAdditional()) {
							outAddTypes.push_back(rType.id);
						} else if (rType.isText()) {
							namesToEncode.insert(std::make_pair(rType, val));
						}
					}
				}
			}
		}
        // sort to get most important features as first type (important for rendering)
        outTypes.sort();
        outAddTypes.sort();
		return area;
	}