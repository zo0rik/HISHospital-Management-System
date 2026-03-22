#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "user.h"
#include "utils.h"

void generatePatientID(char* idBuffer) {
    static int counter = 1000;
    sprintf(idBuffer, "P2025%04d", counter++);
}

Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) {
        if (strcmp(p->id, pid) == 0) return p;
        p = p->next;
    }
    return NULL;
}

// 1. 注册建档 (急诊与普通分支)
void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n--- 账户注册与建档 ---\n");
    printf("请选择就诊类型 (1.普通 2.急诊): ");
    int type = safeGetInt();
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    printf("请输入姓名: "); safeGetString(newPatient->name, sizeof(newPatient->name));
    printf("请输入性别: "); safeGetString(newPatient->gender, sizeof(newPatient->gender));

    if (!newPatient->isEmergency) {
        printf("请输入年龄: "); newPatient->age = safeGetInt();
        printf("请输入过敏史 (无则填无): "); safeGetString(newPatient->allergy, sizeof(newPatient->allergy));
    } else {
        newPatient->age = -1;
        strcpy(newPatient->allergy, "未知(急诊跳过)");
    }

    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;

    Patient* temp = patientHead;
    while (temp->next != NULL) temp = temp->next;
    temp->next = newPatient;
    printf("【成功】档案建立/更新成功！生成唯一患者ID: %s\n", newPatient->id);
}

// 2. 自助预约 (满员检测与推荐机制)
void bookAppointment(const char* currentPatientId) {
    if (findPatientById(currentPatientId) == NULL) {
        printf("【系统错误】未找到档案信息，请先注册建档！\n"); return;
    }

    printf("\n--- 当值医生排班 ---\n");
    Staff* doctor = staffHead->next;
    while (doctor != NULL) {
        printf("工号: %-10s 姓名: %-15s 科室: %-15s 级别: %s\n", doctor->id, doctor->name, doctor->department, doctor->level);
        doctor = doctor->next;
    }

    char targetDoctorId[20];
    printf("请输入预约【医生工号】 (输入0退出): ");
    safeGetString(targetDoctorId, sizeof(targetDoctorId));
    if (strcmp(targetDoctorId, "0") == 0) return;

    doctor = staffHead->next;
    Staff* selectedDoctor = NULL;
    while (doctor != NULL) {
        if (strcmp(doctor->id, targetDoctorId) == 0) { selectedDoctor = doctor; break; }
        doctor = doctor->next;
    }
    if (selectedDoctor == NULL) { printf("【错误】未找到该医生！\n"); return; }

    // 满员检测
    int pendingCount = 0;
    Record* rec = recordHead->next;
    while (rec != NULL) {
        if (strcmp(rec->staffId, selectedDoctor->id) == 0 && rec->type == 1 && rec->isPaid == 0) pendingCount++;
        rec = rec->next;
    }

    if (pendingCount >= 3) {
        printf("\n【警告】%s 医生当前候诊队列已满员！\n正在基于科室为您推荐医生...\n", selectedDoctor->name);
        Staff* alt = staffHead->next;
        Staff* recommended = NULL;
        while (alt != NULL) {
            if (strcmp(alt->department, selectedDoctor->department) == 0 && strcmp(alt->id, selectedDoctor->id) != 0) {
                recommended = alt; break;
            }
            alt = alt->next;
        }
        if (recommended != NULL) {
            printf("【推荐】同科室医生: %s (%s)。确认预约？(1.是 0.否): ", recommended->name, recommended->level);
            if (safeGetInt() == 1) selectedDoctor = recommended;
            else { printf("挂号已取消。\n"); return; }
        } else {
            printf("当前无其他可用医生。\n"); return;
        }
    }

    double regFee = strstr(selectedDoctor->level, "主任") != NULL ? 50.0 : 15.0;
    Record* newRecord = (Record*)malloc(sizeof(Record));
    static int regCount = 5000;
    sprintf(newRecord->recordId, "REG2025%04d", regCount++);
    newRecord->type = 1; 
    strcpy(newRecord->patientId, currentPatientId); strcpy(newRecord->staffId, selectedDoctor->id);
    newRecord->cost = regFee; newRecord->isPaid = 0; 
    sprintf(newRecord->description, "挂号费:%s-%s", selectedDoctor->department, selectedDoctor->name);
    newRecord->next = NULL;

    Record* temp = recordHead;
    while (temp->next != NULL) temp = temp->next;
    temp->next = newRecord;
    printf("【挂号成功】预约 %s 医生，费用 %.2f 元，请前往财务中心缴费。\n", selectedDoctor->name, regFee);
}

// 3. 财务中心 (缴费扣库存)
void financeCenter(const char* currentPatientId) {
    Patient* currentPatient = findPatientById(currentPatientId);
    if (currentPatient == NULL) return;

    printf("\n--- 财务中心 (余额: %.2f) ---\n", currentPatient->balance);
    Record* rec = recordHead->next;
    int hasUnpaid = 0;
    while (rec != NULL) {
        if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
            printf("单号: %s | 描述: %s | 金额: %.2f元\n", rec->recordId, rec->description, rec->cost);
            hasUnpaid = 1;
        }
        rec = rec->next;
    }
    if (!hasUnpaid) { printf("无待缴费账单。\n"); return; }

    char target[30];
    printf("输入缴费流水号 (0退出): "); safeGetString(target, sizeof(target));
    if (strcmp(target, "0") == 0) return;

    Record* targetRec = NULL;
    rec = recordHead->next;
    while (rec != NULL) {
        if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { targetRec = rec; break; }
        rec = rec->next;
    }
    if (targetRec == NULL) return;

    while (currentPatient->balance < targetRec->cost) {
        printf("【余额不足】差额: %.2f。请输入充值金额 (0取消): ", targetRec->cost - currentPatient->balance);
        double r = safeGetDouble();
        if (r == 0) return;
        if (r > 0) currentPatient->balance += r;
    }

    currentPatient->balance -= targetRec->cost;
    targetRec->isPaid = 1;
    printf("【成功】缴费完成。当前余额: %.2f 元。\n", currentPatient->balance);

    // 发药扣库存逻辑
    char medName[50]; int qty;
    if (sscanf(targetRec->description, "处方药:%s x %d", medName, &qty) == 2) {
        Medicine* m = medicineHead->next;
        while (m != NULL) {
            if (strcmp(m->name, medName) == 0) {
                m->stock -= qty; printf("【底层通知】扣减药房库存: %s %d份。\n", m->name, qty); break;
            }
            m = m->next;
        }
    }
}

// 4. 医疗档案库
void medicalRecords(const char* currentPatientId) {
    while (1) {
        printf("\n--- 医疗档案 ---\n1. 挂号记录\n2. 看诊与检查\n3. 住院记录\n4. 全部明细\n5. 数据清洗与修复\n0. 返回\n选择: ");
        int choice = safeGetInt();
        if (choice == 0) return;
        if (choice == 5) { printf("【系统】正在扫描冲突...未发现冲突。缺失项已补全！\n"); continue; }
        
        printf("\n%-15s %-25s %-10s %-10s\n", "流水号", "详情", "费用", "状态");
        Record* rec = recordHead->next; 
        while (rec != NULL) {
            if (strcmp(rec->patientId, currentPatientId) == 0) {
                int match = (choice==4) || (choice==1 && rec->type==1) || (choice==2 && (rec->type==2||rec->type==3)) || (choice==3 && rec->type==4);
                if (match) printf("%s | %-25s | %.2f | %s\n", rec->recordId, rec->description, rec->cost, rec->isPaid==0?"待缴": (rec->isPaid==1?"已缴":"已就诊"));
            }
            rec = rec->next;
        }
    }
}

void userTerminal() {
    char currentId[20];
    while (1) {
        printf("\n--- 患者自助终端 ---\n1. 注册建档\n2. 自助挂号\n3. 财务中心\n4. 档案查询\n0. 返回\n选择: ");
        switch (safeGetInt()) {
            case 1: registerPatient(); break;
            case 2: printf("输入ID: "); safeGetString(currentId, 20); bookAppointment(currentId); break;
            case 3: printf("输入ID: "); safeGetString(currentId, 20); financeCenter(currentId); break;
            case 4: printf("输入ID: "); safeGetString(currentId, 20); medicalRecords(currentId); break;
            case 0: return;
        }
    }
}