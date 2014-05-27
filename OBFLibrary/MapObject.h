#pragma once


class MapObject
{
public:
	MapObject(void) {name = ""; type = ""; lat = -1000; lon = -1000;}
	virtual ~MapObject(void);
	const std::string getName(){return name;}
	const std::string getEnName(){return enName;}
	void setName(std::string tags) { name = tags;}
	void setEnName(std::string tags) { enName= tags;}

	void setLocation(double rlat, double rlon) { lat = rlat; lon = rlon;}
	
	void setId(__int64 objId) {id = objId;}
	std::pair<double , double> getLatLon() { return std::make_pair(lat, lon);}

	static void parseMapObject(MapObject* mo, EntityBase* e);

	 static void setNameFromRef(MapObject& mo, EntityBase& e);

	static void setNameFromOperator(MapObject& mo, EntityBase& e);

	__int64 getID(void) {return id;}
	void setType(std::string Rtype) { type = Rtype;}
	std::string getType() { return type;}

private:
	std::string name;
	std::string enName;
	__int64 id;
	double lat;
	double lon;
	std::string type;
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

class CityObj :
	public MapObject
{
public:
	CityObj(void);
	virtual ~CityObj(void);
	std::size_t operator()(CityObj const& obj) const;
	bool operator==(const CityObj & x) const;
	boost::unordered_map<std::string, std::string, icaseString::ihash, icaseString::iequal_to> streets;
	bool isAlwaysVisible;
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
};


namespace boost
{
	template<>
	struct hash<CityObj>
	{
		std::size_t operator()(const CityObj& hashWork) const
		{
			size_t seed;
			CityObj& objCon = const_cast<CityObj&>(hashWork);
			hash_combine(seed, objCon.getName());
			hash_combine(seed, objCon.getEnName());
			hash_combine(seed, objCon.getID());
			return seed;
		}
	};
}