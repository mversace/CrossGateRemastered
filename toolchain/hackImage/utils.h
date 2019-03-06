/**
 * 一些通用的函数封装
 */

#pragma once

#include <windows.h>
#include <stdio.h>
#include <iostream>

#define SAFE_DELETE(p) { if (p) {delete p; p = nullptr; }}
#define SAFE_DELETE_A(p) { if (p) {delete[] p; p = nullptr; }}

namespace Utils
{
	// 去掉之后的'\\'
	inline std::string extractFileDir(const std::string& strFullPath)
	{
		std::string strDir;
		int nPos = (int)strFullPath.rfind('\\');
		if (nPos != -1)
			strDir = strFullPath.substr(0, nPos);

		return std::move(strDir);
	}

	// 检测目录是否存在
	inline bool dirExists(LPCSTR szDir)
	{
		DWORD dwRet = GetFileAttributesA(szDir);
		return ((dwRet != 0xFFFFFFFF) && ((FILE_ATTRIBUTE_DIRECTORY & dwRet) != 0));
	}

	// 确保文件夹存在，没有则创建
	inline void makeSureDirExsits(const std::string& strDir)
	{
		if (Utils::dirExists(strDir.c_str()))
			return;

		// 递归处理
		Utils::makeSureDirExsits(extractFileDir(strDir));
		::CreateDirectoryA(strDir.c_str(), nullptr);
	}

	// 保存错误信息
	inline void saveError(const std::string &strFile, const std::string &err)
	{
		FILE *pFile = nullptr;
		if (0 != fopen_s(&pFile, strFile.c_str(), "a"))
			return;

		fwrite(err.c_str(), 1, err.length(), pFile);
		fclose(pFile);
	}

	// 窄字符转宽字符
	inline std::wstring StrToWStr(const std::string& str, UINT dwCodePage = CP_THREAD_ACP)
	{
		std::wstring strDest;
		int nSize = ::MultiByteToWideChar(dwCodePage, 0, str.c_str(), -1, 0, 0);
		if (nSize > 0)
		{
			WCHAR* pwszDst = new WCHAR[nSize];
			::MultiByteToWideChar(dwCodePage, 0, str.c_str(), -1, pwszDst, nSize);
			strDest = pwszDst;
			SAFE_DELETE_A(pwszDst);
		}
		return strDest;
	}
};