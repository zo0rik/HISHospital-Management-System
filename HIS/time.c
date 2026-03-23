#define _CRT_SECURE_NO_WARNINGS
#include "time.h"
#include <stdio.h>

void getCurrentTime(char* buffer, int size) {
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}