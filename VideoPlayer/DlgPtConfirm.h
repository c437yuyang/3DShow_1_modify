#pragma once


// CDlgPtConfirm �Ի���

class CDlgPtConfirm : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPtConfirm)

public:
	CDlgPtConfirm(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CDlgPtConfirm();

// �Ի�������
	enum { IDD = IDD_PT_CONFIRM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};
