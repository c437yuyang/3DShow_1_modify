// DlgSnapProg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "DlgSnapProg.h"
#include "afxdialogex.h"


// CDlgSnapProg �Ի���

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


// CDlgSnapProg ��Ϣ�������


void CDlgSnapProg::OnNMCustomdrawSnapProg(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
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

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	//�ô��ھ���
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
	// �쳣:  OCX ����ҳӦ���� FALSE
}


void CDlgSnapProg::OnNcDestroy()
{
	CDialogEx::OnNcDestroy();
	//GetParent()->EnableWindow(TRUE);
	delete this;
	// TODO:  �ڴ˴������Ϣ����������
}


void CDlgSnapProg::OnOK()
{
	// TODO:  �ڴ����ר�ô����/����û���

	CDialogEx::OnOK();
}
