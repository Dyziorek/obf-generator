#pragma once
#include "mapstylevalue.h"
class MapStyleChoiceValue :
	public MapStyleValue
{
public:
	MapStyleChoiceValue(ValType type, std::string& name, std::string& title, std::string& descr, std::vector<std::string> values);
	virtual ~MapStyleChoiceValue(void);

	const std::string title;
	const std::string description;
	const std::vector<std::string> choiceValues;
};

