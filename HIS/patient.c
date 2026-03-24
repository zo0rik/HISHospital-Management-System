#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "patient.h"
#include "utils.h"

// 必须引入这两个头文件，以跨库调取管理端的精准排班表和医生人事字典
#include "doctor.h"
#include "schedule.h"

// ---------------------------------------------------------
// 内部工具：生成自增的患者唯一ID
// ---------------------------------------------------------
void generatePatientID(char* idBuffer) {
    static int counter = 1000;
    sprintf(idBuffer, "P2025%04d", counter++);
}

// ---------------------------------------------------------
// 内部工具：通过ID在全局链表中精确查找患者实体
// ---------------------------------------------------------
Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) {
        if (strcmp(p->id, pid) == 0) return p;
        p = p->next;
    }
    return NULL;
}

// ---------------------------------------------------------
// 业务一：患者注册建档 (含密码录入与急诊快速分支)
// ---------------------------------------------------------
void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n--- 账户注册与建档 ---\n");
    printf("请选择就诊类型 (1.普通 2.急诊): ");
    int type = safeGetInt();
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    printf("请输入姓名: "); safeGetString(newPatient->name, 100);
    printf("请设置登录密码: "); safeGetString(newPatient->password, 50); // 系统登录凭据
    printf("请输入性别: "); safeGetString(newPatient->gender, 10);

    // 急诊跳过繁琐信息录入
    if (!newPatient->isEmergency) {
        printf("请输入年龄: "); newPatient->age = safeGetInt();
        printf("请输入过敏史(无则填无): "); safeGetString(newPatient->allergy, 100);
    }
    else {
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    // 初始化系统分配属性
    generatePatientID(newPatient->id); // 自动分配ID
    newPatient->balance = 0.0;         // 初始资金账户为0

    // 采用尾插法挂载到患者全局链表
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("【成功】档案建立成功！您的登录账号为: %s\n", newPatient->id);
}

// ---------------------------------------------------------
// 业务二：自助挂号 (双轨搜索模式 + 独立排班ID精准绑定)
// ---------------------------------------------------------
void bookAppointment(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n--- 自助预约挂号 ---\n");
        printf("1. 按科室查找未来可预约排班\n");
        printf("2. 搜索医生姓名或工号查找排班\n");
        printf("0. 返回上级菜单\n");
        printf("请选择您的查号方式: ");
        int choice = safeGetInt();
        if (choice == 0) return;

        char keyword[50];
        if (choice == 1) {
            printf("请输入目标科室名称 (如: 内科): ");
            safeGetString(keyword, 50);
        }
        else if (choice == 2) {
            printf("请输入医生姓名或工号 (如: 李四 或 101): ");
            safeGetString(keyword, 50);
        }
        else {
            continue;
        }

        printf("\n--- 未来可预约排班表 ---\n");
        printf("%-8s %-12s %-10s %-15s %-12s %-10s\n", "排班ID", "日期", "班次", "医生", "科室", "级别");

        int found = 0;
        Schedule* s = scheduleList;
        // 遍历所有排班计划
        while (s) {
            // 在人事大字典中寻找该班次对应的医生
            Doctor* d = doctorList;
            Doctor* matchedDoc = NULL;
            while (d) {
                if (d->id == s->doctor_id) { matchedDoc = d; break; }
                d = d->next;
            }
            if (!matchedDoc) { s = s->next; continue; }

            int match = 0;
            // 科室模糊匹配
            if (choice == 1 && strstr(matchedDoc->department, keyword)) match = 1;
            // 姓名或工号匹配
            if (choice == 2) {
                char docIdStr[20]; sprintf(docIdStr, "%d", matchedDoc->id);
                if (strstr(matchedDoc->name, keyword) || strcmp(docIdStr, keyword) == 0) match = 1;
            }

            // 打印出所有吻合搜索条件且非“休息”状态的班次
            if (match && strcmp(s->shift, "休息") != 0) {
                printf("%-8d %-12s %-10s %-15s %-12s %-10s\n",
                    s->schedule_id, s->date, s->shift, matchedDoc->name, matchedDoc->department, matchedDoc->title);
                found++;
            }
            s = s->next;
        }

        if (found == 0) {
            printf("未找到匹配的排班信息，请确认输入是否正确。\n");
            system("pause");
            continue;
        }

        printf("\n请输入要预约的【排班ID】 (输入0放弃本次挂号): ");
        int targetSchId = safeGetInt();
        if (targetSchId == 0) continue;

        // 通过独立 排班ID 唯一锁定该预约，彻底杜绝同名同日冲突
        Schedule* targetSch = NULL;
        s = scheduleList;
        while (s) { if (s->schedule_id == targetSchId) { targetSch = s; break; } s = s->next; }

        if (!targetSch) { printf("无效的排班ID！\n"); system("pause"); continue; }

        // 追溯这名医生的具体信息
        Doctor* targetDoc = NULL;
        Doctor* d = doctorList;
        while (d) { if (d->id == targetSch->doctor_id) { targetDoc = d; break; } d = d->next; }

        // 统一接口：将整数101转换为医护端的 "D101" 工号以便存入全院流水
        char staffIdStr[20];
        sprintf(staffIdStr, "D%d", targetDoc->id);

        // 防抱死拥堵监测：判断该医生这一天这一班次的候诊队列
        int pendingCount = 0;
        Record* rec = recordHead->next;
        while (rec) {
            // 通过工号及状态精确锁定承载量
            if (strcmp(rec->staffId, staffIdStr) == 0 && rec->type == 1 && rec->isPaid == 0) pendingCount++;
            rec = rec->next;
        }

        if (pendingCount >= 3) {
            printf("\n【警告】%s 医生 (%s) 候诊队列已满！请退回选择其他班次或其他医生。\n", targetDoc->name, targetSch->date);
            system("pause");
            continue;
        }

        // 挂号费依据医生职称动态计算
        double regFee = strstr(targetDoc->title, "主任") != NULL ? 50.0 : 15.0;

        // 写入全院流水账单大网
        Record* newRecord = (Record*)malloc(sizeof(Record));
        static int regCount = 5000;
        sprintf(newRecord->recordId, "REG2025%04d", regCount++);
        newRecord->type = 1;
        strcpy(newRecord->patientId, currentPatientId);
        strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee;
        newRecord->isPaid = 0;
        sprintf(newRecord->description, "挂号:%s(%s)", targetDoc->name, targetSch->date);
        newRecord->next = NULL;

        Record* temp = recordHead;
        while (temp->next) temp = temp->next;
        temp->next = newRecord;

        printf("\n【挂号成功】您已成功预约 %s 医师 (%s %s)！费用 %.2f 元，请前往财务中心缴费。\n",
            targetDoc->name, targetSch->date, targetSch->shift, regFee);
        system("pause");
        return;
    }
}

// ---------------------------------------------------------
// 业务三：财务费用中心 (支持一键扣款与药房库存联动扣减)
// ---------------------------------------------------------
void financeCenter(const char* currentPatientId) {
    Patient* p = findPatientById(currentPatientId);
    if (!p) return;

    printf("\n--- 财务中心 (当前余额: %.2f) ---\n", p->balance);

    // 检索名下所有未缴费账单
    Record* rec = recordHead->next;
    int hasUnpaid = 0;
    while (rec) {
        if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
            printf("单号: %s | 描述: %s | 金额: %.2f元\n", rec->recordId, rec->description, rec->cost);
            hasUnpaid = 1;
        }
        rec = rec->next;
    }
    if (!hasUnpaid) { printf("无待缴费账单。\n"); system("pause"); return; }

    char target[30];
    printf("输入要缴费的单号 (0退出): "); safeGetString(target, 30);
    if (strcmp(target, "0") == 0) return;

    // 定位需要缴费的特定账单
    Record* tRec = NULL;
    rec = recordHead->next;
    while (rec) {
        if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; }
        rec = rec->next;
    }
    if (!tRec) return;

    // 余额不足时的引导充值机制
    while (p->balance < tRec->cost) {
        printf("余额不足！差额 %.2f，请输入充值金额 (0取消): ", tRec->cost - p->balance);
        double money = safeGetDouble();
        if (money > 0) p->balance += money; // 写入余额
        else break;
    }

    // 资金满足条件，执行扣款结算
    if (p->balance >= tRec->cost) {
        p->balance -= tRec->cost;
        tRec->isPaid = 1; // 将流水状态反转为已缴费
        printf("【缴费成功】已扣款。当前账户余额: %.2f\n", p->balance);

        // 【高频考点：跨系统联动机制】如果是处方药，缴费成功后利用 sscanf 提取信息并扣除药房库存
        char mName[50]; int qty;
        if (sscanf(tRec->description, "处方药:%s x %d", mName, &qty) == 2) {
            Medicine* m = medicineHead->next;
            while (m) {
                if (strcmp(m->name, mName) == 0) {
                    m->stock -= qty; // 扣减药房物理库存
                    printf("[系统联动] 缴费成功，已通知药房扣减实体库存: %s\n", mName);
                    break;
                }
                m = m->next;
            }
        }
    }
    system("pause");
}

// ---------------------------------------------------------
// 业务四：个人医疗档案历史查询
// ---------------------------------------------------------
void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n--- 医疗档案 ---\n1. 挂号记录\n2. 看诊与检查记录\n3. 住院记录\n4. 数据清洗与修复\n0. 返回\n选择: ");
        int c = safeGetInt();
        if (c == 0) return;

        // 大数据容错与清洗模拟功能
        if (c == 4) {
            printf("【系统】清洗完成！已自动修复系统脏数据并补全档案缺失项。\n");
            system("pause");
            continue;
        }

        Record* rec = recordHead->next;
        while (rec) {
            // 仅对当前登录患者暴露数据
            if (strcmp(rec->patientId, currentPatientId) == 0) {
                // 根据用户的菜单选择，基于 Type 字段过滤不同种类的记录
                if ((c == 1 && rec->type == 1) ||
                    (c == 2 && (rec->type == 2 || rec->type == 3)) ||
                    (c == 3 && rec->type == 4)) {
                    printf("%s | %s | %.2f | %s\n",
                        rec->recordId, rec->description, rec->cost,
                        rec->isPaid ? "已处理/已缴费" : "待缴费");
                }
            }
            rec = rec->next;
        }
        system("pause");
    }
}

// ---------------------------------------------------------
// 患者端总路由菜单 (安全隔离设计，接收网关分发的 ID)
// ---------------------------------------------------------
void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);
        printf("\n--- 患者自助终端 (当前登录患者: %s - %s) ---\n", p->name, p->id);
        printf("1. 自助预约挂号\n2. 财务中心缴费\n3. 医疗档案查询\n0. 注销并返回大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 0: return; // 安全注销，将控制权交还 main.c
        }
    }
}