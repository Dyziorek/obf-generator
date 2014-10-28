#include "stdafx.h"
#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
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

MapStyleResult::MapStyleResult(void)
{
}


MapStyleResult::~MapStyleResult(void)
{
}

bool MapStyleResult::getIntVal(uint32_t id, int32_t& value)
{
	bool bOK = false;

	VARIANT vtData = _values[id];

	VARIANT vtNewData;

	HRESULT hr = VariantChangeType(&vtNewData, &vtData, 0, VT_I4);
	if (SUCCEEDED(hr))
	{
		value = vtNewData.intVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getIntVal(uint32_t id, uint32_t& value)
{
	bool bOK = false;

	VARIANT vtData = _values[id];

	VARIANT vtNewData;

	HRESULT hr = VariantChangeType(&vtNewData, &vtData, 0, VT_UI4);
	if (SUCCEEDED(hr))
	{
		value = vtNewData.ulVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getBoolVal(uint32_t id, int32_t& value)
{
	bool bOK = false;

	VARIANT vtData = _values[id];

	VARIANT vtNewData;

	HRESULT hr = VariantChangeType(&vtNewData, &vtData, 0, VT_BOOL);
	if (SUCCEEDED(hr))
	{
		value = vtNewData.boolVal;
		bOK = true;
	}
	return bOK;
}
bool MapStyleResult::getStringVal(uint32_t id, std::string& value)
{
	bool bOK = false;

	_variant_t vtData = _values[id];
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_BSTR, &vtData);

	HRESULT hr = S_OK;// VariantChangeType(&vtNewData, &vtData, 0, VT_BSTR);
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

bool MapStyleResult::getStringVal(uint32_t id, std::wstring& value)
{
	bool bOK = false;

	_variant_t vtData = _values[id];
	_variant_t vtNewData;
	vtNewData.ChangeType(VT_BSTR, &vtData);

	HRESULT hr = S_OK;// VariantChangeType(&vtNewData, &vtData, 0, VT_BSTR);
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
bool MapStyleResult::getFloatVal(uint32_t id, float& value)
{
	bool bOK = false;

	VARIANT vtData = _values[id];

	VARIANT vtNewData;

	HRESULT hr = VariantChangeType(&vtNewData, &vtData, 0, VT_R4);
	if (SUCCEEDED(hr))
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
		std::string textInfo = _Dvalues[keyVal].first;
#endif
		strmText << L"Values: key:" << keyVal << L", TextValue:" << dumpInfo << std::endl;
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

			switch(valueDef->typeData)
            {
			case ValType::Booltype:
				assert(!ruleValue.isSpecial);
				outInfo->_values[valueDef->id] = (ruleValue.simpleData.asUInt == 1);
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), outInfo->_values[valueDef->id]);
				#endif
                break;
            case ValType::Inttype:
                outInfo->_values[valueDef->id] =
                    ruleValue.isSpecial
					? ruleValue.specialData.asInt.calculate(_factor)
                    : ruleValue.simpleData.asInt;
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), outInfo->_values[valueDef->id]);
				#endif
                break;
            case ValType::Floattype:
                outInfo->_values[valueDef->id] =
                    ruleValue.isSpecial
                    ? ruleValue.specialData.asFloat.calculate(_factor)
                    : ruleValue.simpleData.asFloat;
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), outInfo->_values[valueDef->id]);
				#endif
                break;
            case ValType::Stringtype:
                // Save value of a string instead of it's id
                outInfo->_values[valueDef->id] =
                    owner->lookupStringValue(ruleValue.simpleData.asUInt).c_str();
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), outInfo->_values[valueDef->id]);
				#endif
                break;
            case ValType::Colortype:
                assert(!ruleValue.isSpecial);
                outInfo->_values[valueDef->id] = ruleValue.simpleData.asUInt;
				#ifdef _DEBUG
				outInfo->_Dvalues[valueDef->id] = std::make_pair(owner->lookupStringValue(valueDef->id), outInfo->_values[valueDef->id]);
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

	bool bOK = owner->getRule(MapStyleInfo::encodeRuleId(tag, value), ruleType, ruleHandle);
	if (!bOK)
	{
		bOK = owner->getRule(MapStyleInfo::encodeRuleId(tag, 0), ruleType, ruleHandle);
	}
	if (!bOK)
	{
		bOK = owner->getRule(MapStyleInfo::encodeRuleId(0, 0), ruleType, ruleHandle);
	}
	
	if (!bOK)
	{
		return false;
	}

	return evaluate(mapObject, ruleHandle, outInfo);
}
