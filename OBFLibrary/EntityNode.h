#pragma once
#include "entitybase.h"



class EntityNode :
	public EntityBase
{
public:
	EntityNode(void);
	EntityNode(double olat, double olon, __int64 nid)
	{
		lat = olat; lon = olon; id = nid;
	}
	virtual ~EntityNode(void);

	double lat;
	double lon;
	
	bool isEmpty() { return lat == -1000 && lon == -1000;}
	std::pair<double, double> getLatLon(){return std::make_pair(lat, lon);}
	bool operator<(const EntityNode& op1) const { return id < op1.id;}
	bool operator==(const EntityNode& op1) const { return (id == op1.id && lat == op1.lat && lon == op1.lon);}
};

//struct pairComp 
//{
//	bool operator()(const std::pair<int,std::shared_ptr<EntityBase>>& op1, const std::pair<int,std::shared_ptr<EntityBase>>& op2)
//	{
//		return *op1.second < *op2.second;
//	}
//};

class EntityRelation :
	public EntityBase
{
public:
	EntityRelation(void);
	virtual ~EntityRelation(void);

	std::list<std::pair<__int64, std::pair<int,std::string>>> entityIDs;

	std::unordered_map<__int64, std::tuple<int,std::shared_ptr<EntityBase>, std::string>> relations;

	std::list<std::pair<int, __int64>> getEntityIDforName(std::string name)
	{
		std::list<std::pair<int, __int64>> result;
		for (auto idAuto : entityIDs)
		{
			if (idAuto.second.second == name)
			{
				result.push_back(std::make_pair(idAuto.second.first, idAuto.first));
			}
		}

		return result;
	}

	std::list<std::shared_ptr<EntityBase>> getRelations()
	{
		std::list<std::shared_ptr<EntityBase>> relData;
		for(auto nRel : relations)
		{
			relData.push_back(std::get<1>(nRel.second));
		}
		return relData;
	}

	std::vector<std::shared_ptr<EntityBase>> getMembers(std::string name)
	{
		std::vector<std::shared_ptr<EntityBase>> members;

		for (auto relMemId : entityIDs)
		{
			auto relationMember = relations[relMemId.first];
			{
				if (std::get<2>(relationMember) == name)
				{
					members.push_back(std::get<1>(relationMember));
				}
			}
		}
		return members;
	}

	bool operator<(const EntityRelation& op1) const { return id < op1.id;}
	std::pair<double, double> getLatLon(){return std::make_pair(-1000, -1000);}
};

class EntityWay :
	public EntityBase
{
public:
	EntityWay(__int64 random) {id = random;}
	EntityWay(void);
	virtual ~EntityWay(void);

	std::vector<__int64> nodeIDs;

	std::vector<std::shared_ptr<EntityNode>> nodes;

	bool operator<(const EntityWay& op1) const { return id < op1.id;}
	std::pair<double, double> getLatLon();
	std::list<std::shared_ptr<EntityNode>> getListNodes()
	{
		return std::list<std::shared_ptr<EntityNode>>(nodes.begin(), nodes.end());
	}

	__int64 getFirstNodeId() {if (nodeIDs.empty()) return -1; return *nodeIDs.begin(); }
	__int64 getLastNodeId() {if (nodeIDs.empty()) return -1; return *--nodeIDs.end(); }
	
};