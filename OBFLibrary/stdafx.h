// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// TODO: reference additional headers your program requires here
#include "sqlite3.h"
#include "google\protobuf\io\zero_copy_stream_impl.h"
#include "boost\archive\binary_iarchive.hpp"
#include <fstream>
#include <sstream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include "boost/iostreams/device/back_inserter.hpp"
#include "boost/iostreams/copy.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/map.hpp>
#include <boost/unordered_map.hpp>
#include "zlib.h"

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>


#include "EntityBase.h"
#include "EntityNode.h"
#include "MapObject.h"
#include "MapUtils.h"
