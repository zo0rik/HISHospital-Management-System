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
#include "inpatient_department.h"

extern void generateRecordID(char* buffer);
extern Bed* bedHead;
extern int appendTransactionSafe(int type, double amount, const char* description);

static int getCurrentYearValue(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    return tm_info ? (tm_info->tm_year + 1900) : 2026;
}

static int extractTrailingSequence(const char* id) {
    int len = (int)strlen(id);
    if (len < 4) return -1;
    int seq = -1;
    if (sscanf(id + len - 4, "%4d", &seq) == 1) return seq;
    return -1;
}

static int parseDrugRecordInfo(const Record* rec, char* drugName, size_t drugNameSize, int* qtyOut) {
    if (!rec || rec->type != 3 || !drugName || drugNameSize == 0 || !qtyOut) return 0;
    drugName[0] = '\0';
    *qtyOut = 0;
    if (sscanf(rec->description, "药品:%49[^_]_单价:%*[^_]_数量:%d", drugName, qtyOut) == 2 && *qtyOut > 0) {
        return 1;
    }
    return 0;
}

static Drug* findDrugByName(const char* name) {
    for (Drug* d = drugList->next; d != NULL; d = d->next) {
        if (strcmp(d->name, name) == 0) return d;
    }
    return NULL;
}

static int ensureDrugStockForRecord(const Record* rec) {
    char drugName[50];
    int qty = 0;
    if (!parseDrugRecordInfo(rec, drugName, sizeof(drugName), &qty)) return 1;
    Drug* drug = findDrugByName(drugName);
    return drug && drug->stock >= qty;
}

static DrugHistory* createDrugDispenseHistoryForRecord(const Record* rec, Drug** drugOut, int* qtyOut) {
    char drugName[50];
    int qty = 0;
    if (!parseDrugRecordInfo(rec, drugName, sizeof(drugName), &qty)) {
        if (drugOut) *drugOut = NULL;
        if (qtyOut) *qtyOut = 0;
        return NULL;
    }

    Drug* drug = findDrugByName(drugName);
    if (!drug || drug->stock < qty) return (DrugHistory*)-1;

    DrugHistory* hist = (DrugHistory*)malloc(sizeof(DrugHistory));
    if (!hist) return (DrugHistory*)-1;

    hist->drug_id = drug->id;
    hist->type = 2;
    hist->quantity = qty;
    getCurrentTimeStr(hist->time, sizeof(hist->time));
    hist->next = NULL;

    if (drugOut) *drugOut = drug;
    if (qtyOut) *qtyOut = qty;
    return hist;
}

static void commitDrugDispense(Drug* drug, int qty, DrugHistory* hist) {
    if (!drug || !hist || qty <= 0) return;
    drug->stock -= qty;
    strncpy(drug->last_out, hist->time, sizeof(drug->last_out) - 1);
    drug->last_out[sizeof(drug->last_out) - 1] = '\0';
    hist->next = drugHistoryList->next;
    drugHistoryList->next = hist;
}

void generatePatientID(char* idBuffer) {
    int maxId = 999;
    int currentIdNum;
    int currentYear = getCurrentYearValue();
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        currentIdNum = extractTrailingSequence(p->id);
        if (currentIdNum > maxId) maxId = currentIdNum;
    }
    sprintf(idBuffer, "P%d%04d", currentYear, maxId + 1);
}

Patient* findPatientById(const char* pid) {
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (strcmp(p->id, pid) == 0) return p;
    }
    return NULL;
}

static int isInpatientArrearsRecord(const Record* rec) {
    return rec &&
        rec->type == 5 &&
        strstr(rec->description, "[住院欠费补交单]") != NULL;
}

static void appendSimpleRecord(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description);
static void checkAndAutoDischarge(Record* rec, const char* currentPatientId);

static int determineTransactionTypeForRecord(const Record* rec) {
    if (!rec) return TRANS_OUTPATIENT_INCOME;
    if (rec->type == 3) return TRANS_DRUG_INCOME;
    if (rec->type == 4) return TRANS_EXAM_INCOME;
    if (rec->type == 5) return TRANS_INPATIENT_BED_INCOME;
    return TRANS_OUTPATIENT_INCOME;
}

static int settleSingleRecord(Patient* p, const char* currentPatientId, Record* rec) {
    if (!p || !currentPatientId || !rec || rec->isPaid != 0) return 0;
    if (p->balance < rec->cost) return 0;

    Drug* dispenseDrug = NULL;
    DrugHistory* dispenseHist = NULL;
    int dispenseQty = 0;

    if (rec->type == 3) {
        if (!ensureDrugStockForRecord(rec)) return 0;
        dispenseHist = createDrugDispenseHistoryForRecord(rec, &dispenseDrug, &dispenseQty);
        if (dispenseHist == (DrugHistory*)-1) return 0;
    }

    if (isInpatientArrearsRecord(rec)) {
        if (!appendTransactionSafe(TRANS_INPATIENT_SUPPLEMENT, rec->cost, "住院欠费补交(不计收入)")) {
            if (dispenseHist && dispenseHist != (DrugHistory*)-1) free(dispenseHist);
            return 0;
        }
        p->balance -= rec->cost;
        p->inpatientDeposit += rec->cost;
        rec->isPaid = 2;
        appendSimpleRecord(5, currentPatientId, "SYS", rec->cost, 1, "[住院明细][补缴] 住院欠费补交完成");
    }
    else {
        int transType = determineTransactionTypeForRecord(rec);
        if (!appendTransactionSafe(transType, rec->cost, rec->description)) {
            if (dispenseHist && dispenseHist != (DrugHistory*)-1) free(dispenseHist);
            return 0;
        }
        p->balance -= rec->cost;
        rec->isPaid = 1;
        if (dispenseHist) {
            commitDrugDispense(dispenseDrug, dispenseQty, dispenseHist);
            dispenseHist = NULL;
        }
    }

    checkAndAutoDischarge(rec, currentPatientId);
    return 1;
}

static void appendSimpleRecord(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description) {
    Record* rec = (Record*)malloc(sizeof(Record));
    if (!rec) return;

    generateRecordID(rec->recordId);
    rec->type = type;
    strcpy(rec->patientId, patientId);
    strcpy(rec->staffId, staffId);
    rec->cost = cost;
    rec->isPaid = isPaid;
    strncpy(rec->description, description, sizeof(rec->description) - 1);
    rec->description[sizeof(rec->description) - 1] = '\0';
    getCurrentTimeStr(rec->createTime, 30);

    rec->next = recordHead->next;
    recordHead->next = rec;
}

void registerPatient() {
    printf("\n========== 账户注册与建档 ==========\n");
    printf("请选择就诊类型 (1.普通门诊 2.急诊绿色通道 -1.取消返回): ");

    int type;
    while (1) {
        type = safeGetInt();
        if (type == -1) return;
        if (type == 1 || type == 2) break;
        printf("  [!] 输入有误：只能输入 1 或 2，请重新选择: ");
    }

    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    if (!newPatient) {
        printf("  [!] 内存分配失败，无法注册患者。\n");
        return;
    }
    newPatient->next = NULL;
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    while (1) {
        printf("请输入真实姓名 (输入-1取消): ");
        safeGetString(newPatient->name, 100);
        if (strcmp(newPatient->name, "-1") == 0) {
            free(newPatient);
            return;
        }
        if (strlen(newPatient->name) > 0) break;
        printf("  [!] 输入不能为空，请重新输入！\n");
    }

    printf("请设置登录密码 (至少6位，仅限数字或字母组合, 输入-1取消): ");
    safeGetPassword(newPatient->password, 50);
    if (strcmp(newPatient->password, "-1") == 0) {
        free(newPatient);
        return;
    }

    printf("请输入生理性别 (男/女, 输入-1取消): ");
    safeGetGender(newPatient->gender, 10);
    if (strcmp(newPatient->gender, "-1") == 0) {
        free(newPatient);
        return;
    }

    if (!newPatient->isEmergency) {
        printf("请输入周岁年龄 (1-200, 输入-1取消): ");
        while (1) {
            newPatient->age = safeGetInt();
            if (newPatient->age == -1) {
                free(newPatient);
                return;
            }
            if (newPatient->age >= 1 && newPatient->age <= 200) break;
            printf("  [!] 输入异常：年龄必须在 1~200 之间，请重新输入 (输入-1取消): ");
        }

        while (1) {
            printf("请输入过敏史(无则填无, 输入-1取消): ");
            safeGetString(newPatient->allergy, 100);
            if (strcmp(newPatient->allergy, "-1") == 0) {
                free(newPatient);
                return;
            }
            if (strlen(newPatient->allergy) > 0) break;
            printf("  [!] 输入不能为空！\n");
        }
        if (strlen(newPatient->allergy) == 0) strcpy(newPatient->allergy, "无");
    }
    else {
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;
    newPatient->inpatientDeposit = 0.0;
    newPatient->isInpatient = 0;

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
        printf(" [-1] 放弃挂号，返回上级菜单\n");
        printf("--------------------------------------\n");
        printf("  请选择搜寻引擎: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        char keyword[50];

        if (choice == 1) {
            char depts[20][50];
            int dCount = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                int exists = 0;
                for (int i = 0; i < dCount; i++) {
                    if (strcmp(depts[i], stf->department) == 0) {
                        exists = 1;
                        break;
                    }
                }
                if (!exists && strlen(stf->department) > 0) {
                    strcpy(depts[dCount], stf->department);
                    dCount++;
                }
            }
            printf("\n  [系统当前已开设的门诊科室]: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);

            while (1) {
                printf("\n  请输入您要挂号的目标科室名称 (输入-1返回): ");
                safeGetString(keyword, 50);
                if (strcmp(keyword, "-1") == 0) break;
                if (strlen(keyword) == 0) {
                    printf("  [!] 输入不能为空！");
                    continue;
                }

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
            if (strcmp(keyword, "-1") == 0) continue;
        }
        else if (choice == 2) {
            printf("  请输入医生精确姓名或纯数字工号 (如:李四 / 1001, 输入-1返回): ");
            safeGetString(keyword, 50);
            if (strcmp(keyword, "-1") == 0) continue;
            if (strlen(keyword) == 0) {
                printf("  [!] 输入不能为空！\n");
                system("pause");
                continue;
            }
        }
        else {
            printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
            system("pause");
            continue;
        }

        int matchedSchIds[200];
        int matchedCount = 0;

        printf("\n========== 未来一周可预约排班总表 (%s 至 %s) ==========\n", today, nextWeek);
        printf("  %-8s | %-12s | %-8s | %-18s | %-10s | %-10s\n",
            "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
        printf("  ---------------------------------------------------------------------------\n");

        int found = 0;
        int maxRegId = 4999;
        for (Record* r_temp = recordHead->next; r_temp != NULL; r_temp = r_temp->next) {
            int curReg;
            curReg = extractTrailingSequence(r_temp->recordId);
            if (curReg >= 0) {
                if (curReg > maxRegId) maxRegId = curReg;
            }
        }

        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) continue;

            Staff* matchedDoc = NULL;
            for (Staff* d = staffHead->next; d != NULL; d = d->next) {
                if (strcmp(d->id, s->doctor_id) == 0) {
                    matchedDoc = d;
                    break;
                }
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
                printf("  [%-6d] | %-12s | %-8s | %-18s | %-10s | %-10s\n",
                    s->schedule_id, s->date, s->shift, docDisp,
                    matchedDoc->department, matchedDoc->level);
                if (matchedCount < 200) {
                    matchedSchIds[matchedCount++] = s->schedule_id;
                }
                found++;
            }
        }

        if (found == 0) {
            printf("\n  [!] 数据流反馈：未搜索到满足当前条件的排班资源。\n");
            system("pause");
            continue;
        }

        printf("  ---------------------------------------------------------------------------\n");
        printf("  请输入要确认选择的【排班ID】 (输入-1重新搜索): ");
        int targetSchId = safeGetInt();
        if (targetSchId == -1) continue;

        int idInResult = 0;
        for (int i = 0; i < matchedCount; i++) {
            if (matchedSchIds[i] == targetSchId) {
                idInResult = 1;
                break;
            }
        }
        if (!idInResult) {
            printf("  [!] 当前科室/医生无此排班，请重新输入！\n");
            system("pause");
            continue;
        }

        Schedule* targetSch = NULL;
        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (s->schedule_id == targetSchId) {
                targetSch = s;
                break;
            }
        }
        if (!targetSch) {
            printf("  [!] 参数越界：排班ID不属于有效集合。\n");
            system("pause");
            continue;
        }

        Staff* targetDoc = NULL;
        for (Staff* d = staffHead->next; d != NULL; d = d->next) {
            if (strcmp(d->id, targetSch->doctor_id) == 0) {
                targetDoc = d;
                break;
            }
        }
        if (!targetDoc) {
            printf("  [!] 底层数据异常：医生档案关联引用失败。\n");
            system("pause");
            continue;
        }

        char staffIdStr[22];
        snprintf(staffIdStr, sizeof(staffIdStr), "%s", targetDoc->id);

        int patientDailyActive = 0, patientDeptDailyActive = 0, sameDocSameDay = 0, docDailyCount = 0, hospitalDailyCount = 0;

        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++;
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;
                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;
                    for (Staff* recDoc = staffHead->next; recDoc != NULL; recDoc = recDoc->next) {
                        char tempDId[22];
                        snprintf(tempDId, sizeof(tempDId), "%s", recDoc->id);
                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1;
                            break;
                        }
                    }
                }
            }
        }

        if (hospitalDailyCount >= 1000) {
            printf("\n  [系统过载] 全院日门诊量已达设定阈值。\n");
            system("pause");
            continue;
        }
        if (patientDailyActive >= 5) {
            printf("\n  [策略约束] 患者单日预约次数已达上限。\n");
            system("pause");
            continue;
        }
        if (patientDeptDailyActive >= 1) {
            printf("\n  [策略约束] 同日同科室不允许重复挂号。\n");
            system("pause");
            continue;
        }
        if (sameDocSameDay) {
            printf("\n  [策略约束] 该日已存在相同医师的挂号记录。\n");
            system("pause");
            continue;
        }

        if (docDailyCount >= 50) {
            int recCount = 0;
            printf("\n  [资源售罄] %s 医生在 %s 的有效号源已全部分配完毕。\n", targetDoc->name, targetSch->date);
            printf("  >>> 调度系统为您推荐以下相似接诊资源 <<<\n");
            printf("\n  [分支一：该医师的其他接诊时段]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                if (strcmp(altS->doctor_id, targetDoc->id) == 0 &&
                    strcmp(altS->date, targetSch->date) != 0 &&
                    strcmp(altS->shift, "休息") != 0 &&
                    strcmp(altS->date, today) >= 0 &&
                    strcmp(altS->date, nextWeek) <= 0) {
                    printf("    -> 资源索引 [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift);
                    recCount++;
                }
            }
            printf("\n  [分支二：同日同科室的出诊医师]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                if (strcmp(altS->date, targetSch->date) == 0 &&
                    strcmp(altS->doctor_id, targetDoc->id) != 0 &&
                    strcmp(altS->shift, "休息") != 0) {
                    for (Staff* altD = staffHead->next; altD != NULL; altD = altD->next) {
                        if (strcmp(altD->id, altS->doctor_id) == 0 &&
                            strcmp(altD->department, targetDoc->department) == 0) {
                            printf("    -> 资源索引 [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift);
                            recCount++;
                            break;
                        }
                    }
                }
            }
            if (recCount == 0) printf("  未命中相似属性资源，请重置过滤条件。\n");
            system("pause");
            continue;
        }

        int seqNum = docDailyCount + 1;
        double regFee = strstr(targetDoc->level, "主任") != NULL ? 50.0 : 15.0;

        Record* newRecord = (Record*)malloc(sizeof(Record));
        if (!newRecord) {
            printf("  [!] 内存分配失败。\n");
            return;
        }
        sprintf(newRecord->recordId, "REG%d%04d", getCurrentYearValue(), maxRegId + 1);
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
        printf("  (系统提示：请至费用中心结清账单)\n");
        system("pause");
        return;
    }
}

static void checkAndAutoDischarge(Record* rec, const char* currentPatientId) {
    (void)rec;
    (void)currentPatientId;
}

void financeCenter(const char* currentPatientId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentPatientId);
        if (!p) return;

        printf("\n========== 个人财务结算中心 ==========\n");
        char balanceText[32], depositText[32], arrearsText[32];
        formatMoney(p->balance, balanceText, sizeof(balanceText));
        formatMoney(p->inpatientDeposit, depositText, sizeof(depositText));
        printf("  [当前账户可用余额]:      %s\n", balanceText);
        printf("  [当前住院押金余额]:      %s\n", depositText);
        printf("  [当前住院状态]:          %s\n", p->isInpatient ? "住院中" : "非住院");
        if (p->inpatientDeposit < 0) {
            formatMoney(-p->inpatientDeposit, arrearsText, sizeof(arrearsText));
            printf("  [当前待补交金额]:        %s\n", arrearsText);
        }
        printf("--------------------------------------\n");
        printf("  [1] 在线网银充值 (预存备用金)\n");
        printf("  [2] 待处理账单清算 (聚合结算)\n");
        printf(" [-1] 返回主终端\n");
        printf("  业务指令: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        if (choice == 1) {
            printf("\n  请输入需充值的金额 (输入-1取消): ");
            double money = safeGetMoneyInRange(0.01, 10000.0);
            if (money == -1.0) continue;

            if (money > 0) {
                p->balance += money;
                Record* r7 = (Record*)malloc(sizeof(Record));
                if (!r7) {
                    printf("  [!] 内存分配失败。\n");
                    return;
                }
                generateRecordID(r7->recordId);
                r7->type = 7;
                strcpy(r7->patientId, currentPatientId);
                strcpy(r7->staffId, "SYS");
                r7->cost = money;
                r7->isPaid = 1;
                sprintf(r7->description, "终端自助充值入账");
                getCurrentTimeStr(r7->createTime, 30);
                r7->next = recordHead->next;
                recordHead->next = r7;

                appendTransaction(TRANS_RECHARGE, money, "终端自助充值(不计收入)");

                char moneyText[32];
                formatMoney(money, moneyText, sizeof(moneyText));
                printf("  [完成] 充值业务受理成功，金额: %s。\n", moneyText);
                system("pause");
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
                        case 1: strcpy(typeName, "门诊挂号"); break;
                        case 2: strcpy(typeName, "临床诊疗"); break;
                        case 3: strcpy(typeName, "药事调拨"); break;
                        case 4: strcpy(typeName, "辅助检验"); break;
                        case 5:
                            if (isInpatientArrearsRecord(rec)) strcpy(typeName, "住院补缴");
                            else strcpy(typeName, "住院账目");
                            break;
                        default: strcpy(typeName, "其他"); break;
                        }

                        printf("  流水号: %-12s | 业务项: [%-10s] | 实收: %-8.2f | 备忘录: %s\n",
                            rec->recordId, typeName, rec->cost, rec->description);
                        totalUnpaidCost += rec->cost;
                        hasUnpaid = 1;
                    }
                }

                if (!hasUnpaid) {
                    printf("  [核查反馈] 当前无未决的财务单据。\n");
                    system("pause");
                    break;
                }

                printf("----------------------------------------------------------------------\n");
                char unpaidText[32], balanceText2[32];
                formatMoney(totalUnpaidCost, unpaidText, sizeof(unpaidText));
                formatMoney(p->balance, balanceText2, sizeof(balanceText2));
                printf("  [资金比对] 待清算总额: %s | 普通余额: %s\n\n", unpaidText, balanceText2);
                printf("  1. 一键聚合支付\n");
                printf("  2. 指定流水号单项核销\n");
                printf(" -1. 返回上一级\n");
                printf("  请选择: ");

                int payChoice = safeGetInt();
                if (payChoice == -1) break;

                if (payChoice == 1) {
                    if (p->balance < totalUnpaidCost) {
                        char gapText[32];
                        formatMoney(totalUnpaidCost - p->balance, gapText, sizeof(gapText));
                        printf("  [拒绝执行] 账户流动资金不足。(缺口: %s)\n", gapText);
                        system("pause");
                    }
                    else {
                        Record* pending[512];
                        int pendingCount = 0;
                        int stockOk = 1;

                        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                                if (pendingCount < (int)(sizeof(pending) / sizeof(pending[0]))) {
                                    pending[pendingCount++] = rec;
                                }
                                if (rec->type == 3 && !ensureDrugStockForRecord(rec)) {
                                    printf("  [拒绝执行] 药品库存已发生变化，单据 %s 暂无法支付，请联系药房刷新库存。\n", rec->recordId);
                                    stockOk = 0;
                                    break;
                                }
                            }
                        }
                        if (!stockOk) {
                            system("pause");
                            continue;
                        }

                        int processed = 0;
                        int failed = 0;
                        char failedId[30] = "";
                        for (int i = 0; i < pendingCount; ++i) {
                            if (!settleSingleRecord(p, currentPatientId, pending[i])) {
                                failed = 1;
                                strncpy(failedId, pending[i]->recordId, sizeof(failedId) - 1);
                                failedId[sizeof(failedId) - 1] = '\0';
                                break;
                            }
                            processed++;
                        }

                        if (failed) {
                            printf("  [异常] 聚合支付在单据 %s 处中止，前面已成功处理 %d 笔，其余账单保留待支付状态。\n", failedId, processed);
                            system("pause");
                            continue;
                        }

                        printf("  [处理成功] 所有挂起账单已完成聚合清算。\n");
                        system("pause");
                        break;
                    }
                }
                else if (payChoice == 2) {
                    printf("\n  请输入需独立清算的单据流水号 (输入-1取消): ");
                    char target[30];
                    safeGetString(target, 30);

                    if (strcmp(target, "-1") == 0) continue;
                    if (strlen(target) == 0) {
                        printf("  [!] 输入不能为空！\n");
                        system("pause");
                        continue;
                    }

                    Record* tRec = NULL;
                    for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                        if (strcmp(rec->recordId, target) == 0 &&
                            strcmp(rec->patientId, currentPatientId) == 0 &&
                            rec->isPaid == 0) {
                            tRec = rec;
                            break;
                        }
                    }
                    if (!tRec) {
                        printf("  [!] 流水寻址失败。\n");
                        system("pause");
                        continue;
                    }

                    if (p->balance < tRec->cost) {
                        printf("  [拒绝执行] 余额不足。\n");
                        system("pause");
                    }
                    else {
                        if (tRec->type == 3 && !ensureDrugStockForRecord(tRec)) {
                            printf("  [拒绝执行] 药房库存不足，当前药单无法完成支付。\n");
                            system("pause");
                            continue;
                        }
                        if (!settleSingleRecord(p, currentPatientId, tRec)) {
                            printf("  [异常] 本次支付未提交，请重试或联系管理员核查。\n");
                            system("pause");
                            continue;
                        }

                        if (isInpatientArrearsRecord(tRec)) {
                            printf("  [处理成功] 住院欠费补交完成，%.2f 元。\n", tRec->cost);
                        }
                        else {
                            printf("  [处理成功] 单据 %s 核销完毕，%.2f 元。\n", tRec->recordId, tRec->cost);
                        }
                        system("pause");
                    }
                }
                else {
                    printf("  [!] 无效选项！\n");
                    system("pause");
                }
            }
        }
        else {
            printf("  [!] 无效选项！\n");
            system("pause");
        }
    }
}

void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n========== 个人医疗时序档案检索平台 ==========\n");
        printf("  [1] 调取门诊与排号记录表\n");
        printf("  [2] 调取临床诊疗与医嘱报告\n");
        printf("  [3] 调取门诊处方扣费清单\n");
        printf("  [4] 调取辅助生化与影像开单记录\n");
        printf("  [5] 调取住院与押金账单记录\n");
        printf("  [6] 汇总终身财务进出流水总账\n");
        printf(" [-1] 退出检索平台\n");
        printf("----------------------------------------\n");
        printf("  请指派数据抽取卷宗号: ");

        int c = safeGetInt();
        if (c == -1) return;

        if (c >= 1 && c <= 4) {
            int printed = 0;
            printf("\n--- 门诊业务类型查询结果反馈 ---\n");
            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0 &&
                    rec->type == c &&
                    rec->isPaid != 4 &&
                    !strstr(rec->description, "[住院记账]")) {
                    printf("  记录流水: %-12s | 快照: %s | 净值: %-6.2f | 详述: %s\n",
                        rec->recordId, rec->createTime, rec->cost, rec->description);
                    printed = 1;
                }
            }
            if (!printed) printf("  [数据空置] 无关联数据。\n");
            system("pause");
        }
        else if (c == 5) {
            int printed = 0;
            system("cls");
            printf("\n========== 住院业务数据聚合 ==========\n");

            Patient* pp = findPatientById(currentPatientId);
            if (pp) {
                printf("  当前住院状态: %s\n", pp->isInpatient ? "住院中" : "非住院");
                printf("  当前住院押金余额: %.2f\n", pp->inpatientDeposit);
                if (pp->inpatientDeposit < 0) {
                    printf("  当前待补交金额: %.2f\n", -pp->inpatientDeposit);
                }
            }

            printf("  %-20s | %-10s | %-10s | %-10s | %s\n", "时间", "类型", "金额", "状态", "说明");
            printf("  -----------------------------------------------------------------------------------------\n");

            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) != 0) continue;

                int isInpatientDetail = 0;
                char typeName[20] = "";
                char statusName[20] = "";

                if (rec->type == 3 && strstr(rec->description, "[住院明细][药品]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "药品");
                }
                else if (rec->type == 4 && strstr(rec->description, "[住院明细][检查]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "检查");
                }
                else if (rec->type == 5 && strstr(rec->description, "[住院明细]")) {
                    isInpatientDetail = 1;
                    if (strstr(rec->description, "[床位费]")) strcpy(typeName, "床位费");
                    else if (strstr(rec->description, "[押金]")) strcpy(typeName, "押金");
                    else if (strstr(rec->description, "[补缴]")) strcpy(typeName, "补缴");
                    else if (strstr(rec->description, "[待补交]")) strcpy(typeName, "待补交");
                    else strcpy(typeName, "住院账务");
                }
                else if (rec->type == 8 && strstr(rec->description, "[住院明细][退回]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "退回");
                }

                if (!isInpatientDetail) continue;

                if (rec->isPaid == 0) strcpy(statusName, "待补交");
                else if (rec->isPaid == 1) strcpy(statusName, "已入账");
                else strcpy(statusName, "已完成");

                printf("  %-20s | %-10s | %-10.2f | %-10s | %s\n",
                    rec->createTime, typeName, rec->cost, statusName, rec->description);
                printed = 1;
            }

            if (!printed) printf("  [数据空置] 暂无住院明细。\n");
            printf("  -----------------------------------------------------------------------------------------\n");
            system("pause");
        }
        else if (c == 6) {
            double totalSpent = 0.0, totalRecharged = 0.0, totalRefunded = 0.0;
            int printed = 0;
            char typeName[30];
            printf("\n========== 终身资产流入流出审计报表 ==========\n");
            printf("  %-20s | %-10s | %-10s | %-40s\n", "登记时间", "变动", "金额", "特征");
            printf("  ---------------------------------------------------------------------------------\n");

            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0) {
                    if (rec->type >= 1 && rec->type <= 5 && rec->isPaid > 0 && rec->isPaid != 4) {
                        if (!(rec->type == 3 && strstr(rec->description, "[住院记账]"))) {
                            totalSpent += rec->cost;
                            strcpy(typeName, "耗损(-)");
                            printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                                rec->createTime, typeName, rec->cost, rec->description);
                            printed = 1;
                        }
                    }
                    else if (rec->type == 7) {
                        totalRecharged += rec->cost;
                        strcpy(typeName, "汇入(+)");
                        printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                            rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                    else if (rec->type == 8) {
                        totalRefunded += rec->cost;
                        strcpy(typeName, "清退(+)");
                        printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                            rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                }
            }

            if (!printed) printf("  暂无财务活动数据。\n");
            printf("  ---------------------------------------------------------------------------------\n");
            printf("  【整合】注资: +%.2f | 返还: +%.2f | 核销: -%.2f\n", totalRecharged, totalRefunded, totalSpent);

            Patient* pp = findPatientById(currentPatientId);
            if (pp) printf("  >>> 当前余额: ￥ %.2f <<<\n", pp->balance);

            system("pause");
        }
        else {
            printf("  [!] 无效选项！\n");
            system("pause");
        }
    }
}

void changePatientPassword(const char* currentId) {
    Patient* p = findPatientById(currentId);
    if (!p) return;

    char oldPwd[50] = { 0 }, newPwd[50] = { 0 }, confirmPwd[50] = { 0 };

    printf("\n========== 核心安全凭证更替操作 ==========\n");
    printf("请输入原始密码进行校验 (输入-1取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "-1") == 0) return;

    if (strcmp(p->password, oldPwd) != 0) {
        printf("  [安全阻断] 特征匹配失败！\n");
        system("pause");
        return;
    }

    printf("请输入新密码 (至少6位，仅限数字或字母, 输入-1取消): ");
    safeGetPassword(newPwd, 50);
    if (strcmp(newPwd, "-1") == 0) return;

    printf("重复输入以完成校验 (输入-1取消): ");
    safeGetString(confirmPwd, 50);
    if (strcmp(confirmPwd, "-1") == 0) return;

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [异常] 二次校验不一致，操作取消。\n");
        system("pause");
        return;
    }

    strcpy(p->password, newPwd);
    printf("  [完成] 服务凭证已安全重置！\n");
    system("pause");
}

void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);

        printf("\n======================================================\n");
        printf("   患者专属自助数据终端   [ 登录实体: %s (%s) ]\n", p->name, p->id);
        printf("======================================================\n");
        printf("  [1] 门诊自助挂号系统(在线预约)\n");
        printf("  [2] 财务与费用中心 (账户缴费)\n");
        printf("  [3] 个人医疗档案库 (查询报告)\n");
        printf("  [4] 个人账号安全设置 (密码修改)\n");
        printf(" [-1] 结束当前操作，返回系统主菜单\n");
        printf("------------------------------------------------------\n");
        printf("  请选择: ");

        int option = safeGetInt();
        switch (option) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 4: changePatientPassword(currentId); break;
        case -1: return;
        default:
            printf("  [!] 无效选项！\n");
            system("pause");
            break;
        }
    }
}