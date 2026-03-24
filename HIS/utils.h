#pragma once
#ifndef UTILS_H
#define UTILS_H

// 安全输入函数声明 (替代 scanf 防止死循环)
void safeGetString(char* buffer, int size);
int safeGetInt();
double safeGetDouble();

// 数据读写接口声明
void loadAllDataFromTxt();
void saveAllDataToTxt();

#endif