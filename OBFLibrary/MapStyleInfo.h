#pragma once



class MapStyleInfo
{
private:

	enum {
            RuleIdTagShift = 32,
        };

	std::string read(const char* value)
	{
		if (value == NULL)
		{
			return std::string("");
		}
		else {
			std::string checkVal(value);
			if (!checkVal.empty() && checkVal[0] == '$')
			{
				if (_renderConstants.find(checkVal.substr(1)) != _renderConstants.end())
				{
					return _renderConstants[checkVal.substr(1)];
				}
			}
		}
		return std::string(value);
	}

	uint64_t encodeRuleId( uint32_t tag, uint32_t value )
	{
		return (static_cast<uint64_t>(tag) << RuleIdTagShift) | value;
	}

	const std::string getTagString( uint64_t ruleId ) const;
	const std::string getValueString( uint64_t ruleId ) const;
	void registerValue(MapStyleValue* value);
	void registerDefaultValue(const std::shared_ptr<MapStyleValue>& value);
	void registerDefaultValues();
	std::shared_ptr<DefaultMapStyleValue> getDefaultValueDefinitions();

	int _firstLoadedValue;
	std::string _parentName;
	std::unordered_map<std::string, std::string> _renderConstants;
	std::unordered_map<std::string, std::shared_ptr<MapStyleValue>> _valueDefinitions;
	std::unordered_map<std::string, std::shared_ptr<MapStyleRule>> _attributes;

	 uint32_t _stringsIdBase;
     std::vector< std::string > _stringsLUT;
     std::unordered_map< std::string, uint32_t > _stringsRevLUT;


	std::map< uint64_t, std::shared_ptr<MapStyleRule> > _pointRules;
    std::map< uint64_t, std::shared_ptr<MapStyleRule> > _lineRules;
    std::map< uint64_t, std::shared_ptr<MapStyleRule> > _polygonRules;
    std::map< uint64_t, std::shared_ptr<MapStyleRule> > _textRules;
    std::map< uint64_t, std::shared_ptr<MapStyleRule> > _orderRules;
public:
	MapStyleInfo(void);
	~MapStyleInfo(void);
	void loadRenderStyles(const char *path);
	void parseFilter(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule);
	void parseFilter(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule, std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset);
	void parseGroup(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule, std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset);
	void parseGroupFilter(tinyxml2::XMLElement* workElem, std::shared_ptr<MapStyleRule>& mapRule);
	void parseProperty(tinyxml2::XMLElement* workElem);
	void parseAttribute(tinyxml2::XMLElement* workElem);
	void parseConstant(tinyxml2::XMLElement* workElem);
	void parseOrder(tinyxml2::XMLElement* workElem);
	void parseText(tinyxml2::XMLElement* workElem);
	void parsePoint(tinyxml2::XMLElement* workElem);
	void parsePolygon(tinyxml2::XMLElement* workElem);
	void parseLine(tinyxml2::XMLElement* workElem);

	bool resolveValueDefinition( const std::string& name, std::shared_ptr<const MapStyleValue>& outDefinition ) const;
	bool lookupStringId( const  std::string& value, uint32_t& id ) const;
	const std::string& lookupStringValue( uint32_t id ) const;
	uint32_t lookupStringId( const std::string& value );
	uint32_t registerString( const std::string& value );
	bool registerRule(std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset, const std::shared_ptr<MapStyleRule>& rule );
	std::shared_ptr<MapStyleRule> createTagValueRootWrapperRule( uint64_t id, const std::shared_ptr<MapStyleRule>& rule );
};

