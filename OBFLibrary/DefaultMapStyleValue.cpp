#include "stdafx.h"
#include "MapStyleValue.h"
#include "DefaultMapStyleValue.h"



DefaultMapStyleValue::DefaultMapStyleValue(void)
	:
	#define SETUP_DEFAULT_MAP(nameVar, accType,typeData, name, isIS) \
		nameVar(new MapStyleValue(ValType::typeData, AccessType::accType, name, isIS)),
	#include "DefaultMapValueSet.h"
	#undef SETUP_DEFAULT_MAP

	#define SETUP_DEFAULT_MAP(nameVar, typeData, accType, name, isIS) \
		id_##nameVar(nameVar->id),
		#include "DefaultMapValueSet.h"
	#undef SETUP_DEFAULT_MAP
	 _finalizer(0)
{

}


DefaultMapStyleValue::~DefaultMapStyleValue(void)
{
}
