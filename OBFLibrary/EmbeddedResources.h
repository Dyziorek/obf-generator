#pragma once

class EmbeddedResources
{
public:
	EmbeddedResources(void);
	~EmbeddedResources(void);

	std::vector<char> getDataFromResource(std::string name);
	std::vector<char> getRawFromResource(std::string name);
};


