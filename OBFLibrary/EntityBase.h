#pragma once

#define NODE_ID	0
#define WAY_ID	1
#define REL_ID	2

typedef std::pair<double, double> LatLon;

class EntityBase
{
public:
	EntityBase(void);
	virtual ~EntityBase(void);
	std::unordered_map<std::string, std::string> tags;

	void parseTags(std::string tagList);

	std::string getTag(std::string name) { if ( tags.find(name) != tags.end()) return tags.find(name)->second; return std::string("");}

	void putTag(std::string name, std::string value) { tags.insert(std::make_pair(name, value)); }

	std::unordered_set<std::string> getIsInNames();

	bool operator<(const EntityBase& op1) const { return id < op1.id;}
	__int64 id;
	virtual std::pair<double, double> getLatLon() {return std::make_pair(-1000, -1000);}
};

