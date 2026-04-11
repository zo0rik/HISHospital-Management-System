#define _CRT_SECURE_NO_WARNINGS
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "schedule.h"
#include "utils.h"  
#include "models.h"  
#include "fileio.h"

static void displayAllDoctors() {
    if (staffHead == NULL || staffHead->next == NULL) {
        printf("  [!] 医生列表为空。\n");
        return;
    }
    printf("\n--- 医生/职工 列表 ---\n");
    printf("%-10s %-15s %-15s %-15s %-10s\n", "工号", "姓名", "科室", "职称", "性别");
    Staff* p = staffHead->next;
    while (p) {
        printf("%-10s %-15s %-15s %-15s %-10s\n", p->id, p->name, p->department, p->level, p->sex);
        p = p->next;
    }
}

static void addDoctor() {
    Staff s;
    memset(&s, 0, sizeof(Staff));

    printf("请输入医生工号 (如数字或字母): ");
    while (1) {
        safeGetString(s.id, 20);
        if (strcmp(s.id, "0") == 0) return; // 0代表取消

        int exists = 0;
        for (Staff* p = staffHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, s.id) == 0) { exists = 1; break; }
        }
        if (!exists) break;
        printf("  [!] 该工号已存在！请重新输入新的工号: ");
    }

    strcpy(s.password, "123456"); // 默认密码
    printf("请输入姓名: "); safeGetString(s.name, 100);
    printf("请输入科室: "); safeGetString(s.department, 100);
    printf("请输入职称: "); safeGetString(s.level, 100);
    printf("请输入性别（男/女）: "); safeGetGender(s.sex, 10);

    // 挂载到统一链表
    Staff* node = (Staff*)malloc(sizeof(Staff));
    *node = s;
    node->next = staffHead->next;
    staffHead->next = node;

    saveAllDataToTxt();
    printf("  [√] 医生添加成功，默认登录密码为 123456。\n");
    system("pause");
}

static void deleteDoctor() {
    char id[20];
    printf("请输入要删除的医生工号 (输入0取消): ");
    safeGetString(id, 20);
    if (strcmp(id, "0") == 0 || staffHead == NULL) return;

    Staff* prev = staffHead;
    Staff* curr = staffHead->next;
    while (curr) {
        if (strcmp(curr->id, id) == 0) {
            prev->next = curr->next;
            free(curr);

            // 【修改点】：直接传入字符串 ID 进行排班清理
            deleteScheduleByDoctorId(id);

            saveAllDataToTxt();
            // saveSchedules() 会在 deleteScheduleByDoctorId 内部被调用
            printf("  [√] 档案及登录权限删除成功。\n");
            system("pause");
            return;
        }
        prev = curr; curr = curr->next;
    }
    printf("  [!] 未找到该医生。\n");
    system("pause");
}

static void updateDoctor() {
    char id[20];
    printf("请输入要修改的医生工号 (输入0取消): ");
    safeGetString(id, 20);
    if (strcmp(id, "0") == 0 || staffHead == NULL) return;

    Staff* p = staffHead->next;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            printf("当前医生信息：\n");
            printf("1. 姓名: %s\n2. 科室: %s\n3. 职称: %s\n4. 性别: %s\n", p->name, p->department, p->level, p->sex);
            printf("请选择要修改的单个字段 (1.姓名 2.科室 3.职称 4.性别 | 0.结束保存): ");

            int ch;
            while (1) {
                ch = safeGetInt();
                if (ch >= 0 && ch <= 4) break;
                printf("  [!] 输入格式不合法，请重新选择: ");
            }
            if (ch == 0) return;

            switch (ch) {
            case 1:
                printf("请输入新姓名: "); safeGetString(p->name, 100);
                break;
            case 2:
                printf("请输入新科室: "); safeGetString(p->department, 100);
                break;
            case 3:
                printf("请输入新职称: "); safeGetString(p->level, 100);
                break;
            case 4:
                printf("请输入新性别(男/女): "); safeGetGender(p->sex, 10);
                break;
            }
            saveAllDataToTxt();
            printf("  [√] 医生信息修改成功。\n");
            system("pause");
            return;
        }
        p = p->next;
    }
    printf("  [!] 未找到该医生。\n");
    system("pause");
}

static void queryDoctor() {
    int choice;
    printf("\n查询方式：1-按工号  2-按姓名模糊 3-按职称 0-返回\n请选择: ");

    while (1) {
        choice = safeGetInt();
        if (choice >= 0 && choice <= 3) break;
        printf("  [!] 输入格式不合法，请重新选择: ");
    }
    if (choice == 0) return;
    if (staffHead == NULL) return;

    if (choice == 1) {
        char id[20];
        printf("请输入医生工号: ");
        safeGetString(id, 20);
        Staff* p = staffHead->next;
        while (p) {
            if (strcmp(p->id, id) == 0) {
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s\n",
                    p->id, p->name, p->department, p->level, p->sex);
                system("pause");
                return;
            }
            p = p->next;
        }
        printf("  [!] 未找到匹配记录。\n");
    }
    else if (choice == 2) {
        char name[50];
        printf("请输入姓名关键字: ");
        safeGetString(name, 50);
        int found = 0;
        Staff* p = staffHead->next;
        while (p) {
            if (strstr(p->name, name)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s\n", p->id, p->name, p->department, p->level);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("  [!] 未找到匹配记录。\n");
    }
    else if (choice == 3) {
        char title[50];
        printf("请输入职称关键字: ");
        safeGetString(title, 50);
        int found = 0;
        Staff* p = staffHead->next;
        while (p) {
            if (strstr(p->level, title)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s\n", p->id, p->name, p->department, p->level);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("  [!] 未找到匹配记录。\n");
    }
    system("pause");
}

void doctorMenu() {
    int choice;
    do {
        system("cls");
        printf("\n===== 医生(职工)信息管理 =====\n");
        printf("1. 查看全部医生\n");
        printf("2. 添加医生\n");
        printf("3. 删除医生\n");
        printf("4. 修改医生信息\n");
        printf("5. 查询医生\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice >= 0 && choice <= 5) break;
            printf("  [!] 输入格式不合法，请重新选择: ");
        }

        switch (choice) {
        case 1: displayAllDoctors(); system("pause"); break;
        case 2: addDoctor(); break;
        case 3: deleteDoctor(); break;
        case 4: updateDoctor(); break;
        case 5: queryDoctor(); break;
        case 0: break;
        }
    } while (choice != 0);
}