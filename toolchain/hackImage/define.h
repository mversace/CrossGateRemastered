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
}; // + char* len = size - 16

#pragma pack()

#define MAX_IMG_SIZE (1024*1024)

#define DEFAULT_CPG_LEN 768		// 调色板文件长度256色，每个颜色3个字节存储BGR
// 游戏指定的调色板0-15 BGR
__declspec(selectany) unsigned char g_c0_15[] = {
	0x00, 0x00, 0x00,
	0x80, 0x00, 0x00,
	0x00, 0x80, 0x00,
	0x80, 0x80, 0x00,
	0x00, 0x00, 0x80,
	0x80, 0x00, 0x80,
	0x00, 0x80, 0x80,
	0xc0, 0xc0, 0xc0,
	0xc0, 0xdc, 0xc0,
	0xa6, 0xca, 0xf0,
	0xde, 0x00, 0x00,
	0xff, 0x5f, 0x00,
	0xff, 0xff, 0xa0,
	0x00, 0x5f, 0xd2,
	0x50, 0xd2, 0xff,
	0x28, 0xe1, 0x28,
};
// 游戏指定的调色板240-255 BGR
__declspec(selectany) unsigned char g_c240_245[] = {
	0xf5, 0xc3, 0x96,
	0x1e, 0xa0, 0x5f,
	0xc3, 0x7d, 0x46,
	0x9b, 0x55, 0x1e,
	0x46, 0x41, 0x37,
	0x28, 0x23, 0x1e,
	0xff, 0xfb, 0xf0,
	0x3a, 0x6e, 0x5a,
	0x80, 0x80, 0x80,
	0xff, 0x00, 0x00,
	0x00, 0xff, 0x00,
	0xff, 0xff, 0x00,
	0x00, 0x00, 0xff,
	0xff, 0x80, 0xff,
	0x00, 0xff, 0xff,
	0xff, 0xff, 0xff,
};

// 图片索引与图片库的对照表
// 已经包含所有图片库
// Anime开头的相当于是指定以下库的动作，相当于是配置文件，这个就不解析了，必要性不大
__declspec(selectany) std::unordered_map<std::string, std::string> g_ImgMap = {
	{ "GraphicInfo_20.bin", "Graphic_20.bin" },			// 命运的开启者
	{ "GraphicInfo_Joy_22.bin", "Graphic_Joy_22.bin" },	// 乐园之卵
	{ "GraphicInfoEx_4.bin", "GraphicEx_4.bin" },		// 龙之沙时计
	{ "GraphicInfoV3_18.bin", "GraphicV3_18.bin" }, // 乐园之卵（精灵
	{ "Puk2\\GraphicInfo_PUK2_2.bin", "Puk2\\Graphic_PUK2_2.bin"},
	{ "Puk3\\GraphicInfo_PUK3_1.bin", "Puk3\\Graphic_PUK3_1.bin" },
};