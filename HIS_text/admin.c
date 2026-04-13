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

// 全局管理员结构体变量，存储当前登录管理员信息
Admin admin;
// 管理员链表头结点（哨兵节点）
Admin* adminHead = NULL;
// 当前登录管理员的链表节点指针
Admin* currentAdminNode = NULL;

/**
 * @brief 根据用户名和密码查找管理员
 * @param username 用户名
 * @param password 密码
 * @return 找到返回管理员节点指针，未找到返回NULL
 */
Admin* findAdminByCredentials(const char* username, const char* password) {
    // 从头节点的下一个节点开始遍历
    Admin* p = adminHead ? adminHead->next : NULL;
    while (p) {
        // 比对用户名和密码是否完全一致
        if (strcmp(p->username, username) == 0 && strcmp(p->password, password) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

/**
 * @brief 设置当前登录的管理员节点
 * @param node 管理员链表节点
 */
void setCurrentAdminNode(Admin* node) {
    if (!node) return;
    // 记录当前登录的管理员节点
    currentAdminNode = node;
    // 将节点数据拷贝到全局admin变量
    admin = *node;
    admin.next = NULL;
}

/**
 * @brief 释放管理员链表内存，防止内存泄漏
 */
void freeAdminList(void) {
    Admin* p = adminHead;
    while (p) {
        Admin* next = p->next;
        free(p);
        p = next;
    }
    // 释放后置空指针
    adminHead = NULL;
    currentAdminNode = NULL;
    // 清空全局admin结构体
    memset(&admin, 0, sizeof(admin));
}

/**
 * @brief 将全局admin结构体的数据同步回链表节点
 * 用于修改信息后更新链表中的原始数据
 */
static void syncCurrentAdminBackToNode(void) {
    if (!currentAdminNode) return;

    // 同步用户名，防止溢出
    strncpy(currentAdminNode->username, admin.username, sizeof(currentAdminNode->username) - 1);
    currentAdminNode->username[sizeof(currentAdminNode->username) - 1] = '\0';

    // 同步密码
    strncpy(currentAdminNode->password, admin.password, sizeof(currentAdminNode->password) - 1);
    currentAdminNode->password[sizeof(currentAdminNode->password) - 1] = '\0';

    // 同步手机号
    strncpy(currentAdminNode->phone, admin.phone, sizeof(currentAdminNode->phone) - 1);
    currentAdminNode->phone[sizeof(currentAdminNode->phone) - 1] = '\0';

    // 同步邮箱
    strncpy(currentAdminNode->email, admin.email, sizeof(currentAdminNode->email) - 1);
    currentAdminNode->email[sizeof(currentAdminNode->email) - 1] = '\0';
}

/**
 * @brief 修改管理员密码功能
 */
void changePassword(void) {
    char old[50] = { '\0' }, new1[50] = { '\0' }, new2[50] = { '\0' };

    printf("请输入旧密码 (输入-1取消): ");
    safeGetString(old, 50);
    // 输入-1取消操作
    if (strcmp(old, "-1") == 0) return;

    // 验证旧密码是否正确
    if (strcmp(old, admin.password) != 0) {
        printf("  [!] 旧密码错误！\n");
        system("pause");
        return;
    }

    printf("请输入新密码 (至少6位，仅限字母或数字): ");
    safeGetPassword(new1, 20);
    if (strcmp(new1, "-1") == 0) return;

    // 新密码不能与旧密码相同
    if (strcmp(new1, old) == 0) {
        printf("  [!] 新密码不能与旧密码相同！\n");
        system("pause");
        return;
    }

    // 确认新密码
    printf("请确认新密码: ");
    safeGetString(new2, 20);
    if (strcmp(new1, new2) != 0) {
        printf("  [!] 两次输入不一致！\n");
        system("pause");
        return;
    }

    // 更新密码并同步到链表
    strcpy(admin.password, new1);
    syncCurrentAdminBackToNode();
    printf("  [√] 密码修改成功！\n");
    saveAdminData(); // 保存到文件
    system("pause");
}

/**
 * @brief 编辑管理员个人信息（用户名、电话、邮箱）
 */
void editPersonalInfo(void) {
    char buffer[100];
    // 显示当前信息
    printf("\n当前信息：\n用户名: %s\n手机号: %s\n邮箱: %s\n", admin.username, admin.phone, admin.email);

    // 修改用户名
    printf("\n请输入新用户名 (直接输入-1保留原值): ");
    safeGetString(buffer, 20);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0)
        strcpy(admin.username, buffer);

    // 修改手机号（带格式验证）
    printf("请输入新手机号 (直接输入-1保留原值): ");
    safeGetString(buffer, 12);
    if (strlen(buffer) < 11) {
        printf("  [!] 输入格式不合法！\n");
        system("pause");
        return;
    }
    // 验证手机号必须全为数字
    for (int i = 0;i < 11;i++) {
        if (buffer[i] == '\0') break;
        if (buffer[i] < '0' || buffer[i] > '9') {
            printf("  [!] 输入格式不合法！手机号只能包含数字。\n");
            system("pause");
            return;
        }
    }
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0)
        strcpy(admin.phone, buffer);

    // 修改邮箱
    printf("请输入新邮箱 (直接输入-1保留原值): ");
    safeGetString(buffer, 30);
    if (strcmp(buffer, "-1") != 0 && strlen(buffer) > 0)
        strcpy(admin.email, buffer);

    // 同步并保存
    syncCurrentAdminBackToNode();
    printf("  [√] 个人信息更新成功！\n");
    saveAdminData();
    system("pause");
}

/**
 * @brief 个人设置菜单（修改密码、编辑信息）
 */
void personalMenu(void) {
    int choice;
    do {
        system("cls");
        printf("\n========== 个人设置 ==========\n");
        printf("1. 修改密码\n2. 个人信息编辑\n-1. 返回主菜单\n请选择: ");

        // 输入合法性校验
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

/**
 * @brief 管理员主菜单
 */
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

        // 菜单输入合法性校验
        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新下达管理指令: ");
        }

        // 菜单功能分发
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
            // 子菜单输入校验
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