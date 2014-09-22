#pragma once


class DefaultMapStyleValue
{
public:
	DefaultMapStyleValue(void);
	~DefaultMapStyleValue(void);

#define SETUP_DEFAULT_MAP(nameVar, typeData, accType, name, isIS) const std::shared_ptr<MapStyleValue> nameVar;	
#include "DefaultMapValueSet.h"
#undef SETUP_DEFAULT_MAP

#define SETUP_DEFAULT_MAP(nameVar, typeData, accType, name, isIS) const int id_##nameVar;	
#include "DefaultMapValueSet.h"
#undef SETUP_DEFAULT_MAP
	const int _finalizer;
};

