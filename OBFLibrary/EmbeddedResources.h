#pragma once

class EmbeddedResources
{
public:
	EmbeddedResources(void);
	~EmbeddedResources(void);

	static std::vector<char> getDataFromResource(std::string name);
	static std::vector<char> getRawFromResource(std::string name);
};


