#pragma once

enum rulesetType  :  int
{
	none = 0, 
	point = 1, 
	line = 2, 
	polygon = 3,
	text = 4, 
	order = 5
};

class MapStyleInfo
{
private:
	rulesetType currentRule;
	struct Lexeme
	{
		enum rtype{Rule, Group};
		Lexeme(MapStyleInfo* parent, rtype id) : _parent(parent), _id(id){};
		rtype _id;
		MapStyleInfo* _parent;
	};

	struct Group : public Lexeme
	{
		Group(MapStyleInfo* parent): Lexeme(parent, rtype::Group){}
		void addGroupFilter(const std::shared_ptr<MapStyleRule>& rule)
        {
            for(auto itChild = children.cbegin(); itChild != children.cend(); ++itChild)
            {
                auto child = *itChild;

                child->_ifChildren.push_back(rule);
            }

            for(auto itSubgroup = subgroups.cbegin(); itSubgroup != subgroups.cend(); ++itSubgroup)
            {
                auto subgroup = *itSubgroup;

                //assert(subgroup->type == Lexeme::Group);
                std::static_pointer_cast<Group>(subgroup)->addGroupFilter(rule);
            }
        }

		bool registerGlobalRules(rulesetType type)
        {
            for(auto itChild = children.cbegin(); itChild != children.cend(); ++itChild)
            {
                auto child = *itChild;

                if(!_parent->registerRule(type, child))
                    return false;
            }

            for(auto itSubgroup = subgroups.cbegin(); itSubgroup != subgroups.cend(); ++itSubgroup)
            {
                auto subgroup = *itSubgroup;

                //assert(subgroup->type == Lexeme::Group);
                if(!std::static_pointer_cast<Group>(subgroup)->registerGlobalRules(type))
                    return false;
            }

            return true;
        }
		
		std::unordered_map< std::string, std::string > attributes;
        std::list< std::shared_ptr<MapStyleRule> > children;
        std::list< std::shared_ptr<Lexeme> > subgroups;

	};

	struct Rule : public Lexeme
	{
		Rule(MapStyleInfo* parent, std::shared_ptr<MapStyleRule> rule) : Lexeme(parent, rtype::Rule), _rule(rule){}
		std::shared_ptr<MapStyleRule> _rule;
	};


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

	const std::string getTagString( uint64_t ruleId ) const;
	const std::string getValueString( uint64_t ruleId ) const;
	void registerValue(MapStyleValue* value);
	void registerDefaultValue(const std::shared_ptr<MapStyleValue>& value);
	void registerDefaultValues();
	

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
	void parseFilter(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRule);
	void parseGroup(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRule);
	void parseGroupFilter(tinyxml2::XMLElement* workElem, std::vector<std::shared_ptr<Lexeme>>& mapRule);
	void parseProperty(tinyxml2::XMLElement* workElem);
	void parseAttribute(tinyxml2::XMLElement* workElem);
	void parseConstant(tinyxml2::XMLElement* workElem);
	void parseItemClass(tinyxml2::XMLElement* workElem, std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset);

	bool resolveValueDefinition( const std::string& name, std::shared_ptr<const MapStyleValue>& outDefinition ) const;
	bool resolveAttribute( const std::string& name, std::shared_ptr<const MapStyleRule>& outDefinition ) const;
	bool lookupStringId( const  std::string& value, uint32_t& id ) const;
	const std::string& lookupStringValue( uint32_t id ) const;
	uint32_t lookupStringId( const std::string& value );
	uint32_t registerString( const std::string& value );
	bool registerRule(rulesetType rules, const std::shared_ptr<MapStyleRule>& rule );
	bool registerRule(std::map< uint64_t, std::shared_ptr<MapStyleRule> >& ruleset, const std::shared_ptr<MapStyleRule>& rule );
	std::shared_ptr<MapStyleRule> createTagValueRootWrapperRule( uint64_t id, const std::shared_ptr<MapStyleRule>& rule );
	bool getRule(uint64_t ruleId, rulesetType rulestype, std::shared_ptr<MapStyleRule>& ruleData);
	static std::shared_ptr<DefaultMapStyleValue> getDefaultValueDefinitions();
	
	static uint64_t encodeRuleId( uint32_t tag, uint32_t value )
	{
		return (static_cast<uint64_t>(tag) << RuleIdTagShift) | value;
	}
	uint32_t getTagStringId( uint64_t ruleId ) const
	{
		return ruleId >> RuleIdTagShift;
	}
	uint32_t getValueStringId( uint64_t ruleId ) const
	{
		return ruleId & ((1ull << RuleIdTagShift) - 1);
	}
	void MapStyleInfo::dump( rulesetType type, const std::string& prefix = std::string() ) const;
};

