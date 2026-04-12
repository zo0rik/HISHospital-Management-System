#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>


void safeGetString(char* buffer, int size);
void safeGetPassword(char* buffer, int size);
int safeGetInt();
double safeGetDouble();
int safeGetPositiveInt();
void safeGetGender(char* buffer, int size);

void getCurrentTimeStr(char* buffer, size_t size);
void getCurrentTime(char* buffer, size_t size);

#endif