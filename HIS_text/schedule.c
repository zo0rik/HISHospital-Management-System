#define _CRT_SECURE_NO_WARNINGS
#include "schedule.h"
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time_t.h"

Schedule* scheduleList;
//---------------------------------------------------------
//删除医生排班，当医生被删除时调用，删除相关排班记录
// ---------------------------------------------------------
void deleteScheduleByDoctorId(int doctorId) {
    Schedule* prev = NULL;
    Schedule* curr = scheduleList->next;
    while (curr != NULL) {
        if (curr->doctor_id == doctorId) {
            Schedule* temp = curr;
            if (prev == NULL) {
                scheduleList->next = curr->next;
            }
            else {
                prev->next = curr->next;
            }
            curr = curr->next;
            free(temp);
        }
        else {
            prev = curr;
            curr = curr->next;
        }
    }
    // 删除后保存到文件
    saveSchedules();
}
//---------------------------------------------------------
// 从文件加载排班
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
        if (token) s.schedule_id = atoi(token); else s.schedule_id = 0;
        token = strtok(NULL, ",");
        if (token) s.doctor_id = atoi(token); else s.doctor_id = 0;
        token = strtok(NULL, ",");
        if (token) strcpy(s.date, token); else s.date[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(s.shift, token); else s.shift[0] = '\0';

        Schedule* node = (Schedule*)malloc(sizeof(Schedule));
        *node = s;
        node->next = NULL;
        if (!(scheduleList->next)) { scheduleList->next = node;tail = node; }
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

//----------------------------------------------------------------------
// 保存排班
//----------------------------------------------------------------------
void saveSchedules() {
    FILE* fp = fopen("schedules.txt", "w");
    if (!fp) return;
    Schedule* p = scheduleList->next;
    while (p) {
        fprintf(fp, "%d,%d,%s,%s\n", p->schedule_id, p->doctor_id, p->date, p->shift);
        p = p->next;
    }
    fclose(fp);
}

//--------------------------------------------------------------
// 检查排班是否冲突
//--------------------------------------------------------------
static int checkConflict(int doctor_id, char* date) {
    Schedule* s = scheduleList->next;
    while (s) {
        if (s->doctor_id == doctor_id && strcmp(s->date, date) == 0) return 1;
        s = s->next;
    }
    return 0;
}

//-----------------------------------------------------------------
// 查看排班
//-----------------------------------------------------------------
static void viewSchedule() {
    char start[11], end[11];
    printf("请输入开始日期 (YYYY-MM-DD): ");
    judgetime(start);
    printf("请输入结束日期 (YYYY-MM-DD): ");
    judgetime(end);

    printf("\n--- 排班表 (%s 至 %s) ---\n", start, end);
    printf("%-8s %-12s %-15s %-12s %-10s\n", "排班ID", "日期", "医生姓名", "科室", "班次");
    Schedule* s = scheduleList->next;
    if (!s) {
        printf("暂无排班记录。\n");
        return;
    }
    while (s) {
        if (strcmp(s->date, start) >= 0 && strcmp(s->date, end) <= 0) {
            Doctor* d = doctorList->next;
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
            printf("%-8d %-12s %-15s %-12s %-10s\n", s->schedule_id, s->date, docName, docDept, s->shift);
        }
        s = s->next;
    }
}

//-------------------------------------------------------------------------------
// 添加排班
//-------------------------------------------------------------------------------
static void addSchedule() {
    int doc_id;
    char date[11], shift[10];
    printf("请输入医生ID: ");
    while (scanf("%d", &doc_id) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }

    Doctor* d = doctorList->next;
    int exists = 0;
    while (d) {
        if (d->id == doc_id) {
            exists = 1;
            break;
        }
        d = d->next;
    }
    if (!exists) {
        printf("医生ID不存在！\n");
        return;
    }

    printf("请输入日期 (YYYY-MM-DD): ");
    judgetime(date);

    if (checkConflict(doc_id, date)) {
        printf("该医生当天已有排班，请勿重复添加！\n");
        return;
    }

    printf("请输入班次 (早班/晚班/休息): ");
    scanf("%9s", shift);while (getchar() != '\n');
    if (strcmp(shift, "早班") != 0 && strcmp(shift, "晚班") != 0 && strcmp(shift, "休息") != 0) {
        printf("无效班次！\n");
        return;
    }

    Schedule* node = (Schedule*)malloc(sizeof(Schedule));
    int max_id = 0;
    Schedule* temp = scheduleList->next;
    while (temp) {
        if (temp->schedule_id > max_id) max_id = temp->schedule_id;
        temp = temp->next;
    }
    node->schedule_id = max_id + 1;

    node->doctor_id = doc_id;
    strcpy(node->date, date);
    strcpy(node->shift, shift);
    node->next = NULL;
    if (scheduleList->next == NULL) {
        scheduleList->next = node;
    }
    else {
        Schedule* tail = scheduleList->next;
        while (tail->next)
            tail = tail->next;
        tail->next = node;
    }

    saveSchedules();
    printf("排班添加成功，自动分配排班ID: %d\n", node->schedule_id);
}

//-------------------------------------------------------------------------------
// 删除排班
//-------------------------------------------------------------------------------
static void deleteSchedule() {
    int sid;
    printf("请输入要删除的排班ID: ");
    while (scanf("%d", &sid) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }

    Schedule* prev = NULL;
    Schedule* curr = scheduleList->next;
    while (curr) {
        if (curr->schedule_id == sid) {
            if (prev) prev->next = curr->next;
            else scheduleList->next = curr->next;
            free(curr);
            saveSchedules();
            printf("排班删除成功。\n");
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    printf("未找到该排班ID。\n");
}

//-------------------------------------------------------------------------------
// 修改排班
//-------------------------------------------------------------------------------
static void modifySchedule() {
    int sid;
    printf("请输入要修改的排班ID: ");
    while (scanf("%d", &sid) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }

    Schedule* p = scheduleList->next;
    while (p) {
        if (p->schedule_id == sid) {
            int new_id;
            char new_date[11], new_shift[10];

            printf("当前医生ID: %d, 日期: %s, 班次: %s\n", p->doctor_id, p->date, p->shift);

            printf("请输入新的医生ID: ");
            while (scanf("%d", &new_id) != 1) {
                while (getchar() != '\n');
                printf("输入格式错误，请重新输入: ");
            }

            // 校验医生是否存在
            Doctor* doc = doctorList->next;
            int doc_exist = 0;
            while (doc) {
                if (doc->id == new_id) {
                    doc_exist = 1;
                    break;
                }
                doc = doc->next;
            }
            if (!doc_exist) {
                printf("该医生ID不存在，修改已取消。\n");
                return;
            }

            printf("请输入新的日期 (YYYY-MM-DD): ");
            judgetime(new_date);

            // 排班冲突检查
            if (checkConflict(new_id, new_date)) {
                printf("该医生在该日期已有排班，修改失败。\n");
                return;
            }

            printf("请输入新班次 (早班/晚班/休息): ");
            scanf("%9s", new_shift);
            while (getchar() != '\n');

            if (strcmp(new_shift, "早班") != 0 &&
                strcmp(new_shift, "晚班") != 0 &&
                strcmp(new_shift, "休息") != 0) {
                printf("班次输入无效，修改已取消。\n");
                return;
            }

            // 应用修改
            p->doctor_id = new_id;
            strcpy(p->date, new_date);
            strcpy(p->shift, new_shift);

            saveSchedules();
            printf("排班修改成功！\n");
            return;
        }
        p = p->next;
    }
    printf("未找到该排班ID。\n");
}

//-------------------------------------------------------------------------------
// 排班菜单
//-------------------------------------------------------------------------------
void scheduleMenu() {
    int choice;
    do {
        printf("\n========== 医生排班管理 ==========\n");
        printf("1. 排班查看\n");
        printf("2. 添加排班\n");
        printf("3. 删除排班\n");
        printf("4. 修改排班\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

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