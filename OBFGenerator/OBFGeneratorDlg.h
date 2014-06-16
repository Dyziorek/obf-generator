
// OBFGeneratorDlg.h : header file
//

#pragma once
#include "afxeditbrowsectrl.h"
#include "afxwin.h"

#define WM_MYMESSAGE (WM_USER + 100)


// COBFGeneratorDlg dialog
class COBFGeneratorDlg : public CDialogEx
{
// Construction
public:
	COBFGeneratorDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_OBFGENERATOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	static int shell_callback(void *pArg, int nArg, char **azArg, char **azCol);

// Implementation
protected:
	HICON m_hIcon;
	sqlite3* dbCtx;
	sqlite3* dbRelCtx;
	sqlite3* dbWayCtx;

	sqlite3_stmt* nodeStmt;
	sqlite3_stmt* wayStmt;
	sqlite3_stmt* relStmt;

	sqlite3* dbMapCtx;
	sqlite3_stmt* mapStmt;
	sqlite3_stmt* lowStmt;
	sqlite3* dbRouteCtx;
	sqlite3_stmt* routeStmt;
	sqlite3_stmt* baseRouteStmt;
	sqlite3* dbAddrCtx;
	sqlite3_stmt* streetStmt;
	sqlite3_stmt* streetNodeStmt;
	sqlite3_stmt* buildStmt;
	sqlite3_stmt* searchStrStmt;
	sqlite3_stmt* searchStrNoCityStmt;
	sqlite3_stmt* updateCityStmt;
	sqlite3_stmt* searchBuildStmt;
	sqlite3_stmt* removeBuildStmt;
	sqlite3_stmt* searchStrNodeStmt;
	sqlite3_stmt* cityStmt;

	// selectors from d
	sqlite3_stmt* selNodeStmt;
	sqlite3_stmt* selWayStmt;
	sqlite3_stmt* selRelStmt;
	sqlite3_stmt* itNodeStmt;
	sqlite3_stmt* itWayStmt;
	sqlite3_stmt* itRelStmt;
	sqlite3_stmt* itWayBoundStmt;


	sqlite3* dbPoiCtx;

	sqlite3* dbTransCtx;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedMfcbutton1();
	afx_msg LRESULT OnMyMessage(WPARAM wParam, LPARAM lParam);
	CMFCEditBrowseCtrl m_Browse;
	CString m_filePath;
	CString m_fileName;
	int ParseFile();
	int PrepareTempDB();
	CStatic m_progress;

	OBFResultDB results;
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBnClickedButton1();
	virtual void OnCancel();
	virtual void OnOK();
};
