#pragma once


// CDlgProgress 对话框

class CDlgProgress : public CDialog
{
	DECLARE_DYNAMIC(CDlgProgress)

public:
	CDlgProgress(CWnd* pParent = NULL);   // 标准构造函数
	int SetPos(int nPos);
	virtual ~CDlgProgress();
	int m_nCurPos;
	int m_nTime;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_PROGRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
