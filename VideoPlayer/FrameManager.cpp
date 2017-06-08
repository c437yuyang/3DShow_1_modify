#include "stdafx.h"
#include "FrameManager.h"


//CFrameManager::CFrameManager(int nFrameCount, bool reverse)
//{
//	m_nFrameCount = nFrameCount;
//	m_bReverse = reverse;
//}

CFrameManager::CFrameManager()
{
	m_nFrameCount = 0;
	m_bReverse = false;
}

CFrameManager::~CFrameManager()
{
}

bool CFrameManager::GetReverse()
{
	return m_bReverse;
}

void CFrameManager::SetFrameCount(int i)
{
	m_nFrameCount = i;
}

void CFrameManager::SetFrameIndex(int index)
{
	m_nFrameIndex = index;
}

void CFrameManager::NextFrame()
{
	if (!m_bReverse)
	{
		if (m_nFrameIndex < m_nFrameCount - 1)
		{
			++m_nFrameIndex;
		}
		else
		{
			m_nFrameIndex = 0;
		}

	}
	else
	{
		if (m_nFrameIndex == 0)
		{
			m_nFrameIndex = m_nFrameCount - 1;
		}
		else
		{
			--m_nFrameIndex;
		}
	}
}


void CFrameManager::PreFrame()
{
	if (m_bReverse)
	{
		if (m_nFrameIndex < m_nFrameCount - 1)
		{
			++m_nFrameIndex;
		}
		else
		{
			m_nFrameIndex = 0;
		}

	}
	else
	{
		if (m_nFrameIndex == 0)
		{
			m_nFrameIndex = m_nFrameCount - 1;
		}
		else
		{
			--m_nFrameIndex;
		}
	}
}

void CFrameManager::SetReverse(bool b)
{
	m_bReverse = b;
}
