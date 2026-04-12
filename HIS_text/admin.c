#define _CRT_SECURE_NO_WARNINGS
#include "admin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drug.h"
#include "decision.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"
#include "utils.h"
#include "fileio.h"

Admin admin;
Admin* adminHead = NULL;
Admin* currentAdminNode = NULL;

Admin* findAdminByCredentials(const char* username, const char* password) {
    Admin* p = adminHead ? adminHead->next : NULL;
    while (p) {
        if (strcmp(p->username, username) == 0 && strcmp(p->password, password) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

void setCurrentAdminNode(Admin* node) {
    if (!node) return;
    currentAdminNode = node;
    admin = *node;
    admin.next = NULL;
}

void freeAdminList(void) {
    Admin* p = adminHead;
    while (p) {
        Admin* next = p->next;
        free(p);
        p = next;
    }
    adminHead = NULL;
    currentAdminNode = NULL;
    memset(&admin, 0, sizeof(admin));
}

static void syncCurrentAdminBackToNode(void) {
    if (!currentAdminNode) return;
    strncpy(currentAdminNode->username, admin.username, sizeof(currentAdminNode->username) - 1);
    currentAdminNode->username[sizeof(currentAdminNode->username) - 1] = '\0';
    strncpy(currentAdminNode->password, admin.password, sizeof(currentAdminNode->password) - 1);
    currentAdminNode->password[sizeof(currentAdminNode->password) - 1] = '\0';
    strncpy(currentAdminNode->phone, admin.phone, sizeof(currentAdminNode->phone) - 1);
    currentAdminNode->phone[sizeof(currentAdminNode->phone) - 1] = '\0';
    strncpy(currentAdminNode->email, admin.email, sizeof(currentAdminNode->email) - 1);
    currentAdminNode->email[sizeof(currentAdminNode->email) - 1] = '\0';
}

void changePassword(void) {
    char old[50] = { '\0' }, new1[50] = { '\0' }, new2[50] = { '\0' };
    printf("请输入旧密码 (输入-1取消): ");
    safeGetString(old, 50);
    if (strcmp(old, "-1") == 0) return;
    if (strcmp(old, admin.password) != 0) {
        printf("  [!] 旧密码错误！\n");
        system("pause");
        return;
    }

    printf("请输入新密码 (至少6位，仅限字母或数字): ");
    safeGetPassword(new1, 20);
    if (strcmp(new1, "-1") == 0) return;

    printf("请确认新密码: ");
    safeGetString(new2, 20);
    if (strcmp(new1, new2) != 0) {
        printf("  [!] 两次输入不一致！\n");
        system("pause");
        return;
    }

    strcpy(admin.password, new1);
    syncCurrentAdminBackToNode();
    printf("  [√] 密码修改成功！\n");
    saveAdminData();
    system("pause");
}

void editPersonalInfo(void) {
    char buffer[100];
    printf("\n当前信息：\n用户名: %s\n手机号: %s\n邮箱: %s\n", admin.username, admin.phone, admin.email);

    printf("\n请输入新用户名 (直接输入-1保留原值): ");
    safeGetString(buffer, 20);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.username, buffer);

    printf("请输入新手机号 (直接输入-1保留原值): ");
    safeGetString(buffer, 12);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.phone, buffer);

    printf("请输入新邮箱 (直接输入-1保留原值): ");
    safeGetString(buffer, 30);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.email, buffer);

    syncCurrentAdminBackToNode();
    printf("  [√] 个人信息更新成功！\n");
    saveAdminData();
    system("pause");
}

void personalMenu(void) {
    int choice;
    do {
        system("cls");
        printf("\n========== 个人设置 ==========\n");
        printf("1. 修改密码\n2. 个人信息编辑\n-1. 返回主菜单\n请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice == 1 || choice == 2 || choice == -1) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
        }

        switch (choice) {
        case 1: changePassword(); break;
        case 2: editPersonalInfo(); break;
        case -1: break;
        }
    } while (choice != -1);
}

void adminMenu(void) {
    int choice;
    do {
        system("cls");
        printf("=========================================\n");
        printf("        HIS 系统 - 院长及高管后台中心\n");
        printf("=========================================\n");
        printf("  [1] 药房管理 (入库/出库/库存检索)\n");
        printf("  [2] 智能决策系统 (数据推演/分析)\n");
        printf("  [3] 医生与排班管理 (人事调动/排班)\n");
        printf("  [4] 统计报表中心 (财务/业务流水)\n");
        printf("  [5] 个人设置 (修改密码/联系方式)\n");
        printf("  [-1] 注销并返回网关大厅\n");
        printf("-----------------------------------------\n");
        printf("请下达管理指令: ");

        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新下达管理指令: ");
        }

        switch (choice) {
        case 1:
            drugMenu();
            break;
        case 2:
            decisionMenu();
            break;
        case 3: {
            int sub;
            printf("\n-- 医生与排班管理 --\n1. 医生信息管理\n2. 门诊排班管理\n-1. 取消返回\n选择: ");
            while (1) {
                sub = safeGetInt();
                if (sub == 1 || sub == 2 || sub == -1) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
            }
            if (sub == 1) doctorMenu();
            else if (sub == 2) scheduleMenu();
            break;
        }
        case 4:
            reportMenu();
            break;
        case 5:
            personalMenu();
            break;
        case -1:
            break;
        }
    } while (choice != -1);
}
