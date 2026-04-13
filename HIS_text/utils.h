#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define SAFE_INPUT_BUFFER_SIZE 256
#define DEFAULT_DRUG_QUANTITY_MAX 1000000

void safeGetString(char* buffer, int size);
void safeGetPassword(char* buffer, int size);
int safeGetInt(void);
int safeGetIntInRange(int minValue, int maxValue, const char* promptOnError);
int safeGetPositiveInt(void);
int safeGetDrugQuantity(void);
double safeGetDouble(void);
double safeGetMoney(void);
double safeGetMoneyInRange(double minValue, double maxValue);
void formatMoney(double amount, char* buffer, size_t size);
void safeGetGender(char* buffer, int size);

void getCurrentTimeStr(char* buffer, size_t size);
void getCurrentTime(char* buffer, size_t size);

#endif
