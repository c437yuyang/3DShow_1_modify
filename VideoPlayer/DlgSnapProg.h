#pragma once
#include "afxcmn.h"
#include <iostream>
using namespace std;

// CDlgSnapProg �Ի���

class CDlgSnapProg : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSnapProg)
public:
	CDlgSnapProg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDlgSnapProg();
	friend class CVedioPlayer;
// �Ի�������
	enum { IDD = IDD_SNAP_PROG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdrawSnapProg(NMHDR *pNMHDR, LRESULT *pResult);
	CProgressCtrl m_SnapProg;
	virtual BOOL OnInitDialog();
	afx_msg void OnNcDestroy();
	virtual void OnOK();
};
