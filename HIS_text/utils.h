#pragma once
#ifndef UTILS_H
#define UTILS_H

void safeGetString(char* buffer, int size);
void safeGetPassword(char* buffer, int size);
int safeGetInt();
double safeGetDouble();
int safeGetPositiveInt();
void safeGetGender(char* buffer, int size);
void safeGetPassword(char* buffer, int size); // 新增的密码安全校验组件

void getCurrentTimeStr(char* buffer, int size);
void getCurrentTime(char* buffer, int size);

void loadAllDataFromTxt();
void saveAllDataToTxt();

#endif