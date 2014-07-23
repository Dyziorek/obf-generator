#pragma once


class MapObject
{
public:
	MapObject(void) {name = ""; type = ""; lat = -1000; lon = -1000;}
	virtual ~MapObject(void);
	std::string getName(){return name;} const
	std::string getEnName(){return enName;} const
	void setName(std::string tags) { name = tags;}
	void setEnName(std::string tags) { enName= tags;}

	void setLocation(double rlat, double rlon) { lat = rlat; lon = rlon;}
	
	void setId(__int64 objId) {id = objId;}
	std::pair<double , double> getLatLon() { return std::make_pair(lat, lon);}

	static void parseMapObject(MapObject* mo, EntityBase* e);

	 static void setNameFromRef(MapObject& mo, EntityBase& e);

	static void setNameFromOperator(MapObject& mo, EntityBase& e);

	__int64 getID(void) const {return id;}
	void setType(std::string Rtype) { type = Rtype;}
	std::string getType() { return type;}

	int getFileOffset() {
		return fileOffset;
	}
	
	void setFileOffset(int fileOffset) {
		this->fileOffset = fileOffset;
	}
	
private:
	std::string name;
	std::string enName;
	__int64 id;
	double lat;
	double lon;
	std::string type;
	int fileOffset;
};

namespace icaseString
{
struct iequal_to
        : std::binary_function<std::string, std::string, bool>
    {
        iequal_to() {}
        explicit iequal_to(std::locale const& l) : locale_(l) {}

        template <typename String1, typename String2>
        bool operator()(String1 const& x1, String2 const& x2) const
        {
            return boost::algorithm::iequals(x1, x2, locale_);
        }
    private:
        std::locale locale_;
    };

    struct ihash
        : std::unary_function<std::string, std::size_t>
    {
        ihash() {}
        explicit ihash(std::locale const& l) : locale_(l) {}

        template <typename String>
        std::size_t operator()(String const& x) const
        {
            std::size_t seed = 0;

            for(typename String::const_iterator it = x.begin();
                it != x.end(); ++it)
            {
                boost::hash_combine(seed, std::toupper(*it, locale_));
            }

            return seed;
        }
    private:
        std::locale locale_;
    };

}

class Street;

class CityObj :
	public MapObject
{
private:
	std::string isin;
	std::string postcode;
	static long POSTCODE_INTERNAL_ID;
public:
	CityObj(void);
	CityObj(const CityObj& parent);
	virtual ~CityObj(void);

	std::size_t operator()(CityObj const& obj) const;
	bool operator==(const CityObj & x) const;
	std::unordered_map<std::string, Street, icaseString::ihash, icaseString::iequal_to> streets;
	Street registerStreet(Street street);
	Street unregisterStreet(std::string);
	bool isAlwaysVisible;
	bool isPostcode()
	{
		return getType() == "";
	}
	double getRadius(){
		if (getType() == "city")
			return 10000;
		if (getType() == "town")
			return 5000;
		if (getType() == "village")
			return 1300;
		if (getType() == "hamlet")
			return 1000;
		if (getType() == "suburb")
			return 400;
		if (getType() == "district")
			return 400;
		return 400;
	}

	std::string getIsInValue()
	{
		return isin;
	}
	void setIsin(std::string outisin)
	{
		isin=  outisin;
	}
	std::string getPostcode()
	{
		return postcode;
	}
	void setPostcode(std::string post)
	{
		postcode =  post;
	}

	static  CityObj createPostcode(std::string post);
};


namespace std
{
	template<>
	struct hash<CityObj>
	{
		std::size_t operator()(const CityObj& hashWork) const
		{
			size_t seed;
			CityObj& objCon = const_cast<CityObj&>(hashWork);
			boost::hash_combine(seed, objCon.getName());
			boost::hash_combine(seed, objCon.getEnName());
			boost::hash_combine(seed, objCon.getID());
			return seed;
		}
	};
}



struct POIData
{
	int x, y;
	__int64 id;
	std::string type;
	std::string subType;
	std::unordered_map<MapRulType, std::string> additionalTags;
};

struct POICategory
{
	std::map<std::string, std::set<std::string>> categories;
	std::set<MapRulType> attributes;
	std::unordered_map<std::string, int> catIndexes;
	std::unordered_map<std::string, int> catSubIndexes;
	std::vector<int> cachedCategoriesIds;
	std::vector<int> cachedAdditionalIds;
	std::vector<int> singleThreadVarTypes;

	void addCategory(std::string type, std::string addType, std::unordered_map<MapRulType, std::string>& addTags);
	std::vector<int> buildTypeIds(std::string category, std::string subcategory);
	void internalBuildType(std::string category, std::string subcategory, std::vector<int>& types);
	void buildCategoriesToWrite(POICategory& globalCategories) ;
	static int SPECIAL_CHAR;
	static short SHIFT_BITS_CATEGORY;
};


struct POIBox
{
	int x;
	int y;
	int zoom;
	std::list<POIData> values;
	POICategory category;
	bool operator==(const POIBox& op2) const
	{
		return x == op2.x 
			&& y == op2.y 
			&& zoom == op2.zoom 
			&& values.size() == op2.values.size() 
			&& category.attributes.size() == op2.category.attributes.size();
	}
};

template<>
	struct std::hash<POIBox>
		: public std::unary_function<POIBox, size_t>
	{	
		size_t operator()(const POIBox& argValue) const
		{
			size_t xaHashData = 16777619U;
			xaHashData *= argValue.x;
			xaHashData *= argValue.y;
			xaHashData *= argValue.zoom;
			xaHashData *= argValue.values.size();
			return xaHashData;
		}
	};	
