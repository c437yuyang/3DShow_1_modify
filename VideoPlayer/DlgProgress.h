#pragma once


// CDlgProgress �Ի���

class CDlgProgress : public CDialog
{
	DECLARE_DYNAMIC(CDlgProgress)

public:
	CDlgProgress(CWnd* pParent = NULL);   // ��׼���캯��
	int SetPos(int nPos);
	virtual ~CDlgProgress();
	int m_nCurPos;
	int m_nTime;
// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLG_PROGRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
