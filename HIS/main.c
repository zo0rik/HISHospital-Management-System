#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "patient.h"
#include "staff.h"
#include "admin.h"

// 实例化全新的全局链表头指针
UserList userHead = NULL;
PatientList patientHead = NULL;
StaffList staffHead = NULL;
MedicineList medicineHead = NULL;
ScheduleList scheduleHead = NULL;
BedList bedHead = NULL;
RecordList recordHead = NULL;

// 统一登录验证网关
void login(const char* required_role1, const char* required_role2) {
    char account[20], pwd[50];
    printf("\n>>> 登录身份验证 <<<\n请输入账号/工号: "); safeGetString(account, 20);
    printf("请输入系统密码: "); safeGetString(pwd, 50);

    User* u = userHead;
    while (u) {
        // 注意：此处已经替换为 account 和 password
        if (strcmp(u->account, account) == 0 && strcmp(u->password, pwd) == 0) {
            if (strcmp(u->role, required_role1) == 0 || (required_role2 && strcmp(u->role, required_role2) == 0)) {
                printf("=> 登录成功！系统网关放行。\n");
                system("pause");

                // 分流进入各个子系统
                if (strcmp(u->role, "patient") == 0) patientMenu(u);
                else if (strcmp(u->role, "doctor") == 0 || strcmp(u->role, "nurse") == 0) staffMenu(u);
                else if (strcmp(u->role, "admin") == 0) adminMenu();
                return;
            }
            else {
                printf("=> 权限受阻：您的岗位或科室不匹配当前选择的通道！\n");
                system("pause"); return;
            }
        }
        u = u->next;
    }
    printf("=> 登录失败：账号密码错误 / 或该人员已离职。\n");
    system("pause");
}

int main() {
    // 启动系统底层服务，提取历史记录
    loadAllData();

    int main_choice;
    do {
        system("cls");
        printf("=========================================\n");
        printf("   现代大型综合医院 HIS 系统控制台\n");
        printf("=========================================\n");
        printf("  [1] 院级管理端总控入口\n");
        printf("  [2] 临床医护工作站入口\n");
        printf("  [3] 患者自助服务端入口\n");
        printf("  [0] 断开连接并退出系统\n");
        printf("-----------------------------------------\n");
        printf("请选择您的访问端口: ");
        main_choice = safeGetInt();

        switch (main_choice) {
        case 1:
            login("admin", NULL);
            break;
        case 2:
            login("doctor", "nurse");
            break;
        case 3:
            printf("\n>> 患者自助大厅\n 1. 登录已有医疗档案\n 2. 首次就诊快速注册\n选择: ");
            if (safeGetInt() == 1) login("patient", NULL);
            else patientRegister();
            break;
        case 0:
            printf("\n所有数据封存完毕，进程结束运行...\n");
            break;
        }
    } while (main_choice != 0);

    return 0;
}