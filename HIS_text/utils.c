#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include "models.h"
#include "utils.h"
#include "fileio.h"

// 去掉字符串首尾空格
static void trimSpaces(char* s) {
    char* start;
    char* end;
    if (!s) return;

    start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) {
        *(end - 1) = '\0';
        end--;
    }
}

// 判断字符串是否最多保留两位小数
static int hasAtMostTwoDecimals(const char* s) {
    const char* dot = strchr(s, '.');
    int decimals = 0;
    if (!dot) return 1;

    dot++;
    while (*dot) {
        if (!isdigit((unsigned char)*dot)) return 0;
        decimals++;
        dot++;
    }
    return decimals <= 2;
}

// 安全读取字符串，自动去掉换行和首尾空格
void safeGetString(char* buffer, int size) {
    if (!buffer || size <= 0) return;

    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        trimSpaces(buffer);

        // 把逗号替换成分号，避免影响文件存储格式
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] == ',') buffer[i] = ';';
        }
    }
    else {
        buffer[0] = '\0';
        clearerr(stdin);
    }
}

// 严格解析整数
static int tryParseIntStrict(const char* buffer, int* outValue) {
    char* endptr;
    long value;
    if (!buffer || !outValue) return 0;

    errno = 0;
    value = strtol(buffer, &endptr, 10);

    if (buffer == endptr || *endptr != '\0' || errno == ERANGE) return 0;
    if (value < -2147483647L - 1L || value > 2147483647L) return 0;

    *outValue = (int)value;
    return 1;
}

// 安全读取整数
int safeGetInt(void) {
    char buffer[SAFE_INPUT_BUFFER_SIZE];
    int value;

    while (1) {
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        if (tryParseIntStrict(buffer, &value)) return value;
        printf("  [!] 输入格式不合法，请重新输入一个有效的整数: ");
    }
}

// 读取指定范围内的整数
int safeGetIntInRange(int minValue, int maxValue, const char* promptOnError) {
    int value;
    while (1) {
        value = safeGetInt();
        if (value >= minValue && value <= maxValue) return value;

        if (promptOnError && strlen(promptOnError) > 0) printf("%s", promptOnError);
        else printf("  [!] 输入超出允许范围，请重新输入: ");
    }
}

// 读取正整数，允许输入 -1 返回
int safeGetPositiveInt(void) {
    int val;
    while (1) {
        val = safeGetInt();
        if (val == -1) return -1;
        if (val > 0) return val;
        printf("  [!] 数值必须为正整数，请重新输入 (输入-1返回): ");
    }
}

// 读取药品数量
int safeGetDrugQuantity(void) {
    int quantity;
    while (1) {
        quantity = safeGetInt();
        if (quantity == -1) return -1;
        if (quantity <= 0) {
            printf("  [!] 药品数量必须为正整数，请重新输入 (输入-1返回): ");
            continue;
        }
        if (quantity > DEFAULT_DRUG_QUANTITY_MAX) {
            printf("  [!] 药品数量超出允许范围，请重新输入 (最大%d，输入-1返回): ", DEFAULT_DRUG_QUANTITY_MAX);
            continue;
        }
        return quantity;
    }
}

// 严格解析金额
static int tryParseMoneyStrict(const char* buffer, double* outValue) {
    char* endptr;
    double value;
    if (!buffer || !outValue) return 0;
    if (!hasAtMostTwoDecimals(buffer)) return 0;

    errno = 0;
    value = strtod(buffer, &endptr);

    if (buffer == endptr || *endptr != '\0' || errno == ERANGE) return 0;
    if (!isfinite(value)) return 0;

    *outValue = value;
    return 1;
}

// 安全读取金额
double safeGetMoney(void) {
    char buffer[SAFE_INPUT_BUFFER_SIZE];
    double value;

    while (1) {
        safeGetString(buffer, sizeof(buffer));
        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        if (strcmp(buffer, "-1") == 0) return -1.0;
        if (!tryParseMoneyStrict(buffer, &value)) {
            printf("请输入正确的金额格式，金额最多保留两位小数。\n");
            continue;
        }
        return value;
    }
}

// 读取指定范围内的金额
double safeGetMoneyInRange(double minValue, double maxValue) {
    double value;
    while (1) {
        value = safeGetMoney();
        if (value == -1.0) return -1.0;

        if (value < minValue || value > maxValue) {
            if (fabs(minValue) < 1e-9 && fabs(maxValue - 10000.0) < 1e-9) {
                printf("单次充值金额不能超过10000元。\n");
            }
            else {
                char minBuf[32], maxBuf[32];
                formatMoney(minValue, minBuf, sizeof(minBuf));
                formatMoney(maxValue, maxBuf, sizeof(maxBuf));
                printf("  [!] 金额超出允许范围，请输入 %s ~ %s 之间的金额。\n", minBuf, maxBuf);
            }
            continue;
        }
        return value;
    }
}

// 读取 double，本质上复用金额输入
double safeGetDouble(void) {
    return safeGetMoney();
}

// 把金额格式化成 xx.xx元
void formatMoney(double amount, char* buffer, size_t size) {
    if (!buffer || size == 0) return;
    snprintf(buffer, size, "%.2f元", amount);
}

// 安全读取性别
void safeGetGender(char* buffer, int size) {
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "-1") == 0) return;

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

// 安全读取密码
void safeGetPassword(char* buffer, int size) {
    int i, valid;
    while (1) {
        safeGetString(buffer, size);
        if (strcmp(buffer, "-1") == 0) return;

        if (strlen(buffer) == 0) {
            printf("  [!] 输入不能为空，请重新输入: ");
            continue;
        }
        if (strlen(buffer) < 6) {
            printf("  [!] 密码长度不能少于6位，请重新输入: ");
            continue;
        }

        valid = 1;
        for (i = 0; buffer[i] != '\0'; i++) {
            if (!((buffer[i] >= '0' && buffer[i] <= '9') ||
                (buffer[i] >= 'a' && buffer[i] <= 'z') ||
                (buffer[i] >= 'A' && buffer[i] <= 'Z'))) {
                valid = 0;
                break;
            }
        }

        if (valid) return;
        printf("  [!] 非法输入：密码只能由【数字】和【字母】组合，不能包含特殊字符，请重输 (输入-1取消): ");
    }
}

// 获取当前时间字符串
void getCurrentTimeStr(char* buffer, size_t size) {
    time_t t;
    struct tm* tm_info;
    t = time(NULL);
    tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d_%H:%M:%S", tm_info);
}