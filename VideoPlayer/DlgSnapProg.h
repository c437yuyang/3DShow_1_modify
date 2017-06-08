#pragma once
#include "afxcmn.h"
#include <iostream>
using namespace std;

// CDlgSnapProg 对话框

class CDlgSnapProg : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSnapProg)
public:
	CDlgSnapProg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgSnapProg();
	friend class CVedioPlayer;
// 对话框数据
	enum { IDD = IDD_SNAP_PROG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdrawSnapProg(NMHDR *pNMHDR, LRESULT *pResult);
	CProgressCtrl m_SnapProg;
	virtual BOOL OnInitDialog();
	afx_msg void OnNcDestroy();
	virtual void OnOK();
};
