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
    printf("%-10s %-15s %-15s %-15s %-10s %-10s\n", "工号", "姓名", "科室", "职称", "性别","年龄");
    Staff* p = staffHead->next;
    while (p) {
        printf("%-10s %-15s %-15s %-15s %-10s %-10d\n", p->id, p->name, p->department, p->level, p->sex, p->age);
        p = p->next;
    }
}

static void addDoctor() {
    Staff s;
    memset(&s, 0, sizeof(Staff));

    printf("请输入医生工号 (如数字或字母, 输入-1取消): "); /* 【规则C】 */
    while (1) {
        safeGetString(s.id, 20);
        if (strcmp(s.id, "-1") == 0) return; /* 【规则C】0→-1 */

        int exists = 0;
        for (Staff* p = staffHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, s.id) == 0) { exists = 1; break; }
        }
        if (!exists) break;
        printf("  [!] 该工号已存在！请重新输入新的工号: ");
    }

    strcpy(s.password, "123456"); /* 默认密码保持不变 */
    printf("请输入姓名: "); safeGetString(s.name, 100);
    printf("请输入科室: "); safeGetString(s.department, 100);
    printf("请输入职称: "); safeGetString(s.level, 100);
    printf("请输入性别（男/女）: "); safeGetGender(s.sex, 10);
	printf("请输入年龄(20-99): "); 
    while (1) {
        s.age = safeGetPositiveInt();
        if (s.age >= 20 && s.age <= 99) break;
        printf("  [!] 输入格式不合法，请重新输入: ");
    }
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
    printf("请输入要删除的医生工号 (输入-1取消): "); /* 【规则C】 */
    safeGetString(id, 20);
    if (strcmp(id, "-1") == 0 || staffHead == NULL) return; /* 【规则C】 */

    Staff* prev = staffHead;
    Staff* curr = staffHead->next;
    while (curr) {
        if (strcmp(curr->id, id) == 0) {
            prev->next = curr->next;
            free(curr);

            deleteScheduleByDoctorId(id);

            saveAllDataToTxt();
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
    printf("请输入要修改的医生工号 (输入-1取消): "); /* 【规则C】 */
    safeGetString(id, 20);
    if (strcmp(id, "-1") == 0 || staffHead == NULL) return; /* 【规则C】 */

    Staff* p = staffHead->next;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            printf("当前医生信息：\n");
            printf("1. 姓名: %s\n2. 科室: %s\n3. 职称: %s\n4. 性别: %s\n5. 年龄: %d\n", p->name, p->department, p->level, p->sex, p->age);
            printf("请选择要修改的单个字段 (1.姓名 2.科室 3.职称 4.性别 5.年龄 | -1.结束保存): "); /* 【规则A】0→-1 */

            int ch;
            while (1) {
                ch = safeGetInt();
                if (ch == -1 || (ch >= 1 && ch <= 5)) break; /* 【规则B】0→-1 */
                printf("  [!] 输入格式不合法，请重新选择: ");
            }
            if (ch == -1) return; /* 【规则B】 */

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
            case 5:
                printf("请输入新年龄(20-99): ");
                while (1) {
                    p->age = safeGetPositiveInt();
                    if (p->age >= 20 && p->age <= 99) break;
                    printf("  [!] 输入格式不合法，请重新输入: ");
                }
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
    printf("\n查询方式：1-按工号  2-按姓名模糊 3-按职称 -1-返回\n请选择: "); /* 【规则A】0→-1 */

    while (1) {
        choice = safeGetInt();
        if (choice == -1 || (choice >= 1 && choice <= 3)) break; /* 【规则B】0→-1 */
        printf("  [!] 输入格式不合法，请重新选择: ");
    }
    if (choice == -1) return; /* 【规则B】 */
    if (staffHead == NULL) return;

    if (choice == 1) {
        char id[20];
        printf("请输入医生工号: ");
        safeGetString(id, 20);
        Staff* p = staffHead->next;
        while (p) {
            if (strcmp(p->id, id) == 0) {
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s, 年龄: %d\n",
                    p->id, p->name, p->department, p->level, p->sex, p->age);
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
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s, 年龄: %d\n", p->id, p->name, p->department, p->level, p->sex, p->age);
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
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s, 年龄: %d\n", p->id, p->name, p->department, p->level, p->sex, p->age);
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
        printf("-1. 返回主菜单\n"); /* 【规则A】0→-1 */
        printf("请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break; /* 【规则B】0→-1 */
            printf("  [!] 输入格式不合法，请重新选择: ");
        }

        switch (choice) {
        case 1: displayAllDoctors(); system("pause"); break;
        case 2: addDoctor(); break;
        case 3: deleteDoctor(); break;
        case 4: updateDoctor(); break;
        case 5: queryDoctor(); break;
        case -1: break; /* 【规则B】 */
        }
    } while (choice != -1); /* 【规则B】 */
}