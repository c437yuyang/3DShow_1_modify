#pragma once
class CFrameManager
{
public:
	CFrameManager();
	~CFrameManager();
	int GetFrameIndex() { return m_nFrameIndex; }
	int GetFrameCount() { return m_nFrameCount; }
	void NextFrame();
	void PreFrame();
	void SetReverse(bool);
	bool GetReverse();
	void SetFrameCount(int);
	void SetFrameIndex(int);
private:
	int m_nFrameIndex;
	int m_nFrameCount;
	bool m_bReverse;
};

