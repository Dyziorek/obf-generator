#include "stdafx.h"
#include "MapStyleValue.h"

boost::atomic<int> MapStyleValue::valId(1);



MapStyleValue::MapStyleValue(ValType valType, AccessType acc, std::string name, bool complex) : 
	id(valId.fetch_add(1)), typeAcces(acc), typeData(valType), name(name), isComplex(complex)
{
}


MapStyleValue::~MapStyleValue(void)
{
}
