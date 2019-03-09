/**
* 一些通用的函数封装
*/

#pragma once

#include <gdiplus.h>
#include "utils.h"

#pragma comment(lib, "GdiPlus.lib")


class CGdiSaveImg
{
private:
	ULONG_PTR _gdiplusToken = 0;
	Gdiplus::ImageCodecInfo* _pImageCodecInfo = nullptr;
	UINT _num = 0;
	bool _initSuccess = false;

public:
	CGdiSaveImg()
	{
		_initSuccess = init();
	}

	~CGdiSaveImg()
	{
		Gdiplus::GdiplusShutdown(_gdiplusToken);
		SAFE_DELETE(_pImageCodecInfo);
	}

public:
	static CGdiSaveImg * getInstance()
	{
		static CGdiSaveImg a;
		return &a;
	}

	// 保存图片
	bool saveImage(unsigned int* p, int w, int h, const std::string &strName, const std::string &strExt)
	{
		if (!_initSuccess)
		{
			return false;
		}

		std::wstring wstrName = Utils::StrToWStr(strName);
		std::wstring wstrExt = Utils::StrToWStr(strExt);

		Gdiplus::Bitmap bmp(w, h, PixelFormat32bppARGB);
		int idx = 0;
		for (int row = 0; row < h; ++row)
		{
			for (int col = 0; col < w; ++col)
			{
				bmp.SetPixel(col, row, p[idx++]);
			}
		}

		CLSID encoderClsid;
		std::wstring s = L"image/" + wstrExt;
		if (!GetEncoderClsid(s.c_str(), &encoderClsid))
		{
			return false;
		}

		std::wstring sName = wstrName + L"." + wstrExt;
		bmp.Save(sName.c_str(), &encoderClsid, nullptr);

		return true;
	}

private:
	bool init()
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&_gdiplusToken, &gdiplusStartupInput, nullptr);

		UINT size = 0;
		Gdiplus::GetImageEncodersSize(&_num, &size);
		if (0 == size)
		{
			return false;
		}

		_pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
		if (!_pImageCodecInfo)
		{
			return false;
		}

		Gdiplus::GetImageEncoders(_num, size, _pImageCodecInfo);
		return true;
	}

	bool GetEncoderClsid(const WCHAR* pFormat, CLSID* pClsid)
	{
		bool bfound = FALSE;
		for (unsigned int i = 0; !bfound && i < _num; ++i)
		{
			if (_wcsicmp(_pImageCodecInfo[i].MimeType, pFormat) == 0)
			{
				*pClsid = _pImageCodecInfo[i].Clsid;
				bfound = true;
			}
		}

		return bfound;
	}
};