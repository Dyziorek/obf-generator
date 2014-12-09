#pragma once
#include <comutil.h>

class MapStyleResult;

class MapStyleEval
{
private:

	union InputMatchValue
	{
		InputMatchValue()
		{
			asUInt = 0;
		}
		int asInt;
		unsigned int asUInt;
		float asFloat;
	};

	float _factor;
	const std::shared_ptr<MapStyleInfo> styleInfo;
public:
	MapStyleEval(const std::shared_ptr<MapStyleInfo>& _style, float _densityFactor);
	~MapStyleEval(void);

	const std::shared_ptr<MapStyleInfo> owner;
	std::unordered_map<uint32_t, InputMatchValue> _inputs;
	std::shared_ptr<DefaultMapStyleValue> _builtInDefValues;

	bool evaluate(const std::shared_ptr<MapObjectData>& mapObject, rulesetType ruleType, MapStyleResult* const outInfo);
	bool evaluate(const std::shared_ptr<MapObjectData>& mapObject, std::shared_ptr<MapStyleRule> ruleHandle, MapStyleResult* const outInfo);
	void setBoolValue(const int valDefId, const bool defVal);
	void setIntValue(const int valDefId, const int defVal);
	void setIntValue(const int valDefId, const unsigned int defVal);
	void setFloatValue(const int valDefId, const float defVal);
	void setStringValue(const int valDefId, const std::string& defVal);
};

class MapStyleResult
{
private:
	std::unordered_map<uint32_t, _variant_t> _values;
#ifdef _DEBUG
	std::unordered_map<uint32_t, std::pair<std::string , std::string>> _Dvalues;
#endif
public:
	bool getIntVal(uint32_t id, int32_t& value) const;
	bool getIntVal(uint32_t id, uint32_t& value) const;
	bool getBoolVal(uint32_t id, int32_t& value) const;
	bool getStringVal(uint32_t id, std::string& value) const;
	bool getStringVal(uint32_t id, std::wstring& value) const;
	bool getFloatVal(uint32_t id, float& value) const;
	MapStyleResult(void);
	~MapStyleResult(void);

	std::wstring dump();
	friend class MapStyleEval;
};


