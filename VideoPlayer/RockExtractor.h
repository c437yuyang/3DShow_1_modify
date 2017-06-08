#pragma once

#define PI 3.14159627
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <map>
#include <string>
using namespace cv;
using namespace std;


class CRockExtractor
{
public:
	CRockExtractor();
	~CRockExtractor();

	//这个函数带了放大缩小，返回的是原图一样大的
	void TargetExtract_GrabCut(Mat &imgSrc, Mat &imgDst, Rect rectFG, int nLoopTime = 3, double dScalePara = 1.0);

	void GetBinMask(const Mat& comMask, Mat& binMask);

	void TargetExtract_Canny(Mat &srcImg, Mat &dstImg, double dScalePara = 0.33, double dThresh = 60);

	void MyShowImage(Mat &Image, const string winName, unsigned nScale = 1.0)
	{
		namedWindow(winName, CV_WINDOW_NORMAL);
		resizeWindow(winName, Image.cols / nScale, Image.rows / nScale);
		imshow(winName, Image);
	}

private:

	int m_nThresh;
	int CalcDist(Point p0, Point p1);
	int CalcAngle(Point p0, Point p1);
	map<int, int> GetMapSecondMax(map<int, vector<int>> nMap);
	void Sharp(Mat &imgSrc, Mat &imgDst);
};

