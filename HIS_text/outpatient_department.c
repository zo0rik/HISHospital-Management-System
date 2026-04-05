#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "schedule.h"
#include "doctor.h"
#include "outpatient_department.h"
#include "drug.h"

char currentCallingPatientId[20] = "";

void generateRecordID(char* buffer) {
    int maxId = 1999;
    Record* r = recordHead->next;

    while (r != NULL) {
        int currentIdNum;
        if (sscanf(r->recordId, "R2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) maxId = currentIdNum;
        }
        r = r->next;
    }
    sprintf(buffer, "R2025%04d", maxId + 1);
}

void callPatient(const char* docId) {
    system("cls");
    printf("\n========== 智能排班溯源与叫号 ==========\n");

    Schedule* s = scheduleList; int sCount = 0; char availableDates[20][15];
    while (s) {
        if (s->doctor_id == atoi(docId + 1)) {
            int duplicate = 0;
            for (int i = 0; i < sCount; i++) { if (strcmp(availableDates[i], s->date) == 0) duplicate = 1; }
            if (!duplicate) { strcpy(availableDates[sCount], s->date); sCount++; }
        }
        s = s->next;
    }

    if (sCount == 0) { printf("  [!] 调度台回执：您在系统近期排班表中是真空状态。\n"); return; }

    printf("  [授权排班日期选择树]:\n");
    for (int i = 0; i < sCount; i++) { printf("   [%d] %s\n", i + 1, availableDates[i]); }
    printf("   [0] 脱离排班中心\n----------------------------------------\n  选择检索索引号: ");

    int dChoice = safeGetInt();
    if (dChoice == 0) return;

    if (dChoice < 0 || dChoice > sCount) {
        printf("  [!] 输入越界：无此索引号，操作已终止。\n");
        return;
    }
    char* targetDate = availableDates[dChoice - 1];

    printf("\n========== [%s] 候诊大厅鱼骨图 ==========\n", targetDate);
    printf(" %-8s | %-14s | %-12s | %-12s | %-10s\n", "系统位次", "挂号流转凭证", "患者全局ID", "档案署名", "当前生命周期");
    printf("------------------------------------------------------------------------\n");
    int count = 0; Record* r = recordHead->next;

    while (r) {
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next; char pName[100] = "脱敏未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

            char status[30];
            if (r->isPaid == 0) strcpy(status, "[卡死]缺费");
            else if (r->isPaid == 1) strcpy(status, "[活跃]候诊");
            else strcpy(status, "[完结]离室");

            int seqNum = 0; char* seqPtr = strstr(r->description, "排号:");
            if (seqPtr) sscanf(seqPtr, "排号:%d", &seqNum);

            printf("  %-7d | %-14s | %-12s | %-12s | %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }
    if (count == 0) { printf("  今日无病患来访。\n"); return; }
    printf("------------------------------------------------------------------------\n");

    printf("\n  [业务指令] 是否自动广播叫号，将第一顺位合法患者请入诊室？(1.同意 0.拒绝): ");
    if (safeGetInt() == 1) {
        r = recordHead->next; int found = 0;
        while (r) {
            if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1 && strstr(r->description, targetDate)) {
                Patient* p = patientHead->next; char pName[100] = "未知";
                while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                printf("\n  【走廊大屏广播】 -> \"请 %s 号患者 [%s] 到 %s 诊室就医。\"\n", r->patientId, pName, docId);

                r->isPaid = 2;
                found = 1;

                strcpy(currentCallingPatientId, r->patientId);
                printf("  >>> HIS 引擎已强锁死当前看诊沙盒为: %s <<<\n", pName);
                break;
            }
            r = r->next;
        }
        if (!found) printf("  [提示] 您的鱼骨图上已无任何存活且已缴费的排队病患！\n");
    }
}

void diagnoseAndTest(const char* docId) {
    char pId[20];

    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [系统自动提取当前病患ID]: %s\n", pId);
    }
    else {
        printf("\n  [无关联病患] 请手动键入要下达诊断的病患ID (输入0终止): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;
    }

    char symptoms[100], diag[100];

    while (1) { printf("  请敲入宏观体征表象(如咳嗽发热, 输入0取消): "); safeGetString(symptoms, 100); if (strcmp(symptoms, "0") == 0) return; if (strlen(symptoms) > 0) break; }
    while (1) { printf("  请给出权威确切诊断学结论(输入0取消): "); safeGetString(diag, 100); if (strcmp(diag, "0") == 0) return; if (strlen(diag) > 0) break; }

    Record* r2 = (Record*)malloc(sizeof(Record)); generateRecordID(r2->recordId);
    r2->type = 2; strcpy(r2->patientId, pId); strcpy(r2->staffId, docId);
    r2->cost = 20.0; r2->isPaid = 0; sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);
    getCurrentTimeStr(r2->createTime, 30); r2->next = recordHead->next; recordHead->next = r2;
    printf("  [√] 核心看诊数据入库封存。\n");

    printf("\n  需延伸生化/影像仪器的辅助验证吗？(1.开单 0.放弃): ");

    if (safeGetInt() == 1) {
        char testName[50];
        while (1) { printf("  指令项目全称(如颅脑CT, 输入0取消): "); safeGetString(testName, 50); if (strcmp(testName, "0") == 0) return; if (strlen(testName) > 0) break; }
        printf("  该物料使用耗损费定价 (输入0取消): ");
        double tCost = safeGetDouble();
        if (tCost == 0) return;

        Record* r4 = (Record*)malloc(sizeof(Record)); generateRecordID(r4->recordId);
        r4->type = 4; strcpy(r4->patientId, pId); strcpy(r4->staffId, docId);
        r4->cost = tCost; r4->isPaid = 0; sprintf(r4->description, "检查名称:%s", testName);
        getCurrentTimeStr(r4->createTime, 30); r4->next = recordHead->next; recordHead->next = r4;
        printf("  [√] 第4号子系统触发，检查单已推送患者手机微信端催缴。\n");
    }
}

void prescribeMedicine(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) { strcpy(pId, currentCallingPatientId); printf("\n  [自动挂载处方对象]: %s\n", pId); }
    else { printf("\n  请输入孤立患者ID (输入0终止): "); safeGetString(pId, 20); if (strcmp(pId, "0") == 0) return; }

    while (1) {
        char key[50];
        printf("\n  请检索大药房物资库字典 (拼音/编号/药名，输0退出配药): "); safeGetString(key, 50);
        if (strcmp(key, "0") == 0) break;

        Drug* d = drugList; Drug* matched[100] = { NULL }; int mCount = 0;

        printf("\n  ========== 药品中央储备池 镜像匹配 ==========\n");
        printf("  %-4s | %-8s | %-16s | %-8s | %-8s\n", "映射", "系统内码", "国家标准药名", "单价", "物理库存");
        printf("-----------------------------------------------------------\n");

        while (d) {
            if (strstr(d->name, key) || strstr(d->id, key)) {
                if (mCount < 100) {
                    matched[mCount] = d;
                    printf("  [%-2d] | %-8s | %-16s | %-8.2f | %-8d\n", mCount + 1, d->id, d->name, d->price, d->stock);
                    mCount++;
                }
            }
            d = d->next;
        }

        if (mCount == 0) { printf("  [!] 检索库穿透，查无此药，请换词再试。\n"); continue; }

        char mChoiceStr[50];
        printf("-----------------------------------------------------------\n");
        printf("  击打左侧【映射号】或直接敲【系统内码】定位药品 (输入0取消): ");
        safeGetString(mChoiceStr, 50); if (strcmp(mChoiceStr, "0") == 0) continue;

        Drug* selectedMed = NULL; int isNum = 1;
        for (int i = 0; i < strlen(mChoiceStr); i++) { if (mChoiceStr[i] < '0' || mChoiceStr[i] > '9') { isNum = 0; break; } }
        if (isNum) { int idx = atoi(mChoiceStr); if (idx > 0 && idx <= mCount) selectedMed = matched[idx - 1]; }
        if (!selectedMed) { for (int i = 0; i < mCount; i++) { if (strcmp(matched[i]->id, mChoiceStr) == 0) { selectedMed = matched[i]; break; } } }
        if (!selectedMed) { printf("  [!] 定位脱靶，输入无法解析。\n"); continue; }

        int qty;
        while (1) {
            printf("  提取 [%s] 物理装填数目 (高红线拦截10盒, 输入0取消): ", selectedMed->name);
            qty = safeGetPositiveInt();
            if (qty == 0) break; // 跳出此循环取消本次配药，退回重新检索

            if (qty > 10) { printf("  [越权预警] 卫健委指令：处方药单次过载(超10盒)！打回重填。\n"); }
            else if (qty > selectedMed->stock) { printf("  [缺货熔断] 中央仓库存穿透！当前最高只能开: %d 盒\n", selectedMed->stock); }
            else { break; }
        }

        if (qty == 0) continue;

        selectedMed->stock -= qty;
        printf("  [底层联动] 强行剥离大库实体，[%s]已被扣取 %d盒，残留余量:%d\n", selectedMed->name, qty, selectedMed->stock);

        double totalCost = qty * selectedMed->price;

        Record* r3 = (Record*)malloc(sizeof(Record)); extern void generateRecordID(char* buffer);
        generateRecordID(r3->recordId);
        r3->type = 3; strcpy(r3->patientId, pId); strcpy(r3->staffId, docId);
        r3->cost = totalCost; r3->isPaid = 0;
        sprintf(r3->description, "药品:%s_单价:%.2f_数量:%d_总价:%.2f", selectedMed->name, selectedMed->price, qty, totalCost);
        getCurrentTimeStr(r3->createTime, 30); r3->next = recordHead->next; recordHead->next = r3;

        printf("  [√] 云端处方下达指令送出，价值 %.2f 元。 (循环配药开启)\n", totalCost);
    }
}

void issueAdmissionNotice(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) { strcpy(pId, currentCallingPatientId); printf("\n  [接诊关联]: %s\n", pId); }
    else { printf("\n  键入需强制收容的患者识别码 (输入0返回): "); safeGetString(pId, 20); if (strcmp(pId, "0") == 0) return; }

    printf("  是否开启危重高优先通道越级抢床位？(1.重症直达 0.取消返回): ");
    int isSevere;

    while (1) {
        isSevere = safeGetInt();
        if (isSevere == 0 || isSevere == 1) break;
        else printf("  [!] 输入越界，请规范输入 1 或 0: ");
    }

    if (isSevere == 0) {
        // 由于原逻辑里没有取消的概念，且这里没有要求必须选0取消，但为了满足“0返回”的需求，此处调整判断逻辑
        printf("  您输入了0，为了区分业务逻辑，确认是否返回上一层？(1.返回 0.继续普通常规): ");
        int confirm = safeGetInt();
        if (confirm == 1) return;
        isSevere = 0;
    }

    printf("  填写拟定收治初步说明(严禁为空, 输入0取消): "); char note[100];
    while (1) { safeGetString(note, 100); if (strcmp(note, "0") == 0) return; if (strlen(note) > 0) break; }

    Record* r6 = (Record*)malloc(sizeof(Record)); generateRecordID(r6->recordId);
    r6->type = 6; strcpy(r6->patientId, pId); strcpy(r6->staffId, docId);
    r6->cost = 0.0; r6->isPaid = 0;

    sprintf(r6->description, "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);
    getCurrentTimeStr(r6->createTime, 30); r6->next = recordHead->next; recordHead->next = r6;

    printf("  [√] 通知单越界发送至住院部。(打标属性: %s)\n", isSevere ? "重症" : "普通");
}