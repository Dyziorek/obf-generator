#include "stdafx.h"
#include "MapStyleData.h"
#include "MapStyleValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "Tools.h"

#include <cvt/wstring>
#include <codecvt>
#include <boost\format.hpp>


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


void MapStyleRule::dump( const std::string& prefix /*= QString()*/ ) const
{
    auto newPrefix = prefix + std::string("\t");
    
	//OutputDebugString

    for(auto itValueEntry : _values)
    {
		const auto& valueDef = itValueEntry.first;
		const auto& value = itValueEntry.second;

        std::stringstream strValue;
		
		switch (valueDef->typeData)
        {
        case ValType::Booltype:
            strValue << (value.simpleData.asInt == 1) ? std::string("true") : std::string("false");
            break;
        case ValType::Inttype:
            if(value.isSpecial)
                strValue << boost::format("%1%:%2%") % value.specialData.asInt.dip % value.specialData.asInt.px;
            else
                strValue << value.simpleData.asInt;
            break;
		case ValType::Floattype:
            if(value.isSpecial)
                strValue << boost::format("%1%:%2%") % value.specialData.asFloat.dip % value.specialData.asFloat.px;
            else
                strValue << value.simpleData.asFloat;
            break;
        case ValType::Stringtype:
            strValue << owner->lookupStringValue(value.simpleData.asUInt);
            break;
        case ValType::Colortype:
            {
                auto color = value.simpleData.asUInt;
                if((color & 0xFF000000) == 0xFF000000)
					strValue << boost::format("%#6x") % color;
                else
                    strValue << boost::format("%#8x") % color;
            }
            break;
        }

		std::stringstream logOut;
		logOut << boost::format("%1%%2%%3% = %4%\n") % newPrefix % ((valueDef->typeAcces == AccessType::In) ? ">" : "<") % valueDef->name % strValue.str();
		stdext::cvt::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		OutputDebugString(converter.from_bytes(logOut.str()).c_str());
    }

    if(!_ifChildren.empty())
    {
        std::stringstream logOut;
		logOut << newPrefix << "If("  << std::endl;
		stdext::cvt::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		OutputDebugString(converter.from_bytes(logOut.str()).c_str());
            
        for(auto itChild : _ifChildren)
        {
            auto child = *itChild;

			OutputDebugString(converter.from_bytes((boost::format( "%1%AND\n") %  newPrefix).str()).c_str());
			child.dump(newPrefix);
        }
		logOut.clear();
		logOut << newPrefix << ")" << std::endl;
		OutputDebugString(converter.from_bytes(logOut.str()).c_str());
    }

    if(!_ifElseChildren.empty())
    {
		std::stringstream logOut;
		logOut << newPrefix  << "Selector: [" << std::endl;
		stdext::cvt::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		OutputDebugString(converter.from_bytes(logOut.str()).c_str());

        for(const auto child : _ifElseChildren)
        {
            
			std::stringstream logOut;
			logOut << newPrefix  << "OR" << std::endl;
			OutputDebugString(converter.from_bytes(logOut.str()).c_str());
            child->dump(newPrefix);
        }
        logOut.clear();
		logOut << newPrefix  << "]" << std::endl;
		OutputDebugString(converter.from_bytes(logOut.str()).c_str());
    }
}
