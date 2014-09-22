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



MapStyleInfo::MapStyleInfo(void) : _stringsIdBase(0)
{
	registerDefaultValues();
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
	//struct XMLPart
	//{
	//	enum Type
	//	{
	//		Rule,
	//		Group
	//	} XMLType;
	//	MapStyleInfo* owner;
	//	XMLPart(MapStyleInfo* parent, XMLPart::Type set) : owner(parent), XMLType(set)
	//	{}
	//};

	//struct Rule : XMLPart
	//{
	//	const std::shared_ptr<MapStyleRule> rule;
	//	Rule(MapStyleInfo* parent, std::shared_ptr<MapStyleRule> ruleIn) : XMLPart(parent, XMLPart::Type::Rule), rule(ruleIn)
	//	{}
	//};

	//struct Group : XMLPart
	//{
	//	const std::shared_ptr<MapStyleRule> rule;
	//	Group(MapStyleInfo* parent, std::shared_ptr<MapStyleRule> ruleIn) : XMLPart(parent, XMLPart::Type::Rule), rule(ruleIn)
	//	{}
	//};
	
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
		for ( const tinyxml2::XMLNode* inode=xDoc.FirstChildElement(); inode; inode=inode->NextSibling() )
		{
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
				parseOrder(iElem);
			}
			if (std::string("text") == iElem->Name())
			{
				parseText(iElem);
			}
			if (std::string("point") == iElem->Name())
			{
				parsePoint(iElem);
			}
			if (std::string("polygon") == iElem->Name())
			{
				parsePolygon(iElem);
			}
			if (std::string("line") == iElem->Name())
			{
				parseLine(iElem);
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
	 for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, _attributes[attrName]);
			}

			
	 }

}
void MapStyleInfo::parseConstant(tinyxml2::XMLElement* workElem)
{
	std::string name = read(workElem->Attribute("name"));
	std::string value = read(workElem->Attribute("value"));
	_renderConstants[name] = value;
}

void MapStyleInfo::parseFilter(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule)
{
	std::unordered_map<std::string, std::string> attrVals;
	for ( const tinyxml2::XMLAttribute* xAttr = workElem->FirstAttribute(); xAttr; xAttr=xAttr->Next())
	{
		std::string attrName = xAttr->Name();
		std::string attrVal = xAttr->Value();
		attrVals[attrName] = attrVal;
	}
	std::shared_ptr<MapStyleRule> filterRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, attrVals));
	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	{
		tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
		if (std::string("filter") == iElem->Name())
		{
			parseFilter(iElem, filterRule);
		}
	 }
	if (mapRule->_ifChildren.empty())
	{
		mapRule->_ifChildren.push_back(filterRule);
	}
	else
	{
		mapRule->_ifElseChildren.push_back(filterRule);
	}
}

void MapStyleInfo::parseGroupFilter(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule)
{
	std::shared_ptr<MapStyleRule> groupRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));
	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, groupRule);
			}
			if (std::string("groupFilter") == iElem->Name())
			{
				parseGroupFilter(iElem, groupRule);
			}
			
	 }

}
void MapStyleInfo::parseGroup(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule)
{
	std::shared_ptr<MapStyleRule> groupRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));
	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, groupRule);
			}
			if (std::string("groupFilter") == iElem->Name())
			{
				parseGroupFilter(iElem, groupRule);
			}
			
	 }
}

void MapStyleInfo::parseOrder(tinyxml2::XMLElement* workElem)
{
	std::shared_ptr<MapStyleRule> orderRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, orderRule);
			}
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, orderRule);
			}
			
	 }
}
void MapStyleInfo::parseText(tinyxml2::XMLElement* workElem)
{
	std::shared_ptr<MapStyleRule> textRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, textRule);
			}
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, textRule);
			}
	 }
}
void MapStyleInfo::parsePoint(tinyxml2::XMLElement* workElem)
{
	std::shared_ptr<MapStyleRule> textRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, textRule);
			}
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, textRule);
			}
	 }
}
void MapStyleInfo::parsePolygon(tinyxml2::XMLElement* workElem)
{
	std::shared_ptr<MapStyleRule> textRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, textRule);
			}
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, textRule);
			}
	 }
}
void MapStyleInfo::parseLine(tinyxml2::XMLElement* workElem)
{
	std::shared_ptr<MapStyleRule> textRule = std::shared_ptr<MapStyleRule>(new MapStyleRule(this, std::unordered_map<std::string, std::string>()));

	for ( const tinyxml2::XMLNode* node=workElem->FirstChildElement(); node; node=node->NextSibling() )
	 {
			tinyxml2::XMLElement* iElem = (tinyxml2::XMLElement*)node;
			if (std::string("filter") == iElem->Name())
			{
				parseFilter(iElem, textRule);
			}
			if (std::string("group") == iElem->Name())
			{
				parseGroup(iElem, textRule);
			}
	 }
}

void MapStyleInfo::registerValue(MapStyleValue* value)
{
	_valueDefinitions[value->name] = std::shared_ptr<MapStyleValue>(value);
	
}

void MapStyleInfo::registerDefaultValue(MapStyleValue* value)
{
	_valueDefinitions[value->name] = std::shared_ptr<MapStyleValue>(value);
	_firstLoadedValue = _valueDefinitions.size();
}

void MapStyleInfo::registerDefaultValues()
{
#define SETUP_DEFAULT_MAP(nameVar, typeData, accType, name, isIS) \
	registerDefaultValue(getDefaultValueDefinitions()->nameVar.get());
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