#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include "define.h"
#include "nlohmann/json.hpp"

class CGetCGImage
{
public:
	CGetCGImage();
	~CGetCGImage();

public:
	void doRun();

private:
	void clearData();
	void readCgp();
	void readInfo(const std::string &strInfo);
	void readAndSaveImg(const std::string &strName);
	void saveFileJson();

private:
	bool getImgData(FILE *pFile, const imgInfoHead &imgHead, const std::string &strName, const std::string &strErrorFile);
	std::string filleImgPixel(int w, int h);
	void saveImgData(const std::string &cgpName, const std::string &strPath, const imgInfoHead &tHead);

	int decodeImgData(unsigned char *p, int len);
	bool isNewFormat(const std::string &strName)
	{
		if (strName == "Graphic_20.bin" || strName == "GraphicEx_4.bin")
			return false;

		return true;
	}


	void saveLog(int logLevel, const std::string &strErrorFile, const std::string &strName, const std::string &strTag,
		const imgInfoHead &tIdxHead, const imgData &tImgData);

private:
	std::string _strPath;	// 程序路径
	std::unordered_map<std::string, std::array<unsigned char, DEFAULT_CPG_LEN>> _uMapCgp; // 调色板
	std::vector<imgInfoHead> _vecImginfo; // 图片索引

	unsigned char *_imgEncode = new unsigned char[1024 * 1024 + 256 * 3]; // 记录加密后的图片信息

	unsigned char *_imgData = new unsigned char[1024 * 1024 + 256 * 3]; // 记录解密后的图片数据，有的带有调色板，调色板记录在最后
	unsigned int _imgDataIdx = 0;	// 解密之后的idx
	unsigned int _cgpLen = 0;		// 图片中调色板长度

	unsigned int *_imgPixel = new unsigned int[1024 * 1024]; // 记录图片数据 最大支持4M的图片 如果有图片过大，修改这里

	nlohmann::json _tiledFilesJson; // 存储图片的所有信息
};