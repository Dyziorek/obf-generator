#include "stdafx.h"
#include <mutex>
#include "tinyxml2.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

static std::mutex defualtMapStylesLocker;
static std::shared_ptr<DefaultMapStyleValue> defaultMapStyles;

#include <cvt/wstring>
#include <codecvt>
#include <boost\format.hpp>

MapStyleInfo::MapStyleInfo(void) : _stringsIdBase(0)
{
	registerDefaultValues();
	currentRule = none;
	std::shared_ptr<MapStyleValue> valTypeRek(new MapStyleValue(ValType::Booltype, AccessType::In, std::string("SS"), true));

}


MapStyleInfo::~MapStyleInfo(void)
{
}

std::shared_ptr<DefaultMapStyleValue> MapStyleInfo::getDefaultValueDefinitions()
{
    std::lock_guard<std::mutex> locker(defualtMapStylesLocker);

    if(!static_cast<bool>(defaultMapStyles))
		defaultMapStyles.reset(new DefaultMapStyleValue());

    return defaultMapStyles;
}

void MapStyleInfo::loadRenderStyles(const char* path)
{
	tinyxml2::XMLDocument xDoc;
	if (path == nullptr)
	{
		xDoc.LoadFile("D:\\osmData\\default.render.xml");
	}
	else
	{
		xDoc.LoadFile(path);
	}
	for ( const tinyxml2::XMLNode* node=xDoc.FirstChildElement(); node; node=node->NextSibling() )
	{
		tinyxml2::XMLElement* inElem = (tinyxml2::XMLElement*)node;
		if (std::string("renderingStyle") != inElem->Name())
		{
			std::string tag = read(inElem->Attribute("depends"));
			_parentName = tag;
			continue;
		}
		for ( const tinyxml2::XMLNode* inode=node->FirstChildElement(); inode; inode=inode->NextSibling() )
		{
			if (inode->ToElement() == NULL)
				continue;

			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)inode;
			if (std::string("renderingProperty") == iElem->Name())
			{
				parseProperty(iElem);
			}
			if (std::string("renderingAttribute") == iElem->Name())
			{
				parseAttribute(iElem);
			}
			if (std::string("renderingConstant") == iElem->Name())
			{
				parseConstant(iElem);
			}
			if (std::string("order") == iElem->Name())
			{
				currentRule = order;
				parseItemClass(iElem, _orderRules);
			}
			if (std::string("text") == iElem->Name())
			{
				currentRule = text;
				parseItemClass(iElem, _textRules);
			}
			if (std::string("point") == iElem->Name())
			{
				currentRule = point;
				parseItemClass(iElem, _pointRules);
			}
			if (std::string("polygon") == iElem->Name())
			{
				currentRule = polygon;
				parseItemClass(iElem, _polygonRules);
			}
			if (std::string("line") == iElem->Name())
			{
				currentRule = line;
				parseItemClass(iElem, _lineRules);
			}

		}
	}
}

void MapStyleInfo::parseProperty(tinyxml2::XMLElement* workElem)
{
		MapStyleChoiceValue* inputValue = nullptr;

		auto name = read(workElem->Attribute("attr"));
        auto type = read(workElem->Attribute("type"));
        auto title = read(workElem->Attribute("name"));
        auto description = read(workElem->Attribute("description"));
        auto encodedPossibleValues = read(workElem->Attribute("possibleValues"));
        std::vector<std::string> possibleValues;
		if(!encodedPossibleValues.empty())
		{
			boost::char_separator<char> sep(",");
			boost::tokenizer< boost::char_separator<char> > tokens(encodedPossibleValues, sep);
            BOOST_FOREACH(const std::string token, tokens){
				possibleValues.push_back(token);
			};
		}
        for(auto itPossibleValue = possibleValues.begin(); itPossibleValue != possibleValues.end(); ++itPossibleValue)
        {
            auto& possibleValue = *itPossibleValue;
            possibleValue = read(possibleValue.c_str());
        }
        if(type == "string")
        {
            inputValue = new MapStyleChoiceValue(
				ValType::Stringtype,
                name,
                title,
                description,
                possibleValues);
        }
        else if(type == "boolean")
        {
            inputValue = new MapStyleChoiceValue(
				ValType::Booltype,
                name,
                title,
                description,
                possibleValues);
        }
        else // treat as integer
        {
            inputValue = new MapStyleChoiceValue(
				ValType::Inttype,
                name,
                title,
                description,
                possibleValues);
        }

        registerValue(inputValue);
}


void MapStyleInfo::parseAttribute(tinyxml2::XMLElement* workElem)
{
	 auto attrName = read(workElem->Attribute("name"));
	 if (_attributes.find(attrName) == _attributes.end())
	 {
		_attributes[attrName] = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));
	 }
	 std::vector<std::shared_ptr<Lexeme>> mapRuleStack;
	 mapRuleStack.push_back(std::shared_ptr<Lexeme>(new Rule(this, _attributes[attrName])));

	 for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, mapRuleStack);
			}
	 }

}
void MapStyleInfo::parseConstant(tinyxml2::XMLElement* workElem)
{
	std::string name = read(workElem->Attribute("name"));
	std::string value = read(workElem->Attribute("value"));
	_renderConstants[name] = value;
}

void MapStyleInfo::parseFilter(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRuleStack)
{
	std::unordered_map<std::string, std::string> attrVals;

	if (!mapRuleStack.empty() && mapRuleStack.back()->_id == Lexeme::Group)
	{
		auto attrParent = std::static_pointer_cast<Group>(mapRuleStack.back())->attributes;
		for (auto itAttr : attrParent)
		{
			attrVals[itAttr.first] = itAttr.second;
		}
	}

	for ( const tinyxml2::XMLAttribute* xAttr = workElem->FirstAttribute(); xAttr; xAttr=xAttr->Next())
	{
		std::string attrName = read(xAttr->Name());
		std::string attrVal = read(xAttr->Value());
		attrVals[attrName] = attrVal;
	}
	std::shared_ptr<MapStyleRule> filterRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, attrVals));
	mapRuleStack.push_back(std::shared_ptr<Lexeme>(new Rule(this, filterRule)));
	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	{
		tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
		if (std::string("filter") == iElem->Name())
		{
			parseFilter(iElem, mapRuleStack);
		}
	 }

	auto lastItem = mapRuleStack.back();
	mapRuleStack.pop_back();

	if (mapRuleStack.empty())
	{
		registerRule(currentRule, filterRule);
	}
	else
	{
		if (mapRuleStack.back()->_id == Lexeme::Rule)
		{
			std::static_pointer_cast<Rule>(mapRuleStack.back())->_rule->_ifElseChildren.push_back(filterRule);
		}
		else if (mapRuleStack.back()->_id == Lexeme::Group)
		{
			std::static_pointer_cast<Group>(mapRuleStack.back())->children.push_back(filterRule);
		}
	}
}


void MapStyleInfo::parseGroupFilter(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRuleStack)
{
	std::unordered_map<std::string, std::string> attrVals;
	for ( const tinyxml2::XMLAttribute* xAttr = workElem->FirstAttribute(); xAttr; xAttr=xAttr->Next())
	{
		std::string attrName = read(xAttr->Name());
		std::string attrVal = read(xAttr->Value());
		attrVals[attrName] = attrVal;
	}

	std::shared_ptr<MapStyleRule> groupRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this,attrVals));

	if (mapRuleStack.empty())
	{
		assert(false);
	}
	else if (mapRuleStack.back()->_id == Lexeme::Group)
	{
		auto groupData = std::static_pointer_cast<Group>(mapRuleStack.back());
		groupData->addGroupFilter(groupRule);
	}
	else if (mapRuleStack.back()->_id == Lexeme::Rule)
	{
		auto ruleData = std::static_pointer_cast<Rule>(mapRuleStack.back());
		auto rulePtr = ruleData->_rule;
		rulePtr->_ifChildren.push_back(groupRule);
	}

	
	mapRuleStack.push_back(std::shared_ptr<Lexeme>(new Rule(this, groupRule)));
	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, mapRuleStack);
			}
			if (std::string("groupFilter") == iElem->Name())
			{
				parseGroupFilter(iElem, mapRuleStack);
			}
			
	 }
	mapRuleStack.pop_back();

}
void MapStyleInfo::parseGroup(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRuleStack)
{
	std::unordered_map<std::string, std::string> attrVals;

	if (!mapRuleStack.empty() && mapRuleStack.back()->_id == Lexeme::Group)
	{
		auto attrParent = std::static_pointer_cast<Group>(mapRuleStack.back())->attributes;
		for (auto itAttr : attrParent)
		{
			attrVals[itAttr.first] = itAttr.second;
		}
	}

	for ( const tinyxml2::XMLAttribute* xAttr = workElem->FirstAttribute(); xAttr; xAttr=xAttr->Next())
	{
		std::string attrName = read(xAttr->Name());
		std::string attrVal = read(xAttr->Value());
		attrVals[attrName] = attrVal;
	}
	
	//std::shared_ptr<MapStyleRule> groupRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, attrVals));
	Group* grp = new Group(this);
	for (auto hashVal : attrVals)
	{
		grp->attributes[hashVal.first] = hashVal.second;
	}
	mapRuleStack.push_back(std::shared_ptr<Lexeme>(grp));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	{
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, mapRuleStack);
			}
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, mapRuleStack);
			}
			if (std::string("groupFilter") == iElem->Name())
			{
				parseGroupFilter(iElem, mapRuleStack);
			}
			
	 }
	auto lastItem = mapRuleStack.back();
	mapRuleStack.pop_back();
    if (mapRuleStack.empty())
    {
		assert(lastItem->_id == Lexeme::Group);
		std::static_pointer_cast<Group>(lastItem)->registerGlobalRules(currentRule);
    }
    else
    {
        const auto group = mapRuleStack.back();
		if(group->_id == Lexeme::Group)
			std::static_pointer_cast<Group>(group)->subgroups.push_back(std::move(lastItem));
    }

}

void MapStyleInfo::parseItemClass(tinyxml2::XMLElement* workElem, std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset)
{

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				std::shared_ptr<MapStyleRule> orderRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));
				std::vector<std::shared_ptr<Lexeme>> vecRuleStack;
				//std::shared_ptr<Lexeme> parentItem(new Rule(this, orderRule));
				//vecRuleStack.push_back(parentItem);
				parseFilter(iElem, vecRuleStack);
			}
			if (std::string("group") == iElem->Name())
			{
				std::shared_ptr<MapStyleRule> orderRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));
				std::vector<std::shared_ptr<Lexeme>> vecRuleStack;
				//std::shared_ptr<Lexeme> parentItem(new Group(this, orderRule));
				//vecRuleStack.push_back(parentItem);
				parseGroup(iElem, vecRuleStack);
			}
			
	 }
}

void MapStyleInfo::registerValue(MapStyleValue* value)
{
	_valueDefinitions[value->name] = std::shared_ptr<MapStyleValue>(value);
	
}

void MapStyleInfo::registerDefaultValue(const std::shared_ptr<MapStyleValue>& value)
{
	_valueDefinitions[value->name] = std::shared_ptr<MapStyleValue>(value);
	_firstLoadedValue = _valueDefinitions.size();
}

void MapStyleInfo::registerDefaultValues()
{
#define SETUP_DEFAULT_MAP(nameVar, typeData, accType, name, isIS) \
	registerDefaultValue(getDefaultValueDefinitions()->nameVar);
	#include "DefaultMapValueSet.h"
#undef SETUP_DEFAULT_MAP
}

bool MapStyleInfo::resolveValueDefinition( const std::string& name, std::shared_ptr<const MapStyleValue>& outDefinition ) const
{
    auto itValueDefinition = _valueDefinitions.find(name);
    if(itValueDefinition != _valueDefinitions.end())
    {
		outDefinition = itValueDefinition->second;
        return true;
    }

    return false;
}

bool MapStyleInfo::resolveAttribute( const std::string& name, std::shared_ptr<const MapStyleRule>& outDefinition ) const
{
	auto& attributeVal = _attributes.find(name);
	if (attributeVal != _attributes.end())
	{
		outDefinition = attributeVal->second;
		return true;
	}
	return false;
}

bool MapStyleInfo::lookupStringId( const  std::string& value, uint32_t& id ) const
{
    auto itId = _stringsRevLUT.find(value);
    if(itId != _stringsRevLUT.cend())
    {
		id = itId->second;
        return true;
    }

    return false;
}

uint32_t MapStyleInfo::lookupStringId( const std::string& value )
{
    uint32_t id;
    if(lookupStringId(value, id))
        return id;

    return registerString(value);
}

const std::string& MapStyleInfo::lookupStringValue( uint32_t id ) const
{
    return _stringsLUT[id - _stringsIdBase];
}

uint32_t MapStyleInfo::registerString( const std::string& value )
{
    const auto id = _stringsIdBase + _stringsLUT.size();

    _stringsRevLUT[value] =  id;
    _stringsLUT.push_back(value);

    return id;
}

bool MapStyleInfo::registerRule(rulesetType rules, const std::shared_ptr<MapStyleRule>& rule )
{
	switch(rules)
	{
	case order:
		registerRule(_orderRules, rule);
		break;
		case text:
		registerRule(_textRules, rule);
		break;
		case line:
		registerRule(_lineRules, rule);
		break;
		case polygon:
		registerRule(_polygonRules, rule);
		break;
		case point:
		registerRule(_pointRules, rule);
		break;
	default:
	case none:
		return false;
	}

	return true;
}

bool MapStyleInfo::registerRule(std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset, const std::shared_ptr<MapStyleRule>& rule )
{
    MapStyleData tagData;
    if(!rule->getAttribute(defaultMapStyles->INPUT_TAG, tagData))
    {
        //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
		assert(false);
        return false;
    }

    MapStyleData valueData;
    if(!rule->getAttribute(defaultMapStyles->INPUT_VALUE, valueData))
    {
        //OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Attribute tag should be specified for root filter");
		assert(false);
        return false;
    }

	uint64_t id = encodeRuleId(tagData.simpleData.asUInt, valueData.simpleData.asUInt);

    auto insertedRule = rule;
    
    auto itPrevious = ruleset.find(id);
    if(itPrevious != ruleset.cend())
    {
        // all root rules should have at least tag/value
        insertedRule = createTagValueRootWrapperRule(id, itPrevious->second);
        insertedRule->_ifElseChildren.push_back(rule);
    }

    ruleset[id] = std::move(insertedRule);

    return true;
}

std::shared_ptr<MapStyleRule> MapStyleInfo::createTagValueRootWrapperRule( uint64_t id, const std::shared_ptr<MapStyleRule>& rule )
{
    if(rule->_values.size() <= 2)
        return rule;

    std::unordered_map< std::string, std::string > attributes;
    attributes.insert(std::make_pair(std::string("tag"), getTagString(id)));
    attributes.insert(std::make_pair(std::string("value"), getValueString(id)));
    std::shared_ptr<MapStyleRule> newRule(new MapStyleRule(this, attributes));
    newRule->_ifElseChildren.push_back(rule);
    return newRule;
}

const std::string MapStyleInfo::getTagString( uint64_t ruleId ) const
{
	return lookupStringValue(ruleId >> RuleIdTagShift);
}

const std::string MapStyleInfo::getValueString( uint64_t ruleId ) const
{
    return lookupStringValue(ruleId & ((1ull << RuleIdTagShift) - 1));
}

bool MapStyleInfo::getRule(uint64_t ruleId, rulesetType rulestype, std::shared_ptr<MapStyleRule>& ruleData)
{
	std::map< uint64_t, std::shared_ptr<MapStyleRule> >* currentSet;
	switch(rulestype)
	{
	case order:
		currentSet = &_orderRules;
		break;
		case text:
		currentSet = &_textRules;
		break;
		case line:
		currentSet = &_lineRules;
		break;
		case polygon:
		currentSet = &_polygonRules;
		break;
		case point:
		currentSet = &_pointRules;
		break;
	default:
	case none:
		return false;
	}

	auto setRuleData = currentSet->find(ruleId);
	if (setRuleData != currentSet->end())
	{
		ruleData = setRuleData->second;
		return true;
	}

	return false;
}

void MapStyleInfo::dump( rulesetType type, const std::string& prefix /*= QString()*/ ) const
{
	std::map< uint64_t, std::shared_ptr<MapStyleRule> > currentSet;
	switch(type)
	{
	case order:
		currentSet = _orderRules;
		break;
		case text:
		currentSet = _textRules;
		break;
		case line:
		currentSet = _lineRules;
		break;
		case polygon:
		currentSet = _polygonRules;
		break;
		case point:
		currentSet = _pointRules;
		break;
	default:
	case none:
		return ;
	}
    const auto& rules = currentSet;
    for(auto RuleEntry : rules)
    {
        auto tag = getTagString(RuleEntry.first);
		auto value = getValueString(RuleEntry.first);
		auto rule = RuleEntry.second;

		auto strLog = boost::format("%1%Rule [%2% (%3%):%4% (%5%)]\n") % prefix % tag % getTagStringId(RuleEntry.first) % value % getValueStringId(RuleEntry.first);
		stdext::cvt::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		OutputDebugString(converter.from_bytes(strLog.str()).c_str());
        rule->dump(prefix);
    }
}