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

// ---------------------------------------------------------
// 内部工具：自动生成唯一的患者流水ID
// 为避免中间删除记录或程序重启后，直接利用链表长度+1导致新老ID冲突覆盖，
// 此处采用完整遍历链表寻找当前最大ID值，并在其基础上+1的方式生成唯一ID。
// ---------------------------------------------------------
void generatePatientID(char* idBuffer) {
    int maxId = 999;
    Patient* p = patientHead->next;
    while (p != NULL) {
        int currentIdNum;
        if (sscanf(p->id, "P2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) {
                maxId = currentIdNum;
            }
        }
        p = p->next;
    }
    sprintf(idBuffer, "P2025%04d", maxId + 1);
}

// 内部工具：通过ID检索患者节点
Patient* findPatientById(const char* pid) {
    Patient* p = patientHead->next;
    while (p != NULL) { if (strcmp(p->id, pid) == 0) return p; p = p->next; }
    return NULL;
}

// ---------------------------------------------------------
// 业务一：首次建档注册
// ---------------------------------------------------------
void registerPatient() {
    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    newPatient->next = NULL;

    printf("\n========== 账户注册与建档 ==========\n");
    printf("请选择就诊类型 (1.普通门诊 2.急诊绿色通道 0.取消返回): ");
    int type;

    // 输入校验机制：循环拦截非 1/2/0 的数字或非法字符输入
    while (1) {
        type = safeGetInt();
        if (type == 0) { free(newPatient); return; }
        if (type == 1 || type == 2) break;
        printf("  [!] 输入有误：只能输入 1 或 2，请重新选择: ");
    }
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    // 非空输入校验：强制要求输入有效字符，防止用户敲击空回车导致存入TXT时字段断裂
    while (1) { printf("请输入真实姓名: "); safeGetString(newPatient->name, 100); if (strlen(newPatient->name) > 0) break; }
    while (1) { printf("请设置登录密码: "); safeGetString(newPatient->password, 50); if (strlen(newPatient->password) > 0) break; }

    printf("请输入生理性别 (男性/女性): ");
    safeGetGender(newPatient->gender, 10); // 调用专用的性别格式校验函数

    if (!newPatient->isEmergency) {
        printf("请输入周岁年龄: ");
        newPatient->age = safeGetPositiveInt(); // 年龄必须为大于0的正整数
        printf("请输入过敏史(无则填无): ");
        safeGetString(newPatient->allergy, 100);
        if (strlen(newPatient->allergy) == 0) strcpy(newPatient->allergy, "无"); // 提供默认的兜底值
    }
    else {
        newPatient->age = -1; strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id); newPatient->balance = 0.0;

    // 采用标准链表尾插法，建档成功后立刻将新患者节点挂载到全局患者链表尾部
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("\n  [√] 系统建档圆满成功！\n");
    printf("  ==========================================\n");
    printf("  您的终身就诊识别码为: 【 %s 】 (凭此号登录)\n", newPatient->id);
    printf("  ==========================================\n");
}

// ---------------------------------------------------------
// 业务二：自助挂号 (含恶意刷号拦截与智能推荐)
// ---------------------------------------------------------
void bookAppointment(const char* currentPatientId) {
    char today[11], nextWeek[11];
    time_t t = time(NULL); struct tm* tm_info = localtime(&t);
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);
    t += 7 * 24 * 60 * 60; tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        system("cls");
        printf("\n========== 自助预约门诊挂号 ==========\n");
        printf("  [1] 按【科室名称】搜寻未来一周排班\n");
        printf("  [2] 按【医生姓名/工号】搜寻排班\n");
        printf("  [0] 放弃挂号，返回上级菜单\n");
        printf("--------------------------------------\n");
        printf("  请选择搜寻引擎: ");

        int choice = safeGetInt(); // 安全读取输入选项
        if (choice == 0) return;

        char keyword[50];
        if (choice == 1) {
            // 动态抓取当前有医生出诊的活跃科室列表，提高交互体验
            char depts[20][50]; int dCount = 0; Staff* stf = staffHead->next;
            while (stf) {
                int exists = 0; for (int i = 0; i < dCount; i++) { if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; } }
                if (!exists && strlen(stf->department) > 0) { strcpy(depts[dCount], stf->department); dCount++; }
                stf = stf->next;
            }
            printf("\n  [系统当前已开设的门诊科室]: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);
            printf("\n  请输入您要挂号的目标科室名称: ");
            safeGetString(keyword, 50);
        }
        else if (choice == 2) {
            printf("  请输入医生精确姓名或纯数字工号 (例:李四 / 1001): ");
            safeGetString(keyword, 50);
        }
        else continue;

        // 利用 %-xs 等占位符实现动态长度自适应，确保表格列严格左侧对齐
        printf("\n========== 未来一周可预约排班总表 (%s 至 %s) ==========\n", today, nextWeek);
        printf(" %-8s | %-12s | %-8s | %-16s | %-10s | %-10s\n", "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
        printf("----------------------------------------------------------------------------------\n");

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
                char docDisp[50]; sprintf(docDisp, "%s(%d)", matchedDoc->name, matchedDoc->id);
                printf(" [%-6d] | %-12s | %-8s | %-16s | %-10s | %-10s\n", s->schedule_id, s->date, s->shift, docDisp, matchedDoc->department, matchedDoc->title);
                found++;
            }
            s = s->next;
        }

        if (found == 0) { printf("\n  [!] 很抱歉，未搜索到满足条件的排班源。\n"); system("pause"); continue; }

        printf("----------------------------------------------------------------------------------\n");
        printf("  请输入要锁定的【排班ID】 (输入0重新搜索): ");
        int targetSchId = safeGetInt(); if (targetSchId == 0) continue;

        Schedule* targetSch = NULL; s = scheduleList;
        while (s) { if (s->schedule_id == targetSchId) { targetSch = s; break; } s = s->next; }
        if (!targetSch) { printf("  [!] 输入越界：无效的排班ID！\n"); system("pause"); continue; }

        Doctor* targetDoc = NULL; Doctor* d = doctorList;
        while (d) { if (d->id == targetSch->doctor_id) { targetDoc = d; break; } d = d->next; }
        if (!targetDoc) { printf("  [!] 数据异常：排班表引用的医生档案丢失！\n"); system("pause"); continue; }

        char staffIdStr[20]; sprintf(staffIdStr, "D%d", targetDoc->id);

        // --- 核心业务规则：防刷号与资源限制机制 ---
        // 贴近真实业务场景设定规则：限制单日医院总号源、限制个人单日挂号上限、限制同科室重复挂号
        int patientDailyActive = 0, patientDeptDailyActive = 0, sameDocSameDay = 0, docDailyCount = 0, hospitalDailyCount = 0;
        Record* rec = recordHead->next;
        while (rec) {
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++; // 统计全院当日挂号总数
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++; // 统计单名医生当日挂号数

                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;
                    Doctor* recDoc = doctorList;
                    while (recDoc) {
                        char tempDId[20]; sprintf(tempDId, "D%d", recDoc->id);
                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1; break;
                        }
                        recDoc = recDoc->next;
                    }
                }
            }
            rec = rec->next;
        }

        if (hospitalDailyCount >= 1000) { printf("\n  [拦截] 全院挂号总系统熔断：单日1000号上限已满！\n"); system("pause"); continue; }
        if (patientDailyActive >= 5) { printf("\n  [拦截] 个人防刷限制：您单日最多只允许挂5个号！\n"); system("pause"); continue; }
        if (patientDeptDailyActive >= 1) { printf("\n  [拦截] 同科室互斥：您同日同科室已被限制只能挂一个！\n"); system("pause"); continue; }
        if (sameDocSameDay) { printf("\n  [拦截] 您已成功挂过该医生该日的号，切勿重复挤占医疗资源！\n"); system("pause"); continue; }

        if (docDailyCount >= 50) {
            printf("\n  [号源已空] %s 医生在 %s 的 50 个号源已被抢光！\n", targetDoc->name, targetSch->date);
            printf("  >>> 智能匹配系统为您推荐以下备选方案 <<<\n");
            int recCount = 0; Schedule* altS = scheduleList;

            // 智能推荐一：检索该医师的其他出诊时间
            printf("\n  [方案一：该医师的其他出诊日]\n");
            while (altS) {
                if (altS->doctor_id == targetDoc->id && strcmp(altS->date, targetSch->date) != 0 && strcmp(altS->shift, "休息") != 0 && strcmp(altS->date, today) >= 0 && strcmp(altS->date, nextWeek) <= 0) {
                    printf("    -> 排班ID [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift); recCount++;
                }
                altS = altS->next;
            }

            // 智能推荐二：检索当日同科室的其他大夫
            printf("\n  [方案二：[%s] 同日出诊的其他同事]\n", targetDoc->department);
            altS = scheduleList;
            while (altS) {
                if (strcmp(altS->date, targetSch->date) == 0 && altS->doctor_id != targetDoc->id && strcmp(altS->shift, "休息") != 0) {
                    Doctor* altD = doctorList; while (altD) { if (altD->id == altS->doctor_id && strcmp(altD->department, targetDoc->department) == 0) { printf("    -> 排班ID [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift); recCount++; break; } altD = altD->next; }
                }
                altS = altS->next;
            }
            if (recCount == 0) printf("  系统暂无其他可推荐排班，请改日再试。\n");
            system("pause"); continue;
        }

        // 挂号生成：根据医生职称动态计算挂号费
        int seqNum = docDailyCount + 1;
        double regFee = strstr(targetDoc->title, "主任") != NULL ? 50.0 : 15.0;

        Record* newRecord = (Record*)malloc(sizeof(Record));
        int maxRegId = 4999; Record* r_temp = recordHead->next;
        while (r_temp) { int curReg; if (sscanf(r_temp->recordId, "REG2025%04d", &curReg) == 1) { if (curReg > maxRegId) maxRegId = curReg; } r_temp = r_temp->next; }

        sprintf(newRecord->recordId, "REG2025%04d", maxRegId + 1);
        newRecord->type = 1; strcpy(newRecord->patientId, currentPatientId); strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee; newRecord->isPaid = 0; // 0代表待缴费状态
        sprintf(newRecord->description, "挂号:%s(%s)_排号:%d", targetDoc->name, targetSch->date, seqNum);
        getCurrentTimeStr(newRecord->createTime, 30); newRecord->next = NULL;

        Record* temp = recordHead; while (temp->next) temp = temp->next; temp->next = newRecord;

        printf("\n  [√ 抢号成功] 您已锁定 %s 医师 的号源！待缴费用 %.2f 元。\n", targetDoc->name, regFee);
        printf("  =======================================================\n");
        printf("  >>> 您在当天的专属候诊顺序为：【 第 %d 号 】 <<<\n", seqNum);
        printf("  =======================================================\n");
        printf("  (注：请立即前往[财务中心]缴费，未交钱的挂号单将无法被医生接诊)\n");
        system("pause"); return;
    }
}

// ---------------------------------------------------------
// 内部联动模块：监听缴费行为并触发住院部自动出院流转
// ---------------------------------------------------------
void checkAndAutoDischarge(Record* rec, const char* currentPatientId) {
    if (rec->type == 5 && strstr(rec->description, "出院清算_补缴欠费差额")) {
        char bedId[20] = { 0 }; double bFee = 0, dFee = 0;

        // 利用 sscanf 的正则提取特性，解析隐藏在账单描述中的床位号和金额参数，实现系统间的无缝数据传递
        if (sscanf(rec->description, "出院清算_补缴欠费差额_床位:%19[^_]_床费:%lf_药费:%lf", bedId, &bFee, &dFee) == 3) {

            // 1. 跨模块释放病床资源
            extern Bed* bedHead; Bed* b = bedHead->next;
            while (b) { if (strcmp(b->bedId, bedId) == 0) { b->isOccupied = 0; strcpy(b->patientId, ""); break; } b = b->next; }

            // 2. 将住院期间后台记账的各类处方单统一标记为已付款(1)
            Record* rr = recordHead->next;
            while (rr) { if (rr->type == 3 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 4) rr->isPaid = 1; rr = rr->next; }

            // 3. 将初始的住院押金单更新为包含明细的终极出院结算单(2)
            rr = recordHead->next;
            while (rr) {
                if (rr->type == 5 && strcmp(rr->patientId, currentPatientId) == 0 && rr->isPaid == 1) {
                    rr->isPaid = 2; char summary[200];
                    sprintf(summary, " [出院结算:床费%.2f 药费%.2f 总消费%.2f]", bFee, dFee, bFee + dFee);
                    strcat(rr->description, summary);
                }
                rr = rr->next;
            }

            // 净化当前这张补款单的描述文本，提升界面显示美观度
            strcpy(rec->description, "出院清算尾款结清");
            printf("\n  [系统自动流转] 检测到住院尾款结清！\n  系统已全自动为您解除病床占用，最终出院结算单已生成，请前往档案库提取。\n");
        }
    }
}

// ---------------------------------------------------------
// 业务三：财务结算中心
// ---------------------------------------------------------
void financeCenter(const char* currentPatientId) {
    while (1) {
        system("cls"); Patient* p = findPatientById(currentPatientId); if (!p) return;
        printf("\n========== 个人财务结算中心 ==========\n");
        printf("  [当前账户可用余额]:  %.2f 元\n", p->balance);
        printf("--------------------------------------\n");
        printf("  [1] 在线网银充值 (余额充值)\n  [2] 待处理账单清算 (一键缴费)\n  [0] 返回控制大厅\n");
        printf("  业务指令: ");
        int choice = safeGetInt(); if (choice == 0) return;

        if (choice == 1) {
            printf("\n  请输入需充值的金额 (￥): ");
            double money = safeGetDouble(); // 安全读取浮点数，拦截非法字符
            if (money > 0) {
                p->balance += money;
                Record* r7 = (Record*)malloc(sizeof(Record)); extern void generateRecordID(char* buffer);
                generateRecordID(r7->recordId); r7->type = 7; strcpy(r7->patientId, currentPatientId); strcpy(r7->staffId, "SYS");
                r7->cost = money; r7->isPaid = 1; sprintf(r7->description, "终端自助充值入账");
                getCurrentTimeStr(r7->createTime, 30); r7->next = recordHead->next; recordHead->next = r7;
                printf("  [√] 充值通道顺畅，%.2f 元已实时到账！\n", money); system("pause");
            }
            else { printf("  [!] 充值金额必须大于0！\n"); system("pause"); }
        }
        else if (choice == 2) {
            while (1) {
                system("cls"); printf("\n========== 待结算财务账单 ==========\n");
                Record* rec = recordHead->next; int hasUnpaid = 0; double totalUnpaidCost = 0.0;
                while (rec) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                        char typeName[20];
                        switch (rec->type) { case 1:strcpy(typeName, "挂号费"); break; case 2:strcpy(typeName, "诊疗费"); break; case 3:strcpy(typeName, "药房配药"); break; case 4:strcpy(typeName, "辅助检查"); break; case 5:strcpy(typeName, "住院押金/尾款"); break; }
                                                   printf("  单号: %-12s | 类别: [%-10s] | 金额: %-8.2f | 摘要: %s\n", rec->recordId, typeName, rec->cost, rec->description);
                                                   totalUnpaidCost += rec->cost; hasUnpaid = 1;
                    }
                    rec = rec->next;
                }
                if (!hasUnpaid) { printf("  您目前没有任何处于欠费状态的账单！\n"); system("pause"); break; }

                printf("----------------------------------------------------------------------\n");
                printf("  [系统汇总] 待清算总额: %.2f 元  |  可用余额: %.2f 元\n\n", totalUnpaidCost, p->balance);
                printf("  1. ⚡ 开启一键全选缴费 (统账核销)\n  2. 🔍 输入独立单号逐个清算\n  0. 退出结算\n  请选择: ");
                int payChoice = safeGetInt(); if (payChoice == 0) break;

                if (payChoice == 1) {
                    if (p->balance < totalUnpaidCost) { printf("  [!] 余额不足，请先返回充值！(当前资金缺口: %.2f 元)\n", totalUnpaidCost - p->balance); system("pause"); }
                    else {
                        rec = recordHead->next;
                        while (rec) {
                            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                                p->balance -= rec->cost; rec->isPaid = 1; // 扣减余额并扭转缴费状态

                                // 同步机制：向管理端插入一笔财务交易流水
                                Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction)); int maxId = 0; Transaction* curr = transactionList; while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                                newTrans->id = maxId + 1;
                                if (rec->type == 3) newTrans->type = 3; else if (rec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                                newTrans->amount = rec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, rec->description); newTrans->next = NULL;
                                if (!transactionList) transactionList = newTrans; else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }

                                // 针对每笔已缴单据调用状态检查，触发可能的出院业务流转
                                checkAndAutoDischarge(rec, currentPatientId);
                            }
                            rec = rec->next;
                        }
                        printf("  [√] 账务总清算成功执行！\n"); system("pause"); break;
                    }
                }
                else if (payChoice == 2) {
                    printf("\n  请输入需独立清算的具体单号: "); char target[30]; safeGetString(target, 30); if (strcmp(target, "0") == 0) continue;
                    Record* tRec = NULL; rec = recordHead->next;
                    while (rec) { if (strcmp(rec->recordId, target) == 0 && rec->isPaid == 0) { tRec = rec; break; } rec = rec->next; }
                    if (!tRec) { printf("  [!] 单号输入错误或该单据已结清。\n"); system("pause"); continue; }

                    if (p->balance < tRec->cost) { printf("  [!] 余额告急，无法覆盖本单据支出！\n"); system("pause"); }
                    else {
                        p->balance -= tRec->cost; tRec->isPaid = 1;
                        Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction)); int maxId = 0; Transaction* curr = transactionList; while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                        newTrans->id = maxId + 1;
                        if (tRec->type == 3) newTrans->type = 3; else if (tRec->type == 5) newTrans->type = 2; else newTrans->type = 1;
                        newTrans->amount = tRec->cost; getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, tRec->description); newTrans->next = NULL;
                        if (!transactionList) transactionList = newTrans; else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }

                        printf("  [√] %s 单据核销成功！实扣款 %.2f 元。\n", tRec->recordId, tRec->cost);
                        checkAndAutoDischarge(tRec, currentPatientId);
                        system("pause");
                    }
                }
            }
        }
    }
}

// ---------------------------------------------------------
// 业务四：个人医疗时序档案库 
// ---------------------------------------------------------
void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n========== 个人医疗时序档案库 ==========\n");
        printf("  [1] ① 挂号与分诊排队记录\n");
        printf("  [2] ② 临床看诊与医嘱记录\n");
        printf("  [3] ③ 门诊电子处方与拿药单\n");
        printf("  [4] ④ 影像与生化辅助检查单\n");
        printf("  [5] ⑤ 住院全周期溯源与总账单\n");
        printf("  [6] 个人资金流出入全景报表\n");
        printf("  [0] 关闭档案库\n");
        printf("----------------------------------------\n");
        printf("  请调取卷宗编号: ");
        int c = safeGetInt();
        if (c == 0) return;

        if (c >= 1 && c <= 4) {
            printf("\n--- 门诊单据查询回执 ---\n");
            Record* rec = recordHead->next; int printed = 0;
            while (rec) {
                // 进行数据隔离展示：过滤并隐藏带有 [住院记账] 标签的内部药单，避免与门诊历史记录混淆
                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == c && rec->isPaid != 4 && !strstr(rec->description, "[住院记账]")) {
                    printf("  流水单: %-12s | 时间: %s | 花费: %-6.2f | 细节: %s\n", rec->recordId, rec->createTime, rec->cost, rec->description);
                    printed = 1;
                }
                rec = rec->next;
            }
            if (!printed) printf("  [空] 系统底层未检索到属于该分类的门诊履历。\n");
            system("pause");
        }
        else if (c == 5) {
            // 住院全生命周期与流水账单下钻查询模块
            while (1) {
                system("cls");
                printf("\n========== ⑤ 住院追溯与最终结算账单 ==========\n");
                Record* rec = recordHead->next; int count = 0;
                printf("  %-15s | %-10s | %-16s | %-20s\n", "系统流转单号", "统筹发生额", "实时状态", "发起时间");
                printf("--------------------------------------------------------------------\n");
                while (rec) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->type == 5) {
                        char statusStr[30];
                        if (rec->isPaid == 0) strcpy(statusStr, "[待缴尾款出院]");
                        else if (rec->isPaid == 1) strcpy(statusStr, "[在院(押金担保)]");
                        else strcpy(statusStr, "[已闭环结算出院]");

                        printf("  %-15s | %-10.2f | %-16s | %-20s\n", rec->recordId, rec->cost, statusStr, rec->createTime);
                        count++;
                    }
                    rec = rec->next;
                }
                if (count == 0) { printf("  当前暂无任何病房入住与登记记录。\n"); system("pause"); break; }

                printf("--------------------------------------------------------------------\n");
                printf("  [数据提取] 请输入您要查看详细清单的【系统流转单号】 (输入0返回): ");
                char target[30]; safeGetString(target, 30);
                if (strcmp(target, "0") == 0) break;

                Record* tRec = NULL; rec = recordHead->next;
                while (rec) { if (strcmp(rec->recordId, target) == 0 && rec->type == 5 && strcmp(rec->patientId, currentPatientId) == 0) { tRec = rec; break; } rec = rec->next; }
                if (!tRec) { printf("  [!] 输入越界，流转单号无法解析！\n"); system("pause"); continue; }

                system("cls");
                printf("\n========== 深度住院详情溯源 (%s) ==========\n", tRec->recordId);
                printf("  【时效基础】\n  发起快照: %s\n  摘要信息: %s\n", tRec->createTime, tRec->description);

                if (tRec->isPaid == 2 && strstr(tRec->description, "出院结算")) {
                    printf("\n  【资金多退少补盘点】\n");
                    printf("   + 原始冻结押金: %.2f 元\n", tRec->cost);
                    Record* r_link = recordHead->next;
                    while (r_link) {
                        if (strcmp(r_link->patientId, currentPatientId) == 0) {
                            if (r_link->type == 8 && strstr(r_link->description, "出院清算_押金结余退回")) printf("   - 结余资金核销: %.2f 元 (已退回至个人账户余额, 关联单号:%s)\n", r_link->cost, r_link->recordId);
                            else if (r_link->type == 5 && strstr(r_link->description, "出院清算尾款结清")) printf("   + 差额追缴补清: %.2f 元 (财务端已核销, 关联单号:%s)\n", r_link->cost, r_link->recordId);
                        }
                        r_link = r_link->next;
                    }

                    printf("\n  【住院期间病床用药明细追溯】\n");
                    Record* r_drug = recordHead->next; int hasDrug = 0;
                    while (r_drug) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && strstr(r_drug->description, "[住院记账]")) {
                            printf("   ├─ %s | %s\n", r_drug->recordId, r_drug->description); hasDrug = 1;
                        }
                        r_drug = r_drug->next;
                    }
                    if (!hasDrug) printf("   └─ (住院期间未产生任何药品与耗材调拨数据)\n");
                }
                else if (tRec->isPaid == 1 && !strstr(tRec->description, "出院清算")) {
                    printf("\n  【状态追踪】病患正在积极康复住院中...\n");
                    printf("   + 已上缴统筹押金: %.2f 元\n", tRec->cost);
                    printf("\n  【未出账的内部挂账药单】\n");
                    Record* r_drug = recordHead->next; int hasDrug = 0;
                    while (r_drug) {
                        if (strcmp(r_drug->patientId, currentPatientId) == 0 && r_drug->type == 3 && r_drug->isPaid == 4 && strstr(r_drug->description, "[住院记账]")) {
                            printf("   ├─ %s | %s\n", r_drug->recordId, r_drug->description); hasDrug = 1;
                        }
                        r_drug = r_drug->next;
                    }
                    if (!hasDrug) printf("   └─ (暂未生成挂账的药事明细)\n");
                }
                else if (tRec->isPaid == 0) {
                    printf("\n  【流程锁死】由于押金穿透产生尾款，系统已切断出院流转链路，请移步收银中心进行尾款补缴！\n");
                }
                printf("\n-------------------------------------------------------------------\n");
                system("pause");
            }
        }
        else if (c == 6) {
            printf("\n========== 个人终身资金流动审计报表 ==========\n");
            Record* rec = recordHead->next; double totalSpent = 0, totalRecharged = 0, totalRefunded = 0; int printed = 0;

            // 采用标准对齐输出格式
            printf(" %-20s | %-10s | %-10s | %-40s\n", "交易快照时间", "财务方向", "变动金额", "核心摘要");
            printf("------------------------------------------------------------------------------------------\n");

            while (rec) {
                if (strcmp(rec->patientId, currentPatientId) == 0) {
                    char typeName[30];
                    if (rec->type >= 1 && rec->type <= 5 && rec->isPaid > 0 && rec->isPaid != 4) {
                        // 为避免双重做账（Double Counting），住院期间后台记账的药费
                        // 已在Type 5的押金结算单中统一折算，此处不再单独打印此项支出，确保前端账目清晰一致。
                        if (!(rec->type == 3 && strstr(rec->description, "[住院记账]"))) {
                            totalSpent += rec->cost; strcpy(typeName, "扣减 (-)");
                            printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                        }
                    }
                    else if (rec->type == 7) {
                        totalRecharged += rec->cost; strcpy(typeName, "注资 (+)");
                        printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                    }
                    else if (rec->type == 8) {
                        totalRefunded += rec->cost; strcpy(typeName, "冲退 (+)");
                        printf(" %-20s | %-10s | %-10.2f | %-40s\n", rec->createTime, typeName, rec->cost, rec->description); printed = 1;
                    }
                }
                rec = rec->next;
            }
            if (!printed) printf("  数据盘为空，当前账户尚未发生任何资产变动。\n");
            printf("------------------------------------------------------------------------------------------\n");
            printf("  【宏观收支折算】总充值金额: +%.2f | 结算余款冲退: +%.2f | 实际核销开销: -%.2f\n", totalRecharged, totalRefunded, totalSpent);

            Patient* p = findPatientById(currentPatientId);
            if (p) printf("  >>> [当前资金池可用结余]: ￥ %.2f <<<\n", p->balance);
            printf("\n  (注：住院产生的所有药事与床位开销已被系统打包映射至【⑤住院溯源总账】内进行结算统筹。)\n");
            system("pause");
        }
    }
}

// ---------------------------------------------------------
// 业务五：账号安全管理
// ---------------------------------------------------------
void changePatientPassword(const char* currentId) {
    Patient* p = findPatientById(currentId);
    if (!p) return;
    char oldPwd[50], newPwd[50], confirmPwd[50];
    printf("\n========== 账户安全密钥变更 ==========\n");
    printf("请输入原始登录密码 (输入0取消): "); safeGetString(oldPwd, 50); if (strcmp(oldPwd, "0") == 0) return;

    if (strcmp(p->password, oldPwd) != 0) { printf("  [!] 安全防火墙拦截：密码校验失败，无权操作！\n"); system("pause"); return; }

    while (1) { printf("请输入新的安全密码: "); safeGetString(newPwd, 50); if (strlen(newPwd) > 0) break; }
    printf("请再次输入以确认密码: "); safeGetString(confirmPwd, 50);

    if (strcmp(newPwd, confirmPwd) != 0) { printf("  [!] 校验不符：两次输入结果不一致，拒绝修改请求。\n"); system("pause"); return; }

    strcpy(p->password, newPwd);
    printf("  [√] 数据更新成功，您的登录凭证已完成安全覆写！\n"); system("pause");
}

// ---------------------------------------------------------
// 患者端终端主视图入口
// ---------------------------------------------------------
void userTerminal(const char* currentId) {
    while (1) {
        system("cls"); Patient* p = findPatientById(currentId);
        printf("\n======================================================\n");
        printf("   患者专属自助数据终端   [ 当前登录: %s (%s) ]\n", p->name, p->id);
        printf("======================================================\n");
        printf("  [1] 门诊分诊号源预约 (自助抢号)\n");
        printf("  [2] 全局财务费用中心 (充值/交费)\n");
        printf("  [3] 个人终身医疗档案库 (追溯化验/处方/住院数据)\n");
        printf("  [4] 账户级别安全防控 (修改密码)\n");
        printf("  [0] 注销会话并退出终端\n");
        printf("------------------------------------------------------\n");
        printf("  请选择所需服务编号: ");

        switch (safeGetInt()) { // 顶层输入接管，防止输入非法字符导致控制台死循环
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 4: changePatientPassword(currentId); break;
        case 0: return;
        }
    }
}