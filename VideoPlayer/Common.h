#pragma once
#include <string>
using namespace std;

class Common
{
public:
	Common();
	~Common();
public:
	string&  replace_all_distinct(string& str, const string& old_value, const string& new_value);//string�滻����
	std::string Common::WChar2Ansi(LPCWSTR pwszSrc);//WCHAR*תstring
};

