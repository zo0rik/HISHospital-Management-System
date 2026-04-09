#pragma once
#ifndef SCHEDULE_H
#define SCHEDULE_H

typedef struct Schedule {
    int schedule_id;    // 【新增】排班流水唯一ID (用于唯一确定某天的某个班次)
    int doctor_id;      // 医生工号
    char date[11];      // 日期
    char shift[10];     // 班次
    struct Schedule* next;
} Schedule;

extern Schedule* scheduleList;

void loadSchedules(void);
void saveSchedules(void);
void scheduleMenu(void);
void deleteScheduleByDoctorId(int doctorId);// 根据医生ID删除相关排班（专门为医生被删除时调用）

#endif
