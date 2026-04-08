#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <time.h>

void getCurrentTime(char* buffer, int size);
void getCurrentDate(char* buffer, int size);
void getPastDateAccurate(char* current, char* result, int days_ago);
void judgetime(char* end);
int isLeapYear(int year);
int getDaysInMonth(int year, int month);
#endif
