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
//用于时间格式输入的校验，确保用户输入的时间符合 YYYY-MM-DD 的格式，并且日期合法
void judgetime(char* end) {
    while (1) {
        scanf("%11s", end);
        for (int i = 0;i < 10;i++) {
            if (i == 4 || i == 7) {
                if (end[i] != '-') {
                    printf("格式错误，请重新输入: ");
                    continue;
                }
            }
            else if (end[i] < '0' || end[i]>'9') {
                printf("格式错误，请重新输入: ");
                continue;
            }
        }
        if (end[10] != '\0') {
            printf("格式错误，请重新输入: ");
            continue;
        }
        int year = (end[0] - '0') * 1000 + (end[1] - '0') * 100 + (end[2] - '0') * 10 + (end[3] - '0');
        int month = (end[5] - '0') * 10 + (end[6] - '0');
        int day = (end[8] - '0') * 10 + (end[9] - '0');
        if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0) {
            if (month < 1 || month>12) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && (day < 1 || day>31)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if ((month == 4 || month == 6 || month == 9 || month == 11) && (day < 1 || day>30)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if (month == 2 && (day < 1 || day>29)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
        }
        else {
            if (month < 1 || month>12) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if ((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && (day < 1 || day>31)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if ((month == 4 || month == 6 || month == 9 || month == 11) && (day < 1 || day>30)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
            if (month == 2 && (day < 1 || day>28)) {
                printf("格式错误，请重新输入: ");
                continue;
            }
        }
    }
}