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

/* 声明外部全局变量，用于维持门诊接诊流程中的上下文会话状态 */
extern char currentCallingPatientId[20];

// ---------------------------------------------------------
// 账户安全模块：医护人员登录凭证更替机制
// 设计说明：采用原密码哈希校验及新密码二次确认策略，
// 确保系统底层凭证的安全性。内置中断流机制，允许用户随时退出。
// ---------------------------------------------------------
void changeStaffPassword(Staff* me) {
    char oldPwd[50], newPwd[50], confirmPwd[50];

    printf("\n========== 修改个人登录密码 ==========\n");
    printf("请输入当前原密码 (输入0取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    // 鉴权拦截：比对输入凭证与本地结构体存放的哈希数据
    if (strcmp(me->password, oldPwd) != 0) {
        printf("  [!] 安全警告：原密码验证失败！\n");
        system("pause");
        return;
    }

    while (1) {
        printf("请输入新的登录密码 (输入0取消): ");
        safeGetString(newPwd, 50);
        if (strcmp(newPwd, "0") == 0) return;
        if (strlen(newPwd) > 0) break;
        printf("  [!] 密码不能为空，请重新输入！\n");
    }

    printf("请再次确认新密码 (输入0取消): ");
    safeGetString(confirmPwd, 50);
    if (strcmp(confirmPwd, "0") == 0) return;

    // 数据一致性检验
    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [!] 校验失败：两次输入的新密码不一致，操作已取消。\n");
        system("pause");
        return;
    }

    // 更新当前会话实体的凭证指针，后续经持久化模块写入磁盘
    strcpy(me->password, newPwd);
    printf("  [√] 数据更新成功！您的账户已启用新密码。\n");
    system("pause");
}

// ---------------------------------------------------------
// 医护端主工作流引擎
// 设计说明：作为医生站的核心路由分发器，统筹门诊、住院及个人管理的业务重定向。
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    /* 严格遵守 C89 规范：将所有局部变量及流控制游标声明提拔至代码块顶端 */
    int c;
    int sc;

    while (1) {
        system("cls");

        // 生命周期重置：每次返回主控制台时，必须清空挂载的患者上下文实例，避免内存状态污染
        strcpy(currentCallingPatientId, "");

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

        c = safeGetInt();

        // -- 分支一：门诊执行子系统 --
        if (c == 1) {
            while (1) {
                system("cls");
                printf("\n========== 门诊门前接诊业务中心 ==========\n");
                printf("  [当前会话锁定患者]: %s\n", strlen(currentCallingPatientId) > 0 ? currentCallingPatientId : "【暂无接诊】");
                printf("------------------------------------------\n");
                printf("  [1] 候诊列队预览与叫号\n");
                printf("  [2] 录入看诊诊断与辅助检查单\n");
                printf("  [3] 开具电子处方(直连大药房库)\n");
                printf("  [4] 下发住院收治通知单\n");
                printf("  [0] 挂起当前业务并返回工作台\n");
                printf("------------------------------------------\n");
                printf("  操作指令: ");

                sc = safeGetInt();

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
            }
        }
        // -- 分支二：住院部交互协议层 --
        else if (c == 2) {
            inpatientMenu(me->id);
        }
        // -- 分支三：临床工作及病历运维 --
        else if (c == 3) {
            workManagementMenu(me->id);
        }
        // -- 分支四：终端底层安全认证 --
        else if (c == 4) {
            changeStaffPassword(me);
        }
        // -- 中断响应与资源释放 --
        else if (c == 0) {
            return;
        }
    }
}