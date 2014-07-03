#include "stdafx.h"
#include "MapObject.h"
#include "Amenity.h"
#include "Street.h"
#include "Building.h"

long CityObj::POSTCODE_INTERNAL_ID = -1000;


MapObject::~MapObject(void)
{
}


 void MapObject::parseMapObject(MapObject* mo, EntityBase* e) {
		mo->setId(e->id);
		Amenity* pAm = dynamic_cast<Amenity*>(mo);
		EntityNode* pNode = dynamic_cast<EntityNode*>(e);
		if(pAm != nullptr) {
			pAm->setId((e->id << 1) + ((pNode != nullptr) ? 0 : 1));
		}
		if (mo->getName().size() == 0) {
			mo->setName(e->getTag("name"));
		}
		if (mo->getEnName().size() == 0) {
			mo->setEnName(e->getTag("en:name"));
			if (mo->getName().size() == 0) {
				mo->setName(mo->getEnName());
			}
		}
		if (mo->getLatLon().first == -1000) {
			std::pair<double , double> l = OsmMapUtils::getCenter(e);
			if (l.first != -1000) {
				mo->setLocation(l.first, l.second);
			}
		}
		if (mo->getName().size() == 0) {
			setNameFromOperator(*mo, *e);
		}
		if (mo->getName().size() == 0) {
			setNameFromRef(*mo, *e);
		}
	}


 void MapObject::setNameFromRef(MapObject& mo, EntityBase& e) {
		std::string ref = e.getTag("ref");
		if(ref != ""){
			mo.setName(ref);
		}
	}

 void MapObject::setNameFromOperator(MapObject& mo, EntityBase& e) {
		std::string op = e.getTag("operator");
		if (op == "")
			return;
		std::string ref = e.getTag("ref");
		if (ref != "")
			op += " [" + ref + "]";
		mo.setName(op);
	}


 CityObj::CityObj(void)
 {
	 isAlwaysVisible = false;
	 isin = "";
	 streets.clear();
 }
 
 CityObj::CityObj(const CityObj& parent) : MapObject(parent)
 {
	 isin = parent.isin;
	 postcode = parent.postcode;
	 streets.clear();
	 if (parent.streets.size() > 0)
	 {
		 for (auto stIt : parent.streets)
		 {
			 streets.insert(std::make_pair(stIt.first, stIt.second));
		 }
	 }
 }

CityObj::~CityObj(void)
{
}

std::size_t CityObj::operator()(CityObj const& obj) const
{
		std::size_t seed = 0;
		CityObj& objCon = const_cast<CityObj&>(obj);
		boost::hash_combine(seed, objCon.getName());
		boost::hash_combine(seed, objCon.getEnName());
		boost::hash_combine(seed, objCon.getType());
		boost::hash_combine(seed, objCon.getID());
		boost::hash_combine(seed, objCon.getLatLon().first);
		boost::hash_combine(seed, objCon.getLatLon().second);
		return seed;
}

bool CityObj::operator==(const CityObj & x) const
{
	CityObj& obj1 = const_cast<CityObj&>(*this);
	CityObj& obj2 = const_cast<CityObj&>(x);

	return obj1.getName() == obj2.getName() && obj1.getID() == obj2.getID();
}

Street CityObj::registerStreet(Street street)
{
	std::string name = street.getName();
	boost::to_lower(name);
		if (name != "") {
			if (streets.find(name) == streets.end()) {
				streets.insert(std::make_pair(name, street));
				return street;
			} else {
				// try to merge streets
				Street prev = streets[name];
				prev.mergeWith(street);
				return prev;
			}
		}
	return Street();
}

Street CityObj::unregisterStreet(std::string name)
{
	if (streets.find(name) != streets.end())
	{
		return streets.erase(streets.find(name))->second;
	}
}

 CityObj CityObj::createPostcode(std::string post)
 {
	 CityObj object;

	 object.setType("");
	 object.postcode = post;
	 object.setId(CityObj::POSTCODE_INTERNAL_ID--);

	 return object;
 }