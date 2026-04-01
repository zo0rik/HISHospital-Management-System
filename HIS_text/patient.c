#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "patient.h"
#include "utils.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"
#include "drug.h"
void generatePatientID(char* idBuffer) {
    int maxId = 999; // 基础值
    Patient* p = patientHead->next;
    while (p != NULL) {
        int currentIdNum;
        // 尝试从类似 P20251000 中提取出 1000 这个数字
        if (sscanf(p->id, "P2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) {
                maxId = currentIdNum;
            }
        }
        p = p->next;
    }
    // 取当前最大值 + 1
    sprintf(idBuffer, "P2025%04d", maxId + 1);
}

Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) { if (strcmp(p->id, pid) == 0) return p; p = p->next; }
    return NULL;
}

void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n--- 账户注册与建档 ---\n");
    printf("请选择就诊类型 (1.普通 2.急诊 0.返回): ");
    int type;
    while (1) {
        type = safeGetInt();
        if (type == 0) { free(newPatient); return; }
        if (type == 1 || type == 2) break;
        printf("【输入错误】只能输入 1 或 2，请重新选择: ");
    }
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    printf("请输入姓名: "); safeGetString(newPatient->name, 100);
    printf("请设置登录密码: "); safeGetString(newPatient->password, 50);
    printf("请输入性别 (男性/女性): "); safeGetGender(newPatient->gender, 10);

    if (!newPatient->isEmergency) {
        printf("请输入年龄: "); newPatient->age = safeGetPositiveInt();
        printf("请输入过敏史(无则填无): "); safeGetString(newPatient->allergy, 100);
    }
    else {
        newPatient->age = -1; strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id); newPatient->balance = 0.0;
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;
    printf("【成功】档案建立成功！您的登录账号为: %s\n", newPatient->id);
}

// ---------------------------------------------------------
// 业务二：自助挂号 (极限规则校验、动态科室抓取与排队号抢占)
// ---------------------------------------------------------
void bookAppointment(const char* currentPatientId) {
    char today[11], nextWeek[11];
    time_t t = time(NULL); struct tm* tm_info = localtime(&t);
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);
    t += 7 * 24 * 60 * 60; tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        system("cls");
        printf("\n--- 自助预约挂号 ---\n");
        printf("1. 按科室查找未来一周可预约排班\n");
        printf("2. 搜索医生姓名/工号查找未来一周排班\n");
        printf("0. 返回上级菜单\n");
        printf("请选择您的查号方式: ");
        int choice = safeGetInt();
        if (choice == 0) return;

        char keyword[50];
        if (choice == 1) {
            // 动态抓取当前存在的科室 (遍历医生链表去重)
            char depts[20][50]; int dCount = 0;
            Staff* stf = staffHead->next;
            while (stf) {
                int exists = 0;
                for (int i = 0; i < dCount; i++) {
                    if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; }
                }
                if (!exists && strlen(stf->department) > 0) {
                    strcpy(depts[dCount], stf->department);
                    dCount++;
                }
                stf = stf->next;
            }

            // 动态展示抓取到的科室供患者参考
            printf("\n当前存在科室: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);
            printf("\n请输入目标科室名称: ");
            safeGetString(keyword, 50);
        }
        else if (choice == 2) {
            printf("请输入医生姓名或工号 (如: 李四 或 1001): ");
            safeGetString(keyword, 50);
        }
        else continue;

        // 排班表展示 (增加医生工号显示)
        printf("\n--- 未来一周可预约排班表 (%s 至 %s) ---\n", today, nextWeek);
        printf("%-8s %-12s %-10s %-18s %-12s %-10s\n", "排班ID", "日期", "班次", "医生(工号)", "科室", "级别");

        int found = 0; Schedule* s = scheduleList;
        while (s) {
            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) { s = s->next; continue; }
            Doctor* d = doctorList; Doctor* matchedDoc = NULL;
            while (d) { if (d->id == s->doctor_id) { matchedDoc = d; break; } d = d->next; }
            if (!matchedDoc) { s = s->next; continue; }

            int match = 0;
            if (choice == 1 && strstr(matchedDoc->department, keyword)) match = 1;
            if (choice == 2) {
                char docIdStr[20]; sprintf(docIdStr, "%d", matchedDoc->id);
                if (strstr(matchedDoc->name, keyword) || strcmp(docIdStr, keyword) == 0) match = 1;
            }

            if (match && strcmp(s->shift, "休息") != 0) {
                // 拼接医生姓名与工号
                char docDisp[50];
                sprintf(docDisp, "%s(%d)", matchedDoc->name, matchedDoc->id);
                printf("%-8d %-12s %-10s %-18s %-12s %-10s\n",
                    s->schedule_id, s->date, s->shift, docDisp, matchedDoc->department, matchedDoc->title);
                found++;
            }
            s = s->next;
        }

        if (found == 0) { printf("未找到匹配的排班信息。\n"); system("pause"); continue; }

        printf("\n请输入要预约的【排班ID】 (输入0返回): ");
        int targetSchId = safeGetInt();
        if (targetSchId == 0) continue;

        Schedule* targetSch = NULL; s = scheduleList;
        while (s) { if (s->schedule_id == targetSchId) { targetSch = s; break; } s = s->next; }
        if (!targetSch) { printf("无效的排班ID！\n"); system("pause"); continue; }

        Doctor* targetDoc = NULL; Doctor* d = doctorList;
        while (d) { if (d->id == targetSch->doctor_id) { targetDoc = d; break; } d = d->next; }
        if (!targetDoc) { printf("【数据异常】该医生不存在！\n"); system("pause"); continue; }

        char staffIdStr[20]; sprintf(staffIdStr, "D%d", targetDoc->id);

        // ==========================================
        // 核心统筹：将所有约束精准限定到“同一日期”
        // ==========================================
        int patientDailyActive = 0;   // 规则：同日个人最多挂5个号
        int patientDeptDailyActive = 0; // 规则：同日同科室最多1个
        int sameDocSameDay = 0;       // 规则：同日不能重复挂同一医生
        int docDailyCount = 0;        // 规则：医生单日最多50人
        int hospitalDailyCount = 0;   // 规则：全院单日最多1000号

        Record* rec = recordHead->next;
        while (rec) {
            // 只统计目标日期的挂号记录
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++;
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;

                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;

                    // 追溯看这个单子对应哪个科室，防止同科室同日挂多个
                    Doctor* recDoc = doctorList;
                    while (recDoc) {
                        char tempDId[20]; sprintf(tempDId, "D%d", recDoc->id);
                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1;
                            break;
                        }
                        recDoc = recDoc->next;
                    }
                }
            }
            rec = rec->next;
        }

        // 拦截机制
        if (hospitalDailyCount >= 1000) { printf("【超载】该日全院挂号总数已达1000上限！\n"); system("pause"); continue; }
        if (patientDailyActive >= 5) { printf("【限制】您在 %s 已预约 %d 个号，已达单日最高上限（5个）！\n", targetSch->date, patientDailyActive); system("pause"); continue; }
        if (patientDeptDailyActive >= 1) { printf("【限制】您在 %s 已挂过 [%s] 的号，同日同科室最多挂一个！\n", targetSch->date, targetDoc->department); system("pause"); continue; }
        if (sameDocSameDay) { printf("【限制】您已挂过 %s 医生在 %s 的号，不允许同日重复挂号！\n", targetDoc->name, targetSch->date); system("pause"); continue; }

        if (docDailyCount >= 50) {
            printf("\n【排队已满】%s 医生在 %s 候诊队列已满 (50人)！\n", targetDoc->name, targetSch->date);
            printf(">>> 智能AI推荐系统为您匹配以下备选方案 <<<\n");

            int recCount = 0; Schedule* altS = scheduleList;
            printf("\n[推荐一：该医生 %s 的其他出诊时间]\n", targetDoc->name);
            while (altS) {
                if (altS->doctor_id == targetDoc->id && strcmp(altS->date, targetSch->date) != 0 && strcmp(altS->shift, "休息") != 0) {
                    if (strcmp(altS->date, today) >= 0 && strcmp(altS->date, nextWeek) <= 0) {
                        printf(" - 排班ID [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift); recCount++;
                    }
                }
                altS = altS->next;
            }

            printf("\n[推荐二：同科室 [%s] 其他医生 (%s当日)]\n", targetDoc->department, targetSch->date);
            altS = scheduleList;
            while (altS) {
                if (strcmp(altS->date, targetSch->date) == 0 && altS->doctor_id != targetDoc->id && strcmp(altS->shift, "休息") != 0) {
                    Doctor* altD = doctorList;
                    while (altD) {
                        if (altD->id == altS->doctor_id && strcmp(altD->department, targetDoc->department) == 0) {
                            printf(" - 排班ID [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift); recCount++; break;
                        }
                        altD = altD->next;
                    }
                }
                altS = altS->next;
            }
            if (recCount == 0) printf("暂无其他可推荐排班，请改日再试。\n");
            system("pause"); continue;
        }

        // 生成单据部分 (无需缴费，直接占用排队号)
        int seqNum = docDailyCount + 1;
        double regFee = strstr(targetDoc->title, "主任") != NULL ? 50.0 : 15.0;

        Record* newRecord = (Record*)malloc(sizeof(Record));
        int maxRegId = 4999;
        Record* r_temp = recordHead->next;
        while (r_temp) {
            int curReg;
            if (sscanf(r_temp->recordId, "REG2025%04d", &curReg) == 1) {
                if (curReg > maxRegId) maxRegId = curReg;
            }
            r_temp = r_temp->next;
        }
        sprintf(newRecord->recordId, "REG2025%04d", maxRegId + 1);
        newRecord->type = 1;
        strcpy(newRecord->patientId, currentPatientId); strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee;
        newRecord->isPaid = 0; // 0 是待缴费，但不影响其顺序号的下发

        // 排号内嵌到描述中，供医生端解析
        sprintf(newRecord->description, "挂号:%s(%s)_排号:%d", targetDoc->name, targetSch->date, seqNum);
        getCurrentTimeStr(newRecord->createTime, 30);
        newRecord->next = NULL;

        Record* temp = recordHead;
        while (temp->next) temp = temp->next;
        temp->next = newRecord;

        printf("\n【挂号成功】您已成功抢占 %s 医师 的号源！费用 %.2f 元。\n", targetDoc->name, regFee);
        printf(">>> 您在当日的专属候诊顺序为：【第 %d 号】 <<<\n", seqNum);
        printf("注：请在就诊前前往财务中心完成缴费。\n");
        system("pause"); return;
    }
}
// ---------------------------------------------------------
// 业务三：财务费用中心 (支持充值、一键缴费、逐个缴费)
// ---------------------------------------------------------
// ---------------------------------------------------------
// 内部引擎：监听缴费，如果是出院尾款，则瞬间释放病床并合并出院单！
// ---------------------------------------------------------
void checkAndAutoDischarge(Record* rec, const char* currentPatientId) {
    if (rec->type == 5 && strstr(rec->description, "出院清算_补缴欠费差额")) {
        char bedId[20] = { 0 }; double bFee = 0, dFee = 0;
        // 破译刚才医生埋在单据里的床位号和费用密码
        if (sscanf(rec->description, "出院清算_补缴欠费差额_床位:%19[^_]_床费:%lf_药费:%lf", bedId, &bFee, &dFee) == 3) {

            // 1. 瞬间拔管放床
            extern Bed* bedHead;
            Bed* b = bedHead->next;
            while (b) { if (strcmp(b->bedId, bedId) == 0) { b->isOccupied = 0; strcpy(b->patientId, ""); break; } b = b->next; }

            // 2. 把所有住院期间记账(状态4)的药正式买单(转为状态1)
            Record* rr = recordHead->next;
            while (rr) { if (rr->type == 3 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 4) rr->isPaid = 1; rr = rr->next; }

            // 3. 找到最初的押金单，完美完结并写上总结算明细
            rr = recordHead->next;
            while (rr) {
                if (rr->type == 5 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 1) {
                    rr->isPaid = 2;
                    char summary[200];
                    sprintf(summary, " [出院结算:床费%.2f 药费%.2f 总消费%.2f]", bFee, dFee, bFee + dFee);
                    strcat(rr->description, summary);
                }
                rr = rr->next;
            }
            // 4. 清理当前这条尾款单的显示名字，让他变干净
            strcpy(rec->description, "出院清算尾款结清");
            printf("\n【系统自动联动】检测到住院尾款结清！\n系统已自动为您释放了病床，并生成了详细的【出院结算明细】，请至档案库查阅！\n");
        }
    }
}

// ---------------------------------------------------------
// 业务三：财务费用中心 (含一键缴费及自动出院引擎)
// ---------------------------------------------------------
void financeCenter(const char* currentPatientId) {
    while (1) {
        system("cls"); Patient* p = findPatientById(currentPatientId); if (!p) return;
        printf("\n--- 财务中心 (当前余额: %.2f) ---\n", p->balance);
        printf("1. 账户余额充值\n2. 待缴费账单结算\n0. 返回上级菜单\n请选择操作: ");
        int choice = safeGetInt(); if (choice == 0) return;

        if (choice == 1) {
            printf("\n请输入充值金额 (0取消): "); double money = safeGetDouble();
            if (money > 0) {
                p->balance += money;
                Record* r7 = (Record*)malloc(sizeof(Record)); extern void generateRecordID(char* buffer);
                generateRecordID(r7->recordId); r7->type = 7; strcpy(r7->patientId, currentPatientId); strcpy(r7->staffId, "SYS");
                r7->cost = money; r7->isPaid = 1; sprintf(r7->description, "终端自助充值入账");
                getCurrentTimeStr(r7->createTime, 30); r7->next = recordHead->next; recordHead->next = r7;
                printf("【成功】充值 %.2f 元成功！当前账户余额: %.2f\n", money, p->balance); system("pause");
            }
        }
        else if (choice == 2) {
            while (1) {
                system("cls"); printf("\n--- 待缴费账单明细 ---\n");
                Record* rec = recordHead->next; int hasUnpaid = 0; double totalUnpaidCost = 0.0;
                while (rec) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                        char typeName[20];
                        switch (rec->type) { case 1:strcpy(typeName, "挂号费"); break; case 2:strcpy(typeName, "诊疗费"); break; case 3:strcpy(typeName, "药费"); break; case 4:strcpy(typeName, "检查费"); break; case 5:strcpy(typeName, "住院押金/补缴"); break; }
                                                   printf("单号: %s | [%s] %s | 金额: %.2f元\n", rec->recordId, typeName, rec->description, rec->cost);
                                                   totalUnpaidCost += rec->cost; hasUnpaid = 1;
                    }
                    rec = rec->next;
                }
                if (!hasUnpaid) { printf("目前无待缴费账单。\n"); system("pause"); break; }

                printf("--------------------------------------------------\n待缴费总额: %.2f 元 | 当前账户余额: %.2f 元\n", totalUnpaidCost, p->balance);
                printf("\n1. 一键缴费 (自动结算所有账单)\n2. 逐个缴费 (输入单号独立结算)\n0. 返回上一级\n请选择: ");
                int payChoice = safeGetInt(); if (payChoice == 0) break;

                if (payChoice == 1) {
                    if (p->balance < totalUnpaidCost) { printf("【失败】余额不足，请先返回充值！差额: %.2f 元\n", totalUnpaidCost - p->balance); system("pause"); }
                    else {
                        rec = recordHead->next;
                        while (rec) {
                            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                                p->balance -= rec->cost; rec->isPaid = 1;
                                Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction)); int maxId = 0; Transaction* curr = transactionList; while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                                newTrans->id = maxId + 1;
                                if (rec->type == 3) newTrans->type = 3; else if (rec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                                newTrans->amount = rec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, rec->description); newTrans->next = NULL;
                                if (!transactionList) transactionList = newTrans; else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }

                                // 【核心调用】每次缴款都查一下，如果是住院欠费，立即触发自动放床引擎！
                                checkAndAutoDischarge(rec, currentPatientId);
                            }
                            rec = rec->next;
                        }
                        printf("【成功】一键缴费完成！当前余额: %.2f\n", p->balance); system("pause"); break;
                    }
                }
                else if (payChoice == 2) {
                    printf("\n输入要缴费的单号: "); char target[30]; safeGetString(target, 30); if (strcmp(target, "0") == 0) continue;
                    Record* tRec = NULL; rec = recordHead->next;
                    while (rec) { if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; } rec = rec->next; }
                    if (!tRec) { printf("单号错误或已结算。\n"); system("pause"); continue; }

                    if (p->balance < tRec->cost) { printf("【失败】余额不足！差额: %.2f 元\n", tRec->cost - p->balance); system("pause"); }
                    else {
                        p->balance -= tRec->cost; tRec->isPaid = 1;
                        Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction)); int maxId = 0; Transaction* curr = transactionList; while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                        newTrans->id = maxId + 1;
                        if (tRec->type == 3) newTrans->type = 3; else if (tRec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                        newTrans->amount = tRec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, tRec->description); newTrans->next = NULL;
                        if (!transactionList) transactionList = newTrans; else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }

                        printf("【缴费成功】已扣款 %.2f 元。当前余额: %.2f\n", tRec->cost, p->balance);
                        // 【核心调用】如果是住院欠费，立即触发自动放床引擎！
                        checkAndAutoDischarge(tRec, currentPatientId);
                        system("pause");
                    }
                }
            }
        }
    }
}
// ---------------------------------------------------------
// // 业务四：个人医疗档案库 (新增消费报表与明细查询)
// ---------------------------------------------------------
// ---------------------------------------------------------
// 个人医疗档案库 (修复重复计费、支持一键查询住院总账单明细)
// ---------------------------------------------------------
void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n--- 个人医疗档案库 ---\n");
        printf("1. ① 挂号与分诊记录\n");
        printf("2. ② 临床看诊记录\n");
        printf("3. ③ 电子处方记录\n");
        printf("4. ④ 辅助检查记录\n");
        printf("5. ⑤ 住院记录与出院总账单\n");
        printf("6. 个人资金收支流水报表\n");
        printf("0. 返回上级菜单\n");
        printf("请选择查询大类: ");
        int c = safeGetInt();
        if (c == 0) return;

        if (c >= 1 && c <= 4) {
            printf("\n--- 查询结果 ---\n");
            Record* rec = recordHead->next;
            int printed = 0;
            while (rec) {
                // 不显示住院期间记账的药单(isPaid=4)，那些要在分类5里看
                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == c && rec->isPaid != 4 && !strstr(rec->description, "[住院记账]")) {
                    printf("单号: %-12s | 时间: %s | 金额: %.2f | 详情: %s\n",
                        rec->recordId, rec->createTime, rec->cost, rec->description);
                    printed = 1;
                }
                rec = rec->next;
            }
            if (!printed) printf("该分类下暂无记录。\n");
            system("pause");
        }
        else if (c == 5) {
            // ================= 【升级：住院记录下钻式查询】 =================
            while (1) {
                system("cls");
                printf("\n========== ⑤ 住院记录与出院总账单 ==========\n");
                Record* rec = recordHead->next;
                int count = 0;
                printf("%-15s %-10s %-20s %-20s\n", "流水单号", "发生金额", "当前状态", "时间");

                while (rec) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == 5) {
                        char statusStr[30];
                        if (rec->isPaid == 0) strcpy(statusStr, "待缴尾款");
                        else if (rec->isPaid == 1) strcpy(statusStr, "住院中(押金已交)");
                        else strcpy(statusStr, "已出院结算");

                        printf("%-15s %-10.2f %-20s %-20s\n", rec->recordId, rec->cost, statusStr, rec->createTime);
                        count++;
                    }
                    rec = rec->next;
                }
                if (count == 0) { printf("暂无任何住院记录。\n"); system("pause"); break; }

                printf("-------------------------------------------------------------------\n");
                printf("请输入要查看明细的【流水单号】 (输入0返回): ");
                char target[30]; safeGetString(target, 30);
                if (strcmp(target, "0") == 0) break;

                Record* tRec = NULL; rec = recordHead->next;
                while (rec) {
                    if (strcmp(rec->recordId, target) == 0 && rec->type == 5 && strcmp(rec->patientId, currentPatientId) == 0) {
                        tRec = rec; break;
                    }
                    rec = rec->next;
                }

                if (!tRec) { printf("单号输入错误，未找到该住院记录！\n"); system("pause"); continue; }

                system("cls");
                printf("\n========== 住院详情单 (%s) ==========\n", tRec->recordId);
                printf("【基础信息】\n");
                printf("单据时间: %s\n", tRec->createTime);
                printf("相关操作: %s\n", tRec->description);

                if (tRec->isPaid == 2 && strstr(tRec->description, "出院结算")) {
                    printf("\n【资金流转明细】\n");
                    printf(" - 缴纳住院押金: %.2f 元\n", tRec->cost);

                    // 尝试找寻关联的退款单(Type 8) 或 补缴单(Type 5 包含"出院清算尾款结清")
                    Record* r_link = recordHead->next;
                    while (r_link) {
                        if (strcmp(r_link->patientId, currentPatientId) == 0) {
                            if (r_link->type == 8 && strstr(r_link->description, "出院清算_押金结余退回")) {
                                printf(" - 系统退回结余: %.2f 元 (流水号:%s)\n", r_link->cost, r_link->recordId);
                            }
                            else if (r_link->type == 5 && strstr(r_link->description, "出院清算尾款结清")) {
                                printf(" - 补缴欠费差额: %.2f 元 (流水号:%s)\n", r_link->cost, r_link->recordId);
                            }
                        }
                        r_link = r_link->next;
                    }

                    printf("\n【住院期间各项药品费用明细】\n");
                    Record* r_drug = recordHead->next; int hasDrug = 0;
                    while (r_drug) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && strstr(r_drug->description, "[住院记账]")) {
                            printf(" -> %s | %s\n", r_drug->recordId, r_drug->description);
                            hasDrug = 1;
                        }
                        r_drug = r_drug->next;
                    }
                    if (!hasDrug) printf(" -> (本次住院无药品消费)\n");
                }
                else if (tRec->isPaid == 1 && !strstr(tRec->description, "出院清算")) {
                    printf("\n【当前状态】患者正在住院中...\n");
                    printf(" - 已交押金: %.2f 元\n", tRec->cost);
                    printf("\n【住院期间已记账药品 (未结算)】\n");
                    Record* r_drug = recordHead->next; int hasDrug = 0;
                    while (r_drug) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && r_drug->isPaid == 4 && strstr(r_drug->description, "[住院记账]")) {
                            printf(" -> %s | %s\n", r_drug->recordId, r_drug->description);
                            hasDrug = 1;
                        }
                        r_drug = r_drug->next;
                    }
                    if (!hasDrug) printf(" -> (暂无药品消费)\n");
                }
                else if (tRec->isPaid == 0) {
                    printf("\n【当前状态】待缴出院尾款，请至[财务中心]结清后方可办理出院。\n");
                }
                printf("-------------------------------------------------------------------\n");
                system("pause");
            }
        }
        else if (c == 6) {
            // ================= 【升级：剔除住院药费的精确报表】 =================
            printf("\n========== 消费报表与明细查询 ==========\n");
            Record* rec = recordHead->next;
            double totalSpent = 0, totalRecharged = 0, totalRefunded = 0;
            int printed = 0;
            printf("%-20s %-10s %-12s %-40s\n", "交易时间", "收支类型", "发生金额", "流水详情");

            while (rec) {
                if (strcmp(rec->patientId, currentPatientId) == 0) {
                    char typeName[30];
                    if (rec->type >= 1 && rec->type <= 5 && rec->isPaid > 0 && rec->isPaid != 4) {

                        // 【核心拦截】：如果是打上了 [住院记账] 的药单，直接隐身，不计入总支出，不打印！
                        if (rec->type == 3 && strstr(rec->description, "[住院记账]")) {
                            // 略过，因为它已经包含在Type5(住院押金)的账本里了
                        }
                        else {
                            totalSpent += rec->cost;
                            strcpy(typeName, "支出 (-)");
                            printf("%-20s %-10s %-10.2f %-40s\n", rec->createTime, typeName, rec->cost, rec->description);
                            printed = 1;
                        }
                    }
                    else if (rec->type == 7) {
                        totalRecharged += rec->cost;
                        strcpy(typeName, "充值 (+)");
                        printf("%-20s %-10s %-10.2f %-40s\n", rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                    else if (rec->type == 8) {
                        totalRefunded += rec->cost;
                        strcpy(typeName, "退款 (+)");
                        printf("%-20s %-10s %-10.2f %-40s\n", rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                }
                rec = rec->next;
            }
            if (!printed) printf("暂无任何资金收支流水记录。\n");
            printf("-------------------------------------------------------------------\n");
            printf("【资金汇总】总计充值: +%.2f | 出院退回: +%.2f | 实际总支出: -%.2f\n", totalRecharged, totalRefunded, totalSpent);
            Patient* p = findPatientById(currentPatientId);
            if (p) printf(">>> 当前账户可用余额: %.2f <<<\n", p->balance);
            printf("\n【温馨提示】住院期间的床位费与各项药费明细已被打包归拢，请在 [5. ⑤住院记录] 中查阅！\n");
            system("pause");
        }
    }
}
// 患者端总路由菜单 (文案修改)
// ---------------------------------------------------------
void changePatientPassword(const char* currentId) {
    Patient* p = findPatientById(currentId);
    if (!p) return;

    char oldPwd[50], newPwd[50], confirmPwd[50];
    printf("\n--- 修改个人登录密码 ---\n");
    printf("请输入原密码 (输入0返回): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    if (strcmp(p->password, oldPwd) != 0) {
        printf("【错误】原密码不正确！\n");
        system("pause");
        return;
    }

    printf("请输入新密码: ");
    safeGetString(newPwd, 50);
    printf("请再次确认新密码: ");
    safeGetString(confirmPwd, 50);

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("【错误】两次输入的新密码不一致！\n");
        system("pause");
        return;
    }

    strcpy(p->password, newPwd);
    printf("【成功】密码修改成功，请牢记您的新密码！\n");
    system("pause");
}

// ---------------------------------------------------------
// 患者端总路由菜单
// ---------------------------------------------------------
void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);
        printf("\n--- 患者自助终端 (当前登录患者: %s - %s) ---\n", p->name, p->id);

        printf("1. 自助预约挂号\n2. 财务中心缴费\n3. 个人医疗档案库\n4. 修改登录密码\n0. 注销并返回大厅\n选择: ");
        switch (safeGetInt()) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 4: changePatientPassword(currentId); break; // 挂载新增的改密码功能
        case 0: return;
        }
    }
}