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

	//TODO:ȫ������std::string�İ汾
	//GetDirectoryFiles��������ѡ��

	//���в���ʹ��A�汾���Լ�std::string

public:
	static bool FindOrMkDir(const std::string& pszPath);
	static bool FolderExists(const std::string& s);

	static bool FileExists(const std::string& s);
	static bool RecurMkDir(const std::string& path);
	static void GetDirectoryFiles(const std::string &strFolder,
		std::vector<std::string> &strVecFileNames,
		bool OnlyFiles = false,
		bool OnlyDirectories = false,
		const std::string &extInclude = "",
		const std::string &extExclude = "");
	static std::string YXPFileIO::BrowseFolder(const std::string & title = "��ѡ��Ҫ�򿪵�Ŀ¼", const HWND owner = NULL);
	static void DeleteDirectory(const std::string path, bool deleteFolder = false);
	static std::string GetFileNameNoPath(const std::string &filename); //ֻ֧��\\��Ŀ¼

	static std::string GetFileNameNoExt(const std::string &filename); //ֻ�ж�.��λ�ã�����ȥ��·��
	static std::string GetFileNameExt(const std::string &filename);
	static	bool CheckFileExt(const std::string& Path, const std::string &ext);
	static bool Rename(const std::string &path, const std::string &dst,bool IsExistReplace=false);
	static std::string GetAppPath();
	static bool CopyDirFiles(const std::string&src, const std::string &dst,bool IsExistReplace=false);
};

