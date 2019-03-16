#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "nlohmann/json.hpp"

#pragma pack(1)
struct tMapHead
{
	char cFlag[12];
	unsigned int w;
	unsigned int h;
}; // 后续紧跟w*h
#pragma pack()

enum eTiledDataType
{
	TILED_TYPE_TILE = 1,
	TILED_TYPE_OBJECT = 2,
};

class CGetCGMap
{
public:
	CGetCGMap();
	~CGetCGMap();

public:
	static CGetCGMap * getInstance()
	{
		static CGetCGMap a;
		return &a;
	}

	void mapProc();

private:
	void init();
	void readAllTile();
	void clearTempData();
	bool readCGMapData(const std::string &strPath, const std::string &strName);

	// 生成图集json
	void buildTiledData(std::map<int, int> &mapData, const std::string strName, int tileType);

	// 生成地图json
	void buildTileJson();
	void buildObjJson();
	void bulldBlockJson();
	void buildTiledMap();

private:
	std::string _strPath;
	std::string _strReadPath;
	std::string _strTiledMapPath;
	std::string _strErrFile;
	nlohmann::json _tiledFilesJson;

	tMapHead _mapHead;
	std::string _strMapOnlyName;

	std::string _strTileJsonName;	// tile json的相对路径
	nlohmann::json _tileJsonData;	// tile json数据
	std::vector<int> _vTileData;	// 地表信息
	std::map<int, int> _mapTileMap;	// 地表数字映射，映射为tiled map中tile层的新id

	std::string _strObjJsonName;	// obj json的相对路径
	nlohmann::json _objJsonData;	// obj json数据
	std::vector<int> _vObjectData; // 实际就是第二个tiled层
	std::map<int, int> _mapObjectMap; // 物件数字映射，映射为tiled map中object层的新id
};