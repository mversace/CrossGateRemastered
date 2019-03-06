// hackImage.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <chrono>
#include "getCGImage.h"

int main()
{
	auto p0 = std::chrono::high_resolution_clock::now();

	CGetCGImage a;
	a.doRun();

	auto p1 = std::chrono::high_resolution_clock::now();

	std::cout << "use time:" << (float)std::chrono::duration_cast<std::chrono::seconds>(p1 - p0).count() << "秒" << std::endl;

	getchar();

	return 0;
}