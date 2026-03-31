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

Admin admin;

// ---------------------------------------------------------
// 读取高管配置数据
// ---------------------------------------------------------
void loadAdminData(void) {
    FILE* fp = fopen("admin.txt", "r");
    if (!fp) return;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) strcpy(admin.username, token); else admin.username[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(admin.password, token); else admin.password[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(admin.phone, token); else admin.phone[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(admin.email, token); else admin.email[0] = '\0';
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 保存高管配置数据
// ---------------------------------------------------------
void saveAdminData(void) {
    FILE* fp = fopen("admin.txt", "w");
    fprintf(fp, "%s,%s,%s,%s\n", admin.username, admin.password, admin.phone, admin.email);
    fclose(fp);
}
// ---------------------------------------------------------
// 修改用户名模块
// ---------------------------------------------------------
void changename(void) {
    char newName[20] = { '\0' };
    printf("请输入新用户名: "); scanf("%19s", newName);
    if (strlen(newName) == 0) { printf("用户名不能为空！\n"); return; }
    strcpy(admin.username, newName);
    printf("用户名修改成功！\n");
    saveAdminData();
}
// ---------------------------------------------------------
// 修改密码模块
// ---------------------------------------------------------
void changePassword(void) {
    char old[20] = { '\0' }, new1[20] = { '\0' }, new2[20] = { '\0' };
    printf("请输入旧密码: "); scanf("%19s", old);
    if (old[19]!='\0'||strcmp(old, admin.password) != 0) { printf("旧密码错误！\n"); return; }
    printf("请输入新密码: "); scanf("%19s", new1);
	if (new1[19] != '\0' || strlen(new1) == 0) { printf("新密码格式错误！\n"); return; }
    printf("请确认新密码: "); scanf("%19s", new2);
    if (new2[19]!='\0'||strcmp(new1, new2) != 0) { printf("两次输入不一致！\n"); return; }
    strcpy(admin.password, new1);
    printf("密码修改成功！\n");
	saveAdminData();
}

// ---------------------------------------------------------
// 修改联系方式模块
// ---------------------------------------------------------
void editPersonalInfo(void) {
    printf("当前信息：\n用户名: %s\n手机号: %s\n邮箱: %s\n", admin.username, admin.phone, admin.email);
    printf("请输入新手机号（直接回车保留原值）: ");
    getchar(); char newPhone[12];
    while (1) {
        scanf("%11s", newPhone);
        if (newPhone[11] != '\0')
            printf("输入格式错误！请重新输入");
        else
            break;
    }
    if (newPhone[0] != '\n') { newPhone[strcspn(newPhone, "\n")] = 0; strcpy(admin.phone, newPhone); }
    printf("请输入新邮箱（直接回车保留原值）: ");
    char newEmail[30];
    while (1) {
        scanf("%30s", newEmail);
        if (newEmail[29] != '\0')
            printf("输入格式错误！请重新输入");
        else
            break;
    }
    if (newEmail[0] != '\n') { newEmail[strcspn(newEmail, "\n")] = 0; strcpy(admin.email, newEmail); }
    printf("个人信息更新成功！\n");
}

// ---------------------------------------------------------
// 管理员个人设置菜单
// ---------------------------------------------------------
void personalMenu() {
    int choice;
    do {
        printf("\n========== 个人设置 ==========\n");
        printf("1. 修改密码\n2. 个人信息编辑\n0. 返回主菜单\n请选择: ");
        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); choice = -1; }
        switch (choice) {
        case 1: changePassword(); break;
        case 2: editPersonalInfo(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}

// ---------------------------------------------------------
// 管理端核心总路由 (完全按照流程图定制，只做跳转，不涉加载)
// ---------------------------------------------------------
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
        printf("  [0] 注销并返回网关大厅\n");
        printf("-----------------------------------------\n");
        printf("请下达管理指令: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        switch (choice) {
        case 1: drugMenu(); break;
        case 2: decisionMenu(); break;
        case 3:
            // 医生与排班管理的二级菜单
            printf("\n-- 医生与排班管理 --\n1. 医生信息管理\n2. 门诊排班管理\n选择: ");
            int sub;
            if (scanf("%d", &sub) == 1) {
                if (sub == 1) doctorMenu();
                else if (sub == 2) scheduleMenu();
            }
            else {
				while (getchar() != '\n');
				printf("无效选择。\n");
            }
            break;
        case 4: reportMenu(); break;
        case 5: personalMenu(); break;
        case 0: break;
        default: printf("无效指令。\n"); system("pause"); break;
        }
    } while (choice != 0);
}