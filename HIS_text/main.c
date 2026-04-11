#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "models.h"
#include "utils.h"
#include "patient.h"
#include "Medical_Staff_Side.h"
#include "admin.h"
#include "drug.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"
#include "fileio.h"

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
    drugList = (Drug*)malloc(sizeof(Drug)); drugList->next = NULL;
    drugHistoryList = (DrugHistory*)malloc(sizeof(DrugHistory)); drugHistoryList->next = NULL;
    scheduleList = (Schedule*)malloc(sizeof(Schedule)); scheduleList->next = NULL;
    transactionList = (Transaction*)malloc(sizeof(Transaction)); transactionList->next = NULL;
    personnelReportList = (PersonnelReport*)malloc(sizeof(PersonnelReport)); personnelReportList->next = NULL;
}

void freeAllMemory() {
    Patient* p = patientHead; Patient* nextP;
    Staff* s = staffHead; Staff* nextS;
    Medicine* m = medicineHead; Medicine* nextM;
    Record* r = recordHead; Record* nextR;
    Bed* b = bedHead; Bed* nextB;
    Transaction* t = transactionList; Transaction* nextT;
    Schedule* sch = scheduleList; Schedule* nextSch;
    Drug* dr = drugList; Drug* nextDr;
    DrugHistory* dh = drugHistoryList; DrugHistory* nextDh;

    while (p) { nextP = p->next; free(p); p = nextP; }
    while (s) { nextS = s->next; free(s); s = nextS; }
    while (m) { nextM = m->next; free(m); m = nextM; }
    while (r) { nextR = r->next; free(r); r = nextR; }
    while (b) { nextB = b->next; free(b); b = nextB; }
    while (t) { nextT = t->next; free(t); t = nextT; }
    while (sch) { nextSch = sch->next; free(sch); sch = nextSch; }
    while (dr) { nextDr = dr->next; free(dr); dr = nextDr; }
    while (dh) { nextDh = dh->next; free(dh); dh = nextDh; }
}

int main() {
    int port;
    char acc[50], pwd[50];
    int pChoice;
    Staff* s;
    Staff* me_staff;
    Patient* p;
    Patient* me_patient;

    initLists();

    loadAllDataFromTxt();
    loadDrugs();
    loadDrugHistory();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    while (1) {
        system("cls");
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

        // 【修改点】：严格过滤主端口输入
        while (1) {
            port = safeGetInt();
            if (port == 1 || port == 2 || port == 3 || port == 0) break;
            printf("  [!] 无效的登录端口，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
        }

        if (port == 1) {
            while (1) {
                printf("\n  >>> 高管身份验证 (输入0返回大厅) <<<\n");
                printf("  [?] 请输入超级管理账号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入密码: "); safeGetString(pwd, 50);

                if (strcmp(acc, admin.username) == 0 && strcmp(pwd, admin.password) == 0) {
                    adminMenu();
                    break;
                }
                else {
                    printf("  [!] 拦截：账号或密码验证失败，请重新尝试！\n");
                }
            }
        }
        else if (port == 2) {
            while (1) {
                printf("\n  >>> 医护端安全门禁 (输入0返回大厅) <<<\n");
                printf("  [?] 请刷取职工工号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入登录密码: "); safeGetString(pwd, 50);

                s = staffHead->next;
                me_staff = NULL;
                while (s) {
                    if (strcmp(s->id, acc) == 0 && strcmp(s->password, pwd) == 0) {
                        me_staff = s;
                        break;
                    }
                    s = s->next;
                }

                if (me_staff) {
                    staffTerminal(me_staff);
                    break;
                }
                else {
                    printf("  [!] 警告：工号不存在或密码错误！\n");
                }
            }
        }
        else if (port == 3) {
            printf("\n  --- 患者自助服务总机 ---\n");
            printf("  1. 验证身份证件 (账号登录)\n");
            printf("  2. 首次就诊建档 (注册档案)\n");
            printf("  0. 返回控制大厅\n");
            printf("  请选择: ");

            // 【修改点】：严格过滤患者端子菜单输入
            while (1) {
                pChoice = safeGetInt();
                if (pChoice == 1 || pChoice == 2 || pChoice == 0) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
            }

            if (pChoice == 1) {
                while (1) {
                    printf("\n  [?] 请输入患者ID (如P20251000, 输入0返回): ");
                    safeGetString(acc, 50);
                    if (strcmp(acc, "0") == 0) break;

                    printf("  [?] 请输入服务密码: "); safeGetString(pwd, 50);

                    p = patientHead->next;
                    me_patient = NULL;
                    while (p) {
                        if (strcmp(p->id, acc) == 0 && strcmp(p->password, pwd) == 0) {
                            me_patient = p;
                            break;
                        }
                        p = p->next;
                    }

                    if (me_patient) {
                        userTerminal(me_patient->id);
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
        else if (port == 0) {
            printf("\n  [系统广播] 正在将三大终端内存数据强一致性同步至物理磁盘...\n");

            saveAllDataToTxt();
            saveDrugs();
            saveDrugHistory();
            saveSchedules();
            saveTransactions();
            saveAdminData();

            printf("  [系统广播] 各模块数据封存完毕，系统正在安全释放内存碎片...\n");
            freeAllMemory();

            printf("  [系统广播] 内存释放完毕，系统已安全关闭，感谢使用！\n");
            break;
        }
    }
    return 0;
}