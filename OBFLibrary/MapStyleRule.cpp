#include "stdafx.h"
#include "MapStyleData.h"
#include "MapStyleValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "Tools.h"

MapStyleRule::MapStyleRule(MapStyleInfo* _owner, std::unordered_map<std::string, std::string>& _attributes)
{
	owner = _owner;
	acquireAttributes(_attributes);
}


MapStyleRule::~MapStyleRule(void)
{
}

void MapStyleRule::acquireAttributes(std::unordered_map<std::string, std::string>& _attributes)
{
	_values.reserve(_attributes.size());
	_resolvedValueDefinitions.reserve(_attributes.size());
	for(auto itAttribute = _attributes.cbegin(); itAttribute != _attributes.cend(); ++itAttribute)
    {
		const auto& key = itAttribute->first;
        const auto& value = itAttribute->second;

        std::shared_ptr<const MapStyleValue> valueDef;
        bool ok = owner->resolveValueDefinition(key, valueDef);
        assert(ok);

        // Store resolved value definition
        _resolvedValueDefinitions[key] = valueDef;

        MapStyleData parsedValue;
		switch (valueDef->typeData)
        {
        case ValType::Booltype:
			parsedValue.simpleData.asInt = (value == std::string("true")) ? 1 : 0;
            break;
        case ValType::Inttype:
            {
                if(valueDef->isComplex)
                {
					parsedValue.isSpecial = true;
					if(value.find(':') == std::string::npos)
                    {
						parsedValue.specialData.asInt.dip = Tools::parseArbitraryInt(value, -1);
                        parsedValue.specialData.asInt.px = 0.0;
                    }
                    else
                    {
                        // 'dip:px' format
						boost::char_separator<char> sep(":", nullptr, boost::keep_empty_tokens);
						boost::tokenizer< boost::char_separator<char> > tokens(value, sep);
						std::vector<std::string> specialValues;
						for (auto tokenVal : tokens)
						{
							specialValues.push_back(tokenVal);
						}
                        //const auto& complexValue = value.split(':', QString::KeepEmptyParts);

						parsedValue.specialData.asInt.dip = Tools::parseArbitraryInt(specialValues[0], 0);
                        parsedValue.specialData.asInt.px = Tools::parseArbitraryInt(specialValues[1], 0);
                    }
                }
                else
                {
                    //assert(!value.contains(':'));
                    parsedValue.simpleData.asInt = Tools::parseArbitraryInt(value, -1);
                }
            }
            break;
        case ValType::Floattype:
            {
                if(valueDef->isComplex)
                {
					parsedValue.isSpecial = true;
                    if(value.find(':') == std::string::npos)
                    {
                        parsedValue.specialData.asFloat.dip = Tools::parseArbitraryFloat(value, -1.0f);
                        parsedValue.specialData.asFloat.px = 0.0f;
                    }
                    else
                    {
                        // 'dip:px' format
						boost::char_separator<char> sep(":", nullptr, boost::keep_empty_tokens);
						boost::tokenizer< boost::char_separator<char> > tokens(value, sep);
						std::vector<std::string> specialValues;
						for (auto tokenVal : tokens)
						{
							specialValues.push_back(tokenVal);
						}
                        //const auto& complexValue = value.split(':', QString::KeepEmptyParts);

                        parsedValue.specialData.asFloat.dip = Tools::parseArbitraryFloat(specialValues[0], 0);
                        parsedValue.specialData.asFloat.px = Tools::parseArbitraryFloat(specialValues[1], 0);
                    }
                }
                else
                {
                    //assert(!value.contains(':'));
                    parsedValue.simpleData.asFloat = Tools::parseArbitraryFloat(value, -1.0f);
                }
            }
            break;
        case ValType::Stringtype:
            parsedValue.simpleData.asUInt = owner->lookupStringId(value);
            break;
        case ValType::Colortype:
            {
                //assert(value[0] == '#');
                parsedValue.simpleData.asUInt = boost::lexical_cast<Tools::uint32_from_hex>(value.substr(1));
                if(value.size() <= 7)
                    parsedValue.simpleData.asUInt |= 0xFF000000;
            }
            break;
        }
        
        _values[valueDef] = parsedValue;
    }
}

bool MapStyleRule::getAttribute(const std::shared_ptr<const MapStyleValue>& key, MapStyleData& value) const
{
    auto itValue = _values.find(key);
    if(itValue == _values.cend())
        return false;

	value = itValue->second;
    return true;
}


void MapStyleRule::uniteAttributes(std::unordered_map<std::string, std::string>& _attributes)
{
	if (_values.size() == 0)
	{
		acquireAttributes(_attributes);
	}
	else
	{
	}
}