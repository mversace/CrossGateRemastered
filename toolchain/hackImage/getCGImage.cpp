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
	SAFE_DELETE_A(_imgEncode);
	SAFE_DELETE_A(_imgData);
	SAFE_DELETE_A(_imgPixel);
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
	// 该目录下的调色板实际上只存有236个颜色，对应到图片为16-252
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
				// 直接把调色板读写到数组中
				std::array<unsigned char, DEFAULT_CPG_LEN> c;
				unsigned char *p = c.data();
				if (708 == fread_s(p + 16 * 3, 708, 1, 708, pFile))
				{
					// 将默认的调色板写入 前16后16
					memcpy(p, g_c0_15, 16 * 3);
					memcpy(p + 240 * 3, g_c240_245, 16 * 3);
					_uMapCgp[name] = c;
				}
				else
				{
					// 调色板错误
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
	std::string strPath = _strPath + "bin\\";

	if (0 != fopen_s(&pFile, (strPath + strName).c_str(), "rb"))
		return;

	// 记录错误日志
	std::string strErrorFile = _strPath + "\\data\\";
	Utils::makeSureDirExsits(strErrorFile);
	strErrorFile += "error.log";

	// 生成的文件目录
	std::string strSavePath = _strPath + "\\data\\" + strName.substr(0, strName.find_last_of(".")) + "\\";

	for (auto &ii : _vecImginfo)
	{
		if (getImgData(pFile, ii, strName, strErrorFile))
		{
			std::string strCgp = filleImgPixel(ii.width, ii.height);
			saveImgData(strCgp, strSavePath, ii);
		}
	}

	if (pFile) fclose(pFile);
}

bool CGetCGImage::getImgData(FILE *pFile, const imgInfoHead &imgHead, const std::string &strName, const std::string &strErrorFile)
{
	if (!pFile) return false;

	// 定位到图片位置
	fseek(pFile, imgHead.addr, SEEK_SET);

	// 取出对应图片数据头
	imgData tHead = { 0 };
	int len = sizeof(imgData);
	
	if (len == fread_s(&tHead, len, 1, len, pFile))
	{
		// 这种是错误的图
		if (tHead.width > 5000 || tHead.height > 5000)
		{
			saveLog(LOG_ERROR, strErrorFile, strName, "img w or h error", imgHead, tHead);

			return false;
		}


		_cgpLen = 0; // 调色板长度
		if (tHead.cVer == 3)
		{
			// 多读取4个字节，代表的是调色板的长度
			if (4 != fread_s(&_cgpLen, 4, 1, 4, pFile))
			{
				saveLog(LOG_ERROR, strErrorFile, strName, "read cgpLen error", imgHead, tHead);
				return false;
			}

			len += 4;
		}

		int imgLen = imgHead.len - len;
		if (imgLen == fread_s(_imgEncode, imgLen, 1, imgLen, pFile))
		{
			if (tHead.cVer == 0)
			{
				// 未压缩图片 
				_imgDataIdx = imgLen;
				memcpy(_imgData, _imgEncode, imgLen);
			}
			else if (tHead.cVer == 1 || tHead.cVer == 3)
			{
				// 压缩的图片
				_imgDataIdx = decodeImgData(_imgEncode, imgLen);
				if (_imgDataIdx != tHead.width * tHead.height + _cgpLen)
				{
					// 这种情况按说是错的
					if (_imgDataIdx < tHead.width * tHead.height + _cgpLen)
					{
						saveLog(LOG_ERROR, strErrorFile, strName, "decode len more", imgHead, tHead);
						return false;
					}
					else
					{
						// 大于的话应该算是不够严谨
						saveLog(LOG_INFO, strErrorFile, strName, "decode len less", imgHead, tHead);
					}
				}
			}
		}
	}

	return true;
}

std::string CGetCGImage::filleImgPixel(int w, int h)
{
	std::string strCgpName;

	memset(_imgPixel, 0, sizeof(_imgPixel) * sizeof(unsigned int));

	// 默认使用palet_08.cgp(白天) 调色版
	unsigned char *pCgp = _uMapCgp.begin()->second.data();
	strCgpName = _uMapCgp.begin()->first;
	// 使用图片自带调色板
	if (_cgpLen > 0 && (int)_imgDataIdx >= w * h + _cgpLen)
	{
		pCgp = _imgData + (_imgDataIdx - _cgpLen);
		strCgpName = "self";
	}

	// 图片数据，竖向方向是反的，从最后一行开始
	int imgLen = w * h;
	for (int i = 0; i < imgLen; ++i)
	{
		// 调色板编号
		int cIdx = _imgData[i] * 3;
		int idx = (h - i / w - 1) * w + i % w;

		_imgPixel[idx] = (pCgp[cIdx]) + (pCgp[cIdx + 1] << 8) + (pCgp[cIdx + 2] << 16);
		if (pCgp[cIdx] != 0 || pCgp[cIdx + 1] != 0 || pCgp[cIdx + 2] != 0)
			_imgPixel[idx] |= 0xff000000;
	}

	return std::move(strCgpName);
}

void CGetCGImage::saveImgData(const std::string &cgpName, const std::string &strPath, const imgInfoHead &tHead)
{
	// 存储_vecImgData
	// data/name/cgp/*.png

	FILE *pFile = nullptr;

	// 生成不同的目录，地图文件额外一个目录
	int rangeBegin = tHead.id / 20000;
	std::string strSaveName = strPath;
	if (tHead.tileId == 0)
		strSaveName += std::to_string(rangeBegin * 20000) + "--" + std::to_string(rangeBegin * 20000 + 19999) + "\\";
	else
		strSaveName += "tiled\\" + std::to_string(tHead.tileId) + "_";

	strSaveName += cgpName + "_" + std::to_string(tHead.id);

	Utils::makeSureDirExsits(Utils::extractFileDir(strSaveName));

	CGdiSaveImg::getInstance()->saveImage(_imgPixel, tHead.width, tHead.height, strSaveName, "png");

	std::cout << "createImg: id = " << tHead.id << " name = " << strSaveName << std::endl;
}

int CGetCGImage::decodeImgData(unsigned char *p, int len)
{
	// 图片解密 Run-Length压缩
	int iPos = 0;
	int idx = 0;
	while (iPos < len)
	{
		switch (p[iPos] & 0xF0)
		{
		case 0x00:
		{
			// 0x0n 第二个字节c，代表连续n个字符
			int count = p[iPos] & 0x0F;
			++iPos;
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos++];
		}
		break;
		case 0x10:
		{
			// 0x1n 第二个字节x，第三个字节c，代表n*0x100+x个字符
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 1];
			iPos += 2;
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos++];
		}
		break;
		case 0x20:
		{
			// 0x2n 第二个字节x，第三个字节y，第四个字节c，代表n*0x10000+x*0x100+y个字符
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 1] * 0x100 + p[iPos + 2];
			iPos += 3;
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos++];
		}
		break;
		case 0x80:
		{
			// 0x8n 第二个字节X，代表连续n个X
			int count = p[iPos] & 0x0F;
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos + 1];
			iPos += 2;
		}
		break;
		case 0x90:
		{
			// 0x9n 第二个字节X，第三个字节m，代表连续n*0x100+m个X
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 2];
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos + 1];
			iPos += 3;
		}
		break;
		case 0xa0:
		{
			// 0xan 第二个字节X，第三个字节m，第四个字节z，代表连续n*0x10000+m*0x100+z个X
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 2] * 0x100 + p[iPos + 3];
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = p[iPos + 1];
			iPos += 4;
		}
		break;
		case 0xc0:
		{
			// 0xcn 同0x8n，只不过填充的是背景色
			int count = p[iPos] & 0x0F;
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = 0;
			iPos += 1;
		}
		break;
		case 0xd0:
		{
			// 0xdn 同0x9n，只不过填充的是背景色
			int count = (p[iPos] & 0x0F) * 0x100 + p[iPos + 1];
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = 0;
			iPos += 2;
		}
		break;
		case 0xe0:
		{
			int count = (p[iPos] & 0x0F) * 0x10000 + p[iPos + 1] * 0x100 + p[iPos + 2];
			for (int i = 0; i < count; ++i)
				_imgData[idx++] = 0;
			iPos += 3;
		}
		break;
		default:
			break;
		}
	}

	return idx;
}

void CGetCGImage::saveLog(int logLevel, const std::string &strErrorFile, const std::string &strName, const std::string &strTag,
	const imgInfoHead &tIdxHead, const imgData &tImgData)
{
	std::ostringstream ostr;
	ostr << strTag << " file=" << strName
		<< " id=" << tIdxHead.id << " tileId=" << tIdxHead.tileId
		<< " ver=" << (int)tImgData.cVer << " w=" << tImgData.width << " h=" << tImgData.height
		<< " imgHeadLen=" << tImgData.len << " cgpLen=" << _cgpLen
		<< " decodeLen=" << _imgDataIdx << " expectLen=" << tImgData.width * tImgData.height << "\n";

	Utils::saveError((eLogLevel)logLevel, strErrorFile, ostr.str());
}