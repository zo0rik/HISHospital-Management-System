#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include "inpatient_department.h"
#include "transaction.h"
// ---------------------------------------------------------
// 内部工具：通过住院通知单追溯负责科室
// ---------------------------------------------------------
void getResponsibleDept(const char* patientId, char* deptBuffer) {
    strcpy(deptBuffer, "未知科室");
    Record* r = recordHead->next;
    char staffId[20] = "";
    while (r) {
        if (r->type == 6 && strcmp(r->patientId, patientId) == 0) {
            strcpy(staffId, r->staffId);
        }
        r = r->next;
    }
    if (strlen(staffId) > 0) {
        Staff* s = staffHead->next;
        while (s) {
            if (strcmp(s->id, staffId) == 0) {
                strcpy(deptBuffer, s->department);
                break;
            }
            s = s->next;
        }
    }
}

void initBedsIfEmpty() {
    if (bedHead->next) return;

    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };

    Bed* tail = bedHead;
    int roomNum = 1;
    for (int d = 0; d < 5; d++) {
        for (int i = 0; i < 5; i++) {
            for (int j = 1; j <= 4; j++) {
                Bed* b = (Bed*)malloc(sizeof(Bed));
                sprintf(b->bedId, "%d-%d", roomNum, j);
                b->isOccupied = 0; strcpy(b->patientId, ""); strcpy(b->bedType, types[i]);
                b->price = prices[i]; strcpy(b->wardType, (i >= 3) ? wards[1] : wards[0]);
                b->isRoundsDone = 0;
                b->next = NULL; tail->next = b; tail = b;
            }
            roomNum++;
        }
    }
}

const char* getRoomDepartment(const char* bedId) {
    int roomNum;
    sscanf(bedId, "%d-", &roomNum);
    int deptIdx = (roomNum - 1) / 5;
    if (deptIdx == 0) return "内科";
    if (deptIdx == 1) return "外科";
    if (deptIdx == 2) return "儿科";
    if (deptIdx == 3) return "妇产科";
    if (deptIdx == 4) return "急诊科";
    return "未知科室";
}

void checkAndAdjustBedTension(const char* targetDept) {
    int total = 0, empty = 0;
    Bed* b = bedHead->next;

    // 只统计目标科室的床位情况
    while (b) {
        if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
            total++;
            if (b->isOccupied == 0) empty++;
        }
        b = b->next;
    }

    // 如果该科室空床率低于 20%，触发专属扩容机制
    if (total > 0 && ((float)empty / total) < 0.2f) {
        printf("\n【系统动态调度】%s 专属空床率不足 20%%！触发紧急扩容机制...\n", targetDept);
        printf(">>> 正在将【%s】内的单人陪护疗养病房临时调整为双人病房 <<<\n", targetDept);

        b = bedHead->next;
        int converted = 0;
        while (b) {
            // 只拆分本科室的、且目前空闲的单人疗养房
            if (b->isOccupied == 0 &&
                strcmp(b->bedType, "单人陪护疗养病房") == 0 &&
                strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {

                strcpy(b->bedType, "双人病房");
                b->price = 150.0; // 调整为双人房价格

                Bed* extra = (Bed*)malloc(sizeof(Bed)); *extra = *b;
                sprintf(extra->bedId, "%sA", b->bedId); // 生成附属加床如 5-1A
                extra->next = bedHead->next; bedHead->next = extra;
                converted++;
            }
            b = b->next;
        }
        if (converted > 0) {
            printf("【扩容成功】已为 %s 增加了 %d 张可用双人床位！\n", targetDept, converted);
        }
        else {
            printf("【扩容失败】%s 当前已无空余的单人疗养病房可供拆分。\n", targetDept);
        }
    }
}

// ---------------------------------------------------------
// 内部工具：动态抓取科室名称并拼接成提示字符串
// ---------------------------------------------------------
void getDynamicDeptPrompt(char* promptBuffer) {
    char depts[20][50]; int dCount = 0;
    Staff* stf = staffHead->next;
    while (stf) {
        int exists = 0;
        for (int i = 0; i < dCount; i++) if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; }
        if (!exists && strlen(stf->department) > 0) { strcpy(depts[dCount], stf->department); dCount++; }
        stf = stf->next;
    }

    strcpy(promptBuffer, "");
    for (int i = 0; i < dCount; i++) {
        strcat(promptBuffer, depts[i]);
        if (i < dCount - 1) strcat(promptBuffer, "/");
    }
}

// ---------------------------------------------------------
// 1. 查看科室专属图谱
// ---------------------------------------------------------
void viewAllBeds() {
    initBedsIfEmpty();
    while (1) {
        system("cls");
        printf("\n========== 全院病房与床位实时使用图谱 ==========\n");

        char deptStr[200]; getDynamicDeptPrompt(deptStr);
        char targetDept[50];
        printf("请输入要查看的科室名称 (%s, 0返回): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n>>> 【%s】 专属住院病区 <<<\n", targetDept);
        printf("%-10s %-12s %-18s %-8s %-10s %-15s %-15s\n", "房号-床位", "病区等级", "房型", "价格", "状态", "主治医生", "入住患者(ID)");

        Bed* b = bedHead->next;
        int bedCount = 0;
        while (b) {
            if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                char patName[100] = "-"; char docName[50] = "-";
                if (b->isOccupied) {
                    Patient* p = patientHead->next;
                    while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(patName, p->name); break; } p = p->next; }

                    Record* r = recordHead->next;
                    while (r) {
                        if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                            Staff* s = staffHead->next;
                            while (s) { if (strcmp(s->id, r->staffId) == 0) { strcpy(docName, s->name); break; } s = s->next; }
                            break;
                        }
                        r = r->next;
                    }
                }
                printf("%-10s %-12s %-18s %-8.2f %-10s %-15s %s(%s)\n",
                    b->bedId, b->wardType, b->bedType, b->price,
                    b->isOccupied ? "[占用]" : "[空闲]", docName,
                    b->isOccupied ? patName : "-", b->isOccupied ? b->patientId : "-");
                bedCount++;
            }
            b = b->next;
        }
        if (bedCount == 0) printf("  (未找到该科室的专属病床信息)\n");
        system("pause");
    }
}

// ---------------------------------------------------------
// 2. 办理入院
// ---------------------------------------------------------
void admitPatient(const char* docId) {
    initBedsIfEmpty();
    // 【修改点】移除了顶部的全局 checkAndAdjustBedTension()，改为在确定科室后单独调用

    printf("\n--- 门诊下发《待入院通知单》队列 ---\n");
    Record* r = recordHead->next; int noticeCount = 0;

    printf("【重症优先通道】\n");
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "重症")) {
            Patient* p = patientHead->next; char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
            char deptName[50]; getResponsibleDept(r->patientId, deptName);
            printf(" -> [紧急] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    printf("\n【普通入院队列】\n");
    r = recordHead->next;
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "普通")) {
            Patient* p = patientHead->next; char pName[100] = "未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
            char deptName[50]; getResponsibleDept(r->patientId, deptName);
            printf(" -> [常规] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    if (noticeCount == 0) { printf("当前暂无门诊下发的住院通知单。\n"); system("pause"); return; }

    char pId[20];
    printf("\n请输入需办理入院的患者ID (0返回): "); safeGetString(pId, 20);
    if (strcmp(pId, "0") == 0) return;

    Record* targetNotice = NULL; r = recordHead->next;
    while (r) { if (r->type == 6 && strcmp(r->patientId, pId) == 0 && r->isPaid == 0) { targetNotice = r; break; } r = r->next; }

    if (!targetNotice) {
        Bed* b = bedHead->next; int alreadyIn = 0;
        while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0) { alreadyIn = 1; break; } b = b->next; }
        if (alreadyIn) printf("【拦截警告】该患者已经处于住院状态中，请勿重复办理！\n");
        else printf("【拦截警告】缺乏门诊开具的住院通知，无法强行收治入院！\n");
        return;
    }

    Patient* p = patientHead->next; Patient* targetPat = NULL;
    while (p) { if (strcmp(p->id, pId) == 0) { targetPat = p; break; } p = p->next; }

    char requiredDept[50];
    getResponsibleDept(pId, requiredDept);
    printf("\n>>> 锁定通知单：该患者由【%s】下发，正在为您筛选 %s 的专属空床...\n", requiredDept, requiredDept);

    // 【新增核心升级】此时我们已经知道了目标科室 requiredDept，执行精确的科室级空床校验与扩容！
    checkAndAdjustBedTension(requiredDept);

    printf("请输入拟住院天数: "); int days = safeGetPositiveInt();

    int baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    int finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("按规则系统核算，需缴纳住院押金: %d 元。\n", finalDeposit);

    int isPaid = 0;
    if (targetPat->balance >= finalDeposit) {
        targetPat->balance -= finalDeposit; isPaid = 1;
        printf("已成功从患者账户划扣押金，住院立即生效。\n");

        // 【核心新增：同步押金收取至队友报表】
        Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
        int maxId = 0; Transaction* curr = transactionList;
        while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
        newTrans->id = maxId + 1;
        newTrans->type = 2; // 2 代表队友结构里的住院类
        newTrans->amount = finalDeposit;
        getCurrentTimeStr(newTrans->time, 30);
        sprintf(newTrans->description, "住院押金收取(患者:%s)", targetPat->name);
        newTrans->next = NULL;

        if (!transactionList) transactionList = newTrans;
        else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }
    }
    else {
        printf("【提示】患者余额不足！将生成待缴费住院押金账单，请提醒前往财务缴费。\n");
    }

    printf("\n【%s】可用空闲病床列表:\n", requiredDept);
    Bed* b = bedHead->next; int hasEmpty = 0;
    while (b) {
        if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
            printf("[%s] %s - %s (每日 %.2f)\n", b->bedId, b->wardType, b->bedType, b->price);
            hasEmpty = 1;
        }
        b = b->next;
    }
    if (!hasEmpty) { printf("%s 专属床位已满！请等待出院或调床。\n", requiredDept); if (isPaid) targetPat->balance += finalDeposit; return; }

    char selectBed[20];
    printf("请输入分配的床位号 (如 1-3): "); safeGetString(selectBed, 20);
    b = bedHead->next; Bed* finalBed = NULL;
    while (b) {
        if (strcmp(b->bedId, selectBed) == 0 && !b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
            finalBed = b; break;
        }
        b = b->next;
    }
    if (!finalBed) { printf("无效床位，或该床位不属于【%s】。\n", requiredDept); if (isPaid) targetPat->balance += finalDeposit; return; }

    finalBed->isOccupied = 1; strcpy(finalBed->patientId, pId);
    finalBed->isRoundsDone = 0;
    targetNotice->isPaid = 1;

    Record* r5 = (Record*)malloc(sizeof(Record));
    extern void generateRecordID(char* buffer);
    generateRecordID(r5->recordId);
    r5->type = 5; strcpy(r5->patientId, pId); strcpy(r5->staffId, docId);
    r5->cost = finalDeposit; r5->isPaid = isPaid;

    char adminTime[30]; getCurrentTimeStr(adminTime, 30);
    sprintf(r5->description, "病房:%s_床位:%s_入院日期:%s_预期天数:%d_押金:%d_出院日期:无_总费用:0",
        finalBed->wardType, finalBed->bedId, adminTime, days, finalDeposit);
    getCurrentTimeStr(r5->createTime, 30);
    r5->next = recordHead->next; recordHead->next = r5;

    if (isPaid) printf("【成功】已成功办理入院，分配至 %s 的 %s 床。\n", requiredDept, finalBed->bedId);
    else printf("【待缴费】已预留 %s 床位，并生成账单。\n", finalBed->bedId);
}
// ---------------------------------------------------------
// 4. 日常查房与开医嘱
// ---------------------------------------------------------
void wardRounds(const char* docId) {
    while (1) {
        system("cls");
        printf("\n--- 住院部日常查房 ---\n");

        char deptStr[200]; getDynamicDeptPrompt(deptStr);
        char targetDept[50];
        printf("请输入要查房的科室名称 (%s, 0返回): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n--- 【%s】 住院查房名单 ---\n", targetDept);
        printf("%-10s %-15s %-10s %-10s %-10s\n", "房-床号", "患者ID", "姓名", "类型", "查房状态");

        Bed* b = bedHead->next;
        int pCount = 0;
        while (b) {
            if (b->isOccupied && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                Patient* p = patientHead->next; char pName[100] = "未知";
                while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                char severity[10] = "普通";
                Record* r = recordHead->next;
                while (r) {
                    if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                        if (strstr(r->description, "重症")) strcpy(severity, "重症");
                    }
                    r = r->next;
                }

                printf("%-10s %-15s %-10s %-10s %-10s\n",
                    b->bedId, b->patientId, pName, severity, b->isRoundsDone ? "[已查房]" : "[未查房]");
                pCount++;
            }
            b = b->next;
        }

        if (pCount == 0) { printf("该科室当前无住院患者。\n"); system("pause"); continue; }

        char pId[20];
        printf("\n请输入需查房的住院患者ID (0返回重选科室): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) continue;

        b = bedHead->next; Bed* targetBed = NULL;
        while (b) {
            if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                targetBed = b; break;
            }
            b = b->next;
        }
        if (!targetBed) { printf("未在 %s 找到该住院患者，请核对科室与ID。\n", targetDept); system("pause"); continue; }

        while (1) {
            system("cls");
            printf("\n--- 对患者 %s 的查房选项 ---\n", pId);
            printf("1. 下达日常医嘱笔记\n2. 开具住院药品并计费\n0. 返回重选患者\n请选择: ");

            int choice = safeGetInt();
            if (choice == 0) break;

            if (choice == 1) {
                printf("请输入查房医嘱记录(无空格): ");
                char note[200]; safeGetString(note, 200);
                Record* rx = (Record*)malloc(sizeof(Record));
                extern void generateRecordID(char* buffer);
                generateRecordID(rx->recordId);
                rx->type = 2; strcpy(rx->patientId, pId); strcpy(rx->staffId, docId);
                rx->cost = 0; rx->isPaid = 1;
                sprintf(rx->description, "住院查房医嘱:%s", note);
                getCurrentTimeStr(rx->createTime, 30);
                rx->next = recordHead->next; recordHead->next = rx;
                printf("查房医嘱记录已保存。\n");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
            else if (choice == 2) {
                extern char currentCallingPatientId[20];
                strcpy(currentCallingPatientId, pId);
                extern void prescribeMedicine(const char* docId);
                prescribeMedicine(docId);
                strcpy(currentCallingPatientId, "");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
        }
    }
}

void dailyDeductionSimulation() {
    printf("\n--- 模拟执行每日 08:00 床位费划扣 ---\n");
    Bed* b = bedHead->next; int count = 0;
    while (b) {
        if (b->isOccupied) {
            Patient* p = patientHead->next;
            while (p) {
                if (strcmp(p->id, b->patientId) == 0) {
                    if (p->balance < 1000) printf("【警告】患者 %s 余额 (%.2f) 低于1000元，请立刻停药并催缴！\n", p->name, p->balance);
                    printf("患者 %s (%s床) 已记账当日床位费 %.2f 元。\n", p->name, b->bedId, b->price);
                    count++; break;
                }
                p = p->next;
            }
            b->isRoundsDone = 0;
        }
        b = b->next;
    }
    printf("执行完毕，共扫描处理 %d 名在院患者，查房状态已重置。\n", count);
}

// ---------------------------------------------------------
// 5. 办理出院 
// ---------------------------------------------------------
// 5. 办理出院 (增加前置科室选择、同步交易报表)
// ---------------------------------------------------------
void dischargePatient() {
    while (1) {
        system("cls");
        printf("\n========== 办理患者出院 ==========\n");

        char deptStr[200]; getDynamicDeptPrompt(deptStr);
        char targetDept[50];
        printf("请输入要办理出院的科室名称 (%s, 0返回): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n--- 【%s】 当前住院患者列表 ---\n", targetDept);
        printf("%-10s %-12s %-15s %-10s\n", "房号-床位", "病房区域", "患者ID", "姓名");

        Bed* b_list = bedHead->next;
        int count = 0;
        while (b_list) {
            if (b_list->isOccupied && strcmp(getRoomDepartment(b_list->bedId), targetDept) == 0) {
                Patient* p = patientHead->next;
                char pName[100] = "未知";
                while (p) { if (strcmp(p->id, b_list->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
                printf("%-10s %-12s %-15s %-10s\n", b_list->bedId, b_list->wardType, b_list->patientId, pName);
                count++;
            }
            b_list = b_list->next;
        }

        if (count == 0) { printf("该科室当前无住院患者，无需出院。\n"); system("pause"); continue; }

        char pId[20];
        printf("\n请输入要办理出院的患者ID (0返回): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) continue;

        Bed* b = bedHead->next; Bed* targetBed = NULL;
        while (b) {
            if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                targetBed = b; break;
            }
            b = b->next;
        }
        if (!targetBed) { printf("在 %s 未找到该患者，请核实。\n", targetDept); system("pause"); continue; }

        time_t t = time(NULL); struct tm* tm_info = localtime(&t);
        int currentHour = tm_info->tm_hour;

        printf("\n由于系统无法跨天流转，请输入实际发生住院天数用于结算: ");
        int actualDays = safeGetPositiveInt();
        int billableDays = actualDays;

        // 早 8 点前办理出院，免除当日床位费
        if (currentHour >= 0 && currentHour < 8) {
            printf("【特惠】当前办理时间 %02d:00，符合早8点前免收规定，减免最后一日床位费！\n", currentHour);
            billableDays = (actualDays > 1) ? actualDays - 1 : 0;
        }

        // 核算总消费 (床位费 + 期间药费)
        double totalBedFee = billableDays * targetBed->price;
        double totalDrugFee = 0;
        Record* r = recordHead->next;
        while (r) {
            if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 0) {
                totalDrugFee += r->cost; r->isPaid = 1; // 并入总账后，标记药单为已缴费
            }
            r = r->next;
        }

        double totalHospitalCost = totalBedFee + totalDrugFee;
        r = recordHead->next; Record* r5 = NULL;
        // 提取当时的住院押金单
        while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) { r5 = r; break; } r = r->next; }

        if (r5) {
            char outTime[30]; getCurrentTimeStr(outTime, 30);
            double original_deposit = r5->cost; // 记住患者最初交的押金（比如 2000）

            // 【核心修复】：将最初的押金单，完美转化为真正的“出院消费结算单”
            // 这样患者在财务报表中就能清晰看到床位费和药费的扣款了！
            r5->cost = totalHospitalCost; // 真实的消费总金额（比如床位费 400）
            r5->isPaid = 2; // 闭环
            sprintf(r5->description, "出院结算_实际住院:%d天_床位费:%.2f_药费:%.2f", actualDays, totalBedFee, totalDrugFee);

            // 【情况 A】: 押金有结余，退还差额
            if (original_deposit > totalHospitalCost) {
                double refund = original_deposit - totalHospitalCost;
                Patient* pt = patientHead->next;
                while (pt) { if (strcmp(pt->id, pId) == 0) { pt->balance += refund; break; } pt = pt->next; }
                printf("\n【退款通知】经系统清算，原押金 %.0f 元，结余 %.2f 元。已实时退回至患者账户！\n", original_deposit, refund);

                // --- 业务联动：生成 Type 8 个人退款流水 ---
                Record* r8 = (Record*)malloc(sizeof(Record));
                extern void generateRecordID(char* buffer);
                generateRecordID(r8->recordId);
                r8->type = 8; strcpy(r8->patientId, pId); strcpy(r8->staffId, "SYS");
                r8->cost = refund; r8->isPaid = 1;
                sprintf(r8->description, "出院清算_押金结余退回");
                getCurrentTimeStr(r8->createTime, 30);
                r8->next = recordHead->next; recordHead->next = r8;

                // --- 【核心联动】同步退款至队友的 Transaction 报表冲账 ---
                Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
                int maxId = 0; Transaction* curr = transactionList;
                while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                newTrans->id = maxId + 1;
                newTrans->type = 2; // 住院类
                newTrans->amount = -refund; // 【负数】代表医院把钱退给患者了，财报冲减
                getCurrentTimeStr(newTrans->time, 30);
                strcpy(newTrans->description, "出院清算_押金结余退回");
                newTrans->next = NULL;

                if (!transactionList) transactionList = newTrans;
                else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }

            }
            // 【情况 B】: 押金不足，提醒患者补缴
            else if (original_deposit < totalHospitalCost) {
                double arrears = totalHospitalCost - original_deposit;
                printf("\n【补缴通知】经系统清算，押金透支 %.2f 元。请通知患者前往财务中心补缴差额！\n", arrears);

                // 生成一笔特殊的待缴费记录 (Type 5，因为算是住院补缴)
                Record* r_arrears = (Record*)malloc(sizeof(Record));
                extern void generateRecordID(char* buffer);
                generateRecordID(r_arrears->recordId);
                r_arrears->type = 5; strcpy(r_arrears->patientId, pId); strcpy(r_arrears->staffId, "SYS");
                r_arrears->cost = arrears; r_arrears->isPaid = 0; // 0 表示待缴费
                sprintf(r_arrears->description, "出院清算_补缴欠费差额");
                getCurrentTimeStr(r_arrears->createTime, 30);
                r_arrears->next = recordHead->next; recordHead->next = r_arrears;
            }
            else {
                printf("\n【结算通知】押金刚好抵扣所有消费，无需退补。\n");
            }
        }
        // 释放床位
        targetBed->isOccupied = 0; strcpy(targetBed->patientId, "");
        printf("\n========== 出院结算单 ==========\n患者: %s | 床位总费: %.2f | 期间药费: %.2f | 总费用: %.2f\n病床 %s 已释放空闲。\n", pId, totalBedFee, totalDrugFee, totalHospitalCost, targetBed->bedId);

        system("pause");
        return;
    }
}

void inpatientMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n===== 住院管理调度中心 =====\n");
        printf("1. 查看全院病房实时图谱\n");
        printf("2. 办理患者入院 (核算押金/床位分配)\n");
        printf("3. 执行每日早8点查床扣费\n");
        printf("4. 日常查房与下达医嘱记录\n");
        printf("5. 办理患者出院 (特惠与统账流转)\n");
        printf("0. 返回上级医生大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: viewAllBeds(); break;
        case 2: admitPatient(docId); system("pause"); break;
        case 3: dailyDeductionSimulation(); system("pause"); break;
        case 4: wardRounds(docId); break;
        case 5: dischargePatient(); break;
        case 0: return;
        }
    }
}