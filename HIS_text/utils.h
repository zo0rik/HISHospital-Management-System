#pragma once
#ifndef UTILS_H
#define UTILS_H

void loadAllDataFromTxt();
void saveAllDataToTxt();

// 安全与防呆输入接口
void safeGetString(char* buffer, int size);
int safeGetInt();
double safeGetDouble();
int safeGetPositiveInt(); // 防呆: 必须输入正整数
void safeGetGender(char* buffer, int size); // 防呆: 只能输入"男性"或"女性"

void getCurrentTimeStr(char* buffer, int size); // 获取无空格的时间戳

#endif