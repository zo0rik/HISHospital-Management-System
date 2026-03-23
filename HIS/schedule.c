#define _CRT_SECURE_NO_WARNINGS
#include "schedule.h"
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Schedule* scheduleList = NULL;

void loadSchedules() {
    FILE* fp = fopen("schedules.txt", "r");
    if (!fp) return;

    char line[256];
    Schedule s;
    Schedule* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) s.doctor_id = atoi(token); else s.doctor_id = 0;
        token = strtok(NULL, ",");
        if (token) strcpy(s.date, token); else s.date[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(s.shift, token); else s.shift[0] = '\0';

        Schedule* node = (Schedule*)malloc(sizeof(Schedule));
        *node = s;
        node->next = NULL;
        if (!scheduleList) scheduleList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

void saveSchedules() {
    FILE* fp = fopen("schedules.txt", "w");
    if (!fp) return;
    Schedule* p = scheduleList;
    while (p) {
        fprintf(fp, "%d,%s,%s\n", p->doctor_id, p->date, p->shift);
        p = p->next;
    }
    fclose(fp);
}

static int checkConflict(int doctor_id, char* date) {
    Schedule* s = scheduleList;
    while (s) {
        if (s->doctor_id == doctor_id && strcmp(s->date, date) == 0)
            return 1;
        s = s->next;
    }
    return 0;
}

static void viewSchedule() {
    char start[11], end[11];
    printf("请输入起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    printf("\n--- 排班表 (%s 至 %s) ---\n", start, end);
    printf("%-12s %-15s %-12s %-10s\n", "日期", "医生姓名", "科室", "班次");
    Schedule* s = scheduleList;
    if (!s) { printf("暂无排班记录。\n"); return; }
    while (s) {
        if (strcmp(s->date, start) >= 0 && strcmp(s->date, end) <= 0) {
            Doctor* d = doctorList;
            char docName[50] = "未知";
            char docDept[30] = "";
            while (d) {
                if (d->id == s->doctor_id) {
                    strcpy(docName, d->name);
                    strcpy(docDept, d->department);
                    break;
                }
                d = d->next;
            }
            printf("%-12s %-15s %-12s %-10s\n", s->date, docName, docDept, s->shift);
        }
        s = s->next;
    }
}

static void addSchedule() {
    int doc_id;
    char date[11], shift[10];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    Doctor* d = doctorList;
    int exists = 0;
    while (d) { if (d->id == doc_id) { exists = 1; break; } d = d->next; }
    if (!exists) { printf("医生ID不存在！\n"); return; }
    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);
    if (checkConflict(doc_id, date)) { printf("该医生当天已有排班，不能重复添加。\n"); return; }
    printf("请输入班次 (早班/晚班/休息): "); scanf("%s", shift);
    if (strcmp(shift, "早班") != 0 && strcmp(shift, "晚班") != 0 && strcmp(shift, "休息") != 0) {
        printf("无效班次。\n"); return;
    }
    Schedule* node = (Schedule*)malloc(sizeof(Schedule));
    node->doctor_id = doc_id;
    strcpy(node->date, date);
    strcpy(node->shift, shift);
    node->next = scheduleList;
    scheduleList = node;
    printf("排班添加成功。\n");
}

static void deleteSchedule() {
    int doc_id;
    char date[11];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);
    Schedule* prev = NULL, * curr = scheduleList;
    while (curr) {
        if (curr->doctor_id == doc_id && strcmp(curr->date, date) == 0) {
            if (prev) prev->next = curr->next;
            else scheduleList = curr->next;
            free(curr);
            printf("排班删除成功。\n");
            return;
        }
        prev = curr; curr = curr->next;
    }
    printf("未找到该排班。\n");
}

static void modifySchedule() {
    int doc_id;
    char date[11];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);
    Schedule* p = scheduleList;
    while (p) {
        if (p->doctor_id == doc_id && strcmp(p->date, date) == 0) {
            printf("当前班次: %s\n", p->shift);
            printf("请输入新班次 (早班/晚班/休息): "); scanf("%s", p->shift);
            if (strcmp(p->shift, "早班") != 0 && strcmp(p->shift, "晚班") != 0 && strcmp(p->shift, "休息") != 0) {
                printf("无效班次，操作取消。\n");
                return;
            }
            printf("排班修改成功。\n");
            return;
        }
        p = p->next;
    }
    printf("未找到该排班。\n");
}

void scheduleMenu() {
    int choice;
    do {
        printf("\n========== 医生排班管理 ==========\n");
        printf("1. 排班表查看\n");
        printf("2. 手动排班（添加）\n");
        printf("3. 删除排班\n");
        printf("4. 修改排班\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: viewSchedule(); break;
        case 2: addSchedule(); break;
        case 3: deleteSchedule(); break;
        case 4: modifySchedule(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}