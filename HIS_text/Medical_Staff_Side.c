#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "Medical_Staff_Side.h"
#include "utils.h"
#include "outpatient_department.h"
#include "inpatient_department.h"
#include "work_management.h"

extern char currentCallingPatientId[20];

void changeStaffPassword(Staff* me) {
    char oldPwd[50], newPwd[50], confirmPwd[50];

    printf("\n========== 修改个人登录密码 ==========\n");
    printf("请输入当前原密码 (输入0取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    if (strcmp(me->password, oldPwd) != 0) {
        printf("  [!] 安全警告：原密码验证失败！\n");
        system("pause");
        return;
    }

    printf("请输入新的登录密码 (仅限字母或数字组合, 输入0取消): ");
    safeGetPassword(newPwd, 50);
    if (strcmp(newPwd, "0") == 0) return;

    printf("请再次确认新密码: ");
    safeGetString(confirmPwd, 50);

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [!] 校验失败：两次输入的新密码不一致，操作已取消。\n");
        system("pause");
        return;
    }

    strcpy(me->password, newPwd);
    printf("  [√] 数据更新成功！您的账户已启用新密码。\n");
    system("pause");
}

void staffTerminal(Staff* me) {
    int c;
    int sc;

    while (1) {
        system("cls");
        strcpy(currentCallingPatientId, "");

        // 菜单界面对齐优化，使用占位符控制宽度
        printf("\n================================================\n");
        printf("   临床医生工作台 [所属: %-10s 医师: %s]   \n", me->department, me->name);
        printf("================================================\n");
        printf("  [1] 门诊业务中心 (接诊/开药/检查)\n");
        printf("  [2] 住院业务中心 (住院部专网及查房)\n");
        printf("  [3] 工作管理模块 (患者追踪与记录修改)\n");
        printf("  [4] 账户安全设置 (修改登录密码)\n");
        printf("  [0] 注销当前会话并退出\n");
        printf("------------------------------------------------\n");
        printf("  请选择业务: ");

        // 统一使用封装的输入函数，防止输入字母导致的死循环崩溃
        int c = safeGetInt();

        if (c == 1) {
            // 进入门诊专属子菜单
            while (1) {
                system("cls");
                printf("\n========== 门诊门前接诊业务中心 ==========\n");

                // 动态显示当前是否正在接诊某位患者，体验更好
                printf("  [当前会话锁定患者]: %s\n", strlen(currentCallingPatientId) > 0 ? currentCallingPatientId : "【暂无接诊】");
                printf("------------------------------------------\n");
                printf("  [1] 候诊列队预览与叫号\n");
                printf("  [2] 录入看诊诊断与辅助检查单\n");
                printf("  [3] 开具电子处方(直连大药房库)\n");
                printf("  [4] 下发住院收治通知单\n");
                printf("  [0] 挂起当前业务并返回工作台\n");
                printf("------------------------------------------\n");
                printf("  操作指令: ");

                int sc = safeGetInt();

                if (sc == 1) {
                    callPatient(me->id);
                    system("pause");
                }
                else if (sc == 2) {
                    diagnoseAndTest(me->id);
                    system("pause");
                }
                else if (sc == 3) {
                    prescribeMedicine(me->id);
                    system("pause");
                }
                else if (sc == 4) {
                    issueAdmissionNotice(me->id);
                    system("pause");
                }
                else if (sc == 0) {
                    break;
                }
                else {
                    printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
                    system("pause");
                }
            }
        }
        else if (c == 2) {
            inpatientMenu(me->id);
        }
        else if (c == 3) {
            workManagementMenu(me->id);
        }
        else if (c == 4) {
            changeStaffPassword(me);
        }
        else if (c == 0) {
            return;
        }
        else {
            printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
            system("pause");
        }
    }
}