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
	 cityName = getName();
	 streets.clear();
 }
 
 CityObj::CityObj(const CityObj& parent) : MapObject(parent)
 {
	 isin = parent.isin;
	 postcode = parent.postcode;
	 cityName = parent.cityName;
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
		auto strRemove = streets.find(name);
		Street removeStreet = strRemove->second;
		streets.erase(strRemove);
		return removeStreet;
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

 int POICategory::SPECIAL_CHAR = -1;
 short POICategory::SHIFT_BITS_CATEGORY = 7;

 void POICategory::addCategory(std::string type, std::string addType, std::unordered_map<MapRulType*, std::string>& addTags)
{
	for (std::pair<MapRulType* , std::string> rvalue : addTags)
	{
		if (rvalue.first->isAdditional() && rvalue.first->getValue() == "")
		{
			throw std::bad_exception("Empty value for additional tags");
		}
		attributes.insert(rvalue.first);
	}
	if (categories.find(type) == categories.end())
	{
		categories.insert(std::make_pair(type,std::set<std::string>()));
	}
	if (std::find_if(addType.begin(), addType.end(), boost::is_any_of(std::string(",;"))) !=  addType.end())
	{
		std::vector<std::string> splits;
		boost::split(splits, addType, boost::is_any_of(std::string(",;")), boost::token_compress_on);
		for (std::string strTag : splits)
			categories[type].insert(boost::trim_copy(strTag));
	}
	else
	{
		categories[type].insert(boost::trim_copy(addType));
	}
}

 std::vector<int> POICategory::buildTypeIds(std::string category, std::string subcategory) {
			singleThreadVarTypes.clear();
			std::vector<int> types = singleThreadVarTypes;
			internalBuildType(category, subcategory, types);
			return types;
}
		
void POICategory::internalBuildType(std::string category, std::string subcategory, std::vector<int>& types) {
	int catInd = catIndexes.find(category) == catIndexes.end() ? -1 : catIndexes[category];
			if (std::find_if(subcategory.begin(), subcategory.end(), boost::is_any_of(std::string(",;"))) !=  subcategory.end())
			{
				std::vector<std::string> splits;
				boost::split(splits, subcategory, boost::is_any_of(std::string(",;")), boost::token_compress_on);
				for (std::string sub : splits) {
					int subcatInd = catSubIndexes.find(category + (char)SPECIAL_CHAR + boost::trim_copy(sub)) == catSubIndexes.end() ? -1 : catSubIndexes[category + (char)SPECIAL_CHAR + boost::trim_copy(sub)];
					if (subcatInd == -1) {
						throw new std::bad_exception("Should not be here");
					}
					types.push_back((subcatInd << SHIFT_BITS_CATEGORY) | catInd);
				}
			} else {
				
				int subcatInd = catSubIndexes.find(category + (char)SPECIAL_CHAR + boost::trim_copy(subcategory)) == catSubIndexes.end() ? -1 : catSubIndexes[category + (char)SPECIAL_CHAR + boost::trim_copy(subcategory)];
				if (subcatInd == -1) {
					std::string msg = std::string("Unknown subcategory ") + subcategory + " category " + category;
					throw new std::bad_exception(msg.c_str());
				}
				types.push_back((subcatInd << SHIFT_BITS_CATEGORY) | catInd);
			}
		}

void POICategory::buildCategoriesToWrite(POICategory& globalCategories) {
			cachedCategoriesIds.clear();
			cachedAdditionalIds.clear();

			for(auto cats : categories) {
				for(std::string subcat : cats.second){
					std::string cat = cats.first;
					globalCategories.internalBuildType(cat, subcat, cachedCategoriesIds);
				}
			}
			for(MapRulType* rt : attributes){
				if(rt->getTargetPoiId() == -1) {
					throw new std::bad_exception("Map rule type is not registered for poi : ");
				}
				cachedAdditionalIds.push_back(rt->getTargetPoiId());
			}
		}