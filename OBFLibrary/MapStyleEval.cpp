#include "stdafx.h"
#include <Ole2.h>
#include <comdef.h>
#include <comutil.h>

#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapObjectData.h"
#include "MapStyleEval.h"

#include <sstream>
#include <cvt/wstring>
#include <codecvt>

MapStyleResult::MapStyleResult(void)
{
}


MapStyleResult::~MapStyleResult(void)
{
}

bool MapStyleResult::getIntVal(uint32_t id, int32_t& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_I4, &vtData->second);

	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		value = vtNewData.intVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getIntVal(uint32_t id, uint32_t& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_UI4, &vtData->second);

	HRESULT hr = S_OK;
	if (SUCCEEDED(hr))
	{
		value = vtNewData.ulVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getBoolVal(uint32_t id, int32_t& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_BOOL, &vtData->second);

	HRESULT hr = S_OK;if (SUCCEEDED(hr))
	{
		value = vtNewData.boolVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getStringVal(uint32_t id, std::string& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_BSTR, &vtData->second);

	HRESULT hr = S_OK;// VariantChangeType(&vtNewData, &vtData->second, 0, VT_BSTR);
	if (SUCCEEDED(hr))
	{
		_bstr_t bstrData(vtNewData.bstrVal);
		const char* result = bstrData;
		value.clear();
		value.assign(result);
		bOK = true;
	}
	return bOK;
}

bool MapStyleResult::getStringVal(uint32_t id, std::wstring& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_BSTR, &vtData->second);

	HRESULT hr = S_OK;// VariantChangeType(&vtNewData, &vtData->second, 0, VT_BSTR);
	if (SUCCEEDED(hr))
	{
		_bstr_t bstrData(vtNewData.bstrVal);
		const wchar_t* result = bstrData;
		value.clear();
		value.assign(result);
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getFloatVal(uint32_t id, float& value) const
{
	bool bOK = false;

	auto& vtData = _values.find(id);
	if (vtData == _values.end())
		return false;

	_variant_t vtNewData;
	vtNewData.ChangeType(VT_R4, &vtData->second);

	HRESULT hr = S_OK;if (SUCCEEDED(hr))
	{
		value = vtNewData.fltVal;
		bOK = true;
	}
	return bOK;
}
		

std::wstring MapStyleResult::dump()
{
	std::wstring dumpInfo;
	std::wstringstream strmText; 
	for (auto& valData : _values)
	{
		auto& keyVal = valData.first;
		auto& value = valData.second;
		getStringVal(keyVal, dumpInfo);
#ifdef _DEBUG
		stdext::cvt::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring keyText = converter.from_bytes(_Dvalues[keyVal].first).c_str();
		strmText << L"Values: keyID:" << keyVal << L", keyText:"<< keyText << L", TextValue:" << dumpInfo << std::endl;
#endif
#ifndef  _DEBUG
		strmText << L"Values: key:" << keyVal << L", TextValue:" << dumpInfo << std::endl;
#endif
	}

	dumpInfo = strmText.str();
	return dumpInfo;
}

MapStyleEval::MapStyleEval(const std::shared_ptr<MapStyleInfo>& _style, float _densityFactor) : _builtInDefValues(MapStyleInfo::getDefaultValueDefinitions()), owner(_style), _factor(_densityFactor)
{
}


MapStyleEval::~MapStyleEval(void)
{
}

void MapStyleEval::setBoolValue(const int valDefId, const bool defVal)
{
	auto& valCheck = _inputs[valDefId];
	valCheck.asInt = defVal;
}
void MapStyleEval::setIntValue(const int valDefId, const int defVal)
{
	auto& valCheck = _inputs[valDefId];
	valCheck.asInt = defVal;
}
void MapStyleEval::setIntValue(const int valDefId, const unsigned int defVal)
{
	auto& valCheck = _inputs[valDefId];
	valCheck.asUInt = defVal;
}
void MapStyleEval::setFloatValue(const int valDefId, const float defVal)
{
	auto& valCheck = _inputs[valDefId];
	valCheck.asFloat = defVal;
}
#pragma push_macro("max")
#undef max

void MapStyleEval::setStringValue(const int valDefId, const std::string& defVal)
{
	auto& valCheck = _inputs[valDefId];
	bool bOK = owner->lookupStringId(defVal, valCheck.asUInt);
	if (!bOK)
	{
		valCheck.asUInt = std::numeric_limits<uint32_t>::max();
	}
}

#pragma pop_macro("max")

bool MapStyleEval::evaluate(const std::shared_ptr<MapObjectData>& mapObject, std::shared_ptr<MapStyleRule> ruleHandle, MapStyleResult* const outInfo)
{
	for (auto ruleVal : ruleHandle->_values)
	{
		std::shared_ptr<const MapStyleValue> defStyleValue = ruleVal.first;
		MapStyleData dataValue = ruleVal.second;
		auto inputValue = _inputs[defStyleValue->id];
		if (defStyleValue->typeAcces != AccessType::In)
		{
			continue;
		}

		bool evalOK = false;

		if(defStyleValue->id == _builtInDefValues->id_INPUT_MINZOOM)
        {
            assert(!dataValue.isSpecial);
            evalOK = (dataValue.simpleData.asInt <= inputValue.asInt);
        }
        else if(defStyleValue->id == _builtInDefValues->id_INPUT_MAXZOOM)
        {
            assert(!dataValue.isSpecial);
            evalOK = (dataValue.simpleData.asInt >= inputValue.asInt);
        }
        else if(defStyleValue->id == _builtInDefValues->id_INPUT_ADDITIONAL)
        {
			if(!mapObject)
                evalOK = true;
            else
            {
                assert(!dataValue.isSpecial);
                const std::string& strValue = owner->lookupStringValue(dataValue.simpleData.asUInt);
                auto equalSignIdx = strValue.find('=');
                if(equalSignIdx != std::string::npos)
                {
                    const auto& tag = strValue.substr(0, equalSignIdx);
                    const auto& value = strValue.substr(equalSignIdx + 1);
                    evalOK = mapObject->containsTypeSlow(tag, value, true);
                }
                else
                    evalOK = false;
            }
		}
		else if(defStyleValue->typeData == ValType::Floattype)
        {
            const auto lvalue = dataValue.isSpecial ? dataValue.specialData.asFloat.calculate(_factor) : dataValue.simpleData.asFloat;

            evalOK = MapUtils::fuzzyCompare(lvalue, inputValue.asFloat);
        }
        else
        {
            const auto lvalue = dataValue.isSpecial ? dataValue.specialData.asInt.calculate(_factor) : dataValue.simpleData.asInt;

            evalOK = (lvalue == inputValue.asInt);
        }

		if (!evalOK)
			return false;
	}

	// Fill output values from rule to result storage, if requested
    if(outInfo)
    {
		for(const auto& ruleVal : ruleHandle->_values)
        {
			const auto& valueDef = ruleVal.first;
			if(valueDef->typeAcces != AccessType::Out)
                continue;

            const auto& ruleValue = ruleVal.second;
			int valueData;
			float valueReal;
			std::string valTxt;
			switch(valueDef->typeData)
            {
			case ValType::Booltype:
				assert(!ruleValue.isSpecial);
				outInfo->_values[valueDef->id] = (ruleValue.simpleData.asUInt == 1);
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), ruleValue.simpleData.asUInt == 1 ? "true" : "false");
				#endif
                break;
            case ValType::Inttype:
				valueData = ruleValue.isSpecial
					? ruleValue.specialData.asInt.calculate(_factor)
                    : ruleValue.simpleData.asInt;
                outInfo->_values[valueDef->id] = valueData;
                    
				#ifdef _DEBUG
				valTxt = boost::lexical_cast<std::string>(valueData);
				outInfo->_Dvalues[valueDef->id] = std::make_pair(valueDef->name, valTxt);
				#endif
                break;
            case ValType::Floattype:
				valueReal = ruleValue.isSpecial
                    ? ruleValue.specialData.asFloat.calculate(_factor)
                    : ruleValue.simpleData.asFloat;
                outInfo->_values[valueDef->id] = valueReal;
				#ifdef _DEBUG
				valTxt = boost::lexical_cast<std::string>(valueReal);
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), valTxt);
				#endif
                break;
            case ValType::Stringtype:
                // Save value of a string instead of it's id
                outInfo->_values[valueDef->id] =
                    owner->lookupStringValue(ruleValue.simpleData.asUInt).c_str();
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), owner->lookupStringValue(ruleValue.simpleData.asUInt));
				#endif
                break;
            case ValType::Colortype:
                assert(!ruleValue.isSpecial);
                outInfo->_values[valueDef->id] = ruleValue.simpleData.asUInt;
				#ifdef _DEBUG
				valTxt = boost::lexical_cast<std::string>(ruleValue.simpleData.asUInt);
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), boost::lexical_cast<std::string>(ruleValue.simpleData.asUInt));
				#endif
                break;
            }
        }
    }

    {
        for(auto itChild : ruleHandle->_ifElseChildren)
        {
            const auto evaluationResult = evaluate(mapObject, itChild, outInfo);
            if(evaluationResult)
                break;
        }

        for(auto itChild : ruleHandle->_ifChildren)
        {
            evaluate(mapObject, itChild, outInfo);
        }
    }

	return true;

}

bool MapStyleEval::evaluate(const std::shared_ptr<MapObjectData>& mapObject, rulesetType ruleType, MapStyleResult* const outInfo)
{
	std::shared_ptr<MapStyleRule> ruleHandle;
	const auto tag = _inputs[_builtInDefValues->id_INPUT_TAG].asUInt;
	const auto value = _inputs[_builtInDefValues->id_INPUT_VALUE].asUInt;
	bool ruleOK = false;
	bool evalOK = false;
	uint32_t emptyID;
	owner->lookupStringId("", emptyID);

	ruleOK = owner->getRule(MapStyleInfo::encodeRuleId(tag, value), ruleType, ruleHandle);
	if (ruleOK)
	{
		evalOK = evaluate(mapObject, ruleHandle, outInfo);
	}
	if (!evalOK)
	{
		ruleOK = owner->getRule(MapStyleInfo::encodeRuleId(tag, emptyID), ruleType, ruleHandle);
	}
	else
	{
		return true;
	}
	if (ruleOK)
	{
		_inputs[_builtInDefValues->id_INPUT_VALUE].asUInt = emptyID;
		evalOK = evaluate(mapObject, ruleHandle, outInfo);
	}
	if (!evalOK)
	{
		ruleOK = owner->getRule(MapStyleInfo::encodeRuleId(emptyID, emptyID), ruleType, ruleHandle);
	}
	else
	{
		return true;
	}
	if (ruleOK)
	{
		_inputs[_builtInDefValues->id_INPUT_TAG].asUInt = emptyID;
		_inputs[_builtInDefValues->id_INPUT_VALUE].asUInt = emptyID;
		evalOK = evaluate(mapObject, ruleHandle, outInfo);
	}
	
	return evalOK;

	//return evaluate(mapObject, ruleHandle, outInfo);
}
