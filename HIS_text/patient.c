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

extern void generateRecordID(char* buffer);
extern Bed* bedHead;

void generatePatientID(char* idBuffer) {
    int maxId = 999;
    int currentIdNum;

    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (sscanf(p->id, "P2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) {
                maxId = currentIdNum;
            }
        }
    }
    sprintf(idBuffer, "P2025%04d", maxId + 1);
}

Patient* findPatientById(const char* pid) {
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (strcmp(p->id, pid) == 0) return p;
    }
    return NULL;
}

void registerPatient() {
    printf("\n========== 账户注册与建档 ==========\n");
    printf("请选择就诊类型 (1.普通门诊 2.急诊绿色通道 0.取消返回): ");

    int type;
    while (1) {
        type = safeGetInt();
        if (type == 0) return;
        if (type == 1 || type == 2) break;
        printf("  [!] 输入有误：只能输入 1 或 2，请重新选择: ");
    }

    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    if (!newPatient) { printf("  [!] 内存分配失败，无法注册患者。\n"); return; }
    newPatient->next = NULL;
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    while (1) {
        printf("请输入真实姓名 (输入0取消): ");
        safeGetString(newPatient->name, 100);
        if (strcmp(newPatient->name, "0") == 0) { free(newPatient); return; }
        if (strlen(newPatient->name) > 0) break;
    }

    /* 拦截并验证密码合法性 */
    printf("请设置登录密码 (仅限数字或字母组合, 输入0取消): ");
    safeGetPassword(newPatient->password, 50);
    if (strcmp(newPatient->password, "0") == 0) { free(newPatient); return; }

    printf("请输入生理性别 (男/女, 输入0取消): ");
    safeGetGender(newPatient->gender, 10);

    if (!newPatient->isEmergency) {
        /* 严控年龄约束 */
        printf("请输入周岁年龄 (1-200, 输入0取消): ");
        while (1) {
            newPatient->age = safeGetInt();
            if (newPatient->age == 0) { free(newPatient); return; }
            if (newPatient->age >= 1 && newPatient->age <= 200) break;
            printf("  [!] 输入异常：年龄必须在 1~200 之间，请重新输入正确的年龄 (输入0取消): ");
        }

        printf("请输入过敏史(无则填无, 输入0取消): ");
        safeGetString(newPatient->allergy, 100);
        if (strcmp(newPatient->allergy, "0") == 0) { free(newPatient); return; }
        if (strlen(newPatient->allergy) == 0) strcpy(newPatient->allergy, "无");
    }
    else {
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;

    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("\n  [√] 系统建档操作已完成。\n");
    printf("  ==========================================\n");
    printf("  您的终身就诊识别码为: 【 %s 】 (凭此号登录)\n", newPatient->id);
    printf("  ==========================================\n");
}

void bookAppointment(const char* currentPatientId) {
    char today[11], nextWeek[11];
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);

    t += 7 * 24 * 60 * 60;
    tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        system("cls");
        printf("\n========== 自助预约门诊挂号 ==========\n");
        printf("  [1] 按【科室名称】搜寻未来一周排班\n");
        printf("  [2] 按【医生姓名/工号】搜寻排班\n");
        printf("  [0] 放弃挂号，返回上级菜单\n");
        printf("--------------------------------------\n");
        printf("  请选择搜寻引擎: ");

        int choice = safeGetInt();
        if (choice == 0) return;

        char keyword[50];

        if (choice == 1) {
            char depts[20][50];
            int dCount = 0;

            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                int exists = 0;
                for (int i = 0; i < dCount; i++) {
                    if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; }
                }
                if (!exists && strlen(stf->department) > 0) {
                    strcpy(depts[dCount], stf->department);
                    dCount++;
                }
            }
            printf("\n  [系统当前已开设的门诊科室]: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);

            while (1) {
                printf("\n  请输入您要挂号的目标科室名称 (输入0返回): ");
                safeGetString(keyword, 50);

                if (strcmp(keyword, "0") == 0) break;

                int isValidDept = 0;
                for (int i = 0; i < dCount; i++) {
                    if (strcmp(depts[i], keyword) == 0) {
                        isValidDept = 1;
                        break;
                    }
                }

                if (isValidDept) break;
                else printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！");
            }
            if (strcmp(keyword, "0") == 0) continue;
        }
        else if (choice == 2) {
            printf("  请输入医生精确姓名或纯数字工号 (如:李四 / 1001, 输入0返回): ");
            safeGetString(keyword, 50);
            if (strcmp(keyword, "0") == 0) continue;
        }
        else {
            printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
            system("pause");
            continue;
        }

        printf("\n========== 未来一周可预约排班总表 (%s 至 %s) ==========\n", today, nextWeek);
        printf(" %-8s | %-12s | %-8s | %-16s | %-10s | %-10s\n", "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
        printf("----------------------------------------------------------------------------------\n");

        int found = 0;
        int maxRegId = 4999;

        // 预先算好最大的挂号ID，方便后面申请
        for (Record* r_temp = recordHead->next; r_temp != NULL; r_temp = r_temp->next) {
            int curReg;
            if (sscanf(r_temp->recordId, "REG2025%04d", &curReg) == 1) {
                if (curReg > maxRegId) maxRegId = curReg;
            }
        }

        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) continue;

            Staff* matchedDoc = NULL;
            for (Staff* d = staffHead->next; d != NULL; d = d->next) {
                // 【修改点】：直接字符串比对
                if (strcmp(d->id, s->doctor_id) == 0) { matchedDoc = d; break; }
            }
            if (!matchedDoc) continue;

            int match = 0;
            if (choice == 1 && strcmp(matchedDoc->department, keyword) == 0) match = 1;
            if (choice == 2) {
                if (strstr(matchedDoc->name, keyword) || strcmp(matchedDoc->id, keyword) == 0) match = 1;
            }

            if (match && strcmp(s->shift, "休息") != 0) {
                char docDisp[130];
                snprintf(docDisp, sizeof(docDisp), "%s(%s)", matchedDoc->name, matchedDoc->id);
                printf(" [%-6d] | %-12s | %-8s | %-16s | %-10s | %-10s\n", s->schedule_id, s->date, s->shift, docDisp, matchedDoc->department, matchedDoc->level);
                found++;
            }
        }

        if (found == 0) { printf("\n  [!] 数据流反馈：未搜索到满足当前条件的排班资源。\n"); system("pause"); continue; }

        printf("----------------------------------------------------------------------------------\n");
        printf("  请输入要确认选择的【排班ID】 (输入0重新搜索): ");
        int targetSchId = safeGetInt();
        if (targetSchId == 0) continue;

        Schedule* targetSch = NULL;
        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (s->schedule_id == targetSchId) { targetSch = s; break; }
        }
        if (!targetSch) { printf("  [!] 参数越界：排班ID不属于有效集合。\n"); system("pause"); continue; }

        Staff* targetDoc = NULL;
        for (Staff* d = staffHead->next; d != NULL; d = d->next) {
            // 【修改点】：直接字符串比对
            if (strcmp(d->id, targetSch->doctor_id) == 0) { targetDoc = d; break; }
        }
        if (!targetDoc) { printf("  [!] 底层数据异常：医生档案关联引用失败。\n"); system("pause"); continue; }

        char staffIdStr[22];
        snprintf(staffIdStr, sizeof(staffIdStr), "D%s", targetDoc->id);

        int patientDailyActive = 0, patientDeptDailyActive = 0, sameDocSameDay = 0, docDailyCount = 0, hospitalDailyCount = 0;

        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++;
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;

                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;
                    for (Staff* recDoc = staffHead->next; recDoc != NULL; recDoc = recDoc->next) {
                        char tempDId[22];
                        snprintf(tempDId, sizeof(tempDId), "D%s", recDoc->id);
                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1;
                            break;
                        }
                    }
                }
            }
        }

        if (hospitalDailyCount >= 1000) { printf("\n  [系统过载] 全院日门诊量已达设定阈值，请求被驳回。\n"); system("pause"); continue; }
        if (patientDailyActive >= 5) { printf("\n  [策略约束] 患者单日预约次数已达上限。\n"); system("pause"); continue; }
        if (patientDeptDailyActive >= 1) { printf("\n  [策略约束] 存在科室互斥，同日同科室不允许重复挂号。\n"); system("pause"); continue; }
        if (sameDocSameDay) { printf("\n  [策略约束] 该日已存在相同医师的挂号记录，系统拒绝重复占用医疗资源。\n"); system("pause"); continue; }

        if (docDailyCount >= 50) {
            int recCount = 0;
            printf("\n  [资源售罄] %s 医生在 %s 的有效号源已全部分配完毕。\n", targetDoc->name, targetSch->date);
            printf("  >>> 调度系统为您推荐以下相似接诊资源 <<<\n");

            printf("\n  [分支一：该医师的其他接诊时段]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                // 【修改点】：直接字符串比对
                if (strcmp(altS->doctor_id, targetDoc->id) == 0 && strcmp(altS->date, targetSch->date) != 0 && strcmp(altS->shift, "休息") != 0 && strcmp(altS->date, today) >= 0 && strcmp(altS->date, nextWeek) <= 0) {
                    printf("    -> 资源索引 [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift); recCount++;
                }
            }

            printf("\n  [分支二：同日同科室的出诊医师]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                // 【修改点】：直接字符串比对
                if (strcmp(altS->date, targetSch->date) == 0 && strcmp(altS->doctor_id, targetDoc->id) != 0 && strcmp(altS->shift, "休息") != 0) {
                    for (Staff* altD = staffHead->next; altD != NULL; altD = altD->next) {
                        // 【修改点】：直接字符串比对
                        if (strcmp(altD->id, altS->doctor_id) == 0 && strcmp(altD->department, targetDoc->department) == 0) {
                            printf("    -> 资源索引 [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift);
                            recCount++; break;
                        }
                    }
                }
            }
            if (recCount == 0) printf("  未命中相似属性资源，请重置过滤条件。\n");
            system("pause"); continue;
        }

        int seqNum = docDailyCount + 1;
        double regFee = strstr(targetDoc->level, "主任") != NULL ? 50.0 : 15.0;

        Record* newRecord = (Record*)malloc(sizeof(Record));
        if (!newRecord) { printf("  [!] 内存分配失败。\n"); return; }
        sprintf(newRecord->recordId, "REG2025%04d", maxRegId + 1);
        newRecord->type = 1;
        strcpy(newRecord->patientId, currentPatientId);
        strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee;
        newRecord->isPaid = 0;
        sprintf(newRecord->description, "挂号:%s(%s)_排号:%d", targetDoc->name, targetSch->date, seqNum);
        getCurrentTimeStr(newRecord->createTime, 30);
        newRecord->next = NULL;

        Record* temp = recordHead;
        while (temp->next) temp = temp->next;
        temp->next = newRecord;

        printf("\n  [√ 事务确认] %s 医师接诊号源申请成功！系统产生待支付流水 %.2f 元。\n", targetDoc->name, regFee);
        printf("  =======================================================\n");
        printf("  >>> 当日分诊系统的预计算序号为：【 第 %d 号 】 <<<\n", seqNum);
        printf("  =======================================================\n");
        printf("  (系统提示：请至费用中心结清账单，否则当前流水将不会被下发至接诊大厅)\n");
        system("pause"); return;
    }
}

static void checkAndAutoDischarge(Record* rec, const char* currentPatientId) {
    if (rec->type == 5 && strstr(rec->description, "出院清算_补缴欠费差额")) {
        char bedId[20] = { 0 };
        double bFee = 0, dFee = 0;
        char summary[200];

        if (sscanf(rec->description, "出院清算_补缴欠费差额_床位:%19[^_]_床费:%lf_药费:%lf", bedId, &bFee, &dFee) == 3) {
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (strcmp(b->bedId, bedId) == 0) {
                    b->isOccupied = 0;
                    strcpy(b->patientId, "");
                    break;
                }
            }

            for (Record* rr = recordHead->next; rr != NULL; rr = rr->next) {
                if (rr->type == 3 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 4) rr->isPaid = 1;
            }

            for (Record* rr = recordHead->next; rr != NULL; rr = rr->next) {
                if (rr->type == 5 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 1) {
                    rr->isPaid = 2;
                    sprintf(summary, " [出院结算:床费%.2f 药费%.2f 总消费%.2f]", bFee, dFee, bFee + dFee);
                    strcat(rr->description, summary);
                }
            }

            strcpy(rec->description, "出院清算尾款结清");
            printf("\n  [业务流转反馈] 资金接口监听到出院尾款入账。\n  系统已释放相关住院资源配置，结算清单归档操作完成。\n");
        }
    }
}

void financeCenter(const char* currentPatientId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentPatientId);
        if (!p) return;

        printf("\n========== 个人财务结算中心 ==========\n");
        printf("  [当前账户可用余额]:  %.2f 元\n", p->balance);
        printf("--------------------------------------\n");
        printf("  [1] 在线网银充值 (预存备用金)\n  [2] 待处理账单清算 (聚合结算)\n  [0] 返回主终端\n");
        printf("  业务指令: ");

        int choice = safeGetInt();
        if (choice == 0) return;

        if (choice == 1) {
            printf("\n  请输入需充值的金额 (输入0取消): ");
            double money = safeGetDouble();
            if (money > 0) {
                p->balance += money;
                Record* r7 = (Record*)malloc(sizeof(Record));
                if (!r7) { printf("  [!] 内存分配失败。\n"); return; }
                extern void generateRecordID(char* buffer);
                generateRecordID(r7->recordId);
                r7->type = 7; strcpy(r7->patientId, currentPatientId); strcpy(r7->staffId, "SYS");
                r7->cost = money; r7->isPaid = 1; sprintf(r7->description, "终端自助充值入账");
                getCurrentTimeStr(r7->createTime, 30); r7->next = recordHead->next; recordHead->next = r7;
                printf("  [完成] 充值业务受理成功，相关资产已到账，金额: %.2f 元。\n", money); system("pause");
            }
            else if (money != 0) {
                printf("  [异常] 充值数额校验失败，必须为正值参数。\n");
                system("pause");
            }
        }
        else if (choice == 2) {
            while (1) {
                int hasUnpaid = 0;
                double totalUnpaidCost = 0.0;
                system("cls");
                printf("\n========== 待清算账单明细列表 ==========\n");

                for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                        char typeName[20];
                        switch (rec->type) {
                        case 1:strcpy(typeName, "门诊挂号"); break;
                        case 2:strcpy(typeName, "临床诊疗"); break;
                        case 3:strcpy(typeName, "药事调拨"); break;
                        case 4:strcpy(typeName, "辅助检验"); break;
                        case 5:strcpy(typeName, "住院账目"); break;
                        }
                        printf("  流水号: %-12s | 业务项: [%-10s] | 实收: %-8.2f | 备忘录: %s\n", rec->recordId, typeName, rec->cost, rec->description);
                        totalUnpaidCost += rec->cost; hasUnpaid = 1;
                    }
                }

                if (!hasUnpaid) { printf("  [核查反馈] 当前用户节点下未检测到未决的财务单据。\n"); system("pause"); break; }

                printf("----------------------------------------------------------------------\n");
                printf("  [资金比对] 待清算债务总额: %.2f 元  |  账户备用余额: %.2f 元\n\n", totalUnpaidCost, p->balance);
                printf("  1. 开启一键聚合支付 (全额核销)\n  2. 提取指定流水号执行单项核销\n  0. 返回上一级\n  请选择: ");

                int payChoice = safeGetInt();
                if (payChoice == 0) break;

                if (payChoice == 1) {
                    if (p->balance < totalUnpaidCost) {
                        printf("  [拒绝执行] 账户流动资金不足以覆盖总负债。(资金缺口: %.2f 元)\n", totalUnpaidCost - p->balance);
                        system("pause");
                    }
                    else {
                        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                                int maxId = 0;
                                p->balance -= rec->cost; rec->isPaid = 1;

                                Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
                                if (!newTrans) { printf("  [!] 内存分配失败，交易记录未生成。\n"); continue; }
                                for (Transaction* curr = transactionList; curr != NULL; curr = curr->next) {
                                    if (curr->id > maxId) maxId = curr->id;
                                }
                                newTrans->id = maxId + 1;
                                if (rec->type == 3) newTrans->type = 3; else if (rec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                                newTrans->amount = rec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, rec->description); newTrans->next = NULL;

                                if (!transactionList) transactionList = newTrans;
                                else {
                                    Transaction* curr = transactionList;
                                    while (curr->next) curr = curr->next;
                                    curr->next = newTrans;
                                }

                                checkAndAutoDischarge(rec, currentPatientId);
                            }
                        }
                        printf("  [处理成功] 当前所有挂起账单已完成聚合清算。\n"); system("pause"); break;
                    }
                }
                else if (payChoice == 2) {
                    printf("\n  请输入需独立清算的单据流水号 (输入0取消): ");
                    char target[30];
                    safeGetString(target, 30);
                    if (strcmp(target, "0") == 0) continue;

                    Record* tRec = NULL;
                    for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                        if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; }
                    }
                    if (!tRec) { printf("  [!] 流水寻址失败：单号输入错误或该笔单据已被系统核销。\n"); system("pause"); continue; }

                    if (p->balance < tRec->cost) { printf("  [拒绝执行] 账户余额不足以抵扣此笔单项交易。\n"); system("pause"); }
                    else {
                        int maxId = 0;
                        p->balance -= tRec->cost; tRec->isPaid = 1;
                        Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));

                        for (Transaction* curr = transactionList; curr != NULL; curr = curr->next) {
                            if (curr->id > maxId) maxId = curr->id;
                        }
                        newTrans->id = maxId + 1;
                        if (tRec->type == 3) newTrans->type = 3; else if (tRec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                        newTrans->amount = tRec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, tRec->description); newTrans->next = NULL;

                        if (!transactionList) transactionList = newTrans;
                        else {
                            Transaction* curr = transactionList;
                            while (curr->next) curr = curr->next;
                            curr->next = newTrans;
                        }

                        printf("  [处理成功] 单据 %s 核销完毕，产生扣除记录 %.2f 元。\n", tRec->recordId, tRec->cost);
                        checkAndAutoDischarge(tRec, currentPatientId);
                        system("pause");
                    }
                }
                else {
                    printf("  [!] 无效的菜单选项，请正确输入提供的数字编号！\n");
                    system("pause");
                }
            }
        }
        else {
            printf("  [!] 无效的菜单选项，请正确输入提供的数字编号！\n");
            system("pause");
        }
    }
}

void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n========== 个人医疗时序档案检索平台 ==========\n");
        printf("  [1] ① 调取门诊与排号时序表\n");
        printf("  [2] ② 调取临床诊疗与医嘱报告\n");
        printf("  [3] ③ 调取门诊处方与耗材划扣清单\n");
        printf("  [4] ④ 调取辅助生化与影像开单记录\n");
        printf("  [5] ⑤ 进入住院生命周期数据沙盘\n");
        printf("  [6] 汇总终身财务进出流水总账\n");
        printf("  [0] 退出检索平台\n");
        printf("----------------------------------------\n");
        printf("  请指派数据抽取卷宗号: ");

        int c = safeGetInt();
        if (c == 0) return;

        if (c >= 1 && c <= 4) {
            int printed = 0;
            printf("\n--- 门诊业务类型查询结果反馈 ---\n");
            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == c && rec->isPaid != 4 && !strstr(rec->description, "[住院记账]")) {
                    printf("  记录流水: %-12s | 快照: %s | 净值: %-6.2f | 业务详述: %s\n", rec->recordId, rec->createTime, rec->cost, rec->description);
                    printed = 1;
                }
            }
            if (!printed) printf("  [数据空置] 底层结构中无关联的可用溯源数据。\n");
            system("pause");
        }
        else if (c == 5) {
            while (1) {
                int count = 0;
                char statusStr[30];

                system("cls");
                printf("\n========== 住院业务数据聚合与清算总单溯源 ==========\n");
                printf("  %-15s | %-10s | %-16s | %-20s\n", "系统统筹编号", "基准发生额", "执行状态界定", "时间流创建戳");
                printf("--------------------------------------------------------------------\n");

                for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == 5) {
                        if (rec->isPaid == 0) strcpy(statusStr, "[挂起待缴]");
                        else if (rec->isPaid == 1) strcpy(statusStr, "[运行期(预存生效)]");
                        else strcpy(statusStr, "[闭环并结档]");

                        printf("  %-15s | %-10.2f | %-16s | %-20s\n", rec->recordId, rec->cost, statusStr, rec->createTime);
                        count++;
                    }
                }
                if (count == 0) { printf("  平台校验反馈：未索引到关联的病区驻留与注册流水。\n"); system("pause"); break; }

                printf("--------------------------------------------------------------------\n");
                printf("  [数据提取] 为执行明细下钻分析，请提供【系统统筹编号】 (输入0返回): ");
                char target[30];
                safeGetString(target, 30);
                if (strcmp(target, "0") == 0) break;

                Record* tRec = NULL;
                for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                    if (strcmp(rec->recordId, target) == 0 && rec->type == 5 && strcmp(rec->patientId, currentPatientId) == 0) { tRec = rec; break; }
                }
                if (!tRec) { printf("  [!] 输入越界，参数无法命中有效的流转树结构。\n"); system("pause"); continue; }

                system("cls");
                printf("\n========== 数据分析视图 - 住院链路拆解 (%s) ==========\n", tRec->recordId);
                printf("  【初始环境约束】\n  触发时间节点: %s\n  核心属性集: %s\n", tRec->createTime, tRec->description);

                if (tRec->isPaid == 2 && strstr(tRec->description, "出院结算")) {
                    int hasDrug = 0;
                    printf("\n  【统筹资金收支盘点域】\n");
                    printf("   + 账期初期投入保障资金: %.2f 元\n", tRec->cost);

                    for (Record* r_link = recordHead->next; r_link != NULL; r_link = r_link->next) {
                        if (strcmp(r_link->patientId, currentPatientId) == 0) {
                            if (r_link->type == 8 && strstr(r_link->description, "出院清算_押金结余退回")) printf("   - 链路完结盈余清退: %.2f 元 (已退池至虚拟账户, 关联依据:%s)\n", r_link->cost, r_link->recordId);
                            else if (r_link->type == 5 && strstr(r_link->description, "出院清算尾款结清")) printf("   + 账期末补录欠款清缴: %.2f 元 (前端聚合支付已消化, 关联依据:%s)\n", r_link->cost, r_link->recordId);
                        }
                    }

                    printf("\n  【物理隔离区药材资源异动表】\n");
                    for (Record* r_drug = recordHead->next; r_drug != NULL; r_drug = r_drug->next) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && strstr(r_drug->description, "[住院记账]")) {
                            printf("   ├─ %s | %s\n", r_drug->recordId, r_drug->description); hasDrug = 1;
                        }
                    }
                    if (!hasDrug) printf("   └─ (数据验证一致：本流转周期内未发生病区级药事调用行为)\n");
                }
                else if (tRec->isPaid == 1 && !strstr(tRec->description, "出院清算")) {
                    int hasDrug = 0;
                    printf("\n  【实时态链路分析】系统正在持续追踪该主体的在院生命周期...\n");
                    printf("   + 系统维系担保金池配额: %.2f 元\n", tRec->cost);
                    printf("\n  【挂载态内部挂账耗材明细】\n");

                    for (Record* r_drug = recordHead->next; r_drug != NULL; r_drug = r_drug->next) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && r_drug->isPaid == 4 && strstr(r_drug->description, "[住院记账]")) {
                            printf("   ├─ %s | %s\n", r_drug->recordId, r_drug->description); hasDrug = 1;
                        }
                    }
                    if (!hasDrug) printf("   └─ (尚未检测到被异步挂起的预记账开支)\n");
                }
                else if (tRec->isPaid == 0) {
                    printf("\n  【流程锁死预警】该生命周期遭遇资金阻断，押金储备已被消耗殆尽。\n  底层资源控制器已关闭病区剥离权限，直至剩余欠款由支付终端平账。\n");
                }
                printf("\n-------------------------------------------------------------------\n");
                system("pause");
            }
        }
        else if (c == 6) {
            double totalSpent = 0.0, totalRecharged = 0.0, totalRefunded = 0.0;
            int printed = 0;
            char typeName[30];

            printf("\n========== 个体终身资产流入流出全局审计报表 ==========\n");
            printf(" %-20s | %-10s | %-10s | %-40s\n", "系统登记时间戳", "变动矢量", "振幅金额", "特征字段");
            printf("------------------------------------------------------------------------------------------\n");

            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0) {
                    if (rec->type >= 1 && rec->type <= 5 && rec->isPaid > 0 && rec->isPaid != 4) {
                        if (!(rec->type == 3 && strstr(rec->description, "[住院记账]"))) {
                            totalSpent += rec->cost; strcpy(typeName, "耗损 (-)");
                            printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                        }
                    }
                    else if (rec->type == 7) {
                        totalRecharged += rec->cost; strcpy(typeName, "汇入 (+)");
                        printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                    }
                    else if (rec->type == 8) {
                        totalRefunded += rec->cost; strcpy(typeName, "清退 (+)");
                        printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                    }
                }
            }
            if (!printed) printf("  检索模块反馈：暂未发现足以构建审计链路的财务活动数据。\n");
            printf("------------------------------------------------------------------------------------------\n");
            printf("  【整合矩阵分析】 绝对注资金额: +%.2f | 系统冲账返还: +%.2f | 业务实发核销: -%.2f\n", totalRecharged, totalRefunded, totalSpent);

            Patient* p = findPatientById(currentPatientId);
            if (p) printf("  >>> [账本比对] 当前存储池预留基数: ￥ %.2f <<<\n", p->balance);
            printf("\n  (报表申明：发生于住院框架内衍生的药事耗减，已被底层结算网格接管执行映射，此处仅作统一披露。)\n");
            system("pause");
        }
        else {
            printf("  [!] 无效的菜单选项，请正确输入提供的数字编号！\n");
            system("pause");
        }
    }
}

void changePatientPassword(const char* currentId) {
    Patient* p = findPatientById(currentId);
    if (!p) return;

    char oldPwd[50] = { 0 }, newPwd[50] = { 0 }, confirmPwd[50] = { 0 };

    printf("\n========== 核心安全凭证更替操作 ==========\n");
    printf("请输入原始加密串进行握手校验 (输入0取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "0") == 0) return;

    if (strcmp(p->password, oldPwd) != 0) {
        printf("  [安全阻断] 特征匹配失败，已拒绝篡改权限的下发！\n");
        system("pause");
        return;
    }

    printf("请输入目标设定的新版密钥 (仅限数字或字母组合, 输入0取消): ");
    safeGetPassword(newPwd, 50);
    if (strcmp(newPwd, "0") == 0) return;

    printf("重复输入以完成校验签名 (输入0取消): ");
    safeGetString(confirmPwd, 50);
    if (strcmp(confirmPwd, "0") == 0) return;

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [异常] 二次校验哈希不一致，本次指令被系统抛弃。\n");
        system("pause");
        return;
    }

    strcpy(p->password, newPwd);
    printf("  [完成] 底层持久化写入成功，您的服务凭证已安全重置！\n");
    system("pause");
}

void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);

        printf("\n======================================================\n");
        printf("   患者专属自助数据终端   [ 登录实体映射: %s (%s) ]\n", p->name, p->id);
        printf("======================================================\n");
        printf("  [1] 门诊自助挂号系统(在线预约)\n");
        printf("  [2] 财务与费用中心 (账户缴费)\n");
        printf("  [3] 个人医疗档案库 (查询报告)\n");
        printf("  [4] 个人账号安全设置 (密码修改)\n");
        printf("  [0] 结束当前操作，返回系统主菜单\n");
        printf("------------------------------------------------------\n");
        printf("  请指派所需调用的系统内核级编号: ");

        int option = safeGetInt();
        switch (option) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 4: changePatientPassword(currentId); break;
        case 0: return;
        default:
            printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
            system("pause");
            break;
        }
    }
}