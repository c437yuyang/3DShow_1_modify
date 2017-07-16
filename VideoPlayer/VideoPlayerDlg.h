
// VideoPlayerDlg.h : ͷ�ļ�
//

#pragma once
#include <opencv2/opencv.hpp>
#include "CvvImage.h"
#include "afxwin.h"
#include "FrameManager.h"
#include "DlgProgress.h"
#include "RockExtractor.h"


/**
*  �ؼ��洰�ڱ仯�Ŀ��Ƶ�Ԫ
**/
typedef struct tagCONTROL
{
	CWnd*	m_pWnd;				//ָ��ؼ���ָ��
	CRect	m_rectWnd;			//�ؼ���ռ����
	int		m_nMoveXPercent,	//�ؼ���x���ƶ��İٷֱ�
			m_nMoveYPercent,	//�ؼ���y���ƶ��İٷֱ�
			m_nZoomXPercent,	//�ؼ���x�����ŵİٷֱ�
			m_nZoomYPercent;	//�ؼ���y�����ŵİٷֱ�
}ControlInfo, *lpControlInfo;

#define WM_USER_CLOSE_DIALOG  WM_USER+200
typedef		CArray<lpControlInfo>	CTRLLIST;	//�ؼ���������Ϣ��

// CVideoPlayerDlg �Ի���
class CVideoPlayerDlg : public CDialogEx
{
// ����
public:
	CVideoPlayerDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CVideoPlayerDlg();   //�ֶ���ӵ���������

	void	SetMinSize(int nWidth, int nHeight);
	void	FreeCtrlInfoList();
	void	MakeCtrlFit(CWnd* pWnd, int nMoveXPercent = 0, int nMoveYPercent = 0, int nZoomXPercent = 0, int nZoomYPercent = 0);
	void	CancelCtrlFit(HWND hWnd);

// �Ի�������
	enum { IDD = IDD_VIDEOPLAYER_DIALOG };


	int			m_nWinWidth,		//���ڵĿ��
				m_nWinHeight;		//���ڵĸ߶�
	POINT		m_ptMinTrackSize;	//���ڵ���С��С
	CTRLLIST	m_listCtrlInfo;		//�ؼ�������Ϣ������

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStartvideo();
	afx_msg void OnBnClickedSuspendvideo();
	afx_msg void OnBnClickedStopvideo();

	afx_msg void OnBnClickedSpeedupbutton();
	afx_msg void OnBnClickedSpeeddownbutton();
	afx_msg void OnBnClickedBackwardinverse();
private:
	double m_dwidth;
	double m_dheight;
	bool m_bSizeChanged;
	int m_nMouseCheck;
public:

	afx_msg void OnSize(UINT nType, int cx, int cy);	
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	/*afx_msg void OnBnClickedUpview();*/
	afx_msg void OnClickedSnap();
	afx_msg void OnBnClickedProcess();
	
	IplImage* m_upViewFrame;
	CFrameManager m_fmgr;
	void ShowToPicCtl(IplImage * img, UINT ID);
	void OnDisplay();
	double GetWidth() const { return m_dwidth; }
	double GetHeight() const { return m_dheight; }
	CButton StopButton;

	afx_msg LRESULT CloseDialog(WPARAM wParam, LPARAM lParam);
	CDlgProgress m_dlgProgress;
	float m_fProgress;
	static void ThradProCtlCreate(LPVOID lpParam); //���𴴽��Ի������;
	static void ProgressThread(LPVOID lpParam);
	
	CRockExtractor m_extractor;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBtnUpview();
	vector<std::string> m_strVecUpViewFiles;
	bool m_bIsUpView;
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	bool m_bLBDown;
	CPoint m_ptOrigin;
	CRect m_rectPic;
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedBtnTest();
	Rect GetMaxRect(const Mat &Frame);
	Rect m_rectMax;
	afx_msg void OnBnClickedAttcut();
	afx_msg void OnBnClickedBtnOnekey();
	string m_strPath,m_strPathMid,m_strPathUp,m_strPathMidSnap,m_strPathMidPed,m_strPathUpSnap,m_strPathUpPed;
	afx_msg void OnExitSizeMove();
};
