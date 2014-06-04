#pragma once
class OSMTags
{
public:
	OSMTags(void);
	~OSMTags(void);

	static std::string	NAME;
	static std::string	NAME_EN; 
		
		// ways
	static std::string	HIGHWAY; 
		static std::string BUILDING; 
	static std::string			BOUNDARY; 
	static std::string			POSTAL_CODE; 
	static std::string			RAILWAY; 
	static std::string			ONEWAY; 
	static std::string			LAYER; 
	static std::string			BRIDGE; 
		static std::string		TUNNEL; 
	static std::string			TOLL; 
	static std::string			JUNCTION; 
		
		
		// transport
	static std::string			ROUTE; 
	static std::string			ROUTE_MASTER; 
	static std::string			OPERATOR; 
	static std::string			REF; 
	static std::string			RCN_REF; 
	static std::string			RWN_REF; 
		
		// address
	static std::string			PLACE; 
	static std::string			ADDR_HOUSE_NUMBER; 
	static std::string			ADDR_HOUSE_NAME; 
	static std::string			ADDR_STREET; 
	static std::string			ADDR_STREET2; 
	static std::string			ADDR_CITY; 
	static std::string			ADDR_POSTCODE; 
	static std::string			ADDR_INTERPOLATION; 
	static std::string			ADDRESS_TYPE; 
	static std::string			ADDRESS_HOUSE; 
	static std::string			TYPE; 
	static std::string			IS_IN; 
	static std::string			LOCALITY; 
		
		// POI
	static std::string			AMENITY; 
	static std::string			SHOP; 
	static std::string			LANDUSE;  
	static std::string			OFFICE;  
	static std::string			EMERGENCY;  
	static std::string			MILITARY;  
	static std::string			ADMINISTRATIVE;  
	static std::string			MAN_MADE;  
	static std::string			BARRIER;  
	static std::string			LEISURE;  
	static std::string			TOURISM; 
	static std::string			SPORT;  
	static std::string			HISTORIC; 
	static std::string			NATURAL; 
	static std::string			INTERNET_ACCESS; 
		
		
	static std::string			CONTACT_WEBSITE; 
	static std::string			CONTACT_PHONE; 
		
	static std::string			OPENING_HOURS;  
	static std::string			PHONE; 
	static std::string			DESCRIPTION; 
	static std::string			WEBSITE; 
	static std::string			URL; 
	static std::string			WIKIPEDIA; 
		
	static std::string			ADMIN_LEVEL; 
	static std::string			PUBLIC_TRANSPORT; 
};

