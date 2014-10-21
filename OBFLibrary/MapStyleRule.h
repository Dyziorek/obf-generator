#pragma once

class MapStyleInfo;

class MapStyleRule
{
private:
        std::unordered_map< std::string, std::shared_ptr<const MapStyleValue> > _resolvedValueDefinitions;
        std::unordered_map< std::shared_ptr<const MapStyleValue>, MapStyleData > _values;
		void acquireAttributes(std::unordered_map<std::string, std::string>& _attributes);

		MapStyleInfo* owner;
public:
        std::vector< std::shared_ptr<MapStyleRule> > _ifElseChildren;
        std::vector< std::shared_ptr<MapStyleRule> > _ifChildren;
public:
	MapStyleRule(MapStyleInfo* owner, std::unordered_map<std::string, std::string>& _attributes);
	void uniteAttributes(std::unordered_map<std::string, std::string>& _attributes);
	bool getAttribute(const std::shared_ptr<const MapStyleValue>& key, MapStyleData& value) const;
	~MapStyleRule(void);


	friend class MapStyleInfo;
};

