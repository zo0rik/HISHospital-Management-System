#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "models.h"
#include "utils.h"
#include "patient.h"
#include "staff.h"
#include "admin.h"
#include "drug.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"

// 全局初始化带有头结点的单链表。
// 经过前期的反复调试验证，带头结点的链表在后续做节点插入和删除操作时，
// 不需要频繁判断头指针是否为空，能极大地减少代码冗余并避免指针越界导致的段错误！
PatientList patientHead = NULL;
StaffList staffHead = NULL;
MedicineList medicineHead = NULL;
RecordList recordHead = NULL;
BedList bedHead = NULL;

// 初始化各模块链表头结点空间
void initLists() {
    patientHead = (PatientList)malloc(sizeof(Patient)); patientHead->next = NULL;
    staffHead = (StaffList)malloc(sizeof(Staff)); staffHead->next = NULL;
    medicineHead = (MedicineList)malloc(sizeof(Medicine)); medicineHead->next = NULL;
    recordHead = (RecordList)malloc(sizeof(Record)); recordHead->next = NULL;
    bedHead = (BedList)malloc(sizeof(Bed)); bedHead->next = NULL;
}

int main() {
    initLists();

    // 系统启动时，一次性把所有的本地 TXT 底层数据全拉到内存链表里。
    // 运行期间完全基于内存交互，实现了极高的响应速度，只有执行安全退出时才覆写回硬盘。
    loadAllDataFromTxt();
    loadDrugs();
    loadDrugHistory();
    loadDoctors();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    while (1) {
        system("cls"); // 每次回到主界面前执行清屏，保持控制台交互界面的整洁
        printf("\n");
        printf("  ========================================================\n");
        printf("  ||                                                    ||\n");
        printf("  ||         现 代 大 型 综 合 医 院 H I S 系 统        ||\n");
        printf("  ||                                                    ||\n");
        printf("  ========================================================\n");
        printf("\n");
        printf("       [1] 管理端登录入口   (高管业务与财务报表)\n");
        printf("       [2] 医护端登录入口   (临床接诊与住院中心)\n");
        printf("       [3] 患者端自助终端   (在线挂号与费用清算)\n");
        printf("       [0] 安全断开连接     (保存所有数据并退出)\n");
        printf("\n  ========================================================\n");
        printf("  请选择要访问的业务端口: ");

        // 这里全面弃用了原生的 scanf，改用自定义封装的严格类型读取函数接管所有输入。
        // 彻底杜绝了用户意外输入字母或乱码所导致的控制台无限死循环崩溃Bug。
        int port = safeGetInt();
        char acc[50], pwd[50];

        // ------------------ 端口 1: 管理端 ------------------
        if (port == 1) {
            while (1) {
                printf("\n  >>> 高管身份验证 (输入0返回大厅) <<<\n");
                printf("  [?] 请输入超级管理账号: "); safeGetString(acc, 50);

                // 提供随时退出的后悔机制，避免用户在登录流中卡死
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入动态口令密码: "); safeGetPassword(pwd, 50);

                if (strcmp(acc, admin.username) == 0 && strcmp(pwd, admin.password) == 0) {
                    adminMenu();
                    break;
                }
                else {
                    printf("  [!] 拦截：账号或密码验证失败，请重新尝试！\n");
                }
            }
        }
        // ------------------ 端口 2: 医护端 ------------------
        else if (port == 2) {
            while (1) {
                printf("\n  >>> 医护端安全门禁 (输入0返回大厅) <<<\n");
                printf("  [?] 请刷取职工工号: "); safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入登录密码: "); safeGetPassword(pwd, 50);

                // 遍历医护人员链表核对身份
                Staff* s = staffHead->next; Staff* me = NULL;
                while (s) {
                    if (strcmp(s->id, acc) == 0 && strcmp(s->password, pwd) == 0) { me = s; break; }
                    s = s->next;
                }

                if (me) {
                    staffTerminal(me);
                    break;
                }
                else {
                    printf("  [!] 警告：工号不存在或密码错误！\n");
                }
            }
        }
        // ------------------ 端口 3: 患者端 ------------------
        else if (port == 3) {
            printf("\n  --- 患者自助服务总机 ---\n");
            printf("  1. 验证身份证件 (账号登录)\n");
            printf("  2. 首次就诊建档 (注册档案)\n");
            printf("  0. 返回控制大厅\n");
            printf("  请选择: ");

            int pChoice = safeGetInt();

            if (pChoice == 1) {
                while (1) {
                    printf("\n  [?] 请输入患者ID (如P20251000, 输入0返回): "); safeGetString(acc, 50);
                    if (strcmp(acc, "0") == 0) break;

                    printf("  [?] 请输入服务密码: "); safeGetPassword(pwd, 50);

                    // 检索患者档案库进行匹配
                    Patient* p = patientHead->next; Patient* me = NULL;
                    while (p) {
                        if (strcmp(p->id, acc) == 0 && strcmp(p->password, pwd) == 0) { me = p; break; }
                        p = p->next;
                    }

                    if (me) {
                        userTerminal(me->id);
                        break;
                    }
                    else {
                        printf("  [!] 拦截：查无此人或密码有误！\n");
                    }
                }
            }
            else if (pChoice == 2) {
                registerPatient();
                system("pause");
            }
        }
        // ------------------ 端口 0: 安全退出 ------------------
        else if (port == 0) {
            printf("\n  [系统广播] 正在将三大终端内存数据强一致性同步至物理磁盘...\n");

            // 退出前统一执行写盘操作，保证所有业务产生的增删改查数据不丢失
            saveAllDataToTxt();
            saveDrugs();
            saveDrugHistory();
            saveDoctors();
            saveSchedules();
            saveTransactions();
            saveAdminData();

            printf("  [系统广播] 各模块数据封存完毕，系统已安全关闭！\n");
            break;
        }
    }
    return 0;
}