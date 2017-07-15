#pragma once
#include <io.h>
#include <string>
#include <vector>
#include <algorithm>

//���е���windows API�ĺ������صĶ���\\

class YXPFileIO
{
public:
	YXPFileIO();
	~YXPFileIO();


public:
	static bool FindOrCreateDirectory(const char *pszPath);
	static bool FolderExists(const CString &s);
	static bool FileExists(const CString &s);
	static bool SuperMkDir(const CString &P);
	static void GetDirectoryFiles(const std::string &strFolder,
		std::vector<std::string> &strVecFileNames,
		bool OnlyFiles = false,
		bool OnlyDirectories = false,
		const std::string &extInclude = "",
		const std::string &extExclude = "");
	static CString BrowseFolder(const HWND owner = NULL);
	static void DeleteDirectory(const std::string path,bool deleteFolder=false);
	static std::string GetFileNameNoPath(const std::string &filename); //ֻ֧��\\��Ŀ¼

	static std::string GetFileNameNoExt(const std::string &filename); //ֻ�ж�.��λ�ã�����ȥ��·��
	static std::string GetFileNameExt(const std::string &filename);
	static	bool CheckFileExt(const std::string& Path, const std::string &ext);


};

