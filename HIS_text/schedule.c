#define _CRT_SECURE_NO_WARNINGS
#include "schedule.h"
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Schedule* scheduleList = NULL;
//---------------------------------------------------------
//从文件中加载排班表
//---------------------------------------------------------
void loadSchedules() {
    FILE* fp = fopen("schedules.txt", "r");
    if (!fp) return;

    char line[256];
    Schedule s;
    Schedule* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) s.schedule_id = atoi(token); else s.schedule_id = 0; // 【新增】解析排班ID
        token = strtok(NULL, ",");
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
//----------------------------------------------------------------------
//保存排班表
//----------------------------------------------------------------------
void saveSchedules() {
    FILE* fp = fopen("schedules.txt", "w");
    if (!fp) return;
    Schedule* p = scheduleList;
    while (p) {
        // 【新增】保存排班ID
        fprintf(fp, "%d,%d,%s,%s\n", p->schedule_id, p->doctor_id, p->date, p->shift);
        p = p->next;
    }
    fclose(fp);
}
//--------------------------------------------------------------
//查看排班表是否存在时间矛盾
//--------------------------------------------------------------
static int checkConflict(int doctor_id, char* date) {
    Schedule* s = scheduleList;
    while (s) {
        if (s->doctor_id == doctor_id && strcmp(s->date, date) == 0) return 1;
        s = s->next;
    }
    return 0;
}
//-----------------------------------------------------------------
//查看排班表
//-----------------------------------------------------------------
static void viewSchedule() {
    char start[11], end[11];
    printf("请输入起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    printf("\n--- 排班表 (%s 至 %s) ---\n", start, end);
    printf("%-8s %-12s %-15s %-12s %-10s\n", "排班ID", "日期", "医生姓名", "科室", "班次");
    Schedule* s = scheduleList;
    if (!s) { printf("暂无排班记录。\n"); return; }
    while (s) {
        if (strcmp(s->date, start) >= 0 && strcmp(s->date, end) <= 0) {
            Doctor* d = doctorList;
            char docName[50] = "未知"; char docDept[30] = "";
            while (d) {
                if (d->id == s->doctor_id) { strcpy(docName, d->name); strcpy(docDept, d->department); break; }
                d = d->next;
            }
            printf("%-8d %-12s %-15s %-12s %-10s\n", s->schedule_id, s->date, docName, docDept, s->shift);
        }
        s = s->next;
    }
}
//-------------------------------------------------------------------------------
//增加排班信息
//-------------------------------------------------------------------------------
static void addSchedule() {
    int doc_id; char date[11], shift[10];
    printf("请输入医生ID: "); scanf("%d", &doc_id);
    Doctor* d = doctorList; int exists = 0;
    while (d) { if (d->id == doc_id) { exists = 1; break; } d = d->next; }
    if (!exists) { printf("医生ID不存在！\n"); return; }

    printf("请输入日期 (YYYY-MM-DD): "); scanf("%s", date);
    if (checkConflict(doc_id, date)) { printf("该医生当天已有排班，不能重复添加。\n"); return; }
    printf("请输入班次 (早班/晚班/休息): "); scanf("%s", shift);
    if (strcmp(shift, "早班") != 0 && strcmp(shift, "晚班") != 0 && strcmp(shift, "休息") != 0) {
        printf("无效班次。\n"); return;
    }

    Schedule* node = (Schedule*)malloc(sizeof(Schedule));

    // 【核心】自动计算并分配唯一的排班ID
    int max_id = 0;
    Schedule* temp = scheduleList;
    while (temp) { if (temp->schedule_id > max_id) max_id = temp->schedule_id; temp = temp->next; }
    node->schedule_id = max_id + 1;

    node->doctor_id = doc_id;
    strcpy(node->date, date);
    strcpy(node->shift, shift);
    node->next = scheduleList;
    scheduleList = node;
    printf("排班添加成功，自动分配排班ID: %d\n", node->schedule_id);
}
//-------------------------------------------------------------------------------
//删除排班信息
//-------------------------------------------------------------------------------
static void deleteSchedule() {
    int sid;
    printf("请输入要删除的排班ID: "); scanf("%d", &sid); // 改用精准的排班ID删除
    Schedule* prev = NULL, * curr = scheduleList;
    while (curr) {
        if (curr->schedule_id == sid) {
            if (prev) prev->next = curr->next;
            else scheduleList = curr->next;
            free(curr);
            printf("排班删除成功。\n");
            return;
        }
        prev = curr; curr = curr->next;
    }
    printf("未找到该排班ID。\n");
}
//-------------------------------------------------------------------------------
//删除排班信息
//-------------------------------------------------------------------------------
static void modifySchedule() {
    int sid;
    printf("请输入要修改的排班ID: "); scanf("%d", &sid); // 改用精准的排班ID修改
    Schedule* p = scheduleList;
    while (p) {
        if (p->schedule_id == sid) {
            printf("当前班次: %s\n", p->shift);
            printf("请输入新班次 (早班/晚班/休息): "); scanf("%s", p->shift);
            if (strcmp(p->shift, "早班") != 0 && strcmp(p->shift, "晚班") != 0 && strcmp(p->shift, "休息") != 0) {
                printf("无效班次，操作取消。\n"); return;
            }
            printf("排班修改成功。\n");
            return;
        }
        p = p->next;
    }
    printf("未找到该排班ID。\n");
}
//-------------------------------------------------------------------------------
//排班总路由
//-------------------------------------------------------------------------------
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
