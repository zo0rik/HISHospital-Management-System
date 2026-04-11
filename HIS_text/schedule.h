#pragma once
#ifndef SCHEDULE_H
#define SCHEDULE_H

typedef struct Schedule {
    int schedule_id;    // 排班流水唯一ID
    char doctor_id[20]; // 【修改】医生工号改成字符串数组，与 Staff 结构体对齐
    char date[11];      // 日期
    char shift[10];     // 班次
    struct Schedule* next;
} Schedule;

extern Schedule* scheduleList;

void scheduleMenu(void);
void deleteScheduleByDoctorId(const char* doctorId);// 根据医生ID删除相关排班（专门为医生被删除时调用）

#endif
