
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


#include "sqlite3.h"
#include "afxdialogex.h"
#include "proto\osmformat.pb.h"
#include "google\protobuf\io\zero_copy_stream_impl.h"
#include "boost\archive\binary_iarchive.hpp"
#include <fstream>
#include <sstream>
#include "BlockReader.h"
#include "portable_binary_iarchive.hpp"
#include "portable_binary_oarchive.hpp"
#include "proto\fileformat.pb.h"
#include "proto\osmformat.pb.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/map.hpp>
#include <boost/container/list.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "zlib.h"

#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>


#include "OBFGenerator.h"
#include "EntityBase.h"
#include "EntityNode.h"
#include "MapObject.h"
#include "MapUtils.h"
#include "Amenity.h"
#include "BatchUpdater.h"
#include "OBFResultDB.h"
#include "OBFGeneratorDlg.h"

//
//#ifdef _DEBUG 
//	#ifndef DBG_NEW  
//		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ ) 
//		#define new DBG_NEW 
//	#endif
//#endif  // _DEBUG