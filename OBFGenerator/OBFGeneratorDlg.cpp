
// OBFGeneratorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "OBFGeneratorDlg.h"
#include "SkGraphics.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkBitmapDevice.h"
#include "SkImageDecoder.h"
#include "SkColorFilter.h"
#include "SkBitmapProcShader.h"
#ifdef _DEBUG_VLD
	#include "vld.h"
#endif
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "RandomAccessFileReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryMapDataReader.h"
#include "BinaryIndexDataReader.h"

#include <locale>
#include <codecvt>

#include "tinyxml2.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"
#include "MapRasterizer.h"
#include "MapRasterizerContext.h"
#include "MapRasterizerProvider.h"
#include "AtlasMapDxRender.h"

namespace io = boost::iostreams;
namespace ar = boost::archive;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()
// COBFGeneratorDlg dialog



COBFGeneratorDlg::COBFGeneratorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COBFGeneratorDlg::IDD, pParent)
	, m_filePath(_T(""))
	, m_fileReadPath(_T(""))
	, m_DecompressFile(_T(""))
	, zoomLevel(10)
	, m_XCoord(0)
	, m_YCoord(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COBFGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE1, m_Browse);
	DDX_Text(pDX, IDC_MFCEDITBROWSE1, m_filePath);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Text(pDX, IDC_MFCEDITBROWSE2, m_fileReadPath);
	DDX_Control(pDX, IDC_MFCEDITBROWSE2, m_BrowseRead);
	DDX_Text(pDX, IDC_MFCEDITBROWSE3, m_DecompressFile);
	DDX_Text(pDX, IDC_EDIT2, zoomLevel);
	DDV_MinMaxInt(pDX, zoomLevel, 0, 31);
	DDX_Text(pDX, IDC_EDIT1, m_XCoord);
	DDX_Text(pDX, IDC_EDIT3, m_YCoord);
}

BEGIN_MESSAGE_MAP(COBFGeneratorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MFCBUTTON1, &COBFGeneratorDlg::OnBnClickedMfcbutton1)
	ON_MESSAGE(WM_MYMESSAGE,  &COBFGeneratorDlg::OnMyMessage)
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BUTTON1, &COBFGeneratorDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_MFCBUTTON2, &COBFGeneratorDlg::OnBnClickedMfcbutton2)
	ON_BN_CLICKED(IDC_MFCBUTTON3, &COBFGeneratorDlg::OnBnClickedMfcbutton3)
	ON_WM_TIMER()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// COBFGeneratorDlg message handlers

BOOL COBFGeneratorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	char* errMsg;
	m_Browse.EnableFileBrowseButton(L"pbf", L"PBF Files|*.pbf|All Files|*.*||");
	m_BrowseRead.EnableFileBrowseButton(L"bin", L"BIN Files|*.bin|All Files|*.*||");
	int dbRes = SQLITE_OK;
	FILE* fp = NULL;
	errno_t err;

	if ((err = fopen_s(&fp, "D:\\osmData\\tempLocalIN.db", "r")) == 0)
	{
		fclose(fp);
		remove("D:\\osmData\\tempLocalIN.db");
	}

	dbRes = sqlite3_open("D:\\osmData\\tempLocalIN.db", &dbCtx);
	//dbRes = sqlite3_open("D:\\osmData\\tempLocalWay.db", &dbWayCtx);
	//dbRes = sqlite3_open("D:\\osmData\\tempLocalRel.db", &dbCtx);
	dbRes = sqlite3_exec(dbCtx, "drop table if exists node" , &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create table node (id bigint primary key, latitude double, longitude double, tags blob)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "drop table if exists ways" , &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create table ways (id bigint primary key, wayid bigint, node bigint, ord smallint, tags blob, boundary smallint)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "drop table if exists relations" , &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create table relations (id bigint primary key, relid bigint,  member bigint, type smallint, role varchar(1024), ord smallint, tags blob)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 

	dbRes = sqlite3_prepare_v2(dbCtx, "insert into node values (?1, ?2, ?3, ?4)", strlen("insert into node values (?1, ?2, ?3, ?4)"), &nodeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbCtx, "insert into ways values (?1, ?2, ?3, ?4, ?5, ?6)", strlen("insert into ways values (?1, ?2, ?3, ?4, ?5, ?6)"), &wayStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbCtx, "insert into relations values (?1, ?2, ?3, ?4, ?5, ?6, ?7)", strlen("insert into relations values (?1, ?2, ?3, ?4, ?5, ?6, ?7)"), &relStmt, NULL);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	//dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=WAL", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	
	/*dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);*/
	//sqlite3_exec(dbCtx, "create index IdWIndex ON ways (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	results.PrepareDB(dbCtx);

	renderer.reset(new AtlasMapDxRender());
	renderer->Initialize(GetDlgItem(IDC_PLACEDX)->GetSafeHwnd());
	
	SetTimer(1000, 5000, NULL);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

int COBFGeneratorDlg::shell_callback(void *pArg, int nArg, char **azArg, char **azCol)
{
	return 0;
}

void COBFGeneratorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COBFGeneratorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();

		renderer->renderScene();

	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COBFGeneratorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#if defined(_M_X64) || defined(__amd64__)
inline double parsegeo(long long degree, long long granularity, int32_t offset) {
      // Support non-zero offsets. (We don't currently generate them)
      return (granularity * degree + offset) * 0.000000001;
}
#else
inline double parsegeo(__int64 degree, __int64 granularity, int offset) {
      // Support non-zero offsets. (We don't currently generate them)
      return (granularity * degree + offset) * 0.000000001;
}
#endif

UINT __cdecl threadProc(LPVOID thParam)
{
	COBFGeneratorDlg* procData = (COBFGeneratorDlg*)thParam;
	if (procData->ParseFile() == 0)
	{
		
		procData->PrepareTempDB();

		
	}
	return 0;
}


boost::thread_group producers, savers;
boost::lockfree::spsc_queue<Blob> thWorkQueue(10);
boost::lockfree::spsc_queue<PrimitiveBlock> thSaveQueue(10);
boost::atomic<BOOL> feedComplete;
boost::atomic<BOOL> active;
boost::atomic<int> NodeElems;
boost::atomic<int> NodeElemTrans;
boost::atomic<int> WayElems;
boost::atomic<int> RelElemns;
boost::atomic<int> DenseElems;
boost::atomic<int> WayElemsTrans;
boost::atomic<int> RelElemnsTrans;
boost::atomic<int> SqlCode;

UINT __cdecl ProcDataGenerate()
{

	while (1)
	{
		if (!thWorkQueue.empty())
		{
			Blob info; 
			thWorkQueue.pop(info);

			std::map<std::string, std::string> assocMap;
			PrimitiveBlock baseBlock;
			if (info.has_raw())
			{
				std::vector<BYTE>buffer(info.raw().size());
				std::copy(info.raw().begin(), info.raw().end(), buffer.begin());
				baseBlock.ParseFromArray((void*)&buffer.front(), buffer.size());
			}
			else if (info.has_zlib_data())
			{
				std::vector<char> decompressed;
				io::filtering_istream is;
				is.push(io::zlib_decompressor());
				is.push(io::array_source(&info.zlib_data().front(), info.zlib_data().size()));
				io::copy(is,io::back_inserter(decompressed));
		
				baseBlock.ParseFromArray((void*)&decompressed.front(), decompressed.size());
		
			}
			while (!thSaveQueue.push(baseBlock))
			{
				Sleep(100);
			}
		}
		else
		{
			Sleep(10);
			if (feedComplete)
			{
				active = false;
				return 0;
			}
		}
	}
}

UINT __cdecl ProcDataSave(	sqlite3* dbCtx , sqlite3* dbWayCtx ,sqlite3* dbRelCtx  ,sqlite3_stmt* nodeStmt,	sqlite3_stmt* wayStmt ,	sqlite3_stmt* relStmt)
{
	while (1)
	{	
		while(thSaveQueue.empty())
		{
			Sleep(10);
			if (!active)
				return 0;
		}
		std::map<std::string, std::string> assocMap;
		std::vector<std::tuple<__int64, int, std::string>> relMembers;
		std::set<__int64> relIdsSet;
		bool firstTimeNode = true;
		bool firstTimeWay = true;
		bool firstTimeRel = true;
		PrimitiveBlock baseBlock;
		thSaveQueue.pop(baseBlock);
		if (baseBlock.IsInitialized())
		{
			std::vector<std::string> strings(baseBlock.stringtable().s_size());

			for (int idx = 0; idx < baseBlock.stringtable().s_size(); idx++)
			{
				strings[idx] = baseBlock.stringtable().s().Get(idx);
			}
			int granularity = baseBlock.granularity();
			long latOff = baseBlock.lat_offset();
			long lonOff = baseBlock.lon_offset();
			int date_gran = baseBlock.date_granularity();
			for (int prIdx=0; prIdx < baseBlock.primitivegroup().size(); prIdx++)
			{
				PrimitiveGroup group = baseBlock.primitivegroup().Get(prIdx);
				if (group.nodes_size() > 0)
				{
							
					char* errMsg;
					if (NodeElems % 100000 == 0 || firstTimeNode)
					{
						sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
						firstTimeNode = false;
					}
					for(int nodeIdx = 0; nodeIdx < group.nodes_size(); nodeIdx++)
					{
						assocMap.clear();
						Node nodeData = group.nodes().Get(nodeIdx);
						double nlat = parsegeo(nodeData.lat(),granularity, latOff);
						double nlon = parsegeo(nodeData.lon(), granularity, lonOff);
						for (int keyId = 0; keyId < nodeData.keys_size(); keyId++)
						{
							if (nodeData.vals_size() > keyId)
							{
								assocMap.insert(std::make_pair(strings[nodeData.keys().Get(keyId)], strings[nodeData.vals().Get(keyId)]));
							}

						}
						std::ostringstream ostrm;
						std::for_each( assocMap.begin(), assocMap.end(), [&](std::pair<std::string, std::string> itM ) {
							ostrm << itM.first;
							ostrm << "~";
							ostrm << itM.second;
							ostrm << "~";
						});

						sqlite3_bind_int64( nodeStmt, 1, nodeData.id());
						sqlite3_bind_double(nodeStmt, 2, nlat);
						sqlite3_bind_double(nodeStmt, 3, nlon);
						sqlite3_bind_blob(nodeStmt, 4, (void*)&ostrm.rdbuf()->str().front(), ostrm.rdbuf()->str().size(), SQLITE_TRANSIENT);
						SqlCode = sqlite3_step(nodeStmt);
						//SqlCode = sqlite3_clear_bindings(nodeStmt);
						SqlCode = sqlite3_reset(nodeStmt);
						NodeElems++;
						NodeElemTrans++;
					}
					
					if (NodeElems > 100000)
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
						NodeElems = 0;
					}
					//sqlite3_finalize(nodeStmt);
				}
				if (group.dense().IsInitialized())
				{
					DenseElems++;
					int keyValId = 0;
					__int64 idNode = 0;
					DenseNodes nodeInfo = group.dense();
					char* errMsg;
					if (NodeElems == 0)
					{
						sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
						firstTimeNode = false;
					}

					__int64 denseLat = 0;
					__int64 denseLon = 0;

					for(int nodeId = 0; nodeId<nodeInfo.id().size(); nodeId++)
					{
						assocMap.clear();
						denseLat += nodeInfo.lat(nodeId);
						denseLon += nodeInfo.lon(nodeId);
						double nlat = parsegeo(denseLat, granularity,latOff);
						double nlon = parsegeo(denseLon, granularity, lonOff);
						idNode += nodeInfo.id().Get(nodeId);
						if (nodeInfo.denseinfo().IsInitialized())
						{
						}
						while (nodeInfo.keys_vals().Get(keyValId) != 0)
						{
							assocMap.insert(std::make_pair(strings[nodeInfo.keys_vals().Get(keyValId)], strings[nodeInfo.keys_vals().Get(keyValId+1)]));
							keyValId+=2;
						}
						keyValId++;

						std::ostringstream ostrm;
						std::for_each( assocMap.begin(), assocMap.end(), [&](std::pair<std::string, std::string> itM ) {
							ostrm << itM.first;
							ostrm << "~";
							ostrm << itM.second;
							ostrm << "~";
						});
						sqlite3_bind_int64( nodeStmt, 1, idNode);
						sqlite3_bind_double(nodeStmt, 2, nlat);
						sqlite3_bind_double(nodeStmt, 3, nlon);
						if (ostrm.rdbuf()->str().size() > 0)
						{
							sqlite3_bind_blob(nodeStmt, 4, (void*)ostrm.rdbuf()->str().c_str(), ostrm.rdbuf()->str().size(), SQLITE_TRANSIENT);
						}
						else
						{
							sqlite3_bind_blob(nodeStmt, 4, "", 0, SQLITE_STATIC);
						}
						SqlCode = sqlite3_step(nodeStmt);
						if (SqlCode != SQLITE_DONE)
						{
							//NodeElems = -100;
						}
						//SqlCode = sqlite3_clear_bindings(nodeStmt);
						SqlCode = sqlite3_reset(nodeStmt);
						NodeElems++;
						NodeElemTrans++;
					}
					if (NodeElems > 100000 )
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						NodeElems = 0;
					}
				}
				if (group.ways().size() > 0)
				{
					char* errMsg;
					assocMap.clear();
					if (NodeElems > 0)
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						NodeElems = 0;
					}
					if (WayElems == 0)
					{
						sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
						firstTimeWay = false;
					}
					for (int wayId = 0; wayId < group.ways().size(); wayId++)
					{
						Way wayData = group.ways().Get(wayId);
						__int64 idWay = wayData.id();
						assocMap.clear();
						__int64 refID = 0;
						std::vector<__int64> intRefs;
						for (int refid = 0; refid < wayData.refs().size(); refid++)
						{
							refID+=wayData.refs(refid);
							intRefs.push_back(refID);
						}
						for (int keyId = 0; keyId < wayData.keys().size(); keyId++)
						{
							if (wayData.vals().size() > keyId)
							{
								assocMap.insert(std::make_pair(strings[wayData.keys().Get(keyId)], strings[wayData.vals().Get(keyId)]));
							}
						}
						int nodeID = 0;
						std::ostringstream ostrm;
						std::for_each( assocMap.begin(), assocMap.end(), [&](std::pair<std::string, std::string> itM ) {
							ostrm << itM.first;
							ostrm << "~";
							ostrm << itM.second;
							ostrm << "~";
						});
						
						std::for_each(intRefs.begin(), intRefs.end(), [&] (__int64 intData)
						{
							__int64 waayID = idWay << 16;
							waayID += nodeID;
							if (nodeID == 0)
							{
								sqlite3_bind_blob(wayStmt, 5, (void*)ostrm.rdbuf()->str().c_str(), ostrm.rdbuf()->str().size(), SQLITE_TRANSIENT);
							}
							else
							{
								sqlite3_bind_zeroblob(wayStmt, 5, 0);
							}
							sqlite3_bind_int64( wayStmt, 1, waayID);
							sqlite3_bind_int64( wayStmt, 2, idWay);
							sqlite3_bind_int64( wayStmt, 3, intRefs[nodeID]);
							sqlite3_bind_int64( wayStmt, 4, nodeID);
							sqlite3_bind_int( wayStmt, 6, assocMap.find(std::string("boundary")) != assocMap.end() ? 1:0);
							sqlite3_step(wayStmt);
							//sqlite3_clear_bindings(wayStmt);
							sqlite3_reset(wayStmt);
							nodeID++;
							WayElems++;
							WayElemsTrans++;
						});
					}
 				    if (WayElems > 100000)
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						WayElems = 0;
					}	
				}
				if (group.relations().size() > 0)
				{
					
					char* errMsg;
					int dbRet = SQLITE_OK;
					if (WayElems > 0)
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						WayElems = 0;
					}
					if (RelElemns  == 0 )
					{
						sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
						firstTimeWay = false;
					}
					for (int relidx = 0; relidx < group.relations().size(); relidx++)
					{
						assocMap.clear();
						Relation relData = group.relations().Get(relidx);
						__int64 relMemId = 0;
						relMembers.clear();
						relIdsSet.clear();
						for (int relMem = 0; relMem < relData.memids().size(); relMem++)
						{
							relMemId += relData.memids().Get(relMem);
							std::string roleId = strings[relData.roles_sid().Get(relMem)];
							int type = relData.types().Get(relMem);
							if (relIdsSet.find(relMemId) == relIdsSet.end())
							{
								relMembers.push_back(std::tuple<__int64, int, std::string>(relMemId, type,roleId));
								relIdsSet.insert(relMemId);
							}
						}
						for (int keyId = 0; keyId < relData.keys().size(); keyId++)
						{
							if (relData.vals().size() > keyId)
							{
								assocMap.insert(std::make_pair(strings[relData.keys().Get(keyId)], strings[relData.vals().Get(keyId)]));
							}
						}
						std::ostringstream ostrm;
						std::for_each( assocMap.begin(), assocMap.end(), [&](std::pair<std::string, std::string> itM ) {
							ostrm << itM.first;
							ostrm << "~";
							ostrm << itM.second;
							ostrm << "~";
						});
						int ord = 0;
						std::for_each(relMembers.begin(), relMembers.end(), [&] (std::tuple<__int64, int, std::string> member)
						{
							__int64 relID = relData.id() << 16;
							relID += ord;
							if (ord == 0)
							{
								sqlite3_bind_blob(relStmt, 7, (void*)ostrm.rdbuf()->str().c_str(), ostrm.rdbuf()->str().size(), SQLITE_TRANSIENT);
							}
							else
							{
								sqlite3_bind_zeroblob(relStmt, 7, 0);
							}
							sqlite3_bind_int64( relStmt, 1, relID);
							sqlite3_bind_int64( relStmt, 2, relData.id());
							sqlite3_bind_int64( relStmt, 3, std::get<0>(member));
							sqlite3_bind_int64( relStmt, 4, std::get<1>(member));
							sqlite3_bind_blob(relStmt, 5,  std::get<2>(member).c_str(), std::get<2>(member).size(), SQLITE_TRANSIENT);
							sqlite3_bind_int( relStmt, 6, ord);
							dbRet = sqlite3_step(relStmt);
							if (dbRet != SQLITE_OK || dbRet != SQLITE_ROW || dbRet != SQLITE_DONE)
							{
								//
							}
							//sqlite3_clear_bindings(relStmt);
							sqlite3_reset(relStmt);
							ord++;
							RelElemns++;
							RelElemnsTrans++;
						});
					}
					if (RelElemns > 100000)
					{
						sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
						RelElemns = 0;
					}		
				}
			}
		}
	}
}

void COBFGeneratorDlg::OnBnClickedMfcbutton1()
{
	UpdateData();
	CFile fileData(m_filePath, CFile::OpenFlags::typeBinary|CFile::modeRead);

	if (fileData.m_hFile != CFile::hFileNull)
	{
		CString strPath = fileData.GetFilePath();
		m_fileName = fileData.GetFileName();
		fileData.Close();
		m_filePath = strPath;
		active = true;
		feedComplete = false;
		CWinThread* thProc = AfxBeginThread(threadProc, (void*)this);

	}
}


LRESULT COBFGeneratorDlg::OnMyMessage(WPARAM wparm, LPARAM lParm)
{
	UNREFERENCED_PARAMETER(wparm);

	m_progress.SetWindowTextW((LPCWSTR)lParm);
	
	return 0;
}

int COBFGeneratorDlg::PrepareTempDB()
{
	char* errMsg;
	int dbRes;
	results.hParentWnd = m_hWnd;
	
	// first index all after initial insert(s)
	dbRes = sqlite3_exec(dbCtx, "create index IdIndex ON node (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	CString strMessage = L"Completed indexing nodes \r\n";
	wchar_t* msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	delete[] msgTxt;
	dbRes = sqlite3_exec(dbCtx, "create index IdWIndex ON ways (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	strMessage = L"Completed indexing ways 1 pass\r\n";
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	delete[] msgTxt;
	dbRes = sqlite3_exec(dbCtx, "create index IdWSearchIndex ON ways (wayid)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	strMessage = L"Completed indexing ways 2 pass\r\n";
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	delete[] msgTxt;
	dbRes = sqlite3_exec(dbCtx, "create index IdRIndex ON relations (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	strMessage = L"Completed indexing relations 1 pass\r\n";
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	delete[] msgTxt;
	dbRes = sqlite3_exec(dbCtx, "create index IdRelIndex ON relations (relid)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	strMessage = L"Completed indexing relations 2 pass\r\n";
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	delete[] msgTxt;
	dbRes = sqlite3_exec(dbCtx, "analyze", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	results.getStats(dbCtx);
	strMessage.Format(L"Iterating over for city");
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);


	std::wstring buff = L"Phase index cities\r\n";
	OutputDebugString(buff.c_str());
	results.iterateOverElements(PHASEINDEXCITY);


	strMessage.Format(L"Iterating over address relations");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
	buff = L"Phase index addres relations\r\n";
	OutputDebugString(buff.c_str());
	results.iterateOverElements(PHASEINDEXADDRREL);
	strMessage.Format(L"Iterating over all of rest");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
	buff = L"Phase index main iteration\r\n";
	OutputDebugString(buff.c_str());
	results.iterateOverElements(PHASEMAINITERATE);
	results.flush();
	strMessage.Format(L"Combining result");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
	results.iterateOverElements(PHASECOMBINE);
	
	strMessage.Format(L"Imaging result");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);

	results.imageResult();
	int pCount[4], pHigh[4];
	sqlite3_status(SQLITE_STATUS_MALLOC_COUNT, &pCount[0], &pHigh[0], FALSE);
	sqlite3_status(SQLITE_STATUS_MALLOC_SIZE, &pCount[1], &pHigh[1], FALSE);
	sqlite3_status(SQLITE_STATUS_MEMORY_USED, &pCount[2], &pHigh[2], FALSE);
	sqlite3_status(SQLITE_STATUS_PAGECACHE_SIZE, &pCount[3], &pHigh[3], FALSE);
	strMessage.Format(L"Pass completed\r\n Status: \r\n Malloc Count: %d, %d\r\n Malloc Size: %d, %d\r\n Malloc Used: %d, %d\r\n Malloc Page Cache: %d, %d\r\n", 
		pCount[0], pHigh[0], pCount[1], pHigh[1],pCount[2], pHigh[2],pCount[3], pHigh[3]);
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
	USES_CONVERSION;
	results.mapName = W2A(m_fileName.GetString());
	strMessage.Format(L"Saving binary result");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
	results.iterateOverElements(PHASESAVE);
	strMessage.Format(L"Work Completed");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);	return 0;
}

int COBFGeneratorDlg::ParseFile()
{
	
	boost::thread * producer = producers.create_thread(ProcDataGenerate);
	//producers.create_thread(ProcDataGenerate);
	boost::thread * saver = savers.create_thread(boost::bind(&ProcDataSave, dbCtx, dbCtx, dbCtx, nodeStmt, wayStmt, relStmt));
	//savers.create_thread(boost::bind(&ProcDataSave, dbCtx , nodeStmt, wayStmt, relStmt));


	USES_CONVERSION;
	
	std::ifstream strmData(m_filePath, std::ios::in|std::ios::binary);
	ar::binary_iarchive inpFile(strmData, ar::archive_flags::no_header|ar::archive_flags::no_codecvt|ar::archive_flags::no_tracking|ar::archive_flags::no_xml_tag_checking);

	int NodeDataElems = 0;

	BlockReader reader;
	bool stopOnError = true;
	std::map<std::string, std::string> assocMap;

	while (!strmData.eof() && stopOnError)
	{
		BlockHeader header = reader.ReadBlockHead(inpFile);

		if (header.IsInitialized() && header.type() == "OSMHeader")
		{
			Blob data = reader.ReadContents(header, inpFile);
			if (data.IsInitialized())
			{
				HeaderBlock block = reader.GetHeaderContents(data);
				if (block.has_bbox())
				{
					HeaderBBox box = block.bbox();
				}
				if (block.has_source())
				{
					std::string text = block.source();
				}
				if (block.has_writingprogram())
				{
					 std::string text = block.writingprogram();
				}
			}
		}
		if (header.IsInitialized() && header.type() == "OSMData")
		{
			
			std::vector<BYTE>buffer(header.datasize());

			inpFile.load_binary((char*)&buffer.front(), header.datasize());

			Blob blobData;
			blobData.ParseFromArray((void*)&buffer.front(), header.datasize());


			while (!thWorkQueue.push(blobData))
			{
				Sleep(100);
			}
				
				int lastBit = strmData.tellg();
				NodeDataElems++;
				if (NodeDataElems % 10 == 0)
				{
					CString strMessage;
					strMessage.Format(L"Processed %d nodes %d denses %d ways %d relations at position %d", NodeElems, DenseElems, WayElems, RelElemns, lastBit);
					wchar_t* msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
					wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
					::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
					//UpdateData();
				}
			}
		char peeked = strmData.peek();
	}

	while (!thWorkQueue.empty())
	{
		Sleep(100);
	}
	feedComplete = true;
	producers.join_all();
	savers.join_all();

	if (NodeElems > 0 || WayElems > 0 || RelElemns > 0)
	{
		char* errMsg;
		sqlite3_exec(dbCtx, "COMMIT TRANSACTION", NULL, NULL, &errMsg);
	}

	sqlite3_finalize(nodeStmt);
	sqlite3_finalize(wayStmt);
	sqlite3_finalize(relStmt);
	//sqlite3_close_v2(dbCtx);
	//sqlite3_close_v2(dbWayCtx);
	//sqlite3_close_v2(dbCtx);
	CString strMessage;
	strMessage.Format(L"Completed %d nodes %d denses %d ways %d relations\r\n", NodeElemTrans, DenseElems, WayElemsTrans, RelElemnsTrans);
	wchar_t* msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	OutputDebugString(msgTxt);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);


	return 0;
}




void COBFGeneratorDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	SkGraphics::Term();
	results.close();
	if (mapData)
	{
		mapData.reset();
	}
	CDialogEx::OnClose();
}


int COBFGeneratorDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	SkGraphics::Init();
	// TODO:  Add your specialized creation code here

	return 0;
}


void COBFGeneratorDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	#ifdef _DEBUG_VLD
		VLDReportLeaks();
	#endif

	std::shared_ptr<MapStyleInfo> info(new MapStyleInfo());
	info->loadRenderStyles(nullptr);

	std::wstring dumpRules = L"Dumping rules: \r\n";
	OutputDebugString(dumpRules.c_str());
	/*OutputDebugString(L"point rules:\n");
	info->dump(rulesetType::point );
	OutputDebugString(L"line rules:\n");
	info->dump(rulesetType::line );
	OutputDebugString(L"polygon rules:\n");
	info->dump(rulesetType::polygon);
	OutputDebugString(L"text rules:\n");
	info->dump(rulesetType::text);
	OutputDebugString(L"order rules:\n");
	info->dump(rulesetType::order);*/

	if (mapData)
	{
		mapData.reset();
	}
}


void COBFGeneratorDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	results.close();
	if (mapData)
	{
		mapData.reset();
	}
	CDialogEx::OnCancel();
}


void COBFGeneratorDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	results.close();
	
	if (mapData)
	{
		mapData.reset();
	}
	CDialogEx::OnOK();
}


void COBFGeneratorDlg::OnBnClickedMfcbutton2()
{
	UpdateData();
	CFile fileData;

	if (!mapData)
	{
		fileData.Open(m_fileReadPath, CFile::OpenFlags::typeBinary|CFile::modeRead);
	}

	std::string newPath;
	if (fileData.m_hFile != CFile::hFileNull || mapData)
	{
		if (!mapData)
		{
			CString strPath = fileData.GetFilePath();
			m_fileName = fileData.GetFileName();
			fileData.Close();
			m_fileReadPath = strPath;
			std::wstring wstrPath = strPath.GetBuffer();
			std::wstring_convert<std::codecvt_utf8<wchar_t>> coder;
			std::string cvt = coder.to_bytes(wstrPath);
		
			boost::filesystem::path pather(cvt);
			pather.replace_extension("png");
			newPath = pather.string();
			mapData.swap(std::shared_ptr<MapRasterizerProvider>(new  MapRasterizerProvider()));
			mapData->obtainMaps(cvt.c_str());
		}
		else
		{
			std::wstring wstrPath = m_fileReadPath.GetBuffer();
			std::wstring_convert<std::codecvt_utf8<wchar_t>> coder;
			std::string cvt = coder.to_bytes(wstrPath);
			mapData->obtainMaps(cvt.c_str());
			boost::filesystem::path pather(cvt);
			pather.replace_extension("png");
			newPath = pather.string();
		}
		
		boxI bgData = mapData->getWholeBox();

		int zoomVal = zoomLevel;
		int tileSide = 256;

		auto top = MapUtils::get31TileNumberY(MapUtils::get31LatitudeY(bgData.min_corner().get<1>()));
		auto left = MapUtils::get31TileNumberX(MapUtils::get31LongitudeX(bgData.min_corner().get<0>()));
		auto bottom = MapUtils::get31TileNumberY(MapUtils::get31LatitudeY(bgData.max_corner().get<1>()));
		auto right = MapUtils::get31TileNumberY(MapUtils::get31LongitudeX(bgData.max_corner().get<0>()));

		auto centerTileX = MapUtils::getTileNumberX(zoomVal, m_XCoord);
		auto centerTileY = MapUtils::getTileNumberY(zoomVal, m_YCoord);

		auto rightT = MapUtils::getTileNumberX(zoomVal, MapUtils::get31LongitudeX(bgData.min_corner().get<0>()));
		auto topT = MapUtils::getTileNumberY(zoomVal, MapUtils::get31LatitudeY(bgData.min_corner().get<1>()));
		auto leftT = MapUtils::getTileNumberX(zoomVal, MapUtils::get31LongitudeX(bgData.max_corner().get<0>()));
		auto bottomT = MapUtils::getTileNumberY(zoomVal, MapUtils::get31LatitudeY(bgData.max_corner().get<1>()));

		auto tileWidth = leftT - rightT;
		auto tileHeight = bottomT - topT;



		std::wstringstream strmText;
		strmText.precision(0);
		if (centerTileX > leftT || centerTileX < rightT)
		{
			if (centerTileY > bottomT || centerTileY < topT)
			{
			strmText << L"Selected point outside of loadaed map";
			wchar_t* msgTxt = new wchar_t[strmText.str().size()+1];
			ZeroMemory(msgTxt, (strmText.str().size()+1)*sizeof(wchar_t));
			wcsncpy_s(msgTxt,strmText.str().size()+1 , strmText.str().data(),strmText.str().size());
			::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
			return;
			}
		}
		strmText << std::fixed << L"Tiles X:=" << tileWidth << L" Y:=" << tileHeight << L" Sum:" << tileWidth*tileHeight << L"Size:" << tileWidth*1024 << L"x" <<tileHeight*1024;
		wchar_t* msgTxt = new wchar_t[strmText.str().size()+1];
		ZeroMemory(msgTxt, (strmText.str().size()+1)*sizeof(wchar_t));
		wcsncpy_s(msgTxt,strmText.str().size()+1 , strmText.str().data(),strmText.str().size());
		::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);
		if (tileHeight > 3 || tileWidth > 3)
		{
			
			bgData.min_corner().set<0>( MapUtils::get31TileNumberX( MapUtils::getLongitudeFromTile(zoomVal, centerTileX)));
			bgData.min_corner().set<1>( MapUtils::get31TileNumberY( MapUtils::getLatitudeFromTile(zoomVal, centerTileY)));
			bgData.max_corner().set<0>( MapUtils::get31TileNumberX( MapUtils::getLongitudeFromTile(zoomVal, centerTileX+1)));
			bgData.max_corner().set<1>( MapUtils::get31TileNumberY( MapUtils::getLatitudeFromTile(zoomVal, centerTileY+1)));
			auto mapDataObjets = mapData->obtainMapData(bgData, zoomVal);
			if (mapDataObjets.size() > 0)
			{
				std::shared_ptr<MapRasterizer> render(new MapRasterizer(*mapData.get()));
				render->createContextData(bgData, zoomVal);
				//SkAutoTUnref<SkBitmap> bitSrc(nullptr);
				//bitSrc->setConfig(SkBitmap::kARGB_8888_Config, 1024, 1024);
				//bitSrc->allocPixels();
				{
					SkAutoTUnref<SkBitmapDevice> device(new SkBitmapDevice(SkBitmap::kARGB_8888_Config, 1024, 1024));
					SkAutoTUnref<SkCanvas> painter(new SkCanvas(device));
					

					bool painted = render->DrawMap(*painter);
					if (painted)
					{
						const SkBitmap& bitmapData = device->accessBitmap(false);
						auto DataB = bitmapData.config();
						renderer->saveSkBitmapToResource(0, bitmapData, 0,0);
						SkImage::Info inf;
						inf.fAlphaType = SkImage::kPremul_AlphaType;
						inf.fColorType = SkImage::kPMColor_ColorType;
						inf.fWidth = bitmapData.width();
						inf.fHeight = bitmapData.height();
						SkAutoTUnref<SkSurface> SrcData(SkSurface::NewRasterDirect(inf, bitmapData.getPixels(), bitmapData.rowBytes()));
						SkAutoTUnref<SkImage> imageSrc(SrcData->newImageSnapshot());
						SkAutoDataUnref data(imageSrc->encode());
						SkFILEWStream stream(newPath.c_str());
						stream.write(data->data(), data->size());
						
					}
				}
				{
					//SkAutoTUnref<SkBitmapDevice> device(new SkBitmapDevice(SkBitmap::kARGB_8888_Config, 1024, 1024));
					//SkAutoTUnref<SkCanvas> painter(new SkCanvas(device));
					std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>> symbolData;
					bool painted = render->GetSymbolData(symbolData);
					if (painted)
					{
						/*std::vector<const std::shared_ptr<const SkBitmap>> symbolGraphs;
						for(auto symbolData : symbols)
						{
							for (auto symbolValue : symbolData->_symbols)
							{

								symbolGraphs.push_back(symbolValue->bitmap);
							}
						}*/
						
						

						//const SkBitmap& bitmapData = device->accessBitmap(false);
						//auto DataB = bitmapData.config();
					
						//SkImage::Info inf;
						//inf.fAlphaType = SkImage::kPremul_AlphaType;
						//inf.fColorType = SkImage::kPMColor_ColorType;
						//inf.fWidth = bitmapData.width();
						//inf.fHeight = bitmapData.height();
						//SkAutoTUnref<SkSurface> SrcData(SkSurface::NewRasterDirect(inf, bitmapData.getPixels(), bitmapData.rowBytes()));
						//SkAutoTUnref<SkImage> imageSrc(SrcData->newImageSnapshot());
						//SkAutoDataUnref data(imageSrc->encode());
						//SkFILEWStream stream(newPath.c_str());
						//stream.write(data->data(), data->size());
						//renderer->saveSkBitmapToResource(0, bitmapData, 0,0);

						renderer->updateTexture(symbolData, render->getCurrentContext());
					}
				}
			}

			//MapUtils::convert31YToMeters
			//float stepLon = (MapUtils::get31LongitudeX(bgData.max_corner().get<0>()) - MapUtils::get31LongitudeX(bgData.min_corner().get<0>()))/idx;
			//float stepLat = (MapUtils::get31LatitudeY(bgData.max_corner().get<1>()) - MapUtils::get31LatitudeY(bgData.min_corner().get<1>()))/idy;

			/*auto idx = tileWidth;
			auto idy = tileHeight;
			float minx = MapUtils::get31LongitudeX(bgData.min_corner().get<0>());
			float miny = MapUtils::get31LatitudeY(bgData.min_corner().get<1>());
			float stepLon = (MapUtils::get31LongitudeX(bgData.max_corner().get<0>()) - MapUtils::get31LongitudeX(bgData.min_corner().get<0>()))/idx;
			float stepLat = (MapUtils::get31LatitudeY(bgData.max_corner().get<1>()) - MapUtils::get31LatitudeY(bgData.min_corner().get<1>()))/idy;
			for (int intx = 0; intx < idx; intx++)
			{
				for(int inty = 0; inty < idy; inty++)
				{
					bgData.min_corner().set<0>( MapUtils::get31TileNumberX(minx+(intx*stepLon)));
					bgData.min_corner().set<1>( MapUtils::get31TileNumberY(miny+(inty*stepLat)));
					bgData.max_corner().set<0>( MapUtils::get31TileNumberX(minx+((intx+1)*stepLon)));
					bgData.max_corner().set<1>( MapUtils::get31TileNumberY(miny+((inty+1)*stepLat)));
					auto mapDataObjets = mapData->obtainMapData(bgData, zoomVal);
					if (mapDataObjets.size() > 0)
					{
						std::shared_ptr<MapRasterizer> render(new MapRasterizer(*mapData.get()));
						render->createContextData(bgData, zoomVal);
						std::stringstream strmText;
						strmText << "_" << intx << "_" << inty;
						boost::filesystem::path pp(newPath);
						std::string newName =  pp.leaf().filename().string() + strmText.str().c_str();
						newName +=  + ".png";
						pp.remove_leaf() /= newName;
						auto result = pp.string();
						render->DrawMap(result);
					}
				}
			}*/
		}
		else
		{
			auto mapDataObjets = mapData->obtainMapData(bgData, zoomVal);
			std::shared_ptr<MapRasterizer> render(new MapRasterizer(*mapData.get()));
			render->createContextData(bgData, zoomVal);
			render->DrawMap(newPath);
		}

		if ((tileHeight > 3 && tileWidth > 3) && (tileHeight < 10 && tileWidth < 10))
		{

		}
	}
}

template<typename T> void read_one(std::ifstream& inp, T& sizeRead)
{
	uint8 c;
	while(1)
	{
		inp.read(reinterpret_cast< char * >(&c), 1);
		sizeRead = (sizeRead << 7) | (c & 0x7f);
		if (!(c & 0x80))
			break;
	}

}

template<typename T> int read_one_buff(const unsigned char* inp, T& sizeRead, size_t limit)
{
	uint8 c;
	uint32_t idx = 0;
	while(1 && limit > idx)
	{
		c = inp[idx];
		sizeRead = (sizeRead << 7) | (c & 0x7f);
		if (!(c & 0x80))
			break;
		idx++;
	}
	return idx;
}

void COBFGeneratorDlg::OnBnClickedMfcbutton3()
{
	UpdateData();
	std::string outFileDec;
	std::string inFileDec;
	std::wstring wstrPath = m_DecompressFile;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> coder;
	outFileDec = coder.to_bytes(wstrPath);
	inFileDec = outFileDec;
	outFileDec += ".outSVG";

	std::ofstream strmOut(outFileDec, std::ios::out|std::ios::binary);
	std::ifstream strmData(m_DecompressFile, std::ios::in|std::ios::binary);


	

	////ar::binary_iarchive inpFile(strmData, ar::archive_flags::no_header|ar::archive_flags::no_codecvt|ar::archive_flags::no_tracking|ar::archive_flags::no_xml_tag_checking);
	//bool hasDELTA = true;
	//bool skipHeader = false;
	//uint8 version = 0;
	//uint32_t cumulative = 0;
	//uint32_t deltaheader = 0;
	//uint32_t currpos = 0;
	//while (hasDELTA && strmData.peek() != std::char_traits<char>::eof())
	//{
	//	
	//	byte buff[7];
	//	if (!skipHeader)
	//	{
	//		deltaheader++;
	//	std::string textLine;
	//	
	//	char c='\0';
	//	while (c != 0x0A)
	//	{
	//		strmData.read(&c, 1);
	//		textLine.push_back(c);
	//	}
	//	
	//	if (textLine.substr(0, 5) != "DELTA")
	//	{
	//		hasDELTA = false;
	//	}
	//	strmOut << textLine << std::endl;
	//	
	//		strmData.read(reinterpret_cast< char * >(buff), 5);
	//		if (buff[0] != 'S' || buff[2] != 'N')
	//		{
	//			hasDELTA = false;
	//			break;
	//		}
	//		version = buff[3];
	//		strmOut << buff << std::endl;
	//		if (buff[4] != '\0' && buff[4] =='E')
	//		{
	//			// empty delta
	//			version = 0;
	//			strmData.read(reinterpret_cast< char * >(buff), 6);
	//			if (buff[0] != 'N' || buff[4] != 'P')
	//			{
	//				hasDELTA = false;
	//				break;
	//			}
	//		}
	//	}
	//	
	//	if (hasDELTA)
	//	{
	//		if (version != 1)
	//		{
	//			// no compress
	//		}
	//		else
	//		{
	//			

	//			// compressed version
	//			uint64_t svoffset = 0;
	//			read_one(strmData, svoffset);

	//			uint64_t svLen = 0;
	//			read_one(strmData, svLen);
	//			uint64_t tvLen = 0;
	//			read_one(strmData, tvLen);
	//			uint64_t nvlen = 0;
	//			read_one(strmData, nvlen);
	//			if (nvlen > 50000000)
	//			{
	//				currpos = strmData.tellg();
	//			}
	//			uint64_t inlen = 0;
	//			read_one(strmData, inlen);

	//			uint64_t newLen = nvlen + inlen;
	//			byte* buffComp = new byte[newLen];
	//			if (deltaheader ==938)
	//			{
	//				currpos = strmData.tellg();
	//			}
	//			strmData.read(reinterpret_cast< char * >(buffComp), newLen);

	//			unsigned char* dataBuff = buffComp+inlen;
	//			

	//			uint64_t sizeDataDec = 0;
	//			int moved = read_one_buff(dataBuff, sizeDataDec, 10);
	//			moved++;
	//			std::string strBuff(reinterpret_cast< char const* >(dataBuff+moved),nvlen) ;

	//			if (sizeDataDec > nvlen)
	//			{
	//				char flagCheck = strBuff.front();
	//				if (flagCheck != 0x78)
	//				{
	//					strmOut.write(&strBuff.front(), sizeDataDec);	
	//					cumulative+=sizeDataDec;
	//				}
	//				else
	//				{
	//					std::vector<char> decompressed;
	//					io::filtering_istream is;
	//					is.push(io::zlib_decompressor());
	//					is.push(io::array_source(&strBuff.front(), nvlen));
	//					io::copy(is,io::back_inserter(decompressed));
	//					strmOut.write(&decompressed.front(), decompressed.size());
	//					cumulative+=decompressed.size();
	//				}
	//			
	//			}
	//			else
	//			{
	//				strmOut.write(&strBuff.front(), nvlen);	
	//				cumulative+=nvlen;
	//			}
	//			char peeker = strmData.peek();
	//			if (peeker == 'E')
	//			{
	//				strmData.read(reinterpret_cast< char * >(buff), 7);
	//				if (buff[0] != 'E' || buff[5] != 'P')
	//				{
	//					hasDELTA = false;
	//					break;
	//				}
	//				skipHeader = false;
	//			}
	//			else if (peeker == 'P')
	//			{
	//				hasDELTA = false;
	//					break;
	//			}
	//			else
	//			{
	//				if (peeker != '\0')
	//				{
	//					currpos = strmData.tellg();
	//				}
	//				skipHeader = true;
	//				strmData.read(reinterpret_cast< char * >(buff), 1);
	//			}
	//			if (cumulative > 60000000)
	//			{
	//				hasDELTA = false;
	//					break;
	//			}
	//		}
	//	}
	//}
}


//HRESULT COBFGeneratorDlg::get_accChildCount(long *pcountChildren)
//{
//	// TODO: Add your specialized code here and/or call the base class
//
//	return CDialogEx::get_accChildCount(pcountChildren);
//}


void COBFGeneratorDlg::OnTimer(UINT_PTR nIDEvent)
{

	renderer->renderScene();


	CDialogEx::OnTimer(nIDEvent);
}


void COBFGeneratorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	CWnd* pWnd = GetDlgItem(IDC_PLACEDX);
	RECT rectWnd;
	RECT dlgRect;
	GetWindowRect(&dlgRect);

	pWnd->GetWindowRect(&rectWnd);
	
	ScreenToClient(&rectWnd);
	rectWnd.bottom = cy - 10;
	rectWnd.right = cx - 10;

	pWnd->MoveWindow(&rectWnd);

	renderer->ResizeWindow(pWnd->GetSafeHwnd());
	// TODO: Add your message handler code here
}
