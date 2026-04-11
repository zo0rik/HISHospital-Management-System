#define _CRT_SECURE_NO_WARNINGS
#include "time_t.h"   // 
#include "utils.h"    // 【新增】引入安全输入库
#include <stdio.h>
#include <time.h>
#include <string.h>

// ---------------------------------------------------------
// 获取当前系统日期，格式：YYYY-MM-DD
// ---------------------------------------------------------
void getCurrentTime(char* buffer, size_t size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d", tm_info);
}

// ---------------------------------------------------------
// 获取当前系统日期，格式：YYYY-MM-DD_HH:MM:SS
// ---------------------------------------------------------
void getCurrentDate(char* buffer, size_t size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}

// ---------------------------------------------------------
// 时间格式校验：强制输入合法的 YYYY-MM-DD
// ---------------------------------------------------------
void judgetime(char* end) {
    while (1) {
        char temp[50];  // 临时接收，留足冗余空间

        // 【修改点】：使用安全输入函数，彻底替代 fflush(stdin) 和 scanf
        safeGetString(temp, sizeof(temp));

        // 【修改点】：增加取消机制，允许输入 0 返回
        if (strcmp(temp, "0") == 0) {
            strcpy(end, "0");
            return;
        }

        // 先解析成数字
        int year, month, day;
        if (sscanf(temp, "%d-%d-%d", &year, &month, &day) != 3) {
            printf("  [!] 格式错误，请重新输入 (YYYY-MM-DD)：");
            continue;
        }

        // 检查月份合法性
        if (month < 1 || month > 12) {
            printf("  [!] 月份无效，请重新输入 (01~12)：");
            continue;
        }

        // 计算当前年份月份的最大天数
        int max_day;
        if (month == 2) {
            if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                max_day = 29;
            else
                max_day = 28;
        }
        else if (month == 4 || month == 6 || month == 9 || month == 11) {
            max_day = 30;
        }
        else {
            max_day = 31;
        }

        // 检查日期合法性
        if (day < 1 || day > max_day) {
            printf("  [!] 日期无效，请重新输入该月的正确天数：");
            continue;
        }

        // 【优化点】：既然数字提取和合法性都过了，直接格式化成标准 10 位字符串。
        // sprintf 自然会补齐前导零（如 2025-5-2 -> 2025-05-02），不再需要手动去查 '-' 的位置。
        sprintf(end, "%04d-%02d-%02d", year, month, day);
        break; // 校验彻底通过，跳出循环
    }
}

// ---------------------------------------------------------
// 判断闰年
// ---------------------------------------------------------
int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// ---------------------------------------------------------
// 获取指定月份的天数
// ---------------------------------------------------------
int getDaysInMonth(int year, int month) {
    int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// ---------------------------------------------------------
// 根据当前日期，获取 N 天前的日期
// ---------------------------------------------------------
// 安全版：不会栈溢出、不会崩溃、日期计算正确
void getPastDateAccurate(char* current, char* result, int days_ago) {
    int y, m, d;
    // 提取年月日 (防止 time 字符串自带的小时分钟干扰，只读前10位)
    sscanf(current, "%4d-%2d-%2d", &y, &m, &d);

    while (days_ago-- > 0) {
        d--;
        if (d > 0) continue;

        m--;
        if (m > 0) {
            d = getDaysInMonth(y, m);
        }
        else {
            y--;
            m = 12;
            d = 31;
        }
    }

    snprintf(result, 30, "%04d-%02d-%02d", y, m, d);
}