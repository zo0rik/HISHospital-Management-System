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

// 初始化各个链表头结点
void initLists() {
    patientHead = (PatientList)malloc(sizeof(Patient));
    staffHead = (StaffList)malloc(sizeof(Staff));
    medicineHead = (MedicineList)malloc(sizeof(Medicine));
    recordHead = (RecordList)malloc(sizeof(Record));
    bedHead = (BedList)malloc(sizeof(Bed));
    drugList = (Drug*)malloc(sizeof(Drug));
    drugHistoryList = (DrugHistory*)malloc(sizeof(DrugHistory));
    scheduleList = (Schedule*)malloc(sizeof(Schedule));
    transactionList = (Transaction*)malloc(sizeof(Transaction));
    personnelReportList = (PersonnelReport*)malloc(sizeof(PersonnelReport));

    // 检查内存分配是否成功
    if (!patientHead || !staffHead || !medicineHead || !recordHead || !bedHead ||
        !drugList || !drugHistoryList || !scheduleList || !transactionList || !personnelReportList) {
        printf("内存分配失败！\n");
        exit(1);
    }

    // 清零头结点，next 默认就是 NULL
    memset(patientHead, 0, sizeof(Patient));
    memset(staffHead, 0, sizeof(Staff));
    memset(medicineHead, 0, sizeof(Medicine));
    memset(recordHead, 0, sizeof(Record));
    memset(bedHead, 0, sizeof(Bed));
    memset(drugList, 0, sizeof(Drug));
    memset(drugHistoryList, 0, sizeof(DrugHistory));
    memset(scheduleList, 0, sizeof(Schedule));
    memset(transactionList, 0, sizeof(Transaction));
    memset(personnelReportList, 0, sizeof(PersonnelReport));
}

// 释放所有链表内存
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

// 主函数：系统总入口
int main() {
    int port;
    char acc[50], pwd[50];
    int pChoice;
    Staff* s;
    Staff* me_staff;
    Patient* p;
    Patient* me_patient;

    // 初始化链表并加载数据
    initLists();
    loadAllDataFromTxt();
    loadDrugs();
    loadDrugHistory();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    // 启动时输出简单诊断信息
    {
        int pc = 0, sc2 = 0, dc = 0;
        Patient* pp = patientHead->next; while (pp) { pc++; pp = pp->next; }
        Staff* ss = staffHead->next; while (ss) { sc2++; ss = ss->next; }
        Drug* dd = drugList->next; while (dd) { dc++; dd = dd->next; }
        printf("  [系统启动诊断] 已加载患者:%d 医生:%d 药品:%d\n", pc, sc2, dc);
        if (pc == 0) printf("  [严重警告] 患者数据为0！请确认patients.txt在程序运行目录下！\n");
    }

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
        printf("      [-1] 安全断开连接     (保存所有数据并退出)\n");
        printf("\n  ========================================================\n");
        printf("  请选择要访问的业务端口: ");

        // 校验端口输入
        while (1) {
            port = safeGetInt();
            if (port == 1 || port == 2 || port == 3 || port == -1) break;
            printf("  [!] 无效的登录端口，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
        }

        // 管理端登录
        if (port == 1) {
            while (1) {
                Admin* me_admin;
                printf("\n  >>> 管理端安全门禁 (输入-1返回大厅) <<<\n");
                printf("  [?] 请输入管理员账号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "-1") == 0) break;
                if (strlen(acc) == 0) {
                    printf("  [!] 管理员账号不能为空！\n");
                    system("pause");
                    continue;
                }

                printf("  [?] 请输入登录密码: ");
                safeGetString(pwd, 50);

                me_admin = findAdminByCredentials(acc, pwd);
                if (me_admin) {
                    setCurrentAdminNode(me_admin);
                    adminMenu();
                    break;
                }
                else {
                    printf("  [!] 拦截：管理员账号不存在或密码错误，请重新尝试！\n");
                    system("pause");
                }
            }
        }

        // 医护端登录
        else if (port == 2) {
            while (1) {
                printf("\n  >>> 医护端安全门禁 (输入-1返回大厅) <<<\n");
                printf("  [?] 请刷取职工工号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "-1") == 0) break;

                printf("  [?] 请输入登录密码: ");
                safeGetString(pwd, 50);

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
                    system("pause");
                }
            }
        }

        // 患者端入口
        else if (port == 3) {
            printf("\n  --- 患者自助服务总机 ---\n");
            printf("  1. 验证身份证件 (账号登录)\n");
            printf("  2. 首次就诊建档 (注册档案)\n");
            printf(" -1. 返回控制大厅\n");
            printf("  请选择: ");

            // 校验患者端菜单输入
            while (1) {
                pChoice = safeGetInt();
                if (pChoice == 1 || pChoice == 2 || pChoice == -1) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
            }

            // 患者登录
            if (pChoice == 1) {
                while (1) {
                    printf("\n  [?] 请输入患者ID (如P20261000, 输入-1返回): ");
                    safeGetString(acc, 50);
                    if (strcmp(acc, "-1") == 0) break;

                    if (strlen(acc) == 0) {
                        printf("  [!] 请输入用户ID，不允许为空！\n");
                        system("pause");
                        continue;
                    }

                    printf("  [?] 请输入服务密码: ");
                    safeGetString(pwd, 50);

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
            // 患者注册
            else if (pChoice == 2) {
                registerPatient();
                system("pause");
            }
        }

        // 安全退出系统
        else if (port == -1) {
            printf("\n  [系统广播] 正在将三大终端内存数据强一致性同步至物理磁盘...\n");

            // 保存所有数据
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