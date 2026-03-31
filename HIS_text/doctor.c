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
        fprintf(fp, "%d,%s,%s,%s\n", p->id, p->name, p->department, p->title);
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
    printf("%-5s %-15s %-15s %-15s\n", "ID", "姓名", "科室", "职称");
    Doctor* p = doctorList;
    while (p) {
        printf("%-5d %-15s %-15s %-15s\n", p->id, p->name, p->department, p->title);
        p = p->next;
    }
}

// ---------------------------------------------------------
// 业务一：录入新医生信息 (头插法)
// ---------------------------------------------------------
static void addDoctor() {
    Doctor d;
    printf("请输入医生ID: "); scanf("%d", &d.id);

    // 查重校验：防止工号冲突
    Doctor* p = doctorList;
    while (p) { if (p->id == d.id) { printf("ID已存在！\n"); return; } p = p->next; }

    printf("请输入姓名: "); scanf("%s", d.name);
    printf("请输入科室: "); scanf("%s", d.department);
    printf("请输入职称: "); scanf("%s", d.title);

    Doctor* node = (Doctor*)malloc(sizeof(Doctor));
    *node = d;
    node->next = doctorList; // 使用头插法接入链表
    doctorList = node;
    printf("医生添加成功。\n");
}

// ---------------------------------------------------------
// 业务二：删除离职医生 (链表节点删除操作)
// ---------------------------------------------------------
static void deleteDoctor() {
    int id; printf("请输入要删除的医生ID: "); scanf("%d", &id);
    Doctor* prev = NULL, * curr = doctorList;
    while (curr) {
        if (curr->id == id) {
            // 处理头节点被删除的情况
            if (prev) prev->next = curr->next;
            else doctorList = curr->next;

            free(curr); // 释放内存
            printf("删除成功。\n");
            return;
        }
        prev = curr; curr = curr->next;
    }
    printf("未找到该医生。\n");
}

// ---------------------------------------------------------
// 业务三：修改医生信息 (支持多字段批量修改)
// ---------------------------------------------------------
static void updateDoctor() {
    int id; printf("请输入要修改的医生ID: "); scanf("%d", &id);
    Doctor* p = doctorList;
    while (p) {
        if (p->id == id) {
            printf("当前医生信息：\n");
            printf("1. 姓名: %s\n2. 科室: %s\n3. 职称: %s\n", p->name, p->department, p->title);
            printf("请选择要修改的字段（输入编号，多个用空格分隔，输入0取消）: ");

            // 允许用户输入诸如 "1 3 0" 进行批量字段更新
            int choices[10], cnt = 0, ch;
            do {
                scanf("%d", &ch);
                if (ch == 0) break;
                choices[cnt++] = ch;
            } while (getchar() != '\n' && cnt < 10);

            if (cnt == 0) { printf("修改取消。\n"); return; }
            for (int i = 0; i < cnt; i++) {
                switch (choices[i]) {
                case 1: printf("输入新姓名: "); scanf("%s", p->name); break;
                case 2: printf("输入新科室: "); scanf("%s", p->department); break;
                case 3: printf("输入新职称: "); scanf("%s", p->title); break;
                default: printf("无效选项 %d，跳过。\n", choices[i]);
                }
            }
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
    printf("查询方式：1-按ID  2-按姓名（模糊）: "); scanf("%d", &choice);

    // 精确匹配
    if (choice == 1) {
        int id; printf("请输入ID: "); scanf("%d", &id);
        Doctor* p = doctorList;
        while (p) {
            if (p->id == id) {
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s\n", p->id, p->name, p->department, p->title);
                return;
            }
            p = p->next;
        }
        printf("未找到。\n");
    }
    // 模糊检索 (使用 strstr)
    else if (choice == 2) {
        char name[20]; printf("请输入姓名关键字: "); scanf("%s", name);
        int found = 0;
        Doctor* p = doctorList;
        while (p) {
            if (strstr(p->name, name)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s\n", p->id, p->name, p->department, p->title);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("未找到。\n");
    }
    //模糊检索 (使用 strstr)
    else if (choice == 3) {
        char title[20]; printf("请输入职称关键字: "); scanf("%s", title);
        int found = 0;
        Doctor* p = doctorList;
        while (p) {
            if (strstr(p->title, title)) {
                if (!found) printf("\n--- 查询结果 ---\n");
                printf("ID: %d, 姓名: %s, 科室: %s, 职称: %s\n", p->id, p->name, p->department, p->title);
                found = 1;
            }
            p = p->next;
        }
    	if (!found) printf("未找到。\n");
    }
    else printf("无效选择。\n");
}

// ---------------------------------------------------------
// 管理端：医生人事管理子路由
// ---------------------------------------------------------
void doctorMenu() {
    int choice;
    do {
        printf("\n===== 医生信息管理 =====\n");
        printf("1. 查看所有医生\n");
        printf("2. 添加医生\n");
        printf("3. 删除医生\n");
        printf("4. 修改医生信息\n");
        printf("5. 查询医生\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
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
