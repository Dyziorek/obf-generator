#pragma once

#include <iostream>
#include "boost\archive\binary_iarchive.hpp"
#include "proto\fileformat.pb.h"
#include "proto\osmformat.pb.h"

namespace ar = boost::archive;

class BlockReader
{
public:
	BlockReader(void);
	~BlockReader(void);


	BlockHeader ReadBlockHead(ar::binary_iarchive& strmData);
	Blob ReadContents(BlockHeader& infoData, ar::binary_iarchive& strmData);
	HeaderBlock GetHeaderContents(Blob& info);
	PrimitiveBlock GetBlockContents(Blob& info);
};

