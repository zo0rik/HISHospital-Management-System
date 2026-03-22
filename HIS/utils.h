#pragma once
#ifndef UTILS_H
#define UTILS_H

// 安全输入接口
void safeGetString(char* buffer, int size);
int safeGetInt();
double safeGetDouble();

// 数据加载与保存接口
void loadAllData();
void saveRecordToFile(const char* filename, int type, const char* typeName, const char* patientId, const char* staffId, const char* desc, double cost, int isPaid);

#endif
