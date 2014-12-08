#pragma once

class EmbeddedResources
{
public:
	EmbeddedResources(void);
	~EmbeddedResources(void);

	std::vector<uint8_t> getDataFromResource(std::string name);
	std::vector<char> getRawFromResource(std::string name);
};

