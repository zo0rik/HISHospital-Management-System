#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "schedule.h"
#include "doctor.h"
#include "outpatient_department.h"

char currentCallingPatientId[20] = "";

void generateRecordID(char* buffer) {
    int maxId = 1999; // 基础值
    Record* r = recordHead->next;
    while (r != NULL) {
        int currentIdNum;
        if (sscanf(r->recordId, "R2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) {
                maxId = currentIdNum;
            }
        }
        r = r->next;
    }
    sprintf(buffer, "R2025%04d", maxId + 1);
}

void callPatient(const char* docId) {
    system("cls");
    printf("\n--- 您的排班日期预览 ---\n");

    Schedule* s = scheduleList;
    int sCount = 0;
    char availableDates[20][15];

    while (s) {
        if (s->doctor_id == atoi(docId + 1)) {
            int duplicate = 0;
            for (int i = 0; i < sCount; i++) {
                if (strcmp(availableDates[i], s->date) == 0) duplicate = 1;
            }
            if (!duplicate) {
                strcpy(availableDates[sCount], s->date);
                sCount++;
            }
        }
        s = s->next;
    }

    if (sCount == 0) { printf("您近期暂无排班安排。\n"); return; }

    for (int i = 0; i < sCount; i++) { printf("[%d] 日期: %s\n", i + 1, availableDates[i]); }
    printf("[0] 返回上一级\n请选择要查看的排班日期编号: ");

    int dChoice = safeGetInt();
    if (dChoice == 0 || dChoice > sCount) return;
    char* targetDate = availableDates[dChoice - 1];

    printf("\n--- [%s] 预约患者名单概览 ---\n", targetDate);
    printf("%-10s %-15s %-15s %-15s %-10s\n", "顺序号", "挂号单流水", "患者ID", "患者姓名", "状态");
    int count = 0;
    Record* r = recordHead->next;

    while (r) {
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next; char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

            char status[30];
            if (r->isPaid == 0) strcpy(status, "待缴费");
            else if (r->isPaid == 1) strcpy(status, "已缴费");
            else strcpy(status, "已接诊");

            int seqNum = 0; char* seqPtr = strstr(r->description, "排号:");
            if (seqPtr) sscanf(seqPtr, "排号:%d", &seqNum);

            printf("%-10d %-15s %-15s %-15s %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }

    if (count == 0) { printf("当日暂无患者预约。\n"); return; }

    printf("\n是否呼叫下一位排队患者进入诊室？(1.是 0.否): ");
    if (safeGetInt() == 1) {
        r = recordHead->next; int found = 0;
        while (r) {
            if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid != 2 && strstr(r->description, targetDate)) {
                Patient* p = patientHead->next; char pName[100] = "未知";
                while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                printf("\n【呼叫系统】请 [%s - %s] 进入诊室就诊!\n", r->patientId, pName);
                r->isPaid = 2; found = 1;
                strcpy(currentCallingPatientId, r->patientId);
                printf(">> 系统已自动锁定该患者身份，后续看诊/开药无需重复输入ID。 <<\n");
                break;
            }
            r = r->next;
        }
        if (!found) printf("当前排队患者已全部接诊完毕！\n");
    }
}

void diagnoseAndTest(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入就诊患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    char symptoms[100], diag[100];
    printf("录入临床症状: "); safeGetString(symptoms, 100);
    printf("录入临床诊断: "); safeGetString(diag, 100);

    Record* r2 = (Record*)malloc(sizeof(Record));
    generateRecordID(r2->recordId);
    r2->type = 2; strcpy(r2->patientId, pId); strcpy(r2->staffId, docId);
    r2->cost = 20.0; r2->isPaid = 0;
    sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);
    getCurrentTimeStr(r2->createTime, 30);
    r2->next = recordHead->next; recordHead->next = r2;
    printf("【通知】已生成包含症状与诊断的 ②看诊记录。\n");

    printf("是否需要开具辅助检查单？(1.是 0.否): ");
    int needTest; while (1) { needTest = safeGetInt(); if (needTest == 0 || needTest == 1) break; }
    if (needTest == 1) {
        char testName[50];
        printf("请输入检查名称(如:血常规/X光): "); safeGetString(testName, 50);
        printf("请输入该项检查费用: "); double tCost = safeGetDouble();

        Record* r4 = (Record*)malloc(sizeof(Record));
        generateRecordID(r4->recordId);
        r4->type = 4; strcpy(r4->patientId, pId); strcpy(r4->staffId, docId);
        r4->cost = tCost; r4->isPaid = 0;
        sprintf(r4->description, "检查名称:%s", testName);
        getCurrentTimeStr(r4->createTime, 30);
        r4->next = recordHead->next; recordHead->next = r4;
        printf("【通知】已生成专属 ④检查单记录，已同步至患者待缴费。\n");
    }
}

// 注意：在 outpatient_department.c 顶部需要加上 #include "drug.h"

void prescribeMedicine(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    while (1) {
        char key[50];
        printf("\n搜索药品名称或编号关键字 (输入0结束开药): "); safeGetString(key, 50);
        if (strcmp(key, "0") == 0) break;

        // 【核心修改：接入队友的 Drug 链表】
        Drug* d = drugList;
        Drug* matched[100] = { NULL }; int mCount = 0;

        printf("\n--- 匹配到的药品库 ---\n");
        while (d) {
            if (strstr(d->name, key) || strstr(d->id, key)) {
                if (mCount < 100) {
                    matched[mCount] = d;
                    printf("[%d] 编号:%s | 名称:%-15s | 单价:%.2f | 库存:%d\n", mCount + 1, d->id, d->name, d->price, d->stock);
                    mCount++;
                }
            }
            d = d->next;
        }

        if (mCount == 0) { printf("未找到包含该关键字的药品，请重新输入。\n"); continue; }

        char mChoiceStr[50];
        printf("请输入要开具的药品【前置序号】或【真实编号】(输入0重新搜索): ");
        safeGetString(mChoiceStr, 50);
        if (strcmp(mChoiceStr, "0") == 0) continue;

        Drug* selectedMed = NULL; int isNum = 1;
        for (int i = 0; i < strlen(mChoiceStr); i++) { if (mChoiceStr[i] < '0' || mChoiceStr[i] > '9') { isNum = 0; break; } }
        if (isNum) {
            int idx = atoi(mChoiceStr);
            if (idx > 0 && idx <= mCount) selectedMed = matched[idx - 1];
        }
        if (!selectedMed) {
            for (int i = 0; i < mCount; i++) { if (strcmp(matched[i]->id, mChoiceStr) == 0) { selectedMed = matched[i]; break; } }
        }
        if (!selectedMed) { printf("输入无效！未找到对应的药品序号或编号。\n"); continue; }

        int qty;
        while (1) {
            printf("请输入 [%s] 的开具数量 (单次最多开10盒): ", selectedMed->name);
            qty = safeGetPositiveInt();
            if (qty > 10) {
                printf("【警告】医保规定单种药品一次最多开具 10 盒！\n");
            }
            else if (qty > selectedMed->stock) {
                printf("【警告】药房库存不足！当前剩余库存为: %d\n", selectedMed->stock);
            }
            else {
                break;
            }
        }

        // 【药房实时同步】：直接扣减队友的 Drug 库存！
        selectedMed->stock -= qty;
        printf("【药房实时同步】已扣减 %s 库存 %d 盒，当前余量: %d\n", selectedMed->name, qty, selectedMed->stock);

        double totalCost = qty * selectedMed->price;

        Record* r3 = (Record*)malloc(sizeof(Record));
        extern void generateRecordID(char* buffer);
        generateRecordID(r3->recordId);
        r3->type = 3; strcpy(r3->patientId, pId); strcpy(r3->staffId, docId);
        r3->cost = totalCost; r3->isPaid = 0;
        sprintf(r3->description, "药品:%s_单价:%.2f_数量:%d_总价:%.2f", selectedMed->name, selectedMed->price, qty, totalCost);
        getCurrentTimeStr(r3->createTime, 30);
        r3->next = recordHead->next; recordHead->next = r3;

        printf("【成功】③处方记录已生成下发，该药总计 %.2f 元。您可以继续搜索开药。\n", totalCost);
    }
}

void issueAdmissionNotice(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n当前接诊患者: %s\n", pId);
    }
    else {
        printf("请输入需下达住院指令的患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    printf("请判定该患者是否为重症？(1.重症优先 0.普通): ");
    int isSevere; while (1) { isSevere = safeGetInt(); if (isSevere == 0 || isSevere == 1) break; }

    printf("请填写简单入院诊断/说明: "); char note[100]; safeGetString(note, 100);

    Record* r6 = (Record*)malloc(sizeof(Record));
    generateRecordID(r6->recordId);
    r6->type = 6; strcpy(r6->patientId, pId); strcpy(r6->staffId, docId);
    r6->cost = 0.0; r6->isPaid = 0;
    sprintf(r6->description, "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);
    getCurrentTimeStr(r6->createTime, 30);
    r6->next = recordHead->next; recordHead->next = r6;
    printf("【成功】住院通知已下发至住院部！(已标记为%s)。\n", isSevere ? "重症" : "普通");
}