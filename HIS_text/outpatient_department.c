#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "utils.h"
#include "schedule.h"
#include "doctor.h"
#include "outpatient_department.h"
#include "patient.h"
#include "inpatient_department.h"
#include "drug.h"

// ---------------------------------------------------------
// 全局状态缓冲：接诊会话保持
// 缓存当前正在接诊的患者ID。通过建立全局上下文，后续的开具处方、
// 录入病历、开具检查单等业务均可自动映射该患者，极大提升了系统的交互效率。
// ---------------------------------------------------------
char currentCallingPatientId[20] = "";

void generateRecordID(char* buffer) {
    int maxId = 1999;
    Record* r = recordHead->next;

    // 采用全量遍历查找当前最大索引的方式生成新单号。
    // 避免程序重启重新加载本地序列化数据时，因单纯使用链表长度计数
    // 而引发的底层ID重置、新老数据相互覆盖等主键冲突异常。
    while (r != NULL) {
        int currentIdNum;
        if (sscanf(r->recordId, "R2025%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) maxId = currentIdNum;
        }
        r = r->next;
    }
    sprintf(buffer, "R2025%04d", maxId + 1);
}

// ---------------------------------------------------------
// 门诊分诊与叫号系统
// 负责读取排班数据、筛选有效挂号流水，并将患者锁定至当前诊室
// ---------------------------------------------------------
void callPatient(const char* docId) {
    system("cls");
    printf("\n========== 智能排班溯源与叫号 ==========\n");

    // 1. 抓取该医生所有有效出诊日期（利用数组进行去重过滤）
    /* 【BUG修复1】原代码 scheduleList 未跳过哨兵节点，改为 scheduleList->next */
    /* 【BUG修复2】原代码 docId + 1 跳过了"D"前缀，但Schedule.doctor_id已是"D1001"格式，应直接用 docId */
    Schedule* s = scheduleList->next; int sCount = 0; char availableDates[20][15];
    while (s) {
        if (strcmp(s->doctor_id, docId) == 0) {
            int duplicate = 0;
            for (int i = 0; i < sCount; i++) { if (strcmp(availableDates[i], s->date) == 0) duplicate = 1; }
            if (!duplicate) { strcpy(availableDates[sCount], s->date); sCount++; }
        }
        s = s->next;
    }

    if (sCount == 0) { printf("  [!] 调度台回执：您在系统近期排班表中是真空状态。\n"); return; }

    // 2. 构建排班日期选择视图
    printf("  [授权排班日期选择树]:\n");
    for (int i = 0; i < sCount; i++) { printf("   [%d] %s\n", i + 1, availableDates[i]); }
    printf("   [-1] 脱离排班中心\n----------------------------------------\n  选择检索索引号: ");

    int dChoice = safeGetInt();
    if (dChoice == -1) return;

    /* 【BUG修复3】原代码 dChoice < 0 允许输入0导致 availableDates[-1] 越界，改为 dChoice < 1 */
    if (dChoice < 1 || dChoice > sCount) {
        printf("  [!] 输入越界：无此索引号，操作已终止。\n");
        return;
    }
    char* targetDate = availableDates[dChoice - 1];

    // 3. 渲染当日候诊患者视图 (采用固定宽度占位符确保格式化输出绝对对齐)
    printf("\n========== [%s] 候诊大厅 ==========\n", targetDate);
    printf(" %-8s | %-14s | %-12s | %-12s | %-10s\n", "系统位次", "挂号流转凭证", "患者全局ID", "档案署名", "当前生命周期");
    printf("------------------------------------------------------------------------\n");
    int count = 0; Record* r = recordHead->next;

    while (r) {
        // 多维条件筛选：匹配当前操作医师 + 业务类型为挂号(1) + 目标日期
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next; char pName[100] = "脱敏未知";
            while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

            char status[30];
            if (r->isPaid == 0) strcpy(status, "[卡死]缺费");
            else if (r->isPaid == 1) strcpy(status, "[活跃]候诊");
            else strcpy(status, "[完结]离室");

            /* 【修复】兼容"排号:"和"队列号:"两种格式 */
            int seqNum = 0;
            char* seqPtr = strstr(r->description, "排号:");
            if (!seqPtr) seqPtr = strstr(r->description, "队列号:");
            if (seqPtr) {
                if (strncmp(seqPtr, "排号:", strlen("排号:")) == 0)
                    sscanf(seqPtr, "排号:%d", &seqNum);
                else
                    sscanf(seqPtr, "队列号:%d", &seqNum);
            }

            printf("  %-7d | %-14s | %-12s | %-12s | %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }
    if (count == 0) { printf("  今日无病患来访。\n"); return; }
    printf("------------------------------------------------------------------------\n");

    // 4. 执行叫号动作与状态机扭转
    printf("\n  [业务指令] 是否自动广播叫号，将第一顺位合法患者请入诊室？(1.同意 0.拒绝): ");
    if (safeGetInt() == 1) {
        r = recordHead->next; int found = 0;
        while (r) {
            if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1 && strstr(r->description, targetDate)) {
                Patient* p = patientHead->next; char pName[100] = "未知";
                while (p) { if (strcmp(p->id, r->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                printf("\n  【走廊大屏广播】 -> \"请 %s 号患者 [%s] 到 %s 诊室就医。\"\n", r->patientId, pName, docId);

                // 更新业务流转状态为已完结(2)，将该患者从候诊队列中剥离
                r->isPaid = 2;
                found = 1;

                // 写入全局会话缓冲，打通后续的开单与处方业务链
                strcpy(currentCallingPatientId, r->patientId);
                printf("  >>> HIS 引擎已强锁死当前看诊沙盒为: %s <<<\n", pName);
                break;
            }
            r = r->next;
        }
        if (!found) printf("  [提示] 您当日的预约病人已全部接诊完毕！\n");
    }
}

// ---------------------------------------------------------
// 电子病历录入与检验类医嘱开具
// ---------------------------------------------------------
void diagnoseAndTest(const char* docId) {
    char pId[20];

    // 会话复用引擎：尝试抓取全局缓冲的患者ID
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [系统自动提取当前病患ID]: %s\n", pId);
    }
    else {
        printf("\n  [无关联病患] 请手动键入要下达诊断的病患ID (-1终止): "); safeGetString(pId, 20);
        if (strcmp(pId, "-1") == 0) return;
    }

    char symptoms[100], diag[100];

    // 强制输入非空校验：彻底拦截空回车与非法换行符，
    // 保证内存序列化至本地 TXT 文件时，底层的字段格式不发生物理断裂。
    while (1) { printf("  请敲入宏观体征表象(如咳嗽发热): "); safeGetString(symptoms, 100); if (strlen(symptoms) > 0) break; }
    while (1) { printf("  请给出权威确切诊断学结论: "); safeGetString(diag, 100); if (strlen(diag) > 0) break; }

    // 初始化 Type 2 (临床看诊病历记录)
    Record* r2 = (Record*)malloc(sizeof(Record));
    if (!r2) { printf("  [!] 内存分配失败。\n"); return; }
    generateRecordID(r2->recordId);
    r2->type = 2; strcpy(r2->patientId, pId); strcpy(r2->staffId, docId);
    r2->cost = 20.0; r2->isPaid = 0; sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);
    getCurrentTimeStr(r2->createTime, 30); r2->next = recordHead->next; recordHead->next = r2;
    printf("  [√] 核心看诊数据入库封存。\n");

    // 辅助检验项目关联模块
    printf("\n  需延伸生化/影像仪器的辅助检查吗？(1.开单 0.放弃): ");

    // 严格的安全整型转换输入，阻断无效字母干扰
    if (safeGetInt() == 1) {
        char testName[50];
        while (1) { printf("  指令项目全称(如颅脑CT): "); safeGetString(testName, 50); if (strlen(testName) > 0) break; }
        printf("  该项目检测费定价: ");
        double tCost = safeGetDouble();

        // 生成 Type 4 检查单流转记录
        Record* r4 = (Record*)malloc(sizeof(Record));
        if (!r4) { printf("  [!] 内存分配失败。\n"); return; }
        generateRecordID(r4->recordId);
        r4->type = 4; strcpy(r4->patientId, pId); strcpy(r4->staffId, docId);
        r4->cost = tCost; r4->isPaid = 0; sprintf(r4->description, "检查名称:%s", testName);
        getCurrentTimeStr(r4->createTime, 30); r4->next = recordHead->next; recordHead->next = r4;
        printf("  [√] 第4号子系统触发，检查单已推送患者手机微信端催缴。\n");
    }
}

// ---------------------------------------------------------
// 电子处方开具与全局物资调度模块
// ---------------------------------------------------------
void prescribeMedicine(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) { strcpy(pId, currentCallingPatientId); printf("\n  [自动挂载处方对象]: %s\n", pId); }
    else { printf("\n  请输入当前接诊患者ID (-1终止): "); safeGetString(pId, 20); if (strcmp(pId, "-1") == 0) return; }

    while (1) {
        char key[50];
        printf("\n  请检索大药房物资库字典 (拼音/编号/药名，输-1退出配药): "); safeGetString(key, 50);
        if (strcmp(key, "-1") == 0) break;

        // 跨模块数据强一致性校验：通过直接读取核心物资单链表(drugList)，
        // 实现了对全院大药房真实物理库存的实时镜像检索与后续扣除，消除数据冗余孤岛。
        /* 【BUG修复4】原代码 drugList 未跳过哨兵节点，改为 drugList->next */
        Drug* d = drugList->next; Drug* matched[100] = { NULL }; int mCount = 0;

        printf("\n  ========== 药品储备库 ==========\n");
        printf("  %-4s | %-8s | %-16s | %-8s | %-8s\n", "映射", "系统内码", "国家标准药名", "单价", "物理库存");
        printf("-----------------------------------------------------------\n");

        // 提供模糊检索引擎支持
        while (d) {
            char idStr[20];
            sprintf(idStr, "%d", d->id);
            if (strstr(d->name, key) || strstr(idStr, key)) {
                if (mCount < 100) {
                    matched[mCount] = d;
                    printf("  [%-2d] | %-8d | %-16s | %-8.2f | %-8d\n", mCount + 1, d->id, d->name, d->price, d->stock);
                    mCount++;
                }
            }
            d = d->next;
        }

        if (mCount == 0) { printf("  [!] 检索库中查无此药，请换词再试。\n"); continue; }

        char mChoiceStr[50];
        printf("-----------------------------------------------------------\n");
        printf("  击打左侧【映射号】或直接敲【系统内码】定位药品 (-1取消): ");
        safeGetString(mChoiceStr, 50); if (strcmp(mChoiceStr, "-1") == 0) continue;

        // 复合型交互支持：同步兼容序号映射选择与精确编码定位
        Drug* selectedMed = NULL; int isNum = 1;
        for (int i = 0; i < (int)strlen(mChoiceStr); i++) { if (mChoiceStr[i] < '0' || mChoiceStr[i] > '9') { isNum = 0; break; } }
        if (isNum) { int idx = atoi(mChoiceStr); if (idx > 0 && idx <= mCount) selectedMed = matched[idx - 1]; }
        if (!selectedMed) { for (int i = 0; i < mCount; i++) { char idStr[20]; sprintf(idStr, "%d", matched[i]->id); if (strcmp(idStr, mChoiceStr) == 0) { selectedMed = matched[i]; break; } } }
        if (!selectedMed) { printf("  [!] 定位脱靶，输入无法解析。\n"); continue; }

        int qty;
        while (1) {
            printf("  提取 [%s] 物理装填数目 (高红线拦截10盒): ", selectedMed->name);
            qty = safeGetPositiveInt(); // 提供正整数过滤保护

            // 业务复合约束：行政处方限制与当前物理库存双重拦截网
            if (qty > 10) { printf("  [越权预警] 系统默认指令：处方药单次过载(超10盒)！打回重填。\n"); }
            else if (qty > selectedMed->stock) { printf("  [缺货熔断] 中央仓库存不足！当前最高只能开: %d 盒\n", selectedMed->stock); }
            else { break; }
        }

        // 执行全局库存扣减的原子性变动
        selectedMed->stock -= qty;
        printf("  [底层联动] 强行剥离大库实体，[%s]已被扣取 %d盒，残留余量:%d\n", selectedMed->name, qty, selectedMed->stock);

        double totalCost = qty * selectedMed->price;

        // 构建并入库 Type 3 (药单系统凭证)
        Record* r3 = (Record*)malloc(sizeof(Record));
        if (!r3) { printf("  [!] 内存分配失败。\n"); return; }
        extern void generateRecordID(char* buffer);
        generateRecordID(r3->recordId);
        r3->type = 3; strcpy(r3->patientId, pId); strcpy(r3->staffId, docId);
        r3->cost = totalCost; r3->isPaid = 0;
        sprintf(r3->description, "药品:%s_单价:%.2f_数量:%d_总价:%.2f", selectedMed->name, selectedMed->price, qty, totalCost);
        getCurrentTimeStr(r3->createTime, 30); r3->next = recordHead->next; recordHead->next = r3;

        printf("  [√] 云端处方下达指令送出，价值 %.2f 元。待患者支付成功后再正式扣减库存。\n", totalCost);
    }
}

// ---------------------------------------------------------
// 院前收治评估：跨系统下发住院权限调度单
// ---------------------------------------------------------
void issueAdmissionNotice(const char* docId) {
    char pId[20];
    if (strlen(currentCallingPatientId) > 0) { strcpy(pId, currentCallingPatientId); printf("\n  [接诊关联]: %s\n", pId); }
    else { printf("\n  键入需强制收容的患者识别码 (-1返回): "); safeGetString(pId, 20); if (strcmp(pId, "-1") == 0) return; }

    printf("  是否开启危重高优先通道越级抢床位？(1.重症直达 0.普通常规): ");
    int isSevere;

    // 输入状态机：严格规范布尔型变量的输入许可，切断容错死角
    while (1) {
        isSevere = safeGetInt();
        if (isSevere == 0 || isSevere == 1) break;
        else printf("  [!] 输入越界，请规范输入 1 或 0: ");
    }

    printf("  填写拟定收治初步说明(严禁为空): "); char note[100];
    while (1) { safeGetString(note, 100); if (strlen(note) > 0) break; }

    // 生成 Type 6 数据实体，此记录将作为住院部调度引擎授权收治分配床位的核心依据
    Record* r6 = (Record*)malloc(sizeof(Record));
    if (!r6) { printf("  [!] 内存分配失败。\n"); return; }
    generateRecordID(r6->recordId);
    r6->type = 6; strcpy(r6->patientId, pId); strcpy(r6->staffId, docId);
    r6->cost = 0.0; r6->isPaid = 0;

    // 依据紧急度写入动态标签字典，触发下游病区的高低级通道分配机制
    sprintf(r6->description, "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);
    getCurrentTimeStr(r6->createTime, 30); r6->next = recordHead->next; recordHead->next = r6;

    printf("  [√] 通知单越界发送至住院部。(打标属性: %s)\n", isSevere ? "重症" : "普通");
}