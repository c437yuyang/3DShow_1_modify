#pragma once
#include <string>
using namespace std;

class Common
{
public:
	Common();
	~Common();
public:
	string&  replace_all_distinct(string& str, const string& old_value, const string& new_value);//stringÌæ»»º¯Êý
	std::string Common::WChar2Ansi(LPCWSTR pwszSrc);//WCHAR*×ªstring
};

