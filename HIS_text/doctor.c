#define _CRT_SECURE_NO_WARNINGS
#include "doctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 管理端独立维护的医生人事档案链表
Doctor* doctorList = NULL;

// ---------------------------------------------------------
// 加载医生人事档案
// ---------------------------------------------------------
void loadDoctors() {
    FILE* fp = fopen("doctors.txt", "r");
    if (!fp) return;

    char line[256];
    Doctor d;
    Doctor* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) d.id = atoi(token); else d.id = 0;
        token = strtok(NULL, ",");
        if (token) strcpy(d.name, token); else d.name[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(d.department, token); else d.department[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(d.title, token); else d.title[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(d.sex, token); else d.sex[0] = '\0';

        Doctor* node = (Doctor*)malloc(sizeof(Doctor));
        *node = d;
        node->next = NULL;
        if (!doctorList) doctorList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 保存医生人事档案
// ---------------------------------------------------------
void saveDoctors() {
    FILE* fp = fopen("doctors.txt", "w");
    if (!fp) return;
    Doctor* p = doctorList;
    while (p) {
        fprintf(fp, "%d,%s,%s,%s,%s\n", p->id, p->name, p->department, p->title, p->sex);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 内部工具：格式化打印全部医生名单
// ---------------------------------------------------------
static void displayAllDoctors() {
    if (!doctorList) { printf("医生列表为空。\n"); return; }
    printf("\n--- 医生列表 ---\n");
    printf("%-5s %-15s %-15s %-15s %-10s\n", "ID", "姓名", "科室", "职称", "性别");
    Doctor* p = doctorList;
    while (p) {
        printf("%-5d %-15s %-15s %-15s %-10s\n", p->id, p->name, p->department, p->title, p->sex);
        p = p->next;
    }
}

// ---------------------------------------------------------
// 业务一：录入新医生信息
// ---------------------------------------------------------
static void addDoctor() {
    Doctor d;
    printf("请输入医生ID: ");
    while (scanf("%d", &d.id) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }

    // ID查重
    Doctor* p = doctorList;
    while (p) {
        if (p->id == d.id) {
            printf("ID已存在！\n");
            return;
        }
        p = p->next;
    }

    printf("请输入姓名: "); scanf("%s", d.name);
    printf("请输入科室: "); scanf("%s", d.department);
    printf("请输入职称: "); scanf("%s", d.title);
    printf("请输入性别（男/女）: ");
    while (1) {
        scanf("%s", d.sex);
        if (strcmp(d.sex, "男") != 0 && strcmp(d.sex, "女") != 0) {
            printf("无效输入，请输入 '男' 或 '女': ");
        }
        else {
            break;
        }
    }

    Doctor* node = (Doctor*)malloc(sizeof(Doctor));
    *node = d;
    node->next = doctorList;
    doctorList = node;
    saveDoctors();
    printf("医生添加成功。\n");
}

// ---------------------------------------------------------
// 业务二：删除离职医生
// ---------------------------------------------------------
static void deleteDoctor() {
    int id;
    printf("请输入要删除的医生ID: ");
    scanf("%d", &id);

    Doctor* prev = NULL;
    Doctor* curr = doctorList;

    while (curr) {
        if (curr->id == id) {
            if (prev) prev->next = curr->next;
            else doctorList = curr->next;

            free(curr);
            saveDoctors();
            printf("删除成功。\n");
            return;
        }
        prev = curr;
        curr = curr->next;
    }
    printf("未找到该医生。\n");
}

// ---------------------------------------------------------
// 业务三：修改医生信息
// ---------------------------------------------------------
static void updateDoctor() {
    int id;
    printf("请输入要修改的医生ID: ");
    while (1) {
        if (scanf("%d", &id) == 1) break;
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }

    Doctor* p = doctorList;
    while (p) {
        if (p->id == id) {
            printf("当前医生信息：\n");
            printf("1. 姓名: %s\n2. 科室: %s\n3. 职称: %s\n4. 性别: %s\n",
                p->name, p->department, p->title, p->sex);
            printf("请选择要修改的字段(空格分隔，0结束): ");

            int choices[100], cnt = 0, ch;
            do {
                while (1) {
                    if (scanf("%d", &ch) == 1) break;
                    getchar();
                }
                if (ch == 0) break;
                if (ch < 1 || ch > 4) {
                    printf("无效选项\n");
                    return;
                }
                choices[cnt++] = ch;
            } while (getchar() != '\n' && cnt < 10);

            if (cnt == 0) {
                printf("修改取消。\n");
                return;
            }

            for (int i = 0; i < cnt; i++) {
                switch (choices[i]) {
                case 1:
                    printf("请输入姓名: ");
                    scanf("%s", p->name);
                    break;
                case 2:
                    printf("请输入科室: ");
                    scanf("%s", p->department);
                    break;
                case 3:
                    printf("请输入职称: ");
                    scanf("%s", p->title);
                    break;
                case 4:
                    printf("请输入性别: ");
                    while (1) {
                        scanf("%s", p->sex);
                        if (strcmp(p->sex, "男") == 0 || strcmp(p->sex, "女") == 0) break;
                        printf("无效输入，请输入 '男' 或 '女': ");
                    }
                    break;
                default:
                    printf("无效选项 %d，跳过\n", choices[i]);
                }
            }
            saveDoctors();
            printf("医生信息修改成功。\n");
            return;
        }
        p = p->next;
    }
    printf("未找到该医生。\n");
}

// ---------------------------------------------------------
// 业务四：按条件查询检索医生
// ---------------------------------------------------------
static void queryDoctor() {
    int choice;
    printf("查询方式：1-按ID  2-按姓名模糊 3-按职称: ");
    if (scanf("%d", &choice) != 1) choice = -1;

    if (choice == 1) {
        int id;
        printf("请输入ID: ");
        scanf("%d", &id);
        Doctor* p = doctorList;
        while (p) {
            if (p->id == id) {
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s, 性别: %s\n",
                    p->id, p->name, p->department, p->title, p->sex);
                return;
            }
            p = p->next;
        }
        printf("未找到。\n");
    }
    else if (choice == 2) {
        char name[20];
        printf("请输入姓名关键字: ");
        scanf("%s", name);
        int found = 0;
        Doctor* p = doctorList;
        while (p) {
            if (strstr(p->name, name)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s\n",
                    p->id, p->name, p->department, p->title);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("未找到。\n");
    }
    else if (choice == 3) {
        char title[20];
        printf("请输入职称关键字: ");
        scanf("%s", title);
        int found = 0;
        Doctor* p = doctorList;
        while (p) {
            if (strstr(p->title, title)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s\n",
                    p->id, p->name, p->department, p->title);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("未找到。\n");
    }
    else {
        printf("无效选项\n");
    }
}

// ---------------------------------------------------------
// 管理端：医生人事管理子路由
// ---------------------------------------------------------
void doctorMenu() {
    int choice;
    do {
        printf("\n===== 医生信息管理 =====\n");
        printf("1. 查看全部医生\n");
        printf("2. 添加医生\n");
        printf("3. 删除医生\n");
        printf("4. 修改医生信息\n");
        printf("5. 查询医生\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice) {
        case 1: displayAllDoctors(); break;
        case 2: addDoctor(); break;
        case 3: deleteDoctor(); break;
        case 4: updateDoctor(); break;
        case 5: queryDoctor(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}