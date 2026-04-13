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
    /* 【修复】使用calloc确保哨兵节点全部清零 */
    patientHead = (PatientList)calloc(1, sizeof(Patient)); patientHead->next = NULL;
    staffHead = (StaffList)calloc(1, sizeof(Staff)); staffHead->next = NULL;
    medicineHead = (MedicineList)calloc(1, sizeof(Medicine)); medicineHead->next = NULL;
    recordHead = (RecordList)calloc(1, sizeof(Record)); recordHead->next = NULL;
    bedHead = (BedList)calloc(1, sizeof(Bed)); bedHead->next = NULL;
    drugList = (Drug*)calloc(1, sizeof(Drug)); drugList->next = NULL;
    drugHistoryList = (DrugHistory*)calloc(1, sizeof(DrugHistory)); drugHistoryList->next = NULL;
    scheduleList = (Schedule*)calloc(1, sizeof(Schedule)); scheduleList->next = NULL;
    transactionList = (Transaction*)calloc(1, sizeof(Transaction)); transactionList->next = NULL;
    personnelReportList = (PersonnelReport*)calloc(1, sizeof(PersonnelReport)); personnelReportList->next = NULL;
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

    /* 【新增】启动时输出加载诊断信息，帮助定位数据文件问题 */
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
        printf("      [-1] 安全断开连接     (保存所有数据并退出)\n"); /* 【修改】0->-1 */
        printf("\n  ========================================================\n");
        printf("  请选择要访问的业务端口: ");

        while (1) {
            port = safeGetInt();
            if (port == 1 || port == 2 || port == 3 || port == -1) break; /* 【修改】0->-1 */
            printf("  [!] 无效的登录端口，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
        }

        if (port == 1) {
            while (1) {
                Admin* me_admin;
                printf("\n  >>> 管理端安全门禁 (输入-1返回大厅) <<<\n");
                printf("  [?] 请输入管理员账号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "-1") == 0) break;
                if (strlen(acc) == 0) { printf("  [!] 管理员账号不能为空！\n"); system("pause"); continue; }

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
        else if (port == 2) {
            while (1) {
                printf("\n  >>> 医护端安全门禁 (输入-1返回大厅) <<<\n"); /* 【修改】 */
                printf("  [?] 请刷取职工工号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "-1") == 0) break; /* 【修改】 */

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
                    system("pause");
                }
            }
        }
        else if (port == 3) {
            printf("\n  --- 患者自助服务总机 ---\n");
            printf("  1. 验证身份证件 (账号登录)\n");
            printf("  2. 首次就诊建档 (注册档案)\n");
            printf(" -1. 返回控制大厅\n"); /* 【修改】 */
            printf("  请选择: ");

            while (1) {
                pChoice = safeGetInt();
                if (pChoice == 1 || pChoice == 2 || pChoice == -1) break; /* 【修改】 */
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
            }

            if (pChoice == 1) {
                while (1) {
                    printf("\n  [?] 请输入患者ID (如P20261000, 输入-1返回): "); /* 【修改】 */
                    safeGetString(acc, 50);
                    if (strcmp(acc, "-1") == 0) break; /* 【修改】 */
                    /* 【新增】空输入校验 */
                    if (strlen(acc) == 0) { printf("  [!] 请输入用户ID，不允许为空！\n"); system("pause"); continue; }

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
        else if (port == -1) { /* 【修改】0->-1 */
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
