#include "stdafx.h"
#include "MapStyleChoiceValue.h"


MapStyleChoiceValue::MapStyleChoiceValue(ValType type, std::string& name, std::string& titleP, std::string& descrP, std::vector<std::string> values)
	: MapStyleValue(type, AccessType::In, name, false),   title(titleP), description(descrP), choiceValues(values)
{
}


MapStyleChoiceValue::~MapStyleChoiceValue(void)
{
}
