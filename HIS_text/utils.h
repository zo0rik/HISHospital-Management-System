#pragma once
#ifndef UTILS_H
#define UTILS_H

// 基础安全输入接口
void safeGetString(char* buffer, int size);
void safeGetPassword(char* buffer, int size);
int safeGetInt();
double safeGetDouble();
int safeGetPositiveInt();
void safeGetGender(char* buffer, int size);

// 时间与数据处理接口
void getCurrentTimeStr(char* buffer, int size);
void getCurrentTime(char* buffer, int size); // 兼容旧模块调用的包装函数

// 全局数据流转接口
void loadAllDataFromTxt();
void saveAllDataToTxt();

#endif