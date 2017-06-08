//
//// VideoPlayerDlg.cpp : 实现
// VideoPlayerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "VideoPlayer.h"
#include "VideoPlayerDlg.h"
#include "afxdialogex.h"
#include <string>
#include <iostream>
#include <fstream>
#include <io.h>
#include <opencv2/opencv.hpp>
#include "DlgSnapProg.h"
#include "DlgPtConfirm.h"
#include "Common.h"
using namespace std;
using namespace cv;

CEvent start_event;
bool g_bIsStarted = false; //开始播放的标志
bool g_bIsPaused = true; //暂停标志
bool g_bReverse = false;    //反转标志 (后期应该丢弃，应该直接用FrameMgr中的reverse变量)

IplImage *g_pFrame = NULL;

double g_dFrameCount = 0.0;  //视频的总帧数
double g_dScaleFactor = 1.0;

int g_SnapWidth = 2600;
int g_GrabWidth = 2600 * 0.8;
int g_SnapHeight = 1450;
int g_GrabHeight = 1450 * 0.9;

CvCapture* g_pCaptureMid;
CvCapture* g_pCaptureUp;

std::string g_strPathMid;
std::string g_strPathUp;

cv::Point g_ptStart;
cv::Point g_ptEnd;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void GetFileName(const string &strFolder, vector<string> &strVecFileNames);
bool  CheckFolderExist(const string &strPath);
string GetFileNameNoPath(string &filename);
void DeleteDirectory(string path);

void MyShowImage(const string winName, Mat &Image, unsigned nScale = 1)
{
	namedWindow(winName, CV_WINDOW_NORMAL);
	resizeWindow(winName, Image.cols / nScale, Image.rows / nScale);
	imshow(winName, Image);
}

#include <io.h>    
#include <fcntl.h>  
void InitConsoleWindow()
{
	FILE *stream;
	if (!AllocConsole() || freopen_s(&stream, "CONOUT$", "w", stdout))
		AfxMessageBox(_T("InitConsoleWindow Failed!")); //分配控制台在重定向输出流至控制台
}


void on_MouseHandle_GrabCut(int event, int x, int y, int flags, void* param);
void on_MouseHandle_Snap(int event, int x, int y, int flags, void *param);
void DrawRectangle(cv::Mat& img, cv::Rect box);
cv::Rect g_rectangle;
bool g_bDrawingBox = false;


int g_nDelay = 80;

DWORD WINAPI PlayVideo(LPVOID lpParam);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CVideoPlayerDlg 对话框



CVideoPlayerDlg::CVideoPlayerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVideoPlayerDlg::IDD, pParent)
	, m_upViewFrame(NULL)
{
	m_ptMinTrackSize.x = 0;
	m_ptMinTrackSize.y = 0;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bSizeChanged = false;
	m_fProgress = 0.0;
	m_bIsUpView = false;
	m_bLBDown = false;
	m_rectMax.width = 0;
	m_rectMax.height = 0;
}
CVideoPlayerDlg::~CVideoPlayerDlg()
{
	FreeCtrlInfoList();
	if (g_bIsStarted)
	{
		cvReleaseCapture(&g_pCaptureMid);
		cvReleaseCapture(&g_pCaptureUp);
		//cvReleaseImage(&g_pFrame);
	}
}

void CVideoPlayerDlg::SetMinSize(int nWidth, int nHeight)
{
	ASSERT(nWidth > 0);
	ASSERT(nHeight > 0);

	//设置窗口最小值
	m_ptMinTrackSize.x = nWidth;
	m_ptMinTrackSize.y = nHeight;
}

void CVideoPlayerDlg::FreeCtrlInfoList()
{
	INT_PTR	nCount = m_listCtrlInfo.GetSize();

	for (int i = 0; i < nCount; i++)
	{
		lpControlInfo pCtrlInfo = m_listCtrlInfo.ElementAt(i);
		delete pCtrlInfo;
	}

	m_listCtrlInfo.RemoveAll();
}

void CVideoPlayerDlg::MakeCtrlFit(CWnd* pWnd, int nMoveXPercent, int nMoveYPercent, int nZoomXPercent, int nZoomYPercent)
{
	ASSERT(pWnd);									//指针是否为空
	ASSERT(nMoveXPercent >= 0 && nMoveXPercent <= 100);	//nMoveXPercent值是否有效
	ASSERT(nMoveYPercent >= 0 && nMoveYPercent <= 100);	//nMoveXPercent值是否有效
	ASSERT(nZoomXPercent >= 0 && nZoomXPercent <= 100);	//nMoveXPercent值是否有效
	ASSERT(nZoomYPercent >= 0 && nZoomYPercent <= 100);	//nMoveXPercent值是否有效

	lpControlInfo	pCtrlInfo = new ControlInfo;	//创建结构指针

													//填充变量
	pCtrlInfo->m_pWnd = pWnd;
	pCtrlInfo->m_nMoveXPercent = nMoveXPercent;
	pCtrlInfo->m_nMoveYPercent = nMoveYPercent;
	pCtrlInfo->m_nZoomXPercent = nZoomXPercent;
	pCtrlInfo->m_nZoomYPercent = nZoomYPercent;

	pWnd->GetWindowRect(pCtrlInfo->m_rectWnd);
	ScreenToClient(&pCtrlInfo->m_rectWnd);

	m_listCtrlInfo.Add(pCtrlInfo);	//加入维护列表
}


void CVideoPlayerDlg::CancelCtrlFit(HWND hWnd)
{
	INT_PTR	nCount = m_listCtrlInfo.GetSize();

	for (int i = 0; i < nCount; i++)
	{
		lpControlInfo pCtrlInfo = m_listCtrlInfo.ElementAt(i);
		if (pCtrlInfo->m_pWnd->GetSafeHwnd() == hWnd)
		{
			delete pCtrlInfo;
			m_listCtrlInfo.RemoveAt(i);
			break;
		}
	}
}


void CVideoPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SuspendVideo, StopButton);
}

BEGIN_MESSAGE_MAP(CVideoPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_StartVideo, &CVideoPlayerDlg::OnBnClickedStartvideo)
	ON_BN_CLICKED(IDC_SuspendVideo, &CVideoPlayerDlg::OnBnClickedSuspendvideo)
	ON_BN_CLICKED(IDC_StopVideo, &CVideoPlayerDlg::OnBnClickedStopvideo)
	ON_BN_CLICKED(ID_SPEEDUPBUTTON, &CVideoPlayerDlg::OnBnClickedSpeedupbutton)
	ON_BN_CLICKED(ID_SPEEDDOWNBUTTON, &CVideoPlayerDlg::OnBnClickedSpeeddownbutton)
	ON_BN_CLICKED(ID_BackwardInverse, &CVideoPlayerDlg::OnBnClickedBackwardinverse)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()

	ON_WM_MOUSEWHEEL()
	/*ON_BN_CLICKED(IDC_UPVIEW, &CVideoPlayerDlg::OnBnClickedUpview)*/
	ON_BN_CLICKED(IDC_Snap, &CVideoPlayerDlg::OnClickedSnap)
	ON_BN_CLICKED(IDC_PROCESS, &CVideoPlayerDlg::OnBnClickedProcess)
	ON_MESSAGE(WM_USER_CLOSE_DIALOG, CloseDialog)
	ON_WM_CHAR()
	ON_BN_CLICKED(IDC_BTN_UPVIEW, &CVideoPlayerDlg::OnBnClickedBtnUpview)
	ON_WM_SIZING()
	ON_WM_NCHITTEST()
	ON_WM_GETMINMAXINFO()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BTN_TEST, &CVideoPlayerDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_AttCut, &CVideoPlayerDlg::OnBnClickedAttcut)
	ON_BN_CLICKED(IDC_BTN_ONEKEY, &CVideoPlayerDlg::OnBnClickedBtnOnekey)
	ON_WM_EXITSIZEMOVE()
END_MESSAGE_MAP()


// CVideoPlayerDlg 消息处理程序

BOOL CVideoPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	CRect rectTemp;
	//获取客户区大小
	GetClientRect(rectTemp);
	//保存客户区信息
	m_nWinWidth = rectTemp.Width();
	m_nWinHeight = rectTemp.Height();
	//获取窗口大小
	GetWindowRect(rectTemp);
	//设置窗口最小值
	SetMinSize(rectTemp.Width(), rectTemp.Height());

	//设置鼠标灵敏度
	m_nMouseCheck = 50;

	//适配控件
	int nXpercent = 100;
	int nYheight = 3;
	int nYpercent = 5;

	MakeCtrlFit(GetDlgItem(IDC_VIDEO), 0, 0, nXpercent, 100);

	MakeCtrlFit(GetDlgItem(IDC_StartVideo), nXpercent, nYpercent * 1, 0, nYheight); //开始按键不调整
	MakeCtrlFit(GetDlgItem(ID_SPEEDUPBUTTON), nXpercent, nYpercent * 2, 0, nYheight);
	MakeCtrlFit(GetDlgItem(ID_SPEEDDOWNBUTTON), nXpercent, nYpercent * 3, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_SuspendVideo), nXpercent, nYpercent * 4, 0, nYheight);
	MakeCtrlFit(GetDlgItem(ID_BackwardInverse), nXpercent, nYpercent * 5, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_BTN_UPVIEW), nXpercent, nYpercent * 6, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_StopVideo), nXpercent, nYpercent * 7, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_Snap), nXpercent, nYpercent * 8, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_PROCESS), nXpercent, nYpercent * 9, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_AttCut), nXpercent, nYpercent * 10, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_BTN_ONEKEY), nXpercent, nYpercent * 11, 0, nYheight);

	MakeCtrlFit(GetDlgItem(IDC_STATIC_WIDTH), nXpercent, nYpercent * 12, 0, nYheight);
	MakeCtrlFit(GetDlgItem(IDC_STATIC_HEIGHT), nXpercent, nYpercent * 13, 0, nYheight);


	InitConsoleWindow();
	//cout << SkinH_AttachEx(_T("0001.she"), NULL) << endl;
	SkinH_Attach();

	//this->ShowWindow(SW_MAXIMIZE);
	GetDlgItem(IDC_VIDEO)->GetWindowRect(&m_rectPic);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}


void CVideoPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CVideoPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CVideoPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CVideoPlayerDlg::OnBnClickedStartvideo()
{
	// TODO:  在此添加控件通知处理程序代码

	if (g_bIsStarted)
		OnBnClickedStopvideo();

	TCHAR wchPath[MAX_PATH];     //存放选择的目录路径 
	CString cStrPath;
	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//转为Cstring类型
	}
	if (cStrPath.IsEmpty())
		return;

	if (cStrPath.Find(_T("MidView")) == -1)
	{
		AfxMessageBox(_T("无效路径!"));
		return;
	}

	g_strPathMid = CT2CA(cStrPath.GetBuffer());
	g_strPathUp = g_strPathMid;
	g_strPathUp.replace(g_strPathUp.find("Mid"), 3, "Up");


	//加入判断上中视角照片数目检测，取二者较小值,然后删掉多余图片
	vector<std::string> vecStrMid, vecStrUp;
	GetFileName(g_strPathMid, vecStrMid);
	GetFileName(g_strPathUp, vecStrUp);
	int nCountMin = -1;
	int nCountMax = -1;
	bool bMidGT = false;
	if (vecStrMid.size() != vecStrUp.size())
	{
		if (vecStrMid.size() > vecStrUp.size())
		{
			bMidGT = true;
			nCountMin = vecStrUp.size();
			nCountMax = vecStrMid.size();
			for (int i = 0; i != nCountMax - nCountMin; ++i)
			{
				DeleteFileA(vecStrMid.back().c_str());
				vecStrMid.pop_back();
			}
		}
		else
		{
			bMidGT = false;
			nCountMin = vecStrMid.size();
			nCountMax = vecStrUp.size();
			for (int i = 0; i != nCountMax - nCountMin; ++i)
			{
				DeleteFileA(vecStrUp.back().c_str());
				vecStrUp.pop_back();
			}
		}

	}

	std::string strVideoPathMid = g_strPathMid + "\\" + "output.avi";
	std::string strPlyPathMid = g_strPathMid + "\\" + "output.ply";//最终输出文件全路径

	std::string strVideoPathUp = g_strPathUp + "\\" + "output.avi";
	std::string strPlyPathUp = g_strPathUp + "\\" + "output.ply";//最终输出文件全路径

	CString cStrPlyPathMid = CString(strPlyPathMid.c_str());//应用A2CW转换 从char *转换为LPCWSTR
	CString cStrPlyPathUp = CString(strPlyPathUp.c_str());//应用A2CW转换 从char *转换为LPCWSTR

	cStrPlyPathMid.ReleaseBuffer();
	cStrPlyPathUp.ReleaseBuffer();

	//取得文件总数
	vector<std::string> strVecTemp;
	GetFileName(g_strPathMid, strVecTemp);
	int nPicCount = strVecTemp.size();
	float fStep = 100.0 / (nPicCount * 2);

	m_fProgress = 0.0;
	_beginthread(ThradProCtlCreate, 0, this);
	_beginthread(ProgressThread, 0, this);

	if (!(PathFileExists(cStrPlyPathMid)))//判断mid视角文件output.ply是否存在
	{

		string strFileExtension = "jpg";
		string strJpgAllPath = g_strPathMid + "\\*." + strFileExtension;

		VideoWriter writer;
		int frameRate = 5;
		Size frameSize;
		char chFileAllName[1000];
		struct _finddata_t fileInfo;    // 文件信息结构体

		// 1. 第一次查找
		long findResult = _findfirst(strJpgAllPath.c_str(), &fileInfo);
		if (findResult == -1)
		{
			_findclose(findResult);
			return;
		}

		// 2. 循环查找
		do
		{
			sprintf_s(chFileAllName, "%s\\%s", g_strPathMid.c_str(), fileInfo.name);
			if (fileInfo.attrib == _A_ARCH)  // 是存档类型文件
			{
				Mat frame;
				frame = imread(chFileAllName);    // 读入图片
				//resize(frame, frame, Size(0.25 * frame.cols, 0.25*frame.rows), 0.0, 0.0);
				if (!writer.isOpened())
				{
					frameSize.width = frame.cols;
					frameSize.height = frame.rows;

					if (!writer.open(strVideoPathMid, CV_FOURCC('M', 'P', '4', '2'), frameRate, frameSize, true))
					{ //这里不能打开后缀名为Ply的文件，所以只能后面再改名
						//cout << "open writer error..." << endl;
						AfxMessageBox(_T("open error!"));
						return;
					}
				}
				// 将图片数据写入
				writer.write(frame);
				m_fProgress += fStep;
				// 显示
				//waitKey(frameRate);
			}

		} while (!_findnext(findResult, &fileInfo));
		writer.release();//一定要释放掉啊！！！！！		
		_findclose(findResult);
		rename(strVideoPathMid.c_str(), strPlyPathMid.c_str()); //更换命名方式，更换后缀名,隐藏文件ply
	}

	m_fProgress = 50.0;
	Sleep(100);

	if (!(PathFileExists(cStrPlyPathUp)))//判断up视角文件output.ply是否存在
	{
		string strFileExtension = "jpg";
		string strJpgAllPath = g_strPathUp + "\\*." + strFileExtension;

		VideoWriter writer;
		int frameRate = 5;
		Size frameSize;
		char chFileAllName[1000];
		struct _finddata_t fileInfo;    // 文件信息结构体

		// 1. 第一次查找
		long findResult = _findfirst(strJpgAllPath.c_str(), &fileInfo);
		if (findResult == -1)
		{
			_findclose(findResult);
			return;
		}

		// 2. 循环查找
		do
		{
			sprintf_s(chFileAllName, "%s\\%s", g_strPathUp.c_str(), fileInfo.name);
			if (fileInfo.attrib == _A_ARCH)  // 是存档类型文件
			{
				Mat frame;
				frame = imread(chFileAllName);    // 读入图片
				//resize(frame, frame, Size(0.25 * frame.cols, 0.25*frame.rows), 0.0, 0.0);

				if (!writer.isOpened())
				{
					frameSize.width = frame.cols;
					frameSize.height = frame.rows;
					if (!writer.open(strVideoPathUp, CV_FOURCC('M', 'P', '4', '2'), frameRate, frameSize, true))
						//if (!writer.open(strVideoPathUp, CV_FOURCC('D', 'I', 'V', 'X'), frameRate, frameSize, true))
					{ //这里不能打开后缀名为Ply的文件，所以只能后面再改名
					  //cout << "open writer error..." << endl;
						AfxMessageBox(_T("open error!"));
						return;
					}
				}
				// 将图片数据写入
				writer.write(frame);
				// 显示
				m_fProgress += fStep;
			}

		} while (!_findnext(findResult, &fileInfo));
		writer.release();//一定要释放掉啊！！！！！

		_findclose(findResult);
		rename(strVideoPathUp.c_str(), strPlyPathUp.c_str()); //更换命名方式，更换后缀名,隐藏文件ply
	}
	m_fProgress = 80.0;

	//在这里统计最大矩形信息
	//cout << vecStrMid.size() << endl;
	for (int i = 0; i != vecStrMid.size(); ++i)
	{
		if (vecStrMid[i].find("ply") != -1)
			continue;
		Mat img = imread(vecStrMid[i]);
		Rect rect = GetMaxRect(img);
		if (m_rectMax.width < rect.width)
			m_rectMax.width = rect.width;
		if (m_rectMax.height < rect.height)
			m_rectMax.height = rect.height;
	}

	m_fProgress = 100.0;
	Sleep(100); //delay is necessary

	AfxMessageBox(_T("Process Done!")); //必要的  用来激活窗口  为啥我也不晓得
	HANDLE hThreadSend;			//创建独立线程发送数据
	DWORD ThreadSendID;

	start_event.SetEvent();
	hThreadSend = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlayVideo, (LPVOID)this, 0, &ThreadSendID);
	g_bIsPaused = false;
	g_bIsStarted = true;
	CloseHandle(hThreadSend);
}


DWORD WINAPI PlayVideo(LPVOID lpParam)
{
	CVideoPlayerDlg* pThis = (CVideoPlayerDlg*)lpParam;
	std::string  fileFolderMid = g_strPathMid + "\\" + "output.ply";
	std::string  fileFolderUp = g_strPathUp + "\\" + "output.ply";
	g_pCaptureMid = cvCreateFileCapture(fileFolderMid.c_str());//读入数据，即读入视频
	g_pCaptureUp = cvCreateFileCapture(fileFolderUp.c_str());
	if (g_pCaptureMid == NULL || g_pCaptureUp == NULL)
	{
		AfxMessageBox(_T("Open PlyFile Failed!"));
		return -1;
	}

	g_dFrameCount = cvGetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_FRAME_COUNT);//读取视频中有多少帧
	double dFrameCountUp = cvGetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_FRAME_COUNT);

	if (g_dFrameCount != dFrameCountUp)
	{
		AfxMessageBox(_T("Frame Count Not Equal!"));
		return -1;
	}

	//初始化帧管理器
	pThis->m_fmgr.SetReverse(false);
	pThis->m_fmgr.SetFrameCount((int)g_dFrameCount);
	pThis->m_fmgr.SetFrameIndex(-1);

	//vector<int> nVecTimes;
	//int64 nFrameTimeStart = 0;
	//int64 nFrameTimeEnd = 0;
	//cvSetCaptureProperty(pCapture, CV_CAP_PROP_POS_FRAMES, 0);
	//for (int i = 0; i != (int)dFrameCount; ++i)
	//{
	//	nFrameTimeStart = getTickCount();
	//	pFrame = cvQueryFrame(pCapture);
	//	nFrameTimeEnd = getTickCount() - nFrameTimeStart;
	//	nVecTimes.push_back(nFrameTimeEnd / getTickFrequency() * 1000);
	//}
	//} 


	while (true)
	{
		//读取是始终采用cvQueryFrame，位置通过g_nFrameIndex来控制

		WaitForSingleObject(start_event, INFINITE);
		start_event.SetEvent();

		pThis->m_fmgr.NextFrame();
		if (!pThis->m_bIsUpView)
		{
			cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, pThis->m_fmgr.GetFrameIndex());
			g_pFrame = cvQueryFrame(g_pCaptureMid);
		}
		else
		{
			cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, pThis->m_fmgr.GetFrameIndex());
			g_pFrame = cvQueryFrame(g_pCaptureUp);
		}

		if (g_bIsStarted == false) //点击结束后标志置位，停止播放
			_endthreadex(0);

		CString itemWidth, itemHeight;
		itemWidth.Format(L"物体高度:\n%0.3fCM", pThis->m_rectMax.height / 110.0);
		itemHeight.Format(L"物体宽度:\n%0.3fCM", pThis->m_rectMax.width / 110.0);
		pThis->GetDlgItem(IDC_STATIC_WIDTH)->SetWindowTextW(itemWidth);
		pThis->GetDlgItem(IDC_STATIC_HEIGHT)->SetWindowTextW(itemHeight);
		pThis->OnDisplay();

		//前面的帧每次播放会快一些，不知道为什么，就只有多延时一段时间了
		if (pThis->m_fmgr.GetFrameIndex() < pThis->m_fmgr.GetFrameCount() * 0.3)
			Sleep(g_nDelay * 1.5);
		else
			Sleep(g_nDelay * 1.0);

	}
	cvReleaseCapture(&g_pCaptureMid);  //释放CvCapture结构
	cvReleaseCapture(&g_pCaptureUp);
	return 0;
}


void CVideoPlayerDlg::ShowToPicCtl(IplImage *imgSrc, UINT ID)
{
	CDC* pDC = GetDlgItem(ID)->GetDC();
	IplImage*imgDst = cvCreateImage(cvSize(m_rectPic.Width(), m_rectPic.Height()), imgSrc->depth, imgSrc->nChannels);	
	cvResize(imgSrc, imgDst, CV_INTER_LINEAR);
	//在控件里显示图片
	CvvImage cvImg;
	cvImg.CopyOf(imgDst);
	cvImg.DrawToHDC(pDC->m_hDC, &m_rectPic);
	cvReleaseImage(&imgDst);
	ReleaseDC(pDC);
}

void CVideoPlayerDlg::OnBnClickedSuspendvideo()   //暂停
{
	// TODO:  在此添加控件通知处理程序代码

	if (!g_bIsStarted)
		return;
	CString buttonText;
	StopButton.GetWindowText(buttonText);

	//正在运行时
	if (buttonText.Compare(_T("Pause")) == 0)
	{
		start_event.ResetEvent();
		StopButton.SetWindowTextW(_T("Resume"));
		g_bIsPaused = true;
		//g_dScaleFactor = 1.0;
	}
	else //暂停时
	{
		start_event.SetEvent();
		m_upViewFrame = NULL;
		StopButton.SetWindowText(_T("Pause"));
		g_bIsPaused = false;
		//g_dScaleFactor = 1.0;
	}
}

void CVideoPlayerDlg::OnBnClickedStopvideo()   //结束
{
	// TODO:  在此添加控件通知处理程序代码
	if (!g_bIsStarted)
		return;
	g_bIsStarted = false;
	g_bIsPaused = true;
	m_bIsUpView = false;
	g_bReverse = false;
	g_dScaleFactor = 1.0;
	g_nDelay = 200;
	g_rectangle.width = 0;
	g_rectangle.height = 0;
	m_bLBDown = false;

	GetDlgItem(IDC_BTN_UPVIEW)->SetWindowTextW(_T("UpView"));
	StopButton.SetWindowTextW(_T("Pause"));
	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_VIDEO);
	CRect lRect;
	pStatic->GetClientRect(&lRect);
	pStatic->GetDC()->FillSolidRect(lRect.left, lRect.top, lRect.Width(), lRect.Height(), RGB(160, 160, 160));
	cvReleaseCapture(&g_pCaptureUp);
	cvReleaseCapture(&g_pCaptureMid);
	//cvReleaseImage(&g_pFrame);
}

void CVideoPlayerDlg::OnBnClickedSpeedupbutton()   //快进
{
	// TODO:  在此添加控件通知处理程序代码
	if (g_bIsStarted)
	{
		if (g_nDelay <= 20)
		{
			return;
		}
		g_nDelay -= 20;
	}
}

void CVideoPlayerDlg::OnBnClickedSpeeddownbutton()  //慢放
{
	// TODO:  在此添加控件通知处理程序代码
	if (g_bIsStarted)
	{
		g_nDelay += 20;
	}
}

void CVideoPlayerDlg::OnBnClickedBackwardinverse()   //翻转180（回放）
{
	// TODO:  在此添加控件通知处理程序代码
	g_bReverse = !g_bReverse;
	m_fmgr.SetReverse(!m_fmgr.GetReverse());
}


void CVideoPlayerDlg::OnDisplay()  //负责调整大小后调用display显示到picture控件
{
	if (g_pFrame == NULL)
	{
		AfxMessageBox(_T("读取数据出错!"));
		return;
	}
	IplImage* imgToDisplay;

	int ogx = 0, ogy = 0;//源图像ROI感兴趣区域起点变量

	CvSize newsize;
	newsize.width = (g_pFrame->width)*g_dScaleFactor;
	newsize.height = (g_pFrame->height)*g_dScaleFactor;

	IplImage* pImageResized = cvCreateImage(newsize, g_pFrame->depth, g_pFrame->nChannels);
	cvResize(g_pFrame, pImageResized, CV_INTER_LINEAR);
	//创建控件显示图像
	imgToDisplay = cvCreateImage(cvSize(g_pFrame->width, g_pFrame->height), g_pFrame->depth, g_pFrame->nChannels);
	//这里计算相对区域一定要是再原图大小进行,最后送进display进行resize
	ogx = abs(g_pFrame->width - newsize.width) / 2;//图片控件中图片区域的起点
	ogy = abs(g_pFrame->height - newsize.height) / 2;

	if (g_dScaleFactor > 1.0) //放大
	{
		cvSetImageROI(pImageResized, cvRect(ogx, ogy, g_pFrame->width, g_pFrame->height));//设置源图像ROI感兴趣区域
		cvCopy(pImageResized, imgToDisplay); //仅复制感兴趣区域图像
		cvResetImageROI(pImageResized);//源图像用完后，清空ROI
	}
	else if (g_dScaleFactor < 1.0) //缩小
		cvCopyMakeBorder(pImageResized, imgToDisplay, cvPoint(ogx, ogy), IPL_BORDER_CONSTANT);  //边界补0,扩大至图片控件大小
	else //不变
		cvCopy(g_pFrame, imgToDisplay);

	ShowToPicCtl(imgToDisplay, IDC_VIDEO);
	cvReleaseImage(&pImageResized);
	cvReleaseImage(&imgToDisplay);
}


void CVideoPlayerDlg::OnLButtonDown(UINT nFlags, CPoint point)  //鼠标左键按下,显示下一帧或者前一帧。鼠标按下之前先按暂停键
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (g_bIsPaused && g_bIsStarted)
	{
		//直接取消下一帧播放
		//CRect rect;
		//CWnd  *pWnd;
		//int picWidth;
		//int picHeight;
		//pWnd = GetDlgItem(IDC_VIDEO);
		//pWnd->GetClientRect(&rect);
		////求图片控件的宽和高
		//picWidth = rect.Width();
		//picHeight = rect.Height();

		////  通过对point的位置判定，选取下一帧或者前一帧。

		//// 左上上一帧
		//if ((point.x >= rect.left && point.x <= picWidth / 2) && (point.y >= rect.top && point.y <= picHeight / 2))
		//{
		//	m_fmgr.PreFrame();
		//	if (m_bIsUpView)
		//	{
		//		cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
		//		g_pFrame = cvQueryFrame(g_pCaptureUp);
		//	}
		//	else
		//	{
		//		cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
		//		g_pFrame = cvQueryFrame(g_pCaptureMid);
		//	}
		//	OnDisplay();
		//}
		//// 右下则下一帧
		//else if ((point.x <= rect.right && point.x >= picWidth / 2) && (point.y <= rect.bottom && point.y >= picHeight / 2))
		//{
		//	m_fmgr.NextFrame();
		//	if (m_bIsUpView)
		//	{
		//		cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
		//		g_pFrame = cvQueryFrame(g_pCaptureUp);
		//	}
		//	else
		//	{
		//		cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
		//		g_pFrame = cvQueryFrame(g_pCaptureMid);
		//	}
		//	OnDisplay();
		//}
		//else
		//{
		//	//return;
		//}

		//if (m_rectPic.PtInRect(point))
		//{
		//	m_ptOrigin = point;
		//	m_bLBDown = true;
		//}
		m_ptOrigin = point;
		m_bLBDown = true;
		//cout << "m_bLBDown = true;" << endl; //这里没问题
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CVideoPlayerDlg::OnLButtonUp(UINT nFlags, CPoint point)  //鼠标左键松开
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (m_bLBDown)
	{
		m_bLBDown = false;
		//cout << "OnLButtonUp" << endl;
	}
	//cout << "m_bLBDown = false;OnLButtonUp" << endl;

	CDialogEx::OnLButtonUp(nFlags, point);
}

//利用滚轮实现放大缩小
BOOL CVideoPlayerDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (g_bIsStarted)
	{
		if (zDelta > 0)  //滚轮向上滑动，放大
		{
			if (g_dScaleFactor <= 1.8)
			{
				g_dScaleFactor += 0.2;
			}
		}
		else if (zDelta < 0)   //滚轮向下滑动，缩小
		{
			if (g_dScaleFactor >= 0.4)
			{
				g_dScaleFactor -= 0.2;

			}
		}
		OnDisplay();
	}

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


//void CVideoPlayerDlg::OnBnClickedUpview()
//{
//	// TODO:  在此添加控件通知处理程序代码
//	string files = g_strPath;
//	if (files == " ")
//	{
//		AfxMessageBox(_T("请先选择开始按钮"));
//		return;
//	}
//	int pos = files.find_last_of("\\") + 1;
//	files = files.erase(pos);
//	files = files.append("upview");//已更改到上视图文件夹
//
//	Common com;//调用公共文件（公共类）中的函数
//	com.replace_all_distinct(files, "\\\\", "\\");   //替换数据\\\\到\\  
//	string picname = files;
//	files = files + "\\*.jpg";
//
//	size_t size = files.length();//string 转为LPCWSTR
//	wchar_t *filena = new wchar_t[size + 1];
//	MultiByteToWideChar(CP_ACP, 0, files.c_str(), size + 1, filena, size * sizeof(wchar_t));
//	filena[size + 1] = 0;  // 确保以 '\0' 结尾 
//
//	WIN32_FIND_DATA FindFileData;
//	HANDLE hFind;
//	vector<string> picnameve;
//	int find = 1;
//	hFind = FindFirstFile(filena, &FindFileData);
//	if (hFind == INVALID_HANDLE_VALUE)
//	{
//		AfxMessageBox(_T("没找到上视图文件夹"));
//		return;
//	}
//	else
//	{
//		while (find)
//		{
//			Common com;
//			string nametem = com.WChar2Ansi(FindFileData.cFileName);
//			picnameve.push_back(nametem);
//			find = FindNextFile(hFind, &FindFileData);
//		}
//		FindClose(hFind);
//	}
//	auto it = picnameve.begin();
//	if (g_nFrameIndex<0 || g_nFrameIndex>picnameve.size())
//	{
//		AfxMessageBox(_T("超出索引范围"));
//		return;
//	}
//	picname = picname + "\\" + (*(it + g_nFrameIndex));//指定图片的详细地址
//	const char* cpicname = picname.c_str();//string转const char*
//	m_upViewFrame = cvLoadImage(cpicname, 1);//1代表读入彩色图片
//	Display(m_upViewFrame, IDC_VIDEO);
//}


bool FindOrCreateDirectory(const char *pszPath)
{
	USES_CONVERSION;
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFileW(A2W(pszPath), &fd);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//::AfxMessageBox(_T("目录存在！"));
			return true;
		}
	}

	if (!::CreateDirectory(A2W(pszPath), NULL))
	{
		//char szDir[MAX_PATH];
		::AfxMessageBox(_T("创建目录失败"));
		return false;
	}
	else
	{
		return true;
		//::AfxMessageBox(_T("创建目录成功"));
	}
}

CString CVideoPlayerDlg::GetAppPath()
{
	TCHAR exeFullPath[MAX_PATH];
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	CString pathName(exeFullPath);

	//返回值最后自带'\\'
	int index = pathName.ReverseFind('\\');
	return pathName.Left(index + 1);
}

void CVideoPlayerDlg::OnClickedSnap()
{
	// TODO:  在此添加控件通知处理程序代码

	USES_CONVERSION;
	TCHAR wchPath[MAX_PATH];     //存放选择的目录路径 
	CString cStrPath;
	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//转为Cstring类型
	}

	if (cStrPath.IsEmpty())
		return;

	g_strPathMid = CT2CA(cStrPath.GetBuffer());

	//if (g_strPathMid.find("MidView") == -1)
	//{
	//	AfxMessageBox(_T("请打开中视角文件夹"));
	//	return;
	//}

	/*g_strPathUp = g_strPathMid.substr(0, g_strPathMid.find("MidView")) + "UpView";*/
	//size_t size = strJpgAllPath.length();//length()返回的没有/0的长度
	//wchar_t *wchPath = new wchar_t[size + 1];
	//MultiByteToWideChar(CP_ACP, 0, strJpgAllPath.c_str(), size + 1, wchPath, size * sizeof(wchar_t));
	//这两种方法都可以
	//string str = W2A(wchPath);


	wcscat_s(wchPath, _T("\\*.jpg"));

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	vector<string> vecPicName;
	int find = 1;
	hFind = FindFirstFile(wchPath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(_T("没找到文件夹"));
		return;
	}
	else
	{
		while (find)
		{
			string strFileName = W2A(FindFileData.cFileName);
			strFileName = g_strPathMid + "\\" + strFileName; //现在是全路径名了
			vecPicName.push_back(strFileName);
			find = FindNextFile(hFind, &FindFileData);
		}
		FindClose(hFind);
	}

	//vecPicName里存的就是文件全路径名称了 双斜杠

	Mat imgOrigin, imgSnaped, imgTemp;
	std::string strImgSnapedName;
	imgOrigin = imread(vecPicName.front());//读取第一张图
	imgOrigin.copyTo(imgTemp);
	MyShowImage("请选择截取范围,选取后按ESC键确认", imgTemp, 3);
	cv::setMouseCallback("请选择截取范围,选取后按ESC键确认", on_MouseHandle_Snap, (void *)&imgOrigin);

	while (1)
	{
		Mat imgTemp1;
		imgOrigin.copyTo(imgTemp1);

		//DrawRectangle(imgTemp, Rect(50, 50, 200, 200));
		//if (g_bDrawingBox) DrawRectangle(imgTemp, g_rectangle);
		DrawRectangle(imgTemp1, g_rectangle);
		MyShowImage("请选择截取范围,选取后按ESC键确认", imgTemp1, 3);
		if (cv::waitKey(10) == 27) //delay is neccesary,来确保内存能读得到
		{
			if (g_ptStart.x != 0 && g_ptStart.y != 0 && g_ptEnd.x != 0 && g_ptEnd.y != 0) //确保获得到了坐标
			{
				int nMsgBox = AfxMessageBox(_T("是否确认选择？"), MB_OKCANCEL);
				if (nMsgBox == IDOK)
				{
					cout << "最终选取坐标为 x：" << g_ptStart.x << " y:" << g_ptStart.y << "  到x：" << g_ptEnd.x << " y:" << g_ptEnd.y << endl;
					cv::destroyWindow("请选择截取范围,选取后按ESC键确认");
					break;
				}
				else
				{
					CWnd *pWnd = FindWindowW(NULL, _T("请选择截取范围,选取后按ESC键确认"));
					::SetForegroundWindow(pWnd->m_hWnd);
				}
			}
		}
	}


	Rect rect(g_ptStart, g_ptEnd);

	//int index = g_strPath.rfind('\\');
	string strSnapedPath = g_strPathMid + "_Snaped";
	cout << FindOrCreateDirectory(strSnapedPath.c_str());

	m_fProgress = 0.0;
	UINT nSnapTotal = vecPicName.size();
	float fStep = (float)100 / nSnapTotal;

	_beginthread(ThradProCtlCreate, 0, this);
	_beginthread(ProgressThread, 0, this);

	int i = 1;
	//不需要nJump了，这里容器这个大小循环完肯定还没完，所以直接之后进度条设为100，销毁窗口就行了
	for (auto it = vecPicName.begin(); it != vecPicName.end(); ++it)
	{
		//cout << *it << endl;
		imgOrigin = imread(*it);
		int width = imgOrigin.cols;
		int height = imgOrigin.rows;
		imgSnaped = imgOrigin(rect);
		int pos = (*it).find_last_of("\\");
		imgSnaped = imgOrigin(rect);
		strImgSnapedName = (*it).insert(pos, "_Snaped");
		imwrite(strImgSnapedName, imgSnaped);
		m_fProgress += fStep;
		//pDlgSnap->m_SnapProg.SetPos(nSnapProg);
		//cout << "第" << i << "张图片处理完成" << endl;
		i++;
	}
	m_fProgress = 100.0;
	//pDlgSnap->m_SnapProg.SetPos(1000);
	//pDlgSnap->DestroyWindow();
	//EnableWindow(TRUE);
	AfxMessageBox(_T("Process Done!")); //必要的  用来激活窗口  为啥我也不晓得

}


void getBinMask(const Mat& comMask, Mat& binMask)
{
	binMask.create(comMask.size(), CV_8UC1);
	binMask = comMask & 1;
	medianBlur(binMask, binMask, 6 * 2 + 1); //中值滤波去除噪声
}

void CVideoPlayerDlg::OnBnClickedProcess()
{
	// TODO:  在此添加控件通知处理程序代码
	USES_CONVERSION;
	TCHAR wchPath[MAX_PATH];     //存放选择的目录路径 
	CString cStrPath;
	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//转为Cstring类型
	}

	if (cStrPath.IsEmpty())
		return;

	g_strPathMid = CT2CA(cStrPath.GetBuffer());

	wcscat_s(wchPath, _T("\\*.jpg"));


	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	vector<string> vecPicName;
	int find = 1;
	hFind = FindFirstFile(wchPath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(_T("没找到文件夹"));
		return;
	}
	else
	{
		while (find)
		{
			string strFileName = W2A(FindFileData.cFileName);
			strFileName = g_strPathMid + "\\" + strFileName; //现在是全路径名了
			vecPicName.push_back(strFileName);
			find = FindNextFile(hFind, &FindFileData);
		}
		FindClose(hFind);
	}

	string strProcessedPath = g_strPathMid + "_Processed";

	if (!FindOrCreateDirectory(strProcessedPath.c_str()))
		return; //若创建文件夹失败则不继续

	Mat imgOrigin, imgTemp;
	//ofstream ofs(g_strPathMid + "\\record.log");
	std::string strImgProcessedName;
	//Rect rect(262, 500, 1966 - 262, 1569 - 500);
	double dTimeSingle = 0.0, dTimeTotal = 0.0;

	imgOrigin = imread(vecPicName.front());//读取第一张图
	imgOrigin.copyTo(imgTemp);
	MyShowImage("请选择处理范围,选取后按ESC键确认", imgTemp, 3);
	cv::setMouseCallback("请选择处理范围,选取后按ESC键确认", on_MouseHandle_GrabCut, (void *)&imgOrigin);
	g_ptStart.x = g_ptStart.y = 0;
	g_ptEnd.x = g_ptEnd.y = 0;
	g_rectangle = Rect(g_ptStart, g_ptEnd);

	while (1)
	{
		imgOrigin.copyTo(imgTemp);
		if (g_rectangle.width != 0 && g_rectangle.height != 0) DrawRectangle(imgTemp, g_rectangle);
		MyShowImage("请选择处理范围,选取后按ESC键确认", imgTemp, 3);
		if (cv::waitKey(10) == 27) //delay is neccesary,来确保内存能读得到
		{
			if (g_ptStart.x != 0 && g_ptStart.y != 0 && g_ptEnd.x != 0 && g_ptEnd.y != 0) //确保获得到了坐标
			{
				int nMsgBox = AfxMessageBox(_T("是否确认选择？"), MB_OKCANCEL);
				if (nMsgBox == IDOK)
				{
					cout << "最终选取坐标为 x：" << g_ptStart.x << " y:" << g_ptStart.y
						<< "  到x：" << g_ptEnd.x << " y:" << g_ptEnd.y << endl;
					cv::destroyWindow("请选择处理范围,选取后按ESC键确认");
					break;
				}
				else
				{
					CWnd *pWnd = FindWindowW(NULL, _T("请选择处理范围,选取后按ESC键确认"));
					::SetForegroundWindow(pWnd->m_hWnd);
				}

			}
		}
	}

	Rect rectProcessed(g_ptStart, g_ptEnd);

	UINT nSnapTotal = vecPicName.size();
	UINT fStep = 100.0 / nSnapTotal;
	_beginthread(ThradProCtlCreate, 0, this);
	_beginthread(ProgressThread, 0, this);

	int i = 1;

	for (auto it = vecPicName.begin(); it != vecPicName.end(); ++it)
	{
		double dTimeSingle = cv::getTickCount();
		Mat imgProcessed;
		imgOrigin = imread(*it);
		int pos = (*it).find_last_of("\\");
		strImgProcessedName = (*it).insert(pos, "_Processed");

		m_extractor.TargetExtract_GrabCut(imgOrigin, imgProcessed, rectProcessed, 3, 1 / 3.0);
		//m_extractor.TargetExtract_Canny(imgOrigin, imgProcessed, 1 / 3.0, 30.0);
		imwrite(strImgProcessedName, imgProcessed);
		m_fProgress += fStep;

		dTimeSingle = cv::getTickCount() - dTimeSingle;
		dTimeTotal += dTimeSingle;
		//ofs << "第" << i << "张图片处理完成，耗时：" << dTimeSingle / cv::getTickFrequency() << "s" << endl;
		//cout << "第" << i << "张图片处理完成，耗时：" << dTimeSingle / cv::getTickFrequency() << "s" << endl;
		//cout << i << "张图片处理完成，耗时：" << dTimeSingle / cv::getTickFrequency() << "s" << endl;

		i++;
	}
	m_fProgress = 100.0;
	//cout << "总耗时：" << dTimeTotal / cv::getTickFrequency() << "s" << endl;
	//ofs << "总耗时：" << dTimeTotal / cv::getTickFrequency() << "s" << endl;
	//ofs.close();

	AfxMessageBox(_T("Process Done!")); //必要的  用来激活窗口  为啥我也不晓得
}



void DrawRectangle(cv::Mat &img, cv::Rect box) {
	cv::rectangle(img, box.tl(), box.br(), cv::Scalar(0, 255, 0), 4);
}


void on_MouseHandle_GrabCut(int event, int x, int y, int flags, void *param) {
	//cv::Mat image = *(cv::Mat *) param;
	switch (event) {
	case cv::EVENT_MOUSEMOVE:
	{
		if (g_bDrawingBox)
		{
			g_rectangle.width = x - g_rectangle.x; //后面有解决为负数的情况
			g_rectangle.height = y - g_rectangle.y;
		}
	}
	break;

	case cv::EVENT_LBUTTONDOWN:
	{
		g_bDrawingBox = true;
		g_ptStart.x = x;
		g_ptStart.y = y;
		g_rectangle = cv::Rect(x, y, 0, 0);
	}
	break;

	case cv::EVENT_LBUTTONUP:
	{
		g_bDrawingBox = false;
		if (g_rectangle.width < 0) {
			g_rectangle.x += g_rectangle.width;
			g_rectangle.width *= -1;
		}

		if (g_rectangle.height < 0) {
			g_rectangle.y += g_rectangle.height;
			g_rectangle.height *= -1;
		}
		//rectangle(image, g_rectangle.tl(), g_rectangle.br(), cv::Scalar(0, 255, 0), 2);
		g_ptEnd.x = x; g_ptEnd.y = y;
	}
	break;
	}
}


void on_MouseHandle_Snap(int event, int x, int y, int flags, void *param)
{
	cv::Mat image = *(cv::Mat *) param;
	switch (event)
	{
	case cv::EVENT_MOUSEMOVE:
	{
		if (g_bDrawingBox)
		{
			g_ptStart.x = x - g_SnapWidth / 2;
			g_ptStart.y = y - g_SnapHeight / 2;
			g_ptEnd.x = x + g_SnapWidth / 2;
			g_ptEnd.y = y + g_SnapHeight / 2;
			if (g_ptStart.x < 0)
			{
				g_ptStart.x = 0;
				g_ptEnd.x = g_SnapWidth;
			}
			if (g_ptStart.y < 0)
			{
				g_ptStart.y = 0;
				g_ptEnd.y = g_SnapHeight;
			}
			if (g_ptEnd.x > image.cols)
			{
				g_ptStart.x = image.cols - g_SnapWidth;
				g_ptEnd.x = image.cols;
			}
			if (g_ptEnd.y > image.rows)
			{
				g_ptStart.y = image.rows - g_SnapHeight;
				g_ptEnd.y = image.rows;
			}
			g_rectangle = Rect(g_ptStart, g_ptEnd);

		}
	}
	break;

	case cv::EVENT_LBUTTONDOWN:
	{
		g_bDrawingBox = true;
		g_ptStart.x = x - g_SnapWidth / 2;
		g_ptStart.y = y - g_SnapHeight / 2;
		g_ptEnd.x = x + g_SnapWidth / 2;
		g_ptEnd.y = y + g_SnapHeight / 2;
		//cout << g_ptEnd << endl;
		if (g_ptStart.x < 0)
		{
			g_ptStart.x = 0;
			g_ptEnd.x = g_SnapWidth;
		}
		if (g_ptStart.y < 0)
		{
			g_ptStart.y = 0;
			g_ptEnd.y = g_SnapHeight;
		}
		if (g_ptEnd.x > image.cols)
		{
			g_ptStart.x = image.cols - g_SnapWidth;
			g_ptEnd.x = image.cols;
		}
		if (g_ptEnd.y > image.rows)
		{
			g_ptStart.y = image.rows - g_SnapHeight;
			g_ptEnd.y = image.rows;
		}
		g_rectangle = Rect(g_ptStart, g_ptEnd);
		//DrawRectangle(image, g_rectangle);
	}
	break;

	case cv::EVENT_LBUTTONUP:
	{
		g_bDrawingBox = false;
		//if (g_rectangle.width < 0) {
		//	g_rectangle.x += g_rectangle.width;
		//	g_rectangle.width *= -1;
		//}

		//if (g_rectangle.height < 0) {
		//	g_rectangle.y += g_rectangle.height;
		//	g_rectangle.height *= -1;
		//}
		//DrawRectangle(image, g_rectangle);
		//g_ptEnd.x = x; g_ptEnd.y = y;
	}
	break;
	}
}


void CVideoPlayerDlg::ThradProCtlCreate(LPVOID lpParam) //负责创建对话框而已
{
	CVideoPlayerDlg* pThis = (CVideoPlayerDlg*)lpParam;
	if (pThis->m_dlgProgress.m_hWnd)
		pThis->m_dlgProgress.DestroyWindow();
	pThis->m_dlgProgress.DoModal();
	//pView->m_dlgProgress.SetForegroundWindow();
	pThis->EnableWindow(FALSE);
	_endthread();
}


void CVideoPlayerDlg::ProgressThread(LPVOID lpParam)
{
	CVideoPlayerDlg* pThis = (CVideoPlayerDlg*)lpParam;
	//CRockAnalysisDoc* pDoc = pView->GetDocument();
	float n = 0.0;
	while ((n = pThis->m_fProgress) < 100) //线程启动后一只检测doc类里的m_fProgress变量来更新状态
	{
		Sleep(30); //这边线程睡眠时间最好小于主线程更新时间，不然进度条更新更不上pos,就会出现没走完进度条就关闭的错觉
				   //::PostMessage(pView->m_hWnd, WM_USER_PROGRESS,(WPARAM)n,0);
		pThis->m_dlgProgress.SetPos((int)n);//(int)wParam此参数影响进度条时间的显示
		pThis->m_dlgProgress.UpdateWindow();
	}
	//	::PostMessage(pView->m_hWnd, WM_USER_PROGRESS,100,0);
	pThis->m_dlgProgress.SetPos(100);
	//pView->m_dlgProgress.UpdateWindow();

	//(pView->m_dlgProgress).ShowWindow(false);
	//(pView->m_dlgProgress).DestroyWindow(); //这里用destroywindow会直接出错，不知道为啥
	::PostMessageW(pThis->m_hWnd, WM_USER_CLOSE_DIALOG, 0, (LPARAM)pThis); //+200是closewindow的消息
	//::SendMessageW(pThis->m_hWnd, WM_USER_CLOSE_DIALOG, 0, (LPARAM)pThis);
	pThis->m_fProgress = 0.0;
	pThis->EnableWindow(TRUE);
	_endthread();
}


LRESULT CVideoPlayerDlg::CloseDialog(WPARAM wParam, LPARAM lParam)
{
	CVideoPlayerDlg* pThis = (CVideoPlayerDlg*)lParam;
	pThis->m_dlgProgress.EndDialog(IDCANCEL);
	return 1;
}

BOOL CVideoPlayerDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 32: //按下空格键暂停
			OnBnClickedSuspendvideo();
			return TRUE;
			break;
		default:
			return TRUE;
			break;
		}
	}
	else //其他情况必须返回父类的才行
		return CDialogEx::PreTranslateMessage(pMsg);
}



void CVideoPlayerDlg::OnBnClickedBtnUpview()
{
	//启动后，如果目前是中视角则切换到上视角,如果目前是上视角则切换到中视角
	if (g_bIsStarted && g_bIsPaused)
	{
		if (m_bIsUpView) //上视角切换为中视角
		{
			cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
			g_pFrame = cvQueryFrame(g_pCaptureMid);
			GetDlgItem(IDC_BTN_UPVIEW)->SetWindowTextW(_T("UpView"));
		}
		else //中视角切换为上视角
		{
			cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
			g_pFrame = cvQueryFrame(g_pCaptureUp);
			GetDlgItem(IDC_BTN_UPVIEW)->SetWindowTextW(_T("MidView"));
		}
		m_bIsUpView = !m_bIsUpView;
		OnDisplay();
	}
	else if (g_bIsStarted && !g_bIsPaused)
	{
		if (m_bIsUpView)
			GetDlgItem(IDC_BTN_UPVIEW)->SetWindowTextW(_T("UpView"));
		else
			GetDlgItem(IDC_BTN_UPVIEW)->SetWindowTextW(_T("MidView"));
		m_bIsUpView = !m_bIsUpView;
		OnDisplay();
	}

}

void GetFileName(const string &strFolder, vector<string> &strVecFileNames)
{
	strVecFileNames.clear();
	struct _finddata_t filefind;
	string  curr = strFolder + "\\*.*";
	int  done = 0;
	int  handle;
	if ((handle = _findfirst(curr.c_str(), &filefind)) == -1)
		return;
	string tempfolder = strFolder + "\\";
	while (!(done = _findnext(handle, &filefind)))
	{
		if (!strcmp(filefind.name, ".."))  //用此方法第一个找到的文件名永远是".."，所以需要单独判断
			continue;
		string temp(filefind.name);
		if (temp.find("ply") != -1 || temp.find("avi") != -1) //判断不需要的文件类型
		{
			continue;
		}
		strVecFileNames.push_back(tempfolder + filefind.name);
	}
	_findclose(handle);
}

void CVideoPlayerDlg::OnSize(UINT nType, int cx, int cy)
{
	//计算窗口宽度和高度的改变量
	int nIncrementX = cx - m_nWinWidth;
	int nIncrementY = cy - m_nWinHeight;

	INT_PTR	nCount = m_listCtrlInfo.GetSize();

	UINT uFlags = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS;
	for (int i = 0; i < nCount; i++)
	{
		//获取变化控制系数
		int	nMoveXPercent = m_listCtrlInfo[i]->m_nMoveXPercent;
		int	nMoveYPercent = m_listCtrlInfo[i]->m_nMoveYPercent;
		int	nZoomXPercent = m_listCtrlInfo[i]->m_nZoomXPercent;
		int	nZoomYPercent = m_listCtrlInfo[i]->m_nZoomYPercent;

		CWnd* pWndCtrl = m_listCtrlInfo[i]->m_pWnd;
		HWND hWnd = pWndCtrl->GetSafeHwnd();
		if ((NULL != pWndCtrl) && IsWindow(hWnd))
		{
			int nLeft = m_listCtrlInfo[i]->m_rectWnd.left;
			int nTop = m_listCtrlInfo[i]->m_rectWnd.top;
			int nWidth = m_listCtrlInfo[i]->m_rectWnd.Width();
			int nHeight = m_listCtrlInfo[i]->m_rectWnd.Height();

			//设置新的位置参数
			nLeft += (nIncrementX*nMoveXPercent / 100);
			nTop += (nIncrementY*nMoveYPercent / 100);
			nWidth += (nIncrementX*nZoomXPercent / 100);
			nHeight += (nIncrementY*nZoomYPercent / 100);

			//  把控件移动到新位置
			pWndCtrl->MoveWindow(nLeft, nTop, nWidth, nHeight);
			if (g_bIsStarted)
				OnDisplay();
		}
	}
	GetDlgItem(IDC_VIDEO)->GetClientRect(&m_rectPic); //clientrect是相对于客户区,windowrect是相对于屏幕
	Invalidate(1); //写在这里不好，性能影响太大
	UpdateWindow();
	CDialogEx::OnSize(nType, cx, cy);
}

void CVideoPlayerDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialogEx::OnSizing(fwSide, pRect);
	// TODO: 在此处添加消息处理程序代码
}


void CVideoPlayerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	lpMMI->ptMinTrackSize = m_ptMinTrackSize;
	CDialogEx::OnGetMinMaxInfo(lpMMI);
}


void CVideoPlayerDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	OnBnClickedBtnUpview();
	CDialogEx::OnRButtonUp(nFlags, point);
}


void CVideoPlayerDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_bLBDown && g_bIsStarted && g_bIsPaused)
	{
		//HCURSOR hCur = LoadCursor(NULL, IDC_SIZE);
		//::SetCursor(hCur);
		int nR = (int)GetRValue(GetPixel(GetDC()->m_hDC, point.x, point.y));
		int nG = (int)GetGValue(GetPixel(GetDC()->m_hDC, point.x, point.y));
		int nB = (int)GetBValue(GetPixel(GetDC()->m_hDC, point.x, point.y));
		if (nR == 0 && nG == 0 && nB == 0)
		{
			//cout << "blcak" << endl;
			return;
		}
		if ((point.x - m_ptOrigin.x) > m_nMouseCheck)
		{
			//m_fmgr.NextFrame();
			if (!g_bReverse)
			{
				m_fmgr.PreFrame();
			}
			else
			{
				m_fmgr.NextFrame();
			}
			if (m_bIsUpView)
			{
				cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
				g_pFrame = cvQueryFrame(g_pCaptureUp);
			}
			else
			{
				cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
				g_pFrame = cvQueryFrame(g_pCaptureMid);
			}
			OnDisplay();
			m_ptOrigin = point;
		}
		else if ((m_ptOrigin.x - point.x) > m_nMouseCheck)
		{
			//m_fmgr.PreFrame();
			if (g_bReverse)
			{
				m_fmgr.PreFrame();
			}
			else
			{
				m_fmgr.NextFrame();
			}
			if (m_bIsUpView)
			{
				cvSetCaptureProperty(g_pCaptureUp, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
				g_pFrame = cvQueryFrame(g_pCaptureUp);
			}
			else
			{
				cvSetCaptureProperty(g_pCaptureMid, CV_CAP_PROP_POS_FRAMES, m_fmgr.GetFrameIndex());
				g_pFrame = cvQueryFrame(g_pCaptureMid);
			}
			OnDisplay();
			m_ptOrigin = point;
		}
	}
	else
	{
		//cout << "undo" << endl;
		/*cout << m_bLBDown << g_bIsPaused << g_bIsStarted << endl;*/
	}

	//暂时先去掉这个判断,有问题
	//if (!m_rectPic.PtInRect(point))
	//{
	//	m_bLBDown = false;
	//	//cout << "m_bLBDown = false;" << endl;
	//}

	CDialogEx::OnMouseMove(nFlags, point);
}


void CVideoPlayerDlg::OnBnClickedBtnTest()
{

}


Rect CVideoPlayerDlg::GetMaxRect(const Mat &Frame)
{
	Mat image;
	cvtColor(Frame, image, CV_BGR2GRAY);
	int Xmin = INT_MAX, Xmax = 0, Ymin = INT_MAX, Ymax = 0;

	int nr = image.rows;
	int nc = image.cols;
	for (int i = 0; i < nr; ++i)
	{
		const uchar* inData = image.ptr<uchar>(i);
		for (int j = 0; j < nc; j++)
		{
			if ((*inData++) != 0)
			{
				if (i < Ymin)
					Ymin = i;
				if (i > Ymax)
					Ymax = i;
				if (j < Xmin)
					Xmin = j;
				if (j > Xmax)
					Xmax = j;
			}
		}
	}

	return Rect(Xmin, Xmax, Xmax - Xmin, Ymax - Ymin);
}

void CVideoPlayerDlg::OnBnClickedAttcut()
{
	// TODO: 在此添加控件通知处理程序代码
	USES_CONVERSION;

	CString wPathAttSrc = GetAppPath() + L"AttCut\\src";
	CString wPathAttDst = GetAppPath() + L"AttCut\\out";
	string pathAttSrc = W2A(wPathAttSrc);
	string pathAttDst = W2A(wPathAttDst);

	FindOrCreateDirectory(pathAttSrc.c_str());
	FindOrCreateDirectory(pathAttDst.c_str());

	DeleteDirectory(pathAttSrc);
	DeleteDirectory(pathAttDst);

	TCHAR wchPath[MAX_PATH];     //存放选择的目录路径 
	CString cStrPath;
	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//转为Cstring类型
	}

	if (cStrPath.IsEmpty())
		return;

	g_strPathMid = CT2CA(cStrPath.GetBuffer());
	string pathProcessed = g_strPathMid + "_Processed";

	if (!FindOrCreateDirectory(pathProcessed.c_str()))
		return; //若创建文件夹失败则不继续

	//先拷贝源文件夹到AttCut需要处理的地方
	vector<string> strVecPics;
	GetFileName(g_strPathMid, strVecPics);
	for (auto it = strVecPics.begin(); it != strVecPics.end(); ++it)
	{
		string filename = it->substr(it->rfind("\\"), it->length());
		CString wfilename = A2W(filename.c_str());
		CString wPathSrc = A2W((*it).c_str());
		CopyFile(wPathSrc, wPathAttSrc + wfilename, FALSE);
	}

	//启动管道操作
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		AfxMessageBox(_T("Error On CreatePipe()"));
		return;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	CString appPath = GetAppPath();
	string strAppPath = W2A(appPath);
	CString attPath = appPath + L"AttCut\\AttCutDemo.bat";
	LPTSTR szCmdline = _tcsdup(_T("\"") + attPath + _T("\""));
	string strCmdline = W2A(szCmdline);

	if (!CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, NULL, NULL, appPath+"\\AttCut\\", &si, &pi))
	{
		AfxMessageBox(_T("Error on CreateProcess()"));
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return;
	}
	CloseHandle(hWrite);
	char buffer[4096] = { 0 };
	DWORD bytesRead;
	while (true)
	{
		if (ReadFile(hRead, buffer, 4095, &bytesRead, NULL) == NULL)
		{
			break;
		}
		Sleep(200);
	}
	CloseHandle(hRead);
	delete[]szCmdline;

	//执行完了再拷贝回来
	vector<std::string> strVecSrcPics;
	vector<std::string> strVecMaskPics;
	GetFileName(pathAttDst, strVecMaskPics);
	GetFileName(g_strPathMid, strVecSrcPics);
	if (strVecMaskPics.size() != strVecSrcPics.size())
	{
		AfxMessageBox(_T("掩模图像数目不一致！"));
		return;
	}

	for (int i = 0; i != strVecSrcPics.size(); ++i)
	{
		Mat img, mask, dst;
		img = imread(strVecSrcPics[i]);
		mask = imread(strVecMaskPics[i]);
		//模板稍微圆滑一下
		blur(mask, mask, Size(11, 11));
		threshold(mask, mask, 128, 255, CV_THRESH_BINARY);
		//拷贝
		img.copyTo(dst, mask);

		cout << pathProcessed + "\\" + GetFileNameNoPath(strVecSrcPics[i]) << endl;
		imwrite(pathProcessed + "\\" + GetFileNameNoPath(strVecSrcPics[i]), dst);
	}


	AfxMessageBox(L"DONE!");


}


void DeleteDirectory(string path)
{
	vector<string> files;
	GetFileName(path, files);
	if (files.size() == 0)
	{
		return;
	}
	for (int i = 0; i != files.size(); ++i)
	{
		DeleteFileA(files[i].c_str());
	}
}

void CVideoPlayerDlg::OnBnClickedBtnOnekey()
{
	// TODO: 在此添加控件通知处理程序代码
	USES_CONVERSION;
	TCHAR wchPath[MAX_PATH];     //存放选择的目录路径 
	CString cStrPath;
	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("请选择需要打开的目录：");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	//弹出选择目录对话框
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//转为Cstring类型
	}

	if (cStrPath.IsEmpty())
		return;

	m_strPath = CT2CA(cStrPath.GetBuffer()); //带前不带后

	if (m_strPath.find("View") != -1)
	{
		AfxMessageBox(_T("无效路径,请选择根目录!"));
		return;
	}

	m_strPathMid = m_strPath + "\\MidView";
	m_strPathMidSnap = m_strPath + "\\MidView_Snaped";
	m_strPathMidPed = m_strPath + "\\MidView_Snaped_Processed";
	m_strPathUp = m_strPath + "\\UpView";
	m_strPathUpSnap = m_strPath + "\\UpView_Snaped";
	m_strPathUpPed = m_strPath + "\\UpView_Snaped_Processed";

	if (!CheckFolderExist(m_strPathMid) || !CheckFolderExist(m_strPathUp))
	{
		AfxMessageBox(_T("未找到View文件夹!"));
		return;
	}

	FindOrCreateDirectory(m_strPathMidSnap.c_str());
	FindOrCreateDirectory(m_strPathMidPed.c_str());
	FindOrCreateDirectory(m_strPathUpSnap.c_str());
	FindOrCreateDirectory(m_strPathUpPed.c_str());

	vector<string> Files_Mid, Files_Up;
	GetFileName(m_strPathMid, Files_Mid);
	GetFileName(m_strPathUp, Files_Up);
	int nMidCount = Files_Mid.size();
	int nUpCount = Files_Up.size();
	float fTotal = nMidCount + nUpCount;
	float fStep = 100.0 / (fTotal * 2);

	m_fProgress = 0.0;
	_beginthread(ThradProCtlCreate, 0, this);
	_beginthread(ProgressThread, 0, this);

	Mat imgOrigin, imgSnaped, imgProcessed;
	imgOrigin = imread(Files_Mid[0]);
	Rect ROI;
	ROI.width = g_SnapWidth;
	ROI.height = g_SnapHeight;
	ROI.x = (imgOrigin.cols - g_SnapWidth) / 2;
	ROI.y = (imgOrigin.rows - g_SnapHeight) / 2;
	string fileNameNoExt;

	for (auto midFile : Files_Mid)
	{
		imgOrigin = imread(midFile);
		imgSnaped = imgOrigin(ROI);
		fileNameNoExt = GetFileNameNoPath(midFile);
		imwrite(m_strPathMidSnap + "\\" + fileNameNoExt, imgSnaped);
		cout << fileNameNoExt << " Snap Mid Done." << endl;
		if (m_fProgress < 25.0)
			m_fProgress += fStep;
	}
	m_fProgress = 25.0;
	for (auto upFile : Files_Up)
	{
		imgOrigin = imread(upFile);
		imgSnaped = imgOrigin(ROI);
		fileNameNoExt = GetFileNameNoPath(upFile);
		imwrite(m_strPathUpSnap + "\\" + fileNameNoExt, imgSnaped);
		cout << fileNameNoExt << " Snap Up Done." << endl;
		if (m_fProgress < 50.0)
			m_fProgress += fStep;
	}
	m_fProgress = 50.0;

	//中视角处理开始
	GetFileName(m_strPathMidSnap, Files_Mid);
	GetFileName(m_strPathUpSnap, Files_Up);
	Rect rectGrabCut;
	rectGrabCut.x = (g_SnapWidth - g_GrabWidth) / 2;
	rectGrabCut.x = (g_SnapHeight - g_GrabHeight) / 2;
	rectGrabCut.width = g_GrabWidth;
	rectGrabCut.height = g_GrabHeight;

	for (auto midFile : Files_Mid)
	{
		Mat imgOrigin, imgProcessed;
		imgOrigin = imread(midFile);
		m_extractor.TargetExtract_GrabCut(imgOrigin, imgProcessed, rectGrabCut, 3, 0.25);
		fileNameNoExt = GetFileNameNoPath(midFile);
		imwrite(m_strPathMidPed + "\\" + fileNameNoExt, imgProcessed);
		cout << fileNameNoExt << " GrabCut Done." << endl;
		if (m_fProgress < 75.0)
			m_fProgress += fStep;
	}
	m_fProgress = 75.0;

	//上视角处理开始
	CString wPathAttSrc = GetAppPath() + L"AttCut\\src";
	CString wPathAttDst = GetAppPath() + L"AttCut\\out";
	string pathAttSrc = W2A(wPathAttSrc);
	string pathAttDst = W2A(wPathAttDst);

	DeleteDirectory(pathAttSrc);
	DeleteDirectory(pathAttDst);

	//先拷贝源文件夹到AttCut需要处理的地方
	vector<string> strVecPics;
	GetFileName(m_strPathUpSnap, strVecPics);
	for (auto it = strVecPics.begin(); it != strVecPics.end(); ++it)
	{
		string filename = it->substr(it->rfind("\\"), it->length());
		CString wfilename = A2W(filename.c_str());
		CString wPathSrc = A2W((*it).c_str());
		CopyFile(wPathSrc, wPathAttSrc + wfilename, FALSE);
	}

	//启动管道操作
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0))
	{
		AfxMessageBox(_T("Error On CreatePipe()"));
		return;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb = sizeof(STARTUPINFO);
	GetStartupInfo(&si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	CString appPath = GetAppPath();
	string strAppPath = W2A(appPath);
	CString attPath = appPath + L"AttCut\\AttCutDemo.bat";
	LPTSTR szCmdline = _tcsdup(_T("\"") + attPath + _T("\""));
	string strCmdline = W2A(szCmdline);

	if (!CreateProcess(NULL, szCmdline, NULL, NULL, TRUE, NULL, NULL, appPath + "\\AttCut\\", &si, &pi))
	{
		AfxMessageBox(_T("Error on CreateProcess()"));
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return;
	}
	CloseHandle(hWrite);
	char buffer[4096] = { 0 };
	DWORD bytesRead;
	while (true)
	{
		if (ReadFile(hRead, buffer, 4095, &bytesRead, NULL) == NULL)
		{
			break;
		}
		Sleep(200);
	}
	CloseHandle(hRead);
	delete[]szCmdline;

	//执行完了再拷贝回来
	vector<std::string> strVecSrcPics;
	vector<std::string> strVecMaskPics;
	GetFileName(pathAttDst, strVecMaskPics);
	GetFileName(m_strPathUpSnap, strVecSrcPics);
	if (strVecMaskPics.size() != strVecSrcPics.size())
	{
		AfxMessageBox(_T("掩模图像数目不一致！"));
		return;
	}

	for (int i = 0; i != strVecSrcPics.size(); ++i)
	{
		Mat img, mask, dst;
		img = imread(strVecSrcPics[i]);
		mask = imread(strVecMaskPics[i]);
		//模板稍微圆滑一下
		blur(mask, mask, Size(11, 11));
		threshold(mask, mask, 128, 255, CV_THRESH_BINARY);
		//拷贝
		img.copyTo(dst, mask);
		cout << GetFileNameNoPath(strVecSrcPics[i]) << " AttCut Done." << endl;
		imwrite(m_strPathUpPed + "\\" + GetFileNameNoPath(strVecSrcPics[i]), dst);
		if (m_fProgress < 100.0)
			m_fProgress += fStep;
	}
	m_fProgress = 100.0;
	Sleep(100); //delay is neccessary
	AfxMessageBox(_T("Process Done!"));

}

bool CheckFolderExist(const string &strPath)
{
	WIN32_FIND_DATA  wfd;
	bool rValue = false;
	USES_CONVERSION;
	CString wstrPath = A2W(strPath.c_str());
	HANDLE hFind = FindFirstFile(wstrPath, &wfd);
	if ((hFind != INVALID_HANDLE_VALUE) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		rValue = true;
	}
	FindClose(hFind);
	return rValue;
}

string GetFileNameNoPath(string &filename)
{
	int pos = filename.rfind("\\");
	return filename.substr(pos + 1, filename.length() - pos);
}

void CVideoPlayerDlg::OnExitSizeMove()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//Invalidate(true); //写在这里还是有问题，直接缩小后是错误的
	//UpdateWindow();
	CDialogEx::OnExitSizeMove();
}
