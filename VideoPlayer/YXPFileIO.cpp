#include "stdafx.h"
#include "YXPFileIO.h"

using namespace std;

YXPFileIO::YXPFileIO()
{
}


YXPFileIO::~YXPFileIO()
{
}


//����ָ��Ŀ¼���丸Ŀ¼�������
bool YXPFileIO::FindOrCreateDirectory(const char *pszPath)
{
	USES_CONVERSION;
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFileW(A2W(pszPath), &fd);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//::AfxMessageBox(_T("Ŀ¼���ڣ�"));
			return true;
		}
	}

	if (!::CreateDirectory(A2W(pszPath), NULL))
	{
		//char szDir[MAX_PATH];
		::AfxMessageBox(_T("����Ŀ¼ʧ��"));
		return false;
	}
	else
	{
		return true;
		//::AfxMessageBox(_T("����Ŀ¼�ɹ�"));
	}
}



// �ж�Ŀ¼�Ƿ����(/��\\�����ԣ����󲻴���\\������)  
bool YXPFileIO::FolderExists(const CString& s)
{
	DWORD attr;
	attr = GetFileAttributes(s);
	return (attr != (DWORD)(-1)) &&
		(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool YXPFileIO::FileExists(const CString& s)
{
	DWORD attr;
	attr = GetFileAttributes(s);
	return (attr != (DWORD)(-1)) &&
		(attr & FILE_ATTRIBUTE_ARCHIVE); //���������صĻ���ֻ���Ķ��ܼ�⵽
}


// �ݹ鴴��Ŀ¼������ʹ��/��ֻ��\\�����󲻴���\\������)
// ���Ŀ¼�Ѿ����ڻ��ߴ����ɹ�����TRUE  
bool YXPFileIO::SuperMkDir(const CString& path)
{
	CString P(path);
	int len = P.GetLength();
	if (len < 2) return false;

	if ('\\' == P[len - 1])
	{
		P = P.Left(len - 1);
		len = P.GetLength();
	}
	if (len <= 0) return false;

	if (len <= 3)
	{
		if (FolderExists(P))return true;
		else return false;
	}

	if (FolderExists(P))return true;

	CString Parent;
	Parent = P.Left(P.ReverseFind('\\'));

	if (Parent.GetLength() <= 0)return false;

	bool Ret = SuperMkDir(Parent);

	if (Ret)
	{
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = 0;
		Ret = (CreateDirectory(P, &sa) == TRUE);
		return Ret;
	}
	else
		return false;
}

//TODO:������CString��mfc�������CString����Ҫ����ô��
//ȡ��ָ���ļ����µ��ļ����ļ������ƣ����ݹ飬ֻȡһ��Ŀ¼����ȫ����\\������/��������Դ�\\(�޸���),
// ���ص����ļ���ȫ·��
void YXPFileIO::GetDirectoryFiles(const string &strFolder,
	vector<string> &strVecFileNames,
	bool OnlyFiles,
	bool OnlyDirectories,
	const string &extInclude, //ָ��ֻ�����ĺ�׺������.
	const string &extExclude) //ָ��Ҫ�ų��ĺ�׺������.
{
	USES_CONVERSION;
	if (OnlyFiles&&OnlyDirectories)  //���������ѡ�У���ô���������ļ���ֱ��Ĭ�ϲ�ѡ��
	{
		OnlyFiles = false;
		OnlyDirectories = false;
	}

	strVecFileNames.clear();
	struct _finddata_t filefind;
	string  curr = strFolder + "\\*.*";
	int  done = 0;
	int  handle;
	if ((handle = _findfirst(curr.c_str(), &filefind)) == -1)
		return;

	string tempfolder = strFolder;
	if (strFolder[strFolder.size() - 1] != '\\')
	{
		tempfolder += "\\";
	}

	while (!(done = _findnext(handle, &filefind)))
	{
		if (!strcmp(filefind.name, ".."))  //�ô˷�����һ���ҵ����ļ�����Զ��".."��������Ҫ�����ж�
			continue;
		if (OnlyFiles)
		{
			//TODO::����Ӧ�û�һ�²�Ҫ��A2W,�����ļ��������ܻ�ܵ�
			CString temp = A2W((tempfolder + filefind.name).c_str());
			if (FileExists(temp))
			{
				strVecFileNames.push_back(tempfolder + filefind.name);
			}
		}
		else if (OnlyDirectories)
		{
			CString temp = A2W((tempfolder + filefind.name).c_str());
			if (FolderExists(temp))
			{
				strVecFileNames.push_back(tempfolder + filefind.name);
			}
		}
		else
		{
			strVecFileNames.push_back(tempfolder + filefind.name);
		}

	}
	_findclose(handle);
	if (OnlyDirectories) return; //ֻ����Ŀ¼�Ļ��ͷ�����
	//ȥ������Ҫ�ĺ�׺���ļ�
	if (!extInclude.empty()) 
	{
		for (auto path = strVecFileNames.begin(); path !=strVecFileNames.end();)
		{
			if (!FileExists(A2W((path->c_str()))))
			{
				++path;
				continue; //������Ŀ¼
			}

			if (!CheckFileExt(*path, extInclude)) 
				path = strVecFileNames.erase(path);
			else
				++path;
		}
	}

	//ȥ������Ҫ�ĺ�׺���ļ�
	if (!extExclude.empty())
	{
		for (auto path = strVecFileNames.begin(); path != strVecFileNames.end();)
		{
			if (!FileExists(A2W((path->c_str())))) 
			{
				++path;
				continue; //������Ŀ¼
			}

			if (CheckFileExt(*path, extExclude))
				path = strVecFileNames.erase(path);
			else
				++path;
		}
	}

}

//����һ��Ҫ�ټ�鷵��ֵ�Ƿ�Ϊ��!,�������ǻ������
CString YXPFileIO::BrowseFolder(const HWND owner)
{

	TCHAR wchPath[MAX_PATH];     //���ѡ���Ŀ¼·�� 

	ZeroMemory(wchPath, sizeof(wchPath));
	BROWSEINFO bi;
	bi.hwndOwner = owner;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wchPath;
	bi.lpszTitle = _T("��ѡ����Ҫ�򿪵�Ŀ¼��");
	bi.ulFlags = 0;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;
	//����ѡ��Ŀ¼�Ի���
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);
	CString cStrPath;
	if (lp && SHGetPathFromIDList(lp, wchPath))
	{
		cStrPath.Format(_T("%s"), wchPath);//תΪCstring����
	}

	return cStrPath;
}

void YXPFileIO::DeleteDirectory(const std::string path,bool delFolder)
{
	USES_CONVERSION;
	if (path.empty() || !FolderExists(A2W(path.c_str()))) return;
	vector<string> files;
	GetDirectoryFiles(path, files);
	if (!files.empty()) 
		for (int i = 0; i != files.size(); ++i)
			DeleteFileA(files[i].c_str());
	if(delFolder)
		RemoveDirectoryA(path.c_str());
}

std::string YXPFileIO::GetFileNameNoPath(const std::string &filename) //ֻ֧��\\��Ŀ¼
{
	int pos = filename.rfind("\\");
	return filename.substr(pos + 1, filename.length() - pos);
}
std::string YXPFileIO::GetFileNameNoExt(const std::string &filename) //ֻ�ж�.��λ�ã�����ȥ��·��
{
	int pos = filename.rfind(".");
	return filename.substr(0, pos);
}
std::string YXPFileIO::GetFileNameExt(const std::string &filename) //���ش�.��
{
	int pos = filename.rfind(".");
	return filename.substr(pos, filename.length() - pos);
}

bool YXPFileIO::CheckFileExt(const std::string& Path, const std::string &ext)
{
	if (Path.empty() || ext.empty()) return false;
	string ext_src = GetFileNameExt(Path);

	string ext_dst(ext);

	std::transform(ext_src.begin(), ext_src.end(), ext_src.begin(), ::tolower); //ת����Сд
	std::transform(ext_dst.begin(), ext_dst.end(), ext_dst.begin(), ::tolower); //ת����Сд

	if (ext_dst.compare(ext_src)) return false;
	return true;

}