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

PatientList patientHead = NULL;
StaffList staffHead = NULL;
MedicineList medicineHead = NULL;
RecordList recordHead = NULL;
BedList bedHead = NULL;

void initLists() {
    patientHead = (PatientList)malloc(sizeof(Patient)); patientHead->next = NULL;
    staffHead = (StaffList)malloc(sizeof(Staff)); staffHead->next = NULL;
    medicineHead = (MedicineList)malloc(sizeof(Medicine)); medicineHead->next = NULL;
    recordHead = (RecordList)malloc(sizeof(Record)); recordHead->next = NULL;
    bedHead = (BedList)malloc(sizeof(Bed)); bedHead->next = NULL;
}

int main() {
    initLists();

    loadAllDataFromTxt();
    loadDrugs();
    loadDrugHistory();
    loadDoctors();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    while (1) {
        system("cls");
        printf("=========================================\n");
        printf("   现代大型综合医院 HIS 系统控制台大厅   \n");
        printf("=========================================\n");
        printf("  [1] 管理端登录口 (高管后台)\n");
        printf("  [2] 医护端登录口 (临床工作站)\n");
        printf("  [3] 患者端登录口 (含自助挂号注册)\n");
        printf("  [0] 断开连接并保存所有数据退出\n");
        printf("=========================================\n");
        printf("请选择您的访问端口: ");

        int port = safeGetInt();
        char acc[50], pwd[50];

        // ------------------ 端口 1: 管理端 ------------------
        if (port == 1) {
            while (1) { // 【新增】死循环拦截
                printf("\n>>> 管理端身份验证 (输入0取消) <<<\n请输入管理账号: "); safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break; // 输入0退出当前验证，返回大厅

                printf("请输入口令密码: "); safeGetString(pwd, 50);

                if (strcmp(acc, admin.username) == 0 && strcmp(pwd, admin.password) == 0) {
                    adminMenu();
                    break;
                }
                else {
                    printf("【拦截】账号或密码错误！请重新尝试。\n");
                }
            }
        }
        // ------------------ 端口 2: 医护端 ------------------
        else if (port == 2) {
            while (1) { // 【新增】死循环拦截
                printf("\n>>> 医护端身份验证 (输入0取消) <<<\n请输入工号: "); safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break;

                printf("请输入密码: "); safeGetString(pwd, 50);

                Staff* s = staffHead->next;
                Staff* me = NULL;
                while (s) {
                    if (strcmp(s->id, acc) == 0 && strcmp(s->password, pwd) == 0) {
                        me = s; break;
                    }
                    s = s->next;
                }
                if (me) {
                    staffTerminal(me);
                    break;
                }
                else {
                    printf("【拦截】工号不存在或密码错误！请重新尝试。\n");
                }
            }
        }
        // ------------------ 端口 3: 患者端 ------------------
        else if (port == 3) {
            printf("\n-- 患者服务端 --\n1. 账号密码登录\n2. 首次就诊建档(注册)\n0. 返回大厅\n请选择: ");
            int pChoice = safeGetInt();

            if (pChoice == 1) {
                while (1) { // 【新增】死循环拦截
                    printf("\n请输入患者ID (如P20251000, 输入0取消): "); safeGetString(acc, 50);
                    if (strcmp(acc, "0") == 0) break;

                    printf("请输入登录密码: "); safeGetString(pwd, 50);

                    Patient* p = patientHead->next;
                    Patient* me = NULL;
                    while (p) {
                        if (strcmp(p->id, acc) == 0 && strcmp(p->password, pwd) == 0) {
                            me = p; break;
                        }
                        p = p->next;
                    }
                    if (me) {
                        userTerminal(me->id);
                        break;
                    }
                    else {
                        printf("【拦截】患者ID或密码错误！请重新尝试。\n");
                    }
                }
            }
            else if (pChoice == 2) {
                registerPatient();
                system("pause");
            }
        }
        // ------------------ 端口 0: 安全退出并保存 ------------------
        else if (port == 0) {
            printf("\n正在将三端数据封存至物理磁盘...\n");
            saveAllDataToTxt();
            saveDrugs();
            saveDrugHistory();
            saveDoctors();
            saveSchedules();
            saveTransactions();
            saveAdminData();
            printf("所有数据已安全持久化保存，系统成功退出！\n");
            break;
        }
    }
    return 0;
}