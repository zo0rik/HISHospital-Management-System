#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include "fileio.h"

// 密码输入：显示 *
/*void safeprintPassword(char pwd[], int maxLen) {
    int i = 0;
    char ch;
    while (1) {
        ch = _getch(); 
        if (ch == '\r') break;   // 回车结束
        // 退格键处理
        if (ch == '\b') {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
            continue;
        }
        if (i < maxLen - 1) {
            pwd[i++] = ch;
            printf("*");        // 只输出 *
        }
    }
    pwd[i] = '\0';
    printf("\n");
}
*/

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
        // 【核心修复】：不仅要处理空格，还必须消灭英文逗号！
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == ' ') buffer[i] = '_';  // 空格转下划线
            if (buffer[i] == ',') buffer[i] = ';';  // 英文逗号转换为分号，避免CSV解析冲突
        }
    }
    else {
        buffer[0] = '\0';
    }
}

int safeGetInt() {
    char buffer[100];
    int value;
    char extra;
    while (1) {
        value = 0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            return 0;
        }
        if (sscanf(buffer, "%d%c", &value, &extra) == 1) {
            return value;
        }
        printf("  [!] 输入格式不合法，请重新输入一个有效的整数 (输入0返回): ");
    }
}

double safeGetDouble() {
    char buffer[100];
    double value;
    char extra;
    while (1) {
        value = 0.0;
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) return 0.0;
        if (strcmp(buffer, "0") == 0 || strcmp(buffer, "0.0") == 0) return 0.0;
        if (sscanf(buffer, "%lf%c", &value, &extra) == 1) {
            if (value < 0) {
                printf("  [!] 金额不能为负数，请重新输入 (输入0取消): ");
                continue;
            }
            return value;
        }
        printf("  [!] 输入格式不合法，请输入有效的金额数值 (输入0取消): ");
    }
}

int safeGetPositiveInt() {
    int val;
    while (1) {
        val = safeGetInt();
        if (val > 0) return val;
        if (val == 0) return 0;
        printf("  [!] 数值不能为负数，请重新输入有效正整数 (输入0返回): ");
    }
}

void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "0") == 0) return;
        if (strcmp(buffer, "男性") == 0 || strcmp(buffer, "女性") == 0) {
            return;
        }
        if (strcmp(buffer, "男") == 0) { strcpy(buffer, "男性"); return; }
        if (strcmp(buffer, "女") == 0) { strcpy(buffer, "女性"); return; }
        if (strlen(buffer) == 0) continue;
        printf("  [!] 性别信息只能填入【男】或【女】，请重新输入 (输入0取消): ");
    }
}

// 严格的密码规则约束器 
void safeGetPassword(char* buffer, int size) {
    int i, valid;
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "0") == 0) return;
        if (strlen(buffer) == 0) continue;

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
        printf("  [!] 非法输入：密码只能由【数字】和【字母】组合，不能包含特殊字符，请重输 (输入0取消): ");
    }
}

void getCurrentTimeStr(char* buffer, size_t size) {
    time_t t;
    struct tm* tm_info;
    t = time(NULL);
    tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}


