#include "stdafx.h"
#include "RockExtractor.h"


CRockExtractor::CRockExtractor()
{
	m_nThresh = 40;

}


CRockExtractor::~CRockExtractor()
{
}

int CRockExtractor::CalcDist(Point p0, Point p1)
{
	return round(sqrt((double)(p0.x - p1.x)*(p0.x - p1.x) + (p0.y - p1.y)*(p0.y - p1.y)));

}

int CRockExtractor::CalcAngle(Point p0, Point p1)
{
	int x1 = p1.x, y1 = p1.y, x0 = p0.x, y0 = p0.y;
	double angle = 0.0;
	if (x1 == x0 && y1 < y0) angle = PI / 2;
	if (x1 == x0 && y1 > y0) angle = 3 * PI / 2;
	if (y1 == y0 && x1 < x0) angle = PI;
	if (y1 == y0 && x1 > x0) angle = PI * 2;

	if (x1 < x0 && y1 < y0)	angle = PI - atan((double)(p1.y - p0.y) / (p1.x - p0.x));
	if (x1 > x0 && y1 < y0) angle = atan((double)(p0.y - p1.y) / (double)(p1.x - p0.x));
	if (x1 < x0 && y1 > y0) angle = PI + atan((double)(p1.y - p0.y) / (double)(p0.x - p1.x));
	if (x1 > x0 && y1 > y0) angle = 2 * PI - atan((double)(p1.y - p0.y) / (double)(p1.x - p0.x));

	//if (angle == 2 * PI) angle = 0.0;
	return round(angle*(180 / PI));
}

map<int, int> CRockExtractor::GetMapSecondMax(map<int, vector<int>> nMap)
{
	map<int, int> nnMap;
	for (auto it1 = nMap.begin(); it1 != nMap.end(); ++it1)
	{
		vector<int> nVec = it1->second;
		sort(nVec.begin(), nVec.end());
		nnMap[it1->first] = nVec.back();
	}
	return nnMap;
}

void CRockExtractor::Sharp(Mat & imgSrc, Mat & imgDst)
{
	Mat kernel(3, 3, CV_32F, Scalar(-1));
	// ����������
	kernel.at<float>(0, 0) = 0;
	kernel.at<float>(2, 0) = 0;
	kernel.at<float>(0, 2) = 0;
	kernel.at<float>(2, 2) = 0;
	kernel.at<float>(1, 1) = 5;
	filter2D(imgSrc, imgDst, imgDst.depth(), kernel);
}


void CRockExtractor::TargetExtract_GrabCut(Mat &imgSrc, Mat &imgDst, Rect rectFG, 
											int nLoopTime, double dScalePara)
{
	double tt = cv::getTickCount();
	int nWidth = imgSrc.cols;
	int nHeight = imgSrc.rows;
	Mat imgResized, imgSharped;
	if (dScalePara == 1.0) 
	{
		imgResized = imgSrc.clone();
	}
	else
	{
		resize(imgSrc, imgResized, Size(nWidth*dScalePara, nHeight*dScalePara), 0, 0, INTER_LINEAR);
		rectFG.width = (int)(dScalePara*rectFG.width);
		rectFG.height = (int)(dScalePara*rectFG.height);
		rectFG.x = (int)(dScalePara*rectFG.x);
		rectFG.y = (int)(dScalePara*rectFG.y);
	}

	//grabcut ���ܴ���Ҷ�ͼ

	//��ֵ�˲����񻯴���ȥ��ϸ��
	medianBlur(imgResized, imgResized, 11);
	Sharp(imgResized, imgSharped);

	/*MyShowImage(imgSharped,"sharped");*/
	Mat bg, fg, mask, maskResized;
	/*waitKey(0);*/

	grabCut(imgSharped, mask, rectFG, bg, fg, nLoopTime, GC_INIT_WITH_RECT);
	tt = cv::getTickCount() - tt;
	//printf("�㷨ִ��ִ��ʱ��:%g s\n", tt / cv::getTickFrequency());
	Mat binMask;

	if (dScalePara == 1.0)
		maskResized = mask.clone();
	else
		resize(mask, maskResized, Size(nWidth, nHeight), 0, 0, INTER_LINEAR);

	GetBinMask(maskResized, binMask);
	//MyShowImage(binMask,"mask");
	imgSrc.copyTo(imgDst, binMask);
}


void CRockExtractor::GetBinMask(const Mat& comMask, Mat& binMask)
{
	binMask.create(comMask.size(), CV_8UC1);
	binMask = comMask & 1;
	//������õ���ģ������ֵ�˲�һ��,�Է�����С������
	medianBlur(binMask, binMask, 6 * 2 + 1);
}


void CRockExtractor::TargetExtract_Canny(Mat &srcImg, Mat &dstImg, double dScalePara, double dThresh)
{
	int nWidth = srcImg.cols;
	int nHeight = srcImg.rows;
	Mat grayImage, resizedImage, cannyImage;

	//Ԥ����
	//1.��С2.ת��Ϊ�Ҷ�ͼ3.��ֵ�˲���ȥϸ���Լ���������Ҫ��ģ���С�ͱ�����������С������4.����ǿ��Ե
	resize(srcImg, resizedImage, Size(nWidth*dScalePara, nHeight*dScalePara), 0, 0, INTER_LINEAR);
	//MyShowImage("ԭͼ", resizedImage, 2);
	cvtColor(resizedImage, grayImage, CV_BGR2GRAY);
	medianBlur(grayImage, grayImage, 11);
	Sharp(grayImage, grayImage);


	//��ʼ����:
	//1.canny�õ���Ե2.��������ɨ������Ե��3.�õ�ÿ��theta��Ӧ���rֵ4.������ģ
	Canny(grayImage, cannyImage, dThresh, dThresh * 2, 3);
	int nH = cannyImage.rows; //����
	int nW = cannyImage.cols;
	int nCountBorder = 0;
	int nCountNotB = 0;
	vector<Point> vecPtBorders;
	Mat maskMerged = Mat::zeros(cannyImage.size(), CV_8UC1);
	Mat maskResized;
	//��canny�����ͼ���ҵ���Ե��
	for (int j = 0; j < nH; j++)
	{
		for (int i = 0; i < nW; i++)
		{
			if (cannyImage.at<uchar>(j, i) == 255)
			{
				vecPtBorders.push_back(Point(i, j));
				nCountBorder++;
			}
			else nCountNotB++;
		}
	}
	cout << "��Ե�㹲��" << nCountBorder << ",�Ǳ�Ե�㹲��" << nCountNotB << endl;

	//�ȼ���õ�ͼ������
	long long sumX = 0, sumY = 0;
	for (auto it = vecPtBorders.begin(); it != vecPtBorders.end(); ++it)
	{
		sumX += it->x;
		sumY += it->y;
	}
	sumX /= vecPtBorders.size();
	sumY /= vecPtBorders.size();
	Point ptCenter(sumX, sumY);
	//circle(g_cannyMat_output, Point(sumX, sumY), 5, Scalar(255, 255, 255), 4);
	//MyShowImage("cannyͼ", cannyImage, 2);

	//�ѱ�Ե�㰴�ռ�����ĽǶȽ��й���
	map<int, vector<int>> mapTheta_R;
	for (auto it = vecPtBorders.begin(); it != vecPtBorders.end(); ++it)
	{
		int theta = CalcAngle(ptCenter, *it);
		//if (theta == 360) theta = 0; //���ﲻ������ Ϊʲô��
		mapTheta_R[theta].push_back(CalcDist(ptCenter, *it));
	}

	//ȡ��ÿ���Ƕȶ�Ӧ�������
	map<int, int> mapTable = GetMapSecondMax(mapTheta_R);

	//����ͼ������������������ģ
	for (int i = 0; i != nHeight; ++i)
	{
		for (int j = 0; j != nWidth; ++j)
		{
			Point p1(j, i);
			int theta = CalcAngle(ptCenter, p1);
			int dist = CalcDist(ptCenter, p1);
			if (dist <= mapTable[theta])
			{
				maskMerged.at<uchar>(i, j) = 255;
			}
		}
	}
	//MyShowImage("maskMerged", maskMerged, 2);


	//��ģ���ڴ���
	//1.��ֵ�ָ���С2.��Եƽ������

	resize(maskMerged, maskResized, Size(nWidth, nHeight), 0, 0, INTER_LINEAR);
	blur(maskResized, maskResized, Size(11, 11));
	threshold(maskResized, maskResized, 128, 255, CV_THRESH_BINARY);

	//MyShowImage(maskResized, "maskResized");
	srcImg.copyTo(dstImg, maskResized);
	//MyShowImage("��ȡ������", dstImg, 2);
	//imwrite("a.jpg", g_finalImage);
}


int CalcDist(Point p0, Point p1)
{
	return round(sqrt((double)(p0.x - p1.x)*(p0.x - p1.x) + (p0.y - p1.y)*(p0.y - p1.y)));
}