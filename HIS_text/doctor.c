#define _CRT_SECURE_NO_WARNINGS
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "schedule.h"
#include "utils.h"
#include "models.h"
#include "fileio.h"

// 合法职称列表
const char* valid_titles[] = {
    "主任医师",
    "副主任医师",
    "主治医师",
    "住院医师"
};

// 合法科室列表
const char* valid_departments[] = {
    "内科",
    "外科",
    "儿科",
    "妇科",
    "骨科",
};

// 判断职称是否在合法范围内
int isTitleValid(const char* title) {
    int n = sizeof(valid_titles) / sizeof(valid_titles[0]);
    for (int i = 0; i < n; i++) {
        if (strcmp(title, valid_titles[i]) == 0) {
            return 1; // 合法
        }
    }
    return 0; // 不合法
}

// 判断科室是否在合法范围内
int isDepartmentValid(const char* dept) {
    int n = sizeof(valid_departments) / sizeof(valid_departments[0]);
    for (int i = 0; i < n; i++) {
        if (strcmp(dept, valid_departments[i]) == 0) {
            return 1; // 合法
        }
    }
    return 0; // 不合法
}

// 显示所有医生/职工信息
static void displayAllDoctors() {
    if (staffHead == NULL || staffHead->next == NULL) {
        printf("  [!] 医生列表为空。\n");
        return;
    }
    printf("\n--- 医生/职工 列表 ---\n");
    printf("%-10s %-15s %-15s %-15s %-10s %-10s\n", "工号", "姓名", "科室", "职称", "性别", "年龄");
    Staff* p = staffHead->next;
    while (p) {
        printf("%-10s %-15s %-15s %-15s %-10s %-10d\n", p->id, p->name, p->department, p->level, p->sex, p->age);
        p = p->next;
    }
}

// 添加医生信息，带工号查重、科室职称校验、默认密码
static void addDoctor() {
    Staff s;
    memset(&s, 0, sizeof(Staff));

    printf("请输入医生工号 (如数字或字母, 输入-1取消): ");
    // 工号输入，防止重复并保证合法输入
    while (1) {
        safeGetString(s.id, 20);
        if (strcmp(s.id, "-1") == 0) return;

        int exists = 0;
        for (Staff* p = staffHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, s.id) == 0) { exists = 1; break; }
        }
        if (!exists) break;
        printf("  [!] 该工号已存在！请重新输入新的工号: ");
    }

    // 默认初始密码
    strcpy(s.password, "123456");
    printf("请输入姓名: "); safeGetString(s.name, 100);

    printf("请输入科室: ");
    // 科室必须在合法列表内，否则重新输入
    while (1) {
        safeGetString(s.department, 100);
        if (isDepartmentValid(s.department)) break;
        printf("  [!] 输入的科室不合法！请从以下列表中选择并重新输入:\n");
        for (int i = 0; i < sizeof(valid_departments) / sizeof(valid_departments[0]); i++) {
            printf("    - %s\n", valid_departments[i]);
        }
        printf("请输入科室: ");
    }

    printf("请输入职称: ");
    // 职称必须在合法列表内，否则重新输入
    while (1) {
        safeGetString(s.level, 100);
        if (isTitleValid(s.level)) break;
        printf("  [!] 输入的职称不合法！请从以下列表中选择并重新输入:\n");
        for (int i = 0; i < sizeof(valid_titles) / sizeof(valid_titles[0]); i++) {
            printf("    - %s\n", valid_titles[i]);
        }
        printf("请输入职称: ");
    }

    printf("请输入性别（男/女）: "); safeGetGender(s.sex, 10);

    printf("请输入年龄(20-99): ");
    // 年龄范围限制
    while (1) {
        s.age = safeGetPositiveInt();
        if (s.age >= 20 && s.age <= 99) break;
        printf("  [!] 输入格式不合法，请重新输入: ");
    }

    // 新建节点并头插法加入链表
    Staff* node = (Staff*)malloc(sizeof(Staff));
    *node = s;
    node->next = staffHead->next;
    staffHead->next = node;

    saveAllDataToTxt();
    printf("  [√] 医生添加成功，默认登录密码为 123456。\n");
    system("pause");
}

// 删除医生信息，同时删除对应排班记录
static void deleteDoctor() {
    char id[20];
    printf("请输入要删除的医生工号 (输入-1取消): ");
    safeGetString(id, 20);
    if (strcmp(id, "-1") == 0 || staffHead == NULL) return;

    Staff* prev = staffHead;
    Staff* curr = staffHead->next;
    while (curr) {
        if (strcmp(curr->id, id) == 0) {
            prev->next = curr->next;
            free(curr);
            // 同步删除该医生的所有排班信息
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

// 修改医生指定字段信息
static void updateDoctor() {
    char id[20];
    printf("请输入要修改的医生工号 (输入-1取消): ");
    safeGetString(id, 20);
    if (strcmp(id, "-1") == 0 || staffHead == NULL) return;

    Staff* p = staffHead->next;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            printf("当前医生信息：\n");
            printf("1. 姓名: %s\n2. 科室: %s\n3. 职称: %s\n4. 性别: %s\n5. 年龄: %d\n",
                p->name, p->department, p->level, p->sex, p->age);
            printf("请选择要修改的单个字段 (1.姓名 2.科室 3.职称 4.性别 5.年龄 | -1.结束保存): ");

            int ch;
            while (1) {
                ch = safeGetInt();
                if (ch == -1 || (ch >= 1 && ch <= 5)) break;
                printf("  [!] 输入格式不合法，请重新选择: ");
            }
            if (ch == -1) return;

            switch (ch) {
            case 1:
                printf("请输入新姓名: "); safeGetString(p->name, 100);
                break;
            case 2:
                printf("请输入新科室: ");
                while (1) {
                    safeGetString(p->department, 100);
                    if (isDepartmentValid(p->department)) break;
                    printf("  [!] 输入的科室不合法！请从以下列表中选择并重新输入:\n");
                    for (int i = 0; i < sizeof(valid_departments) / sizeof(valid_departments[0]); i++) {
                        printf("    - %s\n", valid_departments[i]);
                    }
                    printf("请输入科室: ");
                }
                break;
            case 3:
                printf("请输入新职称: ");
                while (1) {
                    safeGetString(p->level, 100);
                    if (isTitleValid(p->level)) break;
                    printf("  [!] 输入的职称不合法！请从以下列表中选择并重新输入:\n");
                    for (int i = 0; i < sizeof(valid_titles) / sizeof(valid_titles[0]); i++) {
                        printf("    - %s\n", valid_titles[i]);
                    }
                    printf("请输入职称: ");
                }
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

// 多条件查询医生：工号、姓名模糊、职称模糊
static void queryDoctor() {
    int choice;
    printf("\n查询方式：1-按工号  2-按姓名模糊 3-按职称 -1-返回\n请选择: ");

    while (1) {
        choice = safeGetInt();
        if (choice == -1 || (choice >= 1 && choice <= 3)) break;
        printf("  [!] 输入格式不合法，请重新选择: ");
    }
    if (choice == -1) return;
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
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s, 年龄: %d\n",
                    p->id, p->name, p->department, p->level, p->sex, p->age);
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
                printf("工号: %s, 姓名: %s, 科室: %s, 职称: %s, 性别: %s, 年龄: %d\n",
                    p->id, p->name, p->department, p->level, p->sex, p->age);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("  [!] 未找到匹配记录。\n");
    }
    system("pause");
}

// 医生信息管理主菜单
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
        printf("-1. 返回主菜单\n");
        printf("请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break;
            printf("  [!] 输入格式不合法，请重新选择: ");
        }

        switch (choice) {
        case 1: displayAllDoctors(); system("pause"); break;
        case 2: addDoctor(); break;
        case 3: deleteDoctor(); break;
        case 4: updateDoctor(); break;
        case 5: queryDoctor(); break;
        case -1: break;
        }
    } while (choice != -1);
}