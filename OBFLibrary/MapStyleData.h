#pragma once
struct MapStyleData
{
	MapStyleData(void);
	~MapStyleData(void);
	bool isSpecial;

	template<typename Tx> struct specialTXData
	{
		Tx dip;
		Tx px;
		inline Tx calculate(float mul)
		{
			return dip * mul + px;
		}
	};

	union {
		union{
			float asFloat;
            int32_t asInt;
            uint32_t asUInt;

            double asDouble;
            int64_t asInt64;
            uint64_t asUInt64;
		} simpleData;

		union {
			specialTXData<float> asFloat;
			specialTXData<int32_t> asInt;
			specialTXData<uint32_t> asUInt;
		} specialData;
	};
};

