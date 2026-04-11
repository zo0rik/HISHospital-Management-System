#pragma once
#ifndef TIME_T_H
#define TIME_T_H

#include <time.h>
#include <stddef.h>

void getCurrentTime(char* buffer, size_t size);
void getCurrentDate(char* buffer, size_t size);
void getPastDateAccurate(char* current, char* result, int days_ago);
void judgetime(char* end);
int isLeapYear(int year);
int getDaysInMonth(int year, int month);
#endif
