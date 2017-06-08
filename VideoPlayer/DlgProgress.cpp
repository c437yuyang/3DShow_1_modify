// DlgProgress.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "VideoPlayerDlg.h"
#include "DlgProgress.h"
#include "afxdialogex.h"


// CDlgProgress �Ի���

IMPLEMENT_DYNAMIC(CDlgProgress, CDialog)

CDlgProgress::CDlgProgress(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_DLG_PROGRESS, pParent)
{

}



CDlgProgress::~CDlgProgress()
{
}

void CDlgProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgProgress, CDialog)
END_MESSAGE_MAP()


// CDlgProgress ��Ϣ�������
int CDlgProgress::SetPos(int nPos)
{
	m_nCurPos = nPos;
	return ((CProgressCtrl*)(GetDlgItem(IDC_PROGRESS1)))->SetPos(nPos);
}