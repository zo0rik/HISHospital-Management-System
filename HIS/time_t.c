#define _CRT_SECURE_NO_WARNINGS
#include "time_t.h"
#include <stdio.h>
#include <time.h> // 必须引入C标准时间库

// ---------------------------------------------------------
// 获取当前系统的实时时间，并格式化为标准字符串
// @param buffer: 用于存放生成字符串的字符数组指针
// @param size:   buffer 的最大可用长度，防止溢出越界
// ---------------------------------------------------------
void getCurrentTime(char* buffer, int size) {
    // 1. 获取自 Unix 纪元（1970年）以来的秒数
    time_t t = time(NULL);

    // 2. 将秒数转换为本地时区的日期和时间结构体 (tm)
    struct tm* tm_info = localtime(&t);

    // 3. 将 tm 结构体内的离散时间格式化为符合 YYYY-MM-DD HH:MM:SS 的标准串
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}