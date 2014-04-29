#pragma once
#include "tinyxml2.h"
#include <boost\algorithm\string.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
// http://wiki.openstreetmap.org/wiki/Amenity
// POI tags : amenity, leisure, shop, sport, tourism, historic; accessories (internet-access), natural ?
class AmenityType {
	// Some of those types are subtypes of Amenity tag 
public:

private:
	static std::map<std::string, AmenityType> amenityTypes;
public:
	static AmenityType reg(std::string name, std::string defaultTag) {
		boost::algorithm::to_lower(name);
		if(amenityTypes.find(name) != amenityTypes.end()) {
			return amenityTypes.find(name)->second;
		}
		AmenityType t(name, defaultTag, amenityTypes.size());
		amenityTypes.insert(std::make_pair(t.name, t));
		return t;
	}
	
	
	
	 std::string defaultTag;
	 std::string name;
	 int ordinal;
	 AmenityType(void){ ordinal = -1;}

	 bool isEmpty()
	 {
		 return ordinal == -1;
	 }
	AmenityType(std::string name, std::string defaultTag, int ordinal) {
		this->name = name;
		this->defaultTag = defaultTag;
		this->ordinal = ordinal;	
	}
	AmenityType(AmenityType const& other)
	{
		this->name = other.name;
		this->defaultTag = other.defaultTag;
		this->ordinal = other.ordinal;
	}

	AmenityType& operator=(const AmenityType &other)
	{
		this->name = other.name;
		this->defaultTag = other.defaultTag;
		this->ordinal = other.ordinal;

		return *this;
	}
public:
	static AmenityType findOrCreateTypeNoReg(std::string s) {
		AmenityType type(s,s,-1);
		for (auto t : amenityTypes) {
			std::string lowName = t.second.name;
			boost::algorithm::to_lower(lowName);
			if (lowName == s) {
				return t.second;
			}
		}
		return type;
	}
	
	static bool isRegisteredType(AmenityType type) {
		//return amenityTypes.containsKey(type.name);
		return type.ordinal >= 0;
	}
	
	static AmenityType getAndRegisterType(std::string name) {
		return reg(name, name);
	}
	
	std::string getDefaultTag() {
		return defaultTag;
	}
	
	std::string getCategoryName() {
		return name;
	}
	
	
	static int getCategoriesSize() {
		return amenityTypes.size();
	}
	static std::vector<AmenityType> getCategories(){
		std::vector<AmenityType> categories(amenityTypes.size());
		std::transform(amenityTypes.begin(), amenityTypes.end(), categories.begin(), [](std::pair<std::string, AmenityType> pairElem)
		{
			return pairElem.second;
		});
		return categories;
	}


	
	
	
};

	extern AmenityType EMERGENCY;
	extern AmenityType HEALTHCARE;

	extern AmenityType TRANSPORTATION;
	extern AmenityType BARRIER;

	extern AmenityType TOURISM;
	extern AmenityType ENTERTAINMENT;
	extern AmenityType HISTORIC;

	extern AmenityType SPORT;
	extern AmenityType LEISURE;
	extern AmenityType GEOCACHE;

	extern AmenityType OTHER;
	extern AmenityType FINANCE;
	extern AmenityType OFFICE;
	extern AmenityType ADMINISTRATIVE;
	extern AmenityType EDUCATION;
	extern AmenityType MAN_MADE;
	extern AmenityType SEAMARK;
	extern AmenityType SUSTENANCE;

	extern AmenityType SHOP;

	extern AmenityType NATURAL;
	extern AmenityType LANDUSE;
	extern AmenityType MILITARY;

	extern AmenityType OSMWIKI;
	extern AmenityType USER_DEFINED;


class TagValuePattern {
public:
	TagValuePattern()
	{
	}
	std::string tag;
	std::string value;
		TagValuePattern(std::string t, std::string v) {
			this->tag = t;
			this->value = v;
		}
		
public:
	bool isApplicable(std::map<std::string, std::string> e ){
			if(value == "") {
				return e.find(tag) != e.end();
			}
			if (e.find(tag) != e.end())
			{
				return value  == e.find(tag)->second;
			}
			return false;
		}
		
		bool operator==(TagValuePattern obj) {
			if (this == &obj)
				return true;

			TagValuePattern other = (TagValuePattern) obj;
			if (tag == "") {
				if (other.tag != "")
					return false;
			} else if (!(tag == other.tag))
				return false;
			if (value == "") {
				if (other.value != "")
					return false;
			} else if (!(value == other.value))
				return false;
			return true;
		}

		bool operator<(const TagValuePattern& op2) const
		{
			return this->tag.compare(op2.tag) < 0;
		}
		
	};

class MapRulType {
public:
		std::vector<MapRulType> names;
		TagValuePattern tagValuePattern;
		bool additional;
		bool additionalText;
		std::set<TagValuePattern> applyToTagValue;
		
		std::string poiPrefix;
		AmenityType poiCategory;
		// poi_category was specially removed for one tag/value, to skip unnecessary objects
		bool poiSpecified;
		
		
		MapRulType* targetTagValue;
		
		bool relation;
		// creation of only section
		bool onlyMap;
		bool onlyPoi;
		
		// Needed only for map rules
		int minzoom;
		int maxzoom;
		bool onlyPoint;
		std::string namePrefix;
		
		
		// inner id
		int id;
		int freq;
		int targetId ;
		int targetPoiId ;
		
		MapRulType()
		{
			id = -1; 
			targetPoiId = -1;
			freq = 0;
			poiSpecified = false;
			onlyMap = false;
			onlyPoi = false;
			onlyPoint = false;
			relation = false;
			additional = false;
			additionalText = false;
			namePrefix = "";
			targetTagValue = nullptr;
		}
		~MapRulType()
		{
		}

		MapRulType(MapRulType const& other)
		{
			id = other.id;
			targetPoiId = other.targetPoiId;
			freq = other.freq;
			poiSpecified = other.poiSpecified;
			if (poiSpecified && other.poiCategory.ordinal != -1)
			{
				poiCategory = other.poiCategory;
			}
			onlyMap = other.onlyMap;
			onlyPoi = other.onlyPoi;
			onlyPoint = other.onlyPoint;
			relation = other.relation;
			additional = other.additional;
			additionalText = other.additionalText;
			namePrefix = other.namePrefix;
			minzoom = other.minzoom;
			maxzoom = other.maxzoom;
			targetId = other.targetId;
			if (other.targetTagValue != nullptr)
			{
				targetTagValue = other.targetTagValue;
			}
			else
			{
				targetTagValue = nullptr;
			}
			applyToTagValue = other.applyToTagValue;
			tagValuePattern = other.tagValuePattern;
		}

		bool operator<(const MapRulType& op2) const
		{
			return this->id < op2.id;
		}
		bool operator==(const MapRulType& op2) const
		{
			if (op2.isEmpty() && this->isEmpty())
			{
				return true;
			}
			return this->id == op2.id;
		}
public:
		bool isEmpty() const
		{
			return id == -1;
		}
		bool isPOI(){
			return !onlyMap;
		}
		
		bool isPOISpecified() {
			return isPOI() && poiSpecified;

		}
		
		bool isMap(){
			return !onlyPoi;
		}
		
		static MapRulType* createMainEntity(std::string tag, std::string value) {
			MapRulType* rt = new MapRulType();
			rt->tagValuePattern = TagValuePattern(tag, value);
			return rt;
		}
		
		static MapRulType* createText(std::string tag) {
			MapRulType* rt = new MapRulType();
			rt->additionalText = true;
			rt->minzoom = 5;
			rt->maxzoom = 31;
			rt->tagValuePattern = TagValuePattern(tag, ""); 
			return rt;
		}
		
		static MapRulType* createAdditional(std::string tag, std::string value) {
			MapRulType* rt = new MapRulType();
			rt->additional = true;
			rt->minzoom = 5;
			rt->maxzoom = 31;
			rt->tagValuePattern = TagValuePattern(tag, value);
			return rt;
		}


		std::string getTag() {
			return tagValuePattern.tag;
		}
		
		int getTargetId() {
			return targetId;
		}
		
		int getTargetPoiId() {
			return targetPoiId;
		}
		
		void setTargetPoiId(int catId, int valueId) {
			if(catId <= 31) {
				this->targetPoiId  = (valueId << 6) | (catId << 1) ; 
			} else {
				if(catId > (1 << 15)) {
					throw new std::exception("Refer source code");
				}
				this->targetPoiId  = (valueId << 16) | (catId << 1) | 1;
			}
		}
		
		int getInternalId() {
			return id;
		}
		
		void setTargetId(int targetId) {
			this->targetId = targetId;
		}
		
		MapRulType getTargetTagValue() {
			return *targetTagValue;
		}
		
		std::string getValue() {
			return tagValuePattern.value;
		}
		
		int getMinzoom() {
			return minzoom;
		}
		
		bool isAdditional() {
			return additional;
		}
		
		bool isAdditionalOrText() {
			return additional || additionalText;
		}
		
		bool isText() {
			return additionalText;
		}
		
		bool isOnlyPoint() {
			return onlyPoint;
		}
		
		bool isRelation() {
			return relation;
		}
		
		int getFreq() {
			return freq;
		}
		
		int updateFreq(){
			return ++freq;
		}
		
		
	};

	class MapRouteTag {
	public:
		bool relation;
		std::string tag;
		std::string value;
		std::string tag2;
		std::string value2;
		bool _register;
		bool amend;
		bool base; 
		bool text;
		bool replace;
		
	};

class NamedRuleContainer : public boost::ptr_map<std::string, MapRulType>
{
public:
	NamedRuleContainer() {};
	virtual ~NamedRuleContainer() {};
};

class OBFRenderingTypes
{
protected:
	static std::string constructRuleKey(std::string tag, std::string val) {
		if(val == "" || val.size() == 0){
			return tag;
		}
		return tag + TAG_DELIMETER + val;
	}

	static char TAG_DELIMETER;
public:
	OBFRenderingTypes(void);
	virtual ~OBFRenderingTypes(void);

	void loadXmlData();

	static MapRulType* nameRule;
	static MapRulType* nameEnRule;
	std::string read(const char* value)
	{
		if (value == NULL)
			return std::string("");
		return std::string(value);
	}
	
	AmenityType getAmenityTypeForRelation(std::string tag, std::string val);
	
	AmenityType getAmenityType(std::string tag, std::string val, bool relation);

	std::string getAmenitySubtype(std::string tag, std::string val);
	
	std::string getAmenitySubtypePrefix(std::string tag, std::string val);

	std::map<std::string, std::string> getAmenityAdditionalInfo(std::map<std::string, std::string> tags, AmenityType type, std::string subtype);

	static std::map<AmenityType, std::map<std::string, std::string>> amenityNameVal;
	static std::map<std::string, AmenityType> namedAmenity;
	static std::list<MapRouteTag> routeTags;
	static NamedRuleContainer namedRulType;
	static boost::ptr_vector<MapRulType, boost::view_clone_allocator> rules;

	void parseCategoryElement(tinyxml2::XMLElement* elemData,std::string poiParentCategory,std::string poiParentPrefix);
	void parseBasicElement(tinyxml2::XMLElement* elemData,std::string poiParentCategory,std::string poiParentPrefix);
	void parseRouteElement(tinyxml2::XMLElement* elemData);
	MapRulType* registerRuleType(MapRulType* rt);

	std::map<MapRulType, std::string> getRelationPropogatedTags(EntityRelation relation);
	MapRulType* getRelationalTagValue(std::string tag, std::string val);
	MapRulType* getMapRuleType(std::string tag, std::string val);
	MapRulType* getRuleType(std::string tag, std::string val, bool poi);
	void addOSMCSymbolsSpecialTags(std::map<MapRulType,std::string> propogated, std::pair<std::string,std::string> ev);
	static std::list<std::map<std::string, std::string>> splitTagsIntoDifferentObjects(const std::map<std::string, std::string> tags);
	boost::ptr_map<std::string, MapRulType>& getRuleTypes()
	{
		if (namedRulType.size() == 0)
		{
			loadXmlData();
		}
		return namedRulType;
	}

	std::list<MapRouteTag>& getRouteTags() {
		if (routeTags.size() == 0)
		{
			loadXmlData();
		}
		return routeTags;
	}
	
	static BYTE RESTRICTION_NO_RIGHT_TURN;
	static BYTE RESTRICTION_NO_LEFT_TURN;
	static BYTE RESTRICTION_NO_U_TURN;
	static BYTE RESTRICTION_NO_STRAIGHT_ON;
	static BYTE RESTRICTION_ONLY_RIGHT_TURN;
	static BYTE RESTRICTION_ONLY_LEFT_TURN;
	static BYTE RESTRICTION_ONLY_STRAIGHT_ON;

	static std::list<std::map<std::string, std::string>> OBFRenderingTypes::splitOpenSeaMapsTags(const std::map<std::string, std::string> tags);
	static bool splitIsNeeded(const std::map<std::string, std::string> tags) {
		bool seamark = false;
		for(auto s : tags) {
			if(boost::algorithm::starts_with(s.first, "seamark:")) {
				seamark = true;
				break;
			}
		}
		return seamark;
	}
	static std::string openSeaType(std::string value);
	static MapRulType* coastlineRule;


	 bool encodeEntityWithType(std::shared_ptr<EntityBase> e, int zoom, std::list<long>& outTypes, 
			std::list<long>& outAddTypes, std::map<MapRulType, std::string>& namesToEncode, std::list<MapRulType>& tempListNotUsed);
	 bool encodeEntityWithType(bool isNode, std::map<std::string, std::string> tags, int zoom, std::list<long>& outTypes, 
			std::list<long>& outAddTypes, std::map<MapRulType, std::string>& namesToEncode, std::list<MapRulType>& tempListNotUsed);


};

