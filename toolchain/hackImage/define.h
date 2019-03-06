#pragma once

#include <unordered_map>
#include <string>

#pragma pack(1)
// *info*.bin 文件格式
struct imgInfoHead
{
	unsigned int id;
	unsigned int addr;  // 在图像文件中的偏移
	unsigned int len;	// 长度
	long xOffset;		// 在游戏内的偏移量x
	long yOffset;		// 在游戏内的偏移量y
	unsigned int width;
	unsigned int height;
	unsigned char tileEast;	// 地图上横向几格
	unsigned char tileSouth;// 竖向几格
	unsigned char flag;
	unsigned char unKnow[5];
	long tileId;			// 所属的地图tile的id
};

// 图像bin 文件格式
struct imgData
{
	unsigned char cName[2];
	unsigned char cVer;	// 1压缩
	unsigned char cUnknow;
	unsigned int width;
	unsigned int height;
	unsigned int len;	// 包含自身头的总长度，后续跟char数组
	unsigned int cgpLen; // 调色板长度
}; // + char* len = size - 16

// 调色板 *.cgp文件格式，708字节，236色
// idx=1的颜色对应游戏内的编号为16，也就是cgp存储16-252色
struct cgpData
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
};

#pragma pack()

// 图片索引与图片库的对照表
__declspec(selectany) std::unordered_map<std::string, std::string> g_ImgMap = {
 	{ "GraphicInfo_20.bin", "Graphic_20.bin" },			// 命运的开启者
// 	{ "GraphicInfo_Joy_22.bin", "Graphic_Joy_22.bin" },	// 乐园之卵 [TODO]
 	{ "GraphicInfoEx_4.bin", "GraphicEx_4.bin" },		// 龙之沙时计
// 	{ "GraphicInfoV3_18.bin", "GraphicV3_18.bin" }, // 乐园之卵（精灵） [TODO]
};