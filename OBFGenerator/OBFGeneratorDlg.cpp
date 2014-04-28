
// OBFGeneratorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EntityBase.h"
#include "EntityNode.h"
#include "OBFResultDB.h"
#include "OBFGeneratorDlg.h"
#include "SkGraphics.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COBFGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCEDITBROWSE1, m_Browse);
	DDX_Text(pDX, IDC_MFCEDITBROWSE1, m_filePath);
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
}

BEGIN_MESSAGE_MAP(COBFGeneratorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MFCBUTTON1, &COBFGeneratorDlg::OnBnClickedMfcbutton1)
	ON_MESSAGE(WM_MYMESSAGE,  &COBFGeneratorDlg::OnMyMessage)
	ON_WM_CLOSE()
	ON_WM_CREATE()
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
	dbRes = sqlite3_exec(dbCtx, "create index IdIndex ON node (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "drop table if exists ways" , &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create table ways (id bigint primary key, wayid bigint, node bigint, ord smallint, tags blob, boundary smallint)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create index IdWIndex ON ways (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create index IdWSearchIndex ON ways (wayid)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "drop table if exists relations" , &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create table relations (id bigint primary key, relid bigint,  member bigint, type smallint, role varchar(1024), ord smallint, tags blob)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create index IdRIndex ON relations (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
	dbRes = sqlite3_exec(dbCtx, "create index IdRelIndex ON relations (relid)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 

	dbRes = sqlite3_prepare_v2(dbCtx, "insert into node values (?1, ?2, ?3, ?4)", strlen("insert into node values (?1, ?2, ?3, ?4)"), &nodeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbCtx, "insert into ways values (?1, ?2, ?3, ?4, ?5, ?6)", strlen("insert into ways values (?1, ?2, ?3, ?4, ?5, ?6)"), &wayStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbCtx, "insert into relations values (?1, ?2, ?3, ?4, ?5, ?6, ?7)", strlen("insert into relations values (?1, ?2, ?3, ?4, ?5, ?6, ?7)"), &relStmt, NULL);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA cache_size=100000", NULL, NULL, &errMsg);
	/*dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);*/
	//sqlite3_exec(dbCtx, "create index IdWIndex ON ways (id)", &COBFGeneratorDlg::shell_callback,this,&errMsg); 
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
						SqlCode = sqlite3_clear_bindings(nodeStmt);
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
						SqlCode = sqlite3_clear_bindings(nodeStmt);
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
							sqlite3_clear_bindings(wayStmt);
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
						for (int relMem = 0; relMem < relData.memids().size(); relMem++)
						{
							relMemId += relData.memids().Get(relMem);
							std::string roleId = strings[relData.roles_sid().Get(relMem)];
							int type = relData.types().Get(relMem);
							relMembers.push_back(std::tuple<__int64, int, std::string>(relMemId, type,roleId));
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
							sqlite3_clear_bindings(relStmt);
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

	CString strMessage;
	strMessage.Format(L"Iterating over for city");
	wchar_t* msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);

	results.PrepareDB(dbCtx);

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
	strMessage.Format(L"Pass completed");
	delete[] msgTxt;
	msgTxt = new wchar_t[strMessage.GetAllocLength()+2];
	wcscpy_s(msgTxt,strMessage.GetAllocLength()+2 , (LPCWSTR)strMessage);
	::PostMessage(m_hWnd, WM_MYMESSAGE, NULL, (LPARAM)msgTxt);

	return 0;
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
