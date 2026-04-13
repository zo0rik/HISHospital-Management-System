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

void changePassword(void) {
    char old[50] = { '\0' }, new1[50] = { '\0' }, new2[50] = { '\0' };
    printf("请输入旧密码 (输入-1取消): "); safeGetString(old, 50);
    if (strcmp(old, "-1") == 0) return;
    if (strcmp(old, admin.password) != 0) { printf("  [!] 旧密码错误！\n"); system("pause"); return; }

    /* 【BUG修复】原为 safeGetPassword(new1, 50)，但 admin.password 仅 char[20]，改为 20 防止溢出 */
    printf("请输入新密码 (至少6位，仅限字母或数字): "); safeGetPassword(new1, 20);
    if (strcmp(new1, "-1") == 0) return;

    printf("请确认新密码: "); safeGetString(new2, 20);
    if (strcmp(new1, new2) != 0) { printf("  [!] 两次输入不一致！\n"); system("pause"); return; }

    strcpy(admin.password, new1);
    printf("  [√] 密码修改成功！\n");
    saveAdminData();
    system("pause");
}

void editPersonalInfo(void) {
    printf("\n当前信息：\n用户名: %s\n手机号: %s\n邮箱: %s\n", admin.username, admin.phone, admin.email);

    char buffer[100];

    /* 【BUG修复】原为 safeGetString(buffer, 50)，但 admin.username 仅 char[20]，改为 20 防止溢出 */
    printf("\n请输入新用户名 (直接输入-1保留原值): ");
    safeGetString(buffer, 20);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.username, buffer);

    /* 【BUG修复】原为 safeGetString(buffer, 20)，但 admin.phone 仅 char[12]，改为 12 防止溢出 */
    printf("请输入新手机号 (直接输入-1保留原值): ");
    safeGetString(buffer, 12);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.phone, buffer);

    /* 【BUG修复】原为 safeGetString(buffer, 50)，但 admin.email 仅 char[30]，改为 30 防止溢出 */
    printf("请输入新邮箱 (直接输入-1保留原值): ");
    safeGetString(buffer, 30);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0) strcpy(admin.email, buffer);

    printf("  [√] 个人信息更新成功！\n");
    saveAdminData();
    system("pause");
}

void personalMenu() {
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
        case 1: drugMenu(); break;
        case 2: decisionMenu(); break;
        case 3:
            printf("\n-- 医生与排班管理 --\n1. 医生信息管理\n2. 门诊排班管理\n-1. 取消返回\n选择: ");
            int sub;
            while (1) {
                sub = safeGetInt();
                if (sub == 1 || sub == 2 || sub == -1) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
            }
            if (sub == 1) doctorMenu();
            else if (sub == 2) scheduleMenu();
            break;
        case 4: reportMenu(); break;
        case 5: personalMenu(); break;
        case -1: break;
        }
    } while (choice != -1);
}