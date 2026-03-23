#define _CRT_SECURE_NO_WARNINGS
#include "admin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 引入队友写好的各个模块头文件
#include "drug.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"
#include "decision.h"

// 队友原本的 Admin 结构体实体
Admin admin;

// ==========================================
// 队友原本的个人设置与数据加载逻辑 (保持不变)
// ==========================================
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

void saveAdminData(void) {
    FILE* fp = fopen("admin.txt", "w");
    fprintf(fp, "%s,%s,%s,%s\n", admin.username, admin.password, admin.phone, admin.email);
    fclose(fp);
}

void changePassword(void) {
    char old[20] = { '\0' }, new1[20] = { '\0' }, new2[20] = { '\0' };
    printf("请输入旧密码: ");
    scanf("%19s", old);
    if (strcmp(old, admin.password) != 0) {
        printf("旧密码错误！\n");
        return;
    }
    printf("请输入新密码: ");
    scanf("%19s", new1);
    printf("请确认新密码: ");
    scanf("%19s", new2);
    if (strcmp(new1, new2) != 0) {
        printf("两次输入不一致！\n");
        return;
    }
    strcpy(admin.password, new1);
    printf("密码修改成功！\n");
}

void editPersonalInfo(void) {
    printf("当前信息：\n用户名: %s\n手机号: %s\n邮箱: %s\n", admin.username, admin.phone, admin.email);
    printf("请输入新手机号（直接回车保留原值）: ");
    getchar(); // 清除缓冲区
    char newPhone[15];
    fgets(newPhone, 15, stdin);
    if (newPhone[0] != '\n') {
        newPhone[strcspn(newPhone, "\n")] = 0;
        strcpy(admin.phone, newPhone);
    }
    printf("请输入新邮箱（直接回车保留原值）: ");
    char newEmail[30];
    fgets(newEmail, 30, stdin);
    if (newEmail[0] != '\n') {
        newEmail[strcspn(newEmail, "\n")] = 0;
        strcpy(admin.email, newEmail);
    }
    printf("个人信息更新成功！\n");
}

void personalMenu() {
    int choice;
    do {
        printf("\n========== 个人设置 ==========\n");
        printf("1. 修改密码\n2. 个人信息编辑\n0. 返回主菜单\n请选择: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: changePassword(); break;
        case 2: editPersonalInfo(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}

// ==========================================
// 改造后的管理端主路由 (也就是队友写的 main 函数内容)
// ==========================================
void adminMenu(void) {
    // 1. 进入管理端后，加载队友独立模块的数据
    loadDrugs();
    loadDrugHistory();
    loadDoctors();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    // 提示：因为外部的全局 main.c 已经完成了系统级的登录拦截，
    // 所以这里直接进入管理端大屏菜单，无需重复要求输入密码。
    printf("\n【系统提示】管理端数据加载完毕，欢迎登入高管后台！\n");

    int choice;
    do {
        printf("\n========== 管理端系统主菜单 ==========\n");
        printf("1. 药房管理\n");
        printf("2. 医生排班管理\n");
        printf("3. 统计报表中心\n");
        printf("4. 个人设置\n");
        printf("5. 智能辅助决策\n");
        printf("0. 退出管理端系统\n");
        printf("请选择: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // 防御非法输入
            choice = -1;
        }

        switch (choice) {
        case 1: drugMenu(); break;
        case 2: scheduleMenu(); break;
        case 3: reportMenu(); break;
        case 4: personalMenu(); break;
        case 5: decisionMenu(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);

    // 2. 退出管理端前，保存队友独立模块的所有数据
    saveDrugs();
    saveDrugHistory();
    saveDoctors();
    saveSchedules();
    saveTransactions();
    saveAdminData();

    printf("\n【系统提示】管理端数据已安全持久化保存，正在退回全院主菜单...\n");
}