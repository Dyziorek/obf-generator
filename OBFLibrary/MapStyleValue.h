#pragma once
enum ValType
{
	Booltype,
	Inttype,
	Floattype,
	Stringtype,
	Colortype
};

enum AccessType
{
	In,
	Out
};

class MapStyleValue
{
private:


	static boost::atomic<int> valId;


public:
	MapStyleValue(ValType valType, AccessType acc, std::string name, bool complex);
	virtual ~MapStyleValue(void);

	const int id;
	const ValType typeData;
	const AccessType typeAcces;
	const std::string name;
	const bool isComplex;

private:
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MapStyleValue);
};

