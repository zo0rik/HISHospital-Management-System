#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include "fileio.h"

void safeGetString(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == ' ') buffer[i] = '_';
            if (buffer[i] == ',') buffer[i] = ';';
        }
    }
    else {
        buffer[0] = '\0';
    }
}

/* 【修改】空输入时循环重新提示，不返回0 */
int safeGetInt() {
    char buffer[100];
    int value;
    char extra;
    while (1) {
        value = 0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        if (sscanf(buffer, "%d%c", &value, &extra) == 1) {
            return value;
        }
        printf("  [!] 输入格式不合法，请重新输入一个有效的整数: ");
    }
}

/* 【修改】空输入时循环重新提示 */
double safeGetDouble() {
    char buffer[100];
    double value;
    char extra;
    while (1) {
        value = 0.0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        if (strcmp(buffer, "-1") == 0) return -1.0; /* 【修改】-1取消 */
        if (strcmp(buffer, "0") == 0 || strcmp(buffer, "0.0") == 0) return 0.0;
        if (sscanf(buffer, "%lf%c", &value, &extra) == 1) {
            if (value < 0) {
                printf("  [!] 金额不能为负数，请重新输入 (输入-1取消): ");
                continue;
            }
            return value;
        }
        printf("  [!] 输入格式不合法，请输入有效的金额数值 (输入-1取消): ");
    }
}

/* 【修改】-1为取消信号 */
int safeGetPositiveInt() {
    int val;
    while (1) {
        val = safeGetInt();
        if (val == -1) return -1;
        if (val > 0) return val;
        printf("  [!] 数值必须为正整数，请重新输入 (输入-1返回): ");
    }
}

void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "-1") == 0) return; /* 【修改】-1取消 */
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) return;
        if (strcmp(buffer, "男") == 0) { strcpy(buffer, "男性"); return; }
        if (strcmp(buffer, "女") == 0) { strcpy(buffer, "女性"); return; }
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入 (输入-1取消): ");
            continue;
        }
        printf("  [!] 性别信息只能填入【男】或【女】，请重新输入 (输入-1取消): ");
    }
}

/* 【修改】密码规则：至少6位 + 仅限字母数字 + -1取消 */
void safeGetPassword(char* buffer, int size) {
    int i, valid;
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "-1") == 0) return; /* 【修改】-1取消 */
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        /* 【新增】密码长度不能少于6位 */
        if (strlen(buffer) < 6) {
            printf("  [!] 密码长度不能少于6位，请重新输入: ");
            continue;
        }
        valid = 1;
        for (i = 0; buffer[i] != '\0'; i++) {
            if (!((buffer[i] >= '0' && buffer[i] <= '9') ||
                (buffer[i] >= 'a' && buffer[i] <= 'z') ||
                (buffer[i] >= 'A' && buffer[i] <= 'Z')))
            {
                valid = 0;
                break;
            }
        }
        if (valid) return;
        printf("  [!] 非法输入：密码只能由【数字】和【字母】组合，不能包含特殊字符，请重输 (输入-1取消): ");
    }
}

void getCurrentTimeStr(char* buffer, size_t size) {
    time_t t;
    struct tm* tm_info;
    t = time(NULL);
    tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}
