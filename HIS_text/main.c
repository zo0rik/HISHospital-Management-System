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

/*
 * 全局链表头结点初始化。
 * 采用带头结点的单链表结构，目的是在进行节点插入和删除操作时，
 * 保持操作逻辑的一致性，避免对头指针为空的边界情况进行额外判断，提高底层数据操作的稳定性。
 */
PatientList patientHead = NULL;
StaffList staffHead = NULL;
MedicineList medicineHead = NULL;
RecordList recordHead = NULL;
BedList bedHead = NULL;

/* 初始化各个业务模块的链表头节点，分配初始内存空间 */
void initLists() {
    patientHead = (PatientList)malloc(sizeof(Patient)); patientHead->next = NULL;
    staffHead = (StaffList)malloc(sizeof(Staff)); staffHead->next = NULL;
    medicineHead = (MedicineList)malloc(sizeof(Medicine)); medicineHead->next = NULL;
    recordHead = (RecordList)malloc(sizeof(Record)); recordHead->next = NULL;
    bedHead = (BedList)malloc(sizeof(Bed)); bedHead->next = NULL;
}

int main() {
    /* * 严格遵循 C89 编译标准，将所有局部变量声明集中在代码块最顶部。
     * 避免在可执行语句之后声明变量引发编译期语法错误。
     */
    int port;
    char acc[50], pwd[50];
    int pChoice;
    Staff* s;
    Staff* me_staff;
    Patient* p;
    Patient* me_patient;

    initLists();

    /*
     * 系统启动时的全量数据加载机制：
     * 将物理磁盘中的本地 TXT 数据一次性反序列化并加载至内存链表中。
     * 运行时所有增删改查均在内存中完成，以此换取极高的系统响应速度。
     */
    loadAllDataFromTxt();
    loadDrugs();
    loadDrugHistory();
    loadDoctors();
    loadSchedules();
    loadTransactions();
    loadAdminData();

    while (1) {
        system("cls"); // 刷新控制台缓冲区，保持交互界面的视觉整洁
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

        /* 调用封装的安全输入接口，拦截非法字符及越界输入，防止系统陷入死循环 */
        port = safeGetInt();

        // ------------------ 端口 1: 管理端 ------------------
        if (port == 1) {
            while (1) {
                printf("\n  >>> 高管身份验证 (输入0返回大厅) <<<\n");
                printf("  [?] 请输入超级管理账号: ");
                safeGetString(acc, 50);

                // 提供中断当前操作的条件分支，避免用户在交互流中受困
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入动态口令密码 (输入0返回): ");
                safeGetString(pwd, 50);
                if (strcmp(pwd, "0") == 0) break;

                // 校验输入的凭证与本地存储的明文/哈希是否匹配
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
                printf("  [?] 请刷取职工工号: ");
                safeGetString(acc, 50);
                if (strcmp(acc, "0") == 0) break;

                printf("  [?] 请输入登录密码 (输入0返回): ");
                safeGetString(pwd, 50);
                if (strcmp(pwd, "0") == 0) break;

                // 遍历医护人员链表查找对应的身份标识
                s = staffHead->next;
                me_staff = NULL;
                while (s) {
                    if (strcmp(s->id, acc) == 0 && strcmp(s->password, pwd) == 0) {
                        me_staff = s;
                        break;
                    }
                    s = s->next;
                }

                // 根据鉴权结果分发相应的系统资源控制权
                if (me_staff) {
                    staffTerminal(me_staff);
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

            pChoice = safeGetInt();

            if (pChoice == 1) {
                while (1) {
                    printf("\n  [?] 请输入患者ID (如P20251000, 输入0返回): ");
                    safeGetString(acc, 50);
                    if (strcmp(acc, "0") == 0) break;

                    printf("  [?] 请输入服务密码 (输入0返回): ");
                    safeGetString(pwd, 50);
                    if (strcmp(pwd, "0") == 0) break;

                    // 检索患者基础档案树进行并发认证
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
        // ------------------ 端口 0: 安全退出与数据持久化 ------------------
        else if (port == 0) {
            printf("\n  [系统广播] 正在将三大终端内存数据强一致性同步至物理磁盘...\n");

            // 在进程终止前，统一调用序列化函数将内存状态刷入本地文件，防止核心业务数据丢失
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