#include <iostream>
#include <windows.h>
#include <io.h>
#include <sstream>
#include "getCGImage.h"
#include "gdiImg.h"

CGetCGImage::CGetCGImage()
{
	// 获取程序路径
	char szFullPath[MAX_PATH] = { 0 };
	::GetModuleFileNameA(nullptr, szFullPath, MAX_PATH);
	_strPath = szFullPath;
	_strPath.erase(_strPath.begin() + _strPath.find_last_of("\\") + 1, _strPath.end());
}

CGetCGImage::~CGetCGImage()
{

}

void CGetCGImage::doRun()
{
	readCgp();
	for (auto &item : g_ImgMap)
	{
		readInfo(item.first);
		readAndSaveImg(item.second);
		_vecImginfo.clear();
	}
}

void CGetCGImage::clearData()
{
	_uMapCgp.clear();
	_vecImginfo.clear();
}

void CGetCGImage::readCgp()
{
	// 遍历pal目录下的所有文件
	FILE *pFile = nullptr;
	intptr_t hFile = 0;
	struct _finddata_t fileinfo;
	std::string strPath = _strPath + "\\bin\\pal\\";

	if ((hFile = _findfirst((strPath + "*").c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (fileinfo.attrib & _A_SUBDIR) continue;

			std::string name = fileinfo.name;
			if (0 == fopen_s(&pFile, (strPath + name).c_str(), "rb"))
			{
				cgpData tData = { 0 };
				int len = sizeof(cgpData);
				while (len == fread_s(&tData, len, 1, len, pFile))
				{
					// int格式为ARGB，因为是小端模式，所以存储的字节为BGRA
					unsigned int color = (tData.r << 16) + (tData.g << 8) + tData.b;
					if (tData.b != 0 || tData.g != 0 || tData.r != 0)
						color |= 0xff000000;
					_uMapCgp[name].push_back(color);
				}
				
			}

			if (pFile) fclose(pFile);

		} while (_findnext(hFile, &fileinfo) == 0);
	}
}

void CGetCGImage::readInfo(const std::string &strInfo)
{
	FILE *pFile = nullptr;
	std::string strPath = _strPath + "\\bin\\";
	if (0 == fopen_s(&pFile, (strPath + strInfo).c_str(), "rb"))
	{
		imgInfoHead tHead = { 0 };
		int len = sizeof(imgInfoHead);
		while (len == fread_s(&tHead, len, 1, len, pFile))
			_vecImginfo.push_back(tHead);
	}
	if (pFile) fclose(pFile);

	std::cout << "readImgInfo: " << strInfo << " end" << std::endl;
}

void CGetCGImage::readAndSaveImg(const std::string &strName)
{
	// 读取图片
	FILE *pFile = nullptr;
	std::string strPath = _strPath + "\\bin\\";

	if (0 != fopen_s(&pFile, (strPath + strName).c_str(), "rb"))
		return;

	std::string strErrorFile = _strPath + "\\data\\";
	Utils::makeSureDirExsits(strErrorFile);
	strErrorFile += "error.log";

	// 基础目录，每个文件对应所有调色板生成一个文件夹
	std::string strSavePath = _strPath + "\\data\\" + strName.substr(0, strName.find_last_of("."))
		+ "\\"; // + cgp.first.substr(0, cgp.first.find_last_of(".")) + "\\";

	// 如果有自己的调色板，则使用，否则使用第一个调色板
	std::string cgpName;
	std::vector<int>* pCgpData = nullptr;
	if (_uMapCgp.find(strName) != _uMapCgp.end())
	{
		cgpName = strName;
		pCgpData = &_uMapCgp[strName];
	}
	else
	{
		cgpName = _uMapCgp.begin()->first;
		pCgpData = &_uMapCgp.begin()->second;
	}

	cgpName = cgpName.substr(0, cgpName.find_last_of("."));

	for (auto &ii : _vecImginfo)
	{
		// 取出对应图片
		imgData tHead = { 0 };
		int len = sizeof(imgData);

		// 旧格式的图片头少4个字节
		if (!isNewFormat(strName))
			len -= 4;

		fseek(pFile, ii.addr, SEEK_SET);
		if (len == fread_s(&tHead, len, 1, len, pFile))
		{
			bool bEncoded = true;
			// tHead.len != ii.len这个实际上是未压缩的图
			if (tHead.len != ii.len) 
			{
				bEncoded = false;
				tHead.len = ii.len;
			}

			// 这种是错误的图
			if (tHead.width > 5000 || tHead.height > 5000)
			{
				tHead.len = ii.len;

				std::ostringstream ostr;
				ostr << "image data Error file=" << strName << " id=" << ii.id << " thead=[" << tHead.width << ","
					<< tHead.height << "," << tHead.len << "] ii=[" << ii.width << "," << ii.height << "," << ii.len << "]\n";
				Utils::saveError(strErrorFile, ostr.str());

				continue;
			}

			int realLen = tHead.width * tHead.height;
			// 有图库中数据有些许错误，居然会多出几个字节，做个容错
			std::vector<unsigned char> vData(realLen + 10, 0);
			int imgLen = tHead.len - len;
			unsigned char* p = new unsigned char[imgLen];
			if (imgLen == fread_s(p, imgLen, 1, imgLen, pFile))
			{
				if (bEncoded)
				{
					decodeImgData(p, imgLen, tHead.cgpLen, vData, realLen);
					vData.erase(vData.end() - 10, vData.end());
				}
				else
				{
					// 未压缩的图库
					vData.assign(p, p + imgLen);
					// 居然是横向翻折。。。
					// 调色板也不对，以后再说吧
					for (unsigned int i = 0; i < tHead.height; ++i)
					{
						auto iBegin = vData.begin() + i * tHead.width;
						reverse(iBegin, iBegin + tHead.width);
					}
				}

				saveImgData(cgpName, pCgpData, strSavePath, ii, vData);
			}
			SAFE_DELETE_A(p);
		}
	}

	if (pFile) fclose(pFile);
}

void CGetCGImage::saveImgData(const std::string &cgpName, std::vector<int>* pCgpData, const std::string &strPath, const imgInfoHead &tHead, const std::vector<unsigned char> &vPixes)
{
	// 存储_vecImgData
	// data/name/cgp/*.png

	FILE *pFile = nullptr;

	// 根据调色板生成每个像素用unsigned int保存
	int len = tHead.width * tHead.height;
	unsigned int* p = new unsigned int[len];
	memset(p, 0, len * 4);

	unsigned int* pTemp = p + len - 1;
	for (auto pixel : vPixes)
	{
		if (pixel >= 16 && pixel <= 240)
			*pTemp = (*pCgpData)[pixel - 16];

		--pTemp;
	}

	// 生成不同的目录，地图文件额外一个目录
	int rangeBegin = tHead.id / 20000;
	std::string strSaveName = strPath;
	if (tHead.tileId == 0)
		strSaveName += std::to_string(rangeBegin * 20000) + "--" + std::to_string(rangeBegin * 20000 + 19999) + "\\";
	else
		strSaveName += "tiled\\" + std::to_string(tHead.tileId) + "_";

	strSaveName += cgpName + "_" + std::to_string(tHead.id);

	Utils::makeSureDirExsits(Utils::extractFileDir(strSaveName));

	CGdiSaveImg::getInstance()->saveImage(p, tHead.width, tHead.height, strSaveName, "png");

	std::cout << "createImg: id = " << tHead.id << " name = " << strSaveName << std::endl;

	SAFE_DELETE_A(p);
}

int CGetCGImage::decodeImgData(unsigned char *p, int len, int cgpLen, std::vector<unsigned char>& v, int realLen)
{
	// 图片解密 Run-Length压缩
	int iPos = 0;
	int vi = 0;
	std::vector<unsigned char> vvv(p, p + len);
	int imgLen = len - cgpLen;
	while (iPos < imgLen && vi < realLen)
	{
		switch (p[iPos] & 0xF0)
		{
		case 0x00:
		{
			// 0x0n 第二个字节c，代表连续n个字符
			int count = p[iPos] & 0x0F;
			++iPos;
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos++];
		}
		break;
		case 0x10:
		{
			// 0x1n 第二个字节x，第三个字节c，代表n*0x100+x个字符
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 1];
			iPos += 2;
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos++];
		}
		break;
		case 0x20:
		{
			// 0x2n 第二个字节x，第三个字节y，第四个字节c，代表n*0x10000+x*0x100+y个字符
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 1] * 0x100 + p[iPos + 2];
			iPos += 3;
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos++];
		}
		break;
		case 0x80:
		{
			// 0x8n 第二个字节X，代表连续n个X
			int count = p[iPos] & 0x0F;
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos + 1];
			iPos += 2;
		}
		break;
		case 0x90:
		{
			// 0x9n 第二个字节X，第三个字节m，代表连续n*0x100+m个X
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 2];
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos + 1];
			iPos += 3;
		}
		break;
		case 0xa0:
		{
			// 0xan 第二个字节X，第三个字节m，第四个字节z，代表连续n*0x10000+m*0x100+z个X
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 2] * 0x100 + p[iPos + 3];
			for (int i = 0; i < count; ++i)
				v[vi++] = p[iPos + 1];
			iPos += 4;
		}
		break;
		case 0xc0:
		{
			// 0xcn 同0x8n，只不过填充的是背景色
			int count = p[iPos] & 0x0F;
			for (int i = 0; i < count; ++i)
				v[vi++] = 0;
			iPos += 1;
		}
		break;
		case 0xd0:
		{
			// 0xdn 同0x9n，只不过填充的是背景色
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 1];
			for (int i = 0; i < count; ++i)
				v[vi++] = 0;
			iPos += 2;
		}
		break;
		case 0xe0:
		{
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 1] * 0x100 + p[iPos + 2];
			for (int i = 0; i < count; ++i)
				v[vi++] = 0;
			iPos += 3;
		}
		break;
		default:
			break;
		}
	}

	return vi;
}