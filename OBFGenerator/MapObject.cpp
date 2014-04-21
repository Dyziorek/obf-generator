#include "stdafx.h"
#include "MapObject.h"
#include "Amenity.h"




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