// DlgSnapProg.cpp : 实现文件
//

#include "stdafx.h"
#include "DlgSnapProg.h"
#include "afxdialogex.h"


// CDlgSnapProg 对话框

IMPLEMENT_DYNAMIC(CDlgSnapProg, CDialogEx)

CDlgSnapProg::CDlgSnapProg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgSnapProg::IDD, pParent)
{

}

CDlgSnapProg::~CDlgSnapProg()
{
}

void CDlgSnapProg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SNAP_PROG, m_SnapProg);
}


BEGIN_MESSAGE_MAP(CDlgSnapProg, CDialogEx)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SNAP_PROG, &CDlgSnapProg::OnNMCustomdrawSnapProg)
	ON_WM_NCDESTROY()
END_MESSAGE_MAP()


// CDlgSnapProg 消息处理程序


void CDlgSnapProg::OnNMCustomdrawSnapProg(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	//cout << m_SnapProg.GetPos() << endl;
	if (m_SnapProg.GetPos() == 1000)
	{
		PostQuitMessage(NULL);
	}
	*pResult = 0;
}


BOOL CDlgSnapProg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	//让窗口居中
	CRect rectParent, rectChild;
	GetParent()->GetWindowRect(&rectParent);
	GetWindowRect(&rectChild);
	UINT nPHeight = rectParent.bottom - rectParent.top;
	UINT nPWidth = rectParent.right - rectParent.left;
	UINT nCtop = nPHeight / 2 - rectChild.Height()/2;
	UINT nCleft = nPWidth / 2 - rectChild.Width() / 2;
	SetWindowPos(NULL, nCleft,nCtop, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	SkinH_Attach();
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


void CDlgSnapProg::OnNcDestroy()
{
	CDialogEx::OnNcDestroy();
	//GetParent()->EnableWindow(TRUE);
	delete this;
	// TODO:  在此处添加消息处理程序代码
}


void CDlgSnapProg::OnOK()
{
	// TODO:  在此添加专用代码和/或调用基类

	CDialogEx::OnOK();
}
