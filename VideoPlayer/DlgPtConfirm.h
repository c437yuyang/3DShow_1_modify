#pragma once


// CDlgPtConfirm 对话框

class CDlgPtConfirm : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgPtConfirm)

public:
	CDlgPtConfirm(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CDlgPtConfirm();

// 对话框数据
	enum { IDD = IDD_PT_CONFIRM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
