
// VideoPlayerDlg.h : 头文件
//

#pragma once
#include <opencv2/opencv.hpp>
#include "CvvImage.h"
#include "afxwin.h"
#include "FrameManager.h"
#include "DlgProgress.h"
#include "RockExtractor.h"


/**
*  控件随窗口变化的控制单元
**/
typedef struct tagCONTROL
{
	CWnd*	m_pWnd;				//指向控件的指针
	CRect	m_rectWnd;			//控件所占区域
	int		m_nMoveXPercent,	//控件沿x轴移动的百分比
			m_nMoveYPercent,	//控件沿y轴移动的百分比
			m_nZoomXPercent,	//控件沿x轴缩放的百分比
			m_nZoomYPercent;	//控件沿y轴缩放的百分比
}ControlInfo, *lpControlInfo;

#define WM_USER_CLOSE_DIALOG  WM_USER+200
typedef		CArray<lpControlInfo>	CTRLLIST;	//控件的适配信息表

// CVideoPlayerDlg 对话框
class CVideoPlayerDlg : public CDialogEx
{
// 构造
public:
	CVideoPlayerDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CVideoPlayerDlg();   //手动添加的析构函数

	void	SetMinSize(int nWidth, int nHeight);
	void	FreeCtrlInfoList();
	void	MakeCtrlFit(CWnd* pWnd, int nMoveXPercent = 0, int nMoveYPercent = 0, int nZoomXPercent = 0, int nZoomYPercent = 0);
	void	CancelCtrlFit(HWND hWnd);

// 对话框数据
	enum { IDD = IDD_VIDEOPLAYER_DIALOG };


	int			m_nWinWidth,		//窗口的宽度
				m_nWinHeight;		//窗口的高度
	POINT		m_ptMinTrackSize;	//窗口的最小大小
	CTRLLIST	m_listCtrlInfo;		//控件适配信息的数组

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
	static void ThradProCtlCreate(LPVOID lpParam); //负责创建对话框而已;
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
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
};
