#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "staff.h"
#include "utils.h"

// 引入拆分出去的各个业务模块
#include "outpatient_department.h"
#include "inpatient_department.h"
#include "work_management.h"

// ---------------------------------------------------------
// 新增功能：修改医生账号密码
// ---------------------------------------------------------
void changeStaffPassword(Staff* me) {
    char oldPwd[50], newPwd[50], confirmPwd[50];
    printf("\n--- 修改个人登录密码 ---\n");
    printf("请输入原密码 (输入0返回): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    if (strcmp(me->password, oldPwd) != 0) {
        printf("【错误】原密码不正确！\n");
        system("pause");
        return;
    }

    printf("请输入新密码: ");
    safeGetString(newPwd, 50);
    printf("请再次确认新密码: ");
    safeGetString(confirmPwd, 50);

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("【错误】两次输入的新密码不一致！\n");
        system("pause");
        return;
    }

    strcpy(me->password, newPwd);
    printf("【成功】密码修改成功，请牢记您的新密码！\n");
    system("pause");
}

// ---------------------------------------------------------
// 医护端总路由 (精简版)
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    while (1) {
        system("cls");
        // 进大厅自动清空接诊状态
        strcpy(currentCallingPatientId, "");

        printf("\n--- 医生工作台 (%s科: %s医师) ---\n", me->department, me->name);
        printf("1. 门诊业务中心\n2. 住院业务中心 (住院部专网)\n3. 工作管理模块 (患者追踪与记录修改)\n4. 修改登录密码\n0. 注销退出\n选择: ");
        int c = safeGetInt();

        if (c == 1) {
            while (1) {
                system("cls");
                printf("\n>> 门诊业务中心 <<\n");
                printf("当前接诊锁定患者: %s\n", strlen(currentCallingPatientId) > 0 ? currentCallingPatientId : "【无】");
                printf("1. 接诊预览与叫号\n2. 录入看诊与辅助检查\n3. 开具电子处方\n4. 下发住院通知\n0. 返回上级菜单\n请选择: ");
                int sc = safeGetInt();
                if (sc == 1) { callPatient(me->id); system("pause"); }
                else if (sc == 2) { diagnoseAndTest(me->id); system("pause"); }
                else if (sc == 3) { prescribeMedicine(me->id); system("pause"); }
                else if (sc == 4) { issueAdmissionNotice(me->id); system("pause"); }
                else if (sc == 0) break;
            }
        }
        else if (c == 2) { inpatientMenu(me->id); }
        else if (c == 3) { workManagementMenu(me->id); }
        else if (c == 4) { changeStaffPassword(me); }
        else if (c == 0) return;
    }
}