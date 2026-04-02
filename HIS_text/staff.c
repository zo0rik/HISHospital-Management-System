#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "staff.h"
#include "utils.h"
#include "outpatient_department.h"
#include "inpatient_department.h"
#include "work_management.h"

// ---------------------------------------------------------
// 医生修改个人密码功能
// 包含了原密码校验和新密码的二次确认，防止误操作
// ---------------------------------------------------------
void changeStaffPassword(Staff* me) {
    char oldPwd[50], newPwd[50], confirmPwd[50];

    printf("\n========== 修改个人登录密码 ==========\n");
    printf("请输入当前原密码 (输入0取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    // 验证原密码是否匹配
    if (strcmp(me->password, oldPwd) != 0) {
        printf("  [!] 安全警告：原密码验证失败！\n");
        system("pause");
        return;
    }

    // 这里必须加个循环做非空拦截
    // 之前测试的时候发现，如果直接敲回车，密码会被置成空字符串，导致下次完全登不进去
    while (1) {
        printf("请输入新的登录密码: ");
        safeGetString(newPwd, 50);
        if (strlen(newPwd) > 0) break;
        printf("  [!] 密码不能为空，请重新输入！\n");
    }

    printf("请再次确认新密码: ");
    safeGetString(confirmPwd, 50);

    // 二次比对，防止手滑打错
    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [!] 校验失败：两次输入的新密码不一致，操作已取消。\n");
        system("pause");
        return;
    }

    // 校验全部通过，写入新密码
    strcpy(me->password, newPwd);
    printf("  [√] 数据更新成功！您的账户已启用新密码。\n");
    system("pause");
}

// ---------------------------------------------------------
// 医护端总路由菜单
// 负责根据医生的选择，分配到门诊、住院或管理等不同的业务子模块
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    while (1) {
        system("cls"); // 每次回到主界面清屏，保持控制台整洁

        // 每次回到工作台首页，主动清空之前接诊时锁定的患者ID
        // 这样能避免上一个病人的状态残留，防止给下一个病人看病时串号
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

                // 将具体业务分发给各个子函数去处理
                if (sc == 1) { callPatient(me->id); system("pause"); }
                else if (sc == 2) { diagnoseAndTest(me->id); system("pause"); }
                else if (sc == 3) { prescribeMedicine(me->id); system("pause"); }
                else if (sc == 4) { issueAdmissionNotice(me->id); system("pause"); }
                else if (sc == 0) break;
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
    }
}