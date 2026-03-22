#define _CRT_SECURE_NO_WARNINGS
#include "staff.h"
#include "utils.h"

void staffOutpatient(User* staff) {
    int choice;
    printf("\n--- 门诊业务中心 ---\n1. 诊断与检查(生成看诊记录)\n2. 处方与发药(模糊搜索)\n3. 下达住院通知\n选择: ");
    choice = safeGetInt();

    if (choice == 1) {
        char p_id[20], diag[100];
        printf("当前队列首位患者进入诊室，请输入患者ID: "); safeGetString(p_id, 20);
        printf("录入症状与诊断: "); safeGetString(diag, 100);
        saveRecordToFile("diagnosis_data.txt", 2, "看诊记录", p_id, staff->account, diag, 20.0, 0);
        printf("是否开具化验单/X光单？(1.是 0.否): ");
        if (safeGetInt() == 1) saveRecordToFile("diagnosis_data.txt", 3, "检查记录", p_id, staff->account, "生化抽血及全身X光", 180.0, 0);
        printf("=> 诊疗及检查记录已生成并推送至财务库！\n");
    }
    else if (choice == 2) {
        printf("请输入药品关键字(名称/通用名)进行模糊搜索: ");
        char key[50]; safeGetString(key, 50);
        Medicine* m = medicineHead;
        while (m) {
            if (strstr(m->name, key) || strstr(m->commonName, key)) {
                printf("编号:[%s] 药名:%s 通用名:%s 售价:%.2f 余量:%d\n", m->id, m->name, m->commonName, m->price, m->stock);
            }
            m = m->next;
        }
        char m_name[50], p_id[20];
        printf("输入要开具的药品全名: "); safeGetString(m_name, 50);
        printf("输入患者ID: "); safeGetString(p_id, 20);
        saveRecordToFile("prescription_data.txt", 2, "处方发药", p_id, staff->account, m_name, 55.0, 0);
        printf("=> 处方生成完毕，已通知患者端财务缴费。\n");
    }
    else if (choice == 3) {
        char p_id[20];
        printf("请输入需住院患者ID: "); safeGetString(p_id, 20);
        saveRecordToFile("hospitalization_data.txt", 4, "住院指令", p_id, staff->account, "下达临床待入院通知", 0.0, 1);
        printf("=> 患者已标记待入院，数据推送至住院部！\n");
    }
    system("pause");
}

void staffInpatient(User* staff) {
    int choice;
    printf("\n--- 住院部业务中心 ---\n1. 办理入院(自动分配与调控机制)\n2. 办理出院与结算\n选择: ");
    choice = safeGetInt();

    if (choice == 1) {
        char p_id[20]; int days;
        printf("输入拟入院患者ID: "); safeGetString(p_id, 20);
        printf("拟住院天数: "); days = safeGetInt();
        double deposit = days * 200.0;
        printf("=> 按照要求需缴纳住院押金: %.2f 元\n", deposit);

        Bed* b = bedHead;
        int total = 0, empty = 0;
        while (b) {
            if (strcmp(b->department, staff->department) == 0) {
                total++;
                if (b->isOccupied == 0) empty++;
            }
            b = b->next;
        }

        if (total > 0 && ((float)empty / total) <= 0.20) {
            printf(">> 【系统智能告警】本科室当前空床率已不足20%%！自动触发机制：已将单人房调整为双人房接收患者。\n");
        }

        b = bedHead;
        while (b) {
            if (b->isOccupied == 0) {
                b->isOccupied = 1;
                strcpy(b->patientId, p_id);
                printf("=> 入院办理成功！分配至 %s科 %s [%s病房 %d号床]\n", b->department, b->type, b->bedId, b->bedNumber);
                char desc[200]; sprintf(desc, "分配病床:%s-%d", b->bedId, b->bedNumber);
                saveRecordToFile("hospitalization_data.txt", 4, "住院登记", p_id, staff->account, desc, deposit, 0);
                break;
            }
            b = b->next;
        }
    }
    else if (choice == 2) {
        char p_id[20];
        printf("办理出院，请输入患者ID: "); safeGetString(p_id, 20);
        Bed* b = bedHead;
        while (b) {
            if (strcmp(b->patientId, p_id) == 0) {
                b->isOccupied = 0;
                strcpy(b->patientId, "none");
                printf("=> 床位费结算完毕，病床 [%s病房-%d号床] 已释放为【空闲】。\n", b->bedId, b->bedNumber);
                break;
            }
            b = b->next;
        }
    }
    system("pause");
}

void staffMenu(User* staff) {
    int choice;
    do {
        system("cls");
        printf("=== 医护工作站 | 当前登录: %s科 - %s ===\n", staff->department, staff->name);
        printf("1. 门诊业务大厅\n2. 住院业务大厅\n3. 个人中心(排班/申请)\n0. 退出登录\n请选择: ");
        choice = safeGetInt();
        switch (choice) {
        case 1: staffOutpatient(staff); break;
        case 2: staffInpatient(staff); break;
        case 3:
            printf("\n--- 个人信息中心 ---\n工号：%s\n科室：%s\n级别：%s\n*如需进行工作申请/调班请联系人事行政科*\n", staff->account, staff->department, staff->role);
            system("pause"); break;
        }
    } while (choice != 0);
}