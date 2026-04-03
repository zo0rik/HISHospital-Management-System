#define _CRT_SECURE_NO_WARNINGS
#include "time_t.h"
#include <stdio.h>
#include <time.h>

// ---------------------------------------------------------
// 获取当前系统日期，格式：YYYY-MM-DD
// ---------------------------------------------------------
void getCurrentTime(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d", tm_info);
}

// ---------------------------------------------------------
// 时间格式校验：强制输入合法的 YYYY-MM-DD
// ---------------------------------------------------------
void judgetime(char* end) {
    while (1) {
        scanf("%11s", end);

        // 必须是10个字符
        if (strlen(end) != 10) {
            printf("格式错误，请重新输入 (YYYY-MM-DD)：");
            continue;
        }

        // 检查分隔符
        if (end[4] != '-' || end[7] != '-') {
            printf("格式错误，请重新输入 (YYYY-MM-DD)：");
            continue;
        }

        // 检查是否都是数字
        int valid = 1;
        for (int i = 0; i < 10; i++) {
            if (i == 4 || i == 7) continue;
            if (end[i] < '0' || end[i] > '9') {
                valid = 0;
                break;
            }
        }
        if (!valid) {
            printf("格式错误，请重新输入 (YYYY-MM-DD)：");
            continue;
        }

        // 解析年月日
        int year = (end[0] - '0') * 1000 + (end[1] - '0') * 100 + (end[2] - '0') * 10 + (end[3] - '0');
        int month = (end[5] - '0') * 10 + (end[6] - '0');
        int day = (end[8] - '0') * 10 + (end[9] - '0');

        // 检查月份
        if (month < 1 || month > 12) {
            printf("月份无效，请重新输入：");
            continue;
        }

        // 检查日期
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

        if (day < 1 || day > max_day) {
            printf("日期无效，请重新输入：");
            continue;
        }

        // 校验通过
        break;
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
    int days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2 && isLeapYear(year)) return 29;
    return days[month - 1];
}

// ---------------------------------------------------------
// 根据当前日期，获取 N 天前的日期
// ---------------------------------------------------------
void getPastDateAccurate(char* current, char* result, int days_ago) {
    int y, m, d;
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

    sprintf(result, "%04d-%02d-%02d", y, m, d);
}