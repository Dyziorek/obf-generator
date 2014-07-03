#include "stdafx.h"
#include "MapObject.h"
#include "EntityBase.h"
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

//#include <boost/spirit/include/qi.hpp>

//namespace qi = boost::spirit::qi;

/*
enum OSMTagKey {
		NAME("name"), //$NON-NLS-1$
		NAME_EN("name:en"), //$NON-NLS-1$
		
		// ways
		HIGHWAY("highway"), //$NON-NLS-1$
		BUILDING("building"), //$NON-NLS-1$
		BOUNDARY("boundary"), //$NON-NLS-1$
		POSTAL_CODE("postal_code"), //$NON-NLS-1$
		RAILWAY("railway"), //$NON-NLS-1$
		ONEWAY("oneway"), //$NON-NLS-1$
		LAYER("layer"), //$NON-NLS-1$
		BRIDGE("bridge"), //$NON-NLS-1$
		TUNNEL("tunnel"), //$NON-NLS-1$
		TOLL("toll"), //$NON-NLS-1$
		JUNCTION("junction"), //$NON-NLS-1$
		
		
		// transport
		ROUTE("route"), //$NON-NLS-1$
		ROUTE_MASTER("route_master"), //$NON-NLS-1$
		OPERATOR("operator"), //$NON-NLS-1$
		REF("ref"), //$NON-NLS-1$
		RCN_REF("rcn_ref"), //$NON-NLS-1$
		RWN_REF("rwn_ref"), //$NON-NLS-1$
		
		// address
		PLACE("place"), //$NON-NLS-1$
		ADDR_HOUSE_NUMBER("addr:housenumber"), //$NON-NLS-1$
		ADDR_HOUSE_NAME("addr:housename"), //$NON-NLS-1$
		ADDR_STREET("addr:street"), //$NON-NLS-1$
		ADDR_STREET2("addr:street2"), //$NON-NLS-1$
		ADDR_CITY("addr:city"), //$NON-NLS-1$
		ADDR_POSTCODE("addr:postcode"), //$NON-NLS-1$
		ADDR_INTERPOLATION("addr:interpolation"), //$NON-NLS-1$
		ADDRESS_TYPE("address:type"), //$NON-NLS-1$
		ADDRESS_HOUSE("address:house"), //$NON-NLS-1$
		TYPE("type"), //$NON-NLS-1$
		IS_IN("is_in"), //$NON-NLS-1$
		LOCALITY("locality"), //$NON-NLS-1$
		
		// POI
		AMENITY("amenity"), //$NON-NLS-1$
		SHOP("shop"), //$NON-NLS-1$
		LANDUSE("landuse"),  //$NON-NLS-1$
		OFFICE("office"),  //$NON-NLS-1$
		EMERGENCY("emergency"),  //$NON-NLS-1$
		MILITARY("military"),  //$NON-NLS-1$
		ADMINISTRATIVE("administrative"),  //$NON-NLS-1$
		MAN_MADE("man_made"),  //$NON-NLS-1$
		BARRIER("barrier"),  //$NON-NLS-1$
		LEISURE("leisure"),  //$NON-NLS-1$
		TOURISM("tourism"), //$NON-NLS-1$
		SPORT("sport"),  //$NON-NLS-1$
		HISTORIC("historic"), //$NON-NLS-1$
		NATURAL("natural"), //$NON-NLS-1$
		INTERNET_ACCESS("internet_access"), //$NON-NLS-1$
		
		
		CONTACT_WEBSITE("contact:website"), //$NON-NLS-1$
		CONTACT_PHONE("contact:phone"), //$NON-NLS-1$
		
		OPENING_HOURS("opening_hours"),  //$NON-NLS-1$
		PHONE("phone"), //$NON-NLS-1$
		DESCRIPTION("description"), //$NON-NLS-1$
		WEBSITE("website"), //$NON-NLS-1$
		URL("url"), //$NON-NLS-1$
		WIKIPEDIA("wikipedia"), //$NON-NLS-1$
		
		ADMIN_LEVEL("admin_level"), //$NON-NLS-1$
		PUBLIC_TRANSPORT("public_transport") //$NON-NLS-1$
		;
		
		std::string value;
		OSMTagKey(String value) {
			this.value = value;
		}
		
		public String getValue() {
			return value;
		}
	}
	*/

EntityBase::EntityBase(void)
{
}


EntityBase::~EntityBase(void)
{
}


void EntityBase::parseTags(std::string tagList)
{
	boost::char_separator<char> sep("~");
	boost::tokenizer< boost::char_separator<char> > tokens(tagList, sep);
	
	/*
    qi::rule<std::string::const_iterator, std::string()> valid_characters = qi::char_ - '~';
    qi::rule<std::string::const_iterator, std::string()> item = *( valid_characters );
    qi::rule<std::string::const_iterator, std::list<std::string>()> csv_parser = item % '~';

    std::string::const_iterator s_begin = tagList.begin();
    std::string::const_iterator s_end = tagList.end();
    std::list<std::string> result;

    bool r = boost::spirit::qi::parse(s_begin, s_end, csv_parser, result);
    assert(r == true);
    assert(s_begin == s_end);
	
	bool oddVal = true;
	std::string tagName;
    for (auto i : result)
	{
        if (oddVal)
		{
			tagName = i;
			oddVal = false;
		}
		else
		{
			tags.insert(std::make_pair(tagName, i));
			oddVal = true;
		}
	}

	*/
		bool oddVal = true;
	std::string tagName;
	BOOST_FOREACH(const std::string token, tokens){
		if (oddVal)
		{
			tagName = token;
			oddVal = false;
		}
		else
		{
			tags.insert(std::make_pair(tagName, token));
			oddVal = true;
		}
	}
	
}

std::unordered_set<std::string> EntityBase::getIsInNames() {
		std::string values = getTag("is_in");
		if (values == "") {
			return std::unordered_set<std::string>();
		}
		if (values.find(';') != std::string::npos) {
			boost::char_separator<char> sep(";");
			boost::tokenizer< boost::char_separator<char> > tokens(values, sep);
			std::vector<std::string> splitted;
			BOOST_FOREACH(const std::string token, tokens){ splitted.push_back(token);}
			std::unordered_set<std::string> set(splitted.size());
			for (int i = 0; i < splitted.size(); i++) {
				set.insert(boost::trim_copy(splitted[i]));
			}
			return set;
		}
		std::unordered_set<std::string> bset;
		bset.insert(boost::trim_copy(values));
		return bset;
	}
