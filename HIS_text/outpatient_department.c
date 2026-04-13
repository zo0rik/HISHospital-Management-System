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

// 来自 patient.c 的统一公共函数
extern int inputExistingPatientIdCommon(char* pId, size_t size, const char* prompt);

// 当前被医生“叫到诊室”的患者 ID。
// 门诊看诊、开药、开检查时，如果这里有值，就默认对这个患者操作。
// 这样医生在同一次接诊流程里，就不用反复手动输入患者 ID。
char currentCallingPatientId[20] = "";

// 预设的 15 个检查部位名称。
// 辅助检查中的 CT / 核磁共振 / 彩超 都会复用这组部位。
static const char* examBodyParts[15] = {
    "头部", "颈部", "胸部", "上腹部", "下腹部",
    "腰椎", "颈椎", "膝关节", "肩关节", "肘关节",
    "腕关节", "髋关节", "踝关节", "盆腔", "甲状腺"
};

// 打印可选检查部位列表。
// 每行显示 3 个部位，方便医生按编号选择。
static void printExamBodyParts(void) {
    printf("\n可选检查部位如下：\n");
    for (int i = 0; i < 15; ++i) {
        printf("[%2d] %-8s", i + 1, examBodyParts[i]);
        if ((i + 1) % 3 == 0) printf("\n");
    }
    if (15 % 3 != 0) printf("\n");
}

// 把医生选中的部位编号数组，拼接成一个中文字符串。
// 例如 selected = {1, 3, 5} 时，可能拼成：头部、胸部、下腹部
// 最终结果写入 buffer。
static void joinSelectedParts(const int* selected, int count, char* buffer, size_t size) {
    size_t used = 0;

    // 基本防御：目标缓冲区为空或长度为 0 时直接返回
    if (!buffer || size == 0) return;

    // 先清空目标字符串
    buffer[0] = '\0';

    // 逐个把选中的部位名称拼接到 buffer 中
    for (int i = 0; i < count; ++i) {
        // selected 数组里存的是 1~15 的编号，因此要 -1 对应数组下标
        const char* part = examBodyParts[selected[i] - 1];

        // 第一个部位前不加顿号，后面的部位前加“、”
        int written = snprintf(buffer + used, size - used, "%s%s", (i == 0) ? "" : "、", part);

        // 若 snprintf 失败或发生截断，则直接终止，保证字符串仍然以 '\0' 结束
        if (written < 0 || (size_t)written >= size - used) {
            buffer[size - 1] = '\0';
            return;
        }

        used += (size_t)written;
    }
}

// 创建辅助检查单。
// 参数：
// docId         当前医生 ID
// patientId     当前患者 ID
// inpatientMode 是否走住院模式
//               0 = 门诊：生成待支付检查单
//               1 = 住院：直接从住院押金中扣费
// 返回值：
// 1 表示正常执行完成
// 0 表示失败
int createAuxiliaryExamOrders(const char* docId, const char* patientId, int inpatientMode) {
    char pId[20];

    if (!docId || !patientId) return 0;

    // 拷贝患者 ID 到局部变量，避免直接操作外部传入指针
    strncpy(pId, patientId, sizeof(pId) - 1);
    pId[sizeof(pId) - 1] = '\0';

    // 外层 while(1) 表示可以连续多次开检查单，直到医生主动选择结束
    while (1) {
        int examType;               // 检查类型
        char examName[50] = "";     // 检查项目名称
        char examDetail[220] = "";  // 检查部位明细或子项目说明
        double unitPrice = 0.0;     // 单价
        int quantity = 1;           // 数量 / 部位数
        double totalCost = 0.0;     // 总价
        char unitMoney[32], totalMoney[32];  // 格式化金额字符串

        // 一级菜单：选择检查类型
        printf("\n========== 辅助检查开单 =========="
            "\n[1]. 抽血"
            "\n[2]. CT"
            "\n[3]. 核磁共振"
            "\n[4]. 彩超"
            "\n[-1]. 结束开单"
            "\n请选择检查类型: ");

        while (1) {
            examType = safeGetInt();
            if (examType == -1 || (examType >= 1 && examType <= 4)) break;
            printf("  [!] 请输入菜单中提供的数字编号 (-1/1~4): ");
        }
        if (examType == -1) break;

        // 抽血类检查：进入二级菜单
        if (examType == 1) {
            int bloodChoice;

            printf("\n1. 血常规 (25.00元)\n2. 生化 (260.00元)\n-1. 返回上一级\n请选择: ");

            // 二级菜单统一使用 safeGetInt，保留 -1 返回
            while (1) {
                bloodChoice = safeGetInt();
                if (bloodChoice == -1 || (bloodChoice >= 1 && bloodChoice <= 2)) break;
                printf("  [!] 请输入菜单中提供的数字编号 (-1/1~2): ");
            }  
            if (bloodChoice == -1) continue;

            // 根据抽血子项目设置名称、说明和单价
            if (bloodChoice == 1) {
                strcpy(examName, "抽血-血常规");
                strcpy(examDetail, "血常规");
                unitPrice = 25.0;
            }
            else {
                strcpy(examName, "抽血-生化");
                strcpy(examDetail, "生化");
                unitPrice = 260.0;
            }
            quantity = 1;
        }
        else {
            // CT / 核磁共振 / 彩超：都需要选择检查部位
            int selected[15] = { 0 };  // 保存医生选择的部位编号
            int partCount;             // 部位数量
            int cancelPartSelection = 0;

            // 根据类型设置项目名和单价
            if (examType == 2) { strcpy(examName, "CT"); unitPrice = 260.0; }
            else if (examType == 3) { strcpy(examName, "核磁共振"); unitPrice = 400.0; }
            else if (examType == 4) { strcpy(examName, "彩超"); unitPrice = 150.0; }

            // 打印可选部位清单
            printExamBodyParts();

            // 输入检查部位数量
            printf("请输入检查部位数量(1-15): ");
            while (1) {
                partCount = safeGetInt();
                if (partCount == -1) break;
                if (partCount >= 1 && partCount <= 15) break;
                printf("  [!] 检查部位数量必须在 1~15 之间，请重新输入 (输入-1返回): ");
            }
            if (partCount == -1) continue;

            // 逐个输入部位编号，并做重复校验
            for (int i = 0; i < partCount; ++i) {
                while (1) {
                    int duplicated = 0;

                    printf("请输入第%d个部位编号: ", i + 1);

                    // 限制部位编号必须在 1~15
                    selected[i] = safeGetInt();
                    if (selected[i] == -1) {
                        cancelPartSelection = 1;
                        break;
                    }
                    if (selected[i] < 1 || selected[i] > 15) {
                        printf("  [!] 部位编号必须在 1~15 之间，请重新输入 (输入-1返回): ");
                        continue;
                    }

                    // 检查是否重复选择了同一个部位
                    for (int j = 0; j < i; ++j) {
                        if (selected[j] == selected[i]) {
                            duplicated = 1;
                            break;
                        }
                    }

                    // 若重复则提示并重新输入当前这个部位编号
                    if (duplicated) {
                        printf("  [!] 部位编号不能重复，请重新输入。\n");
                        continue;
                    }

                    // 当前部位编号合法且不重复，跳出内层循环
                    break;
                }
            }

            if (cancelPartSelection) continue;

            // 把选择的多个部位拼接成中文字符串，如“头部、胸部、膝关节”
            joinSelectedParts(selected, partCount, examDetail, sizeof(examDetail));

            // 对影像类检查，总价 = 单价 × 部位数
            quantity = partCount;
        }

        // 计算总价
        totalCost = unitPrice * quantity;

        // 金额格式化成 xx.xx元
        formatMoney(unitPrice, unitMoney, sizeof(unitMoney));
        formatMoney(totalCost, totalMoney, sizeof(totalMoney));

        // 创建检查记录节点
        Record* r4 = (Record*)malloc(sizeof(Record));
        if (!r4) {
            printf("  [!] 内存分配失败。\n");
            return 0;
        }

        // 生成唯一记录 ID，并填写基础字段
        generateRecordID(r4->recordId);
        r4->type = 4;  // 4 表示辅助检查
        strcpy(r4->patientId, pId);
        strcpy(r4->staffId, docId);
        r4->cost = totalCost;

        // 住院模式：直接从住院押金中扣费
        if (inpatientMode) {
            // 查找患者对象，并确认其当前确实处于住院状态
            Patient* ip = findPatientById(pId);
            if (!ip || !ip->isInpatient) {
                free(r4);
                printf("  [!] 当前患者并非有效住院病人，无法走住院押金扣费流程。\n");
                return 0;
            }

            // 住院押金直接扣减检查费用
            ip->inpatientDeposit -= totalCost;

            // 住院模式下，这笔检查默认视为已支付
            r4->isPaid = 1;

            // 记录描述中写清：检查类型、部位、单价、数量、总金额、支付状态
            snprintf(r4->description, sizeof(r4->description),
                "[住院明细][检查] 检查类型:%.20s_检查部位:%.120s_单价:%.20s_数量:%d_总金额:%.20s_状态:%.24s",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney,
                "已从住院押金扣除");

            // 若扣完后押金为负，则同步更新住院欠费补交单
            // 这样后续患者端费用中心可以看到待补交账单
            syncInpatientArrearsBill(ip, docId);

            // 打印开单成功提示
            printf("  [√] 住院检查单已生成：类型=%s，部位=%s，单价=%s，数量/部位数=%d，总金额=%s。\n",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney);

            // 显示扣费后的当前押金余额
            printf("  [住院押金] 已直接扣费，当前押金余额：%.2f元\n", ip->inpatientDeposit);

            // 若押金小于 0，说明已欠费，额外给出提醒
            if (ip->inpatientDeposit < 0) {
                printf("  [资金预警] 当前已转为欠费状态，系统已同步更新待补交账单。\n");
            }
        }
        else {
            // 门诊模式：只生成待支付检查单，不立即扣费
            r4->isPaid = 0;

            // 描述中记录检查类型、部位、单价、数量、总金额及“未支付”状态
            snprintf(r4->description, sizeof(r4->description),
                "检查类型:%.20s_检查部位:%.120s_单价:%.20s_数量:%d_总金额:%.20s_状态:%.12s",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney,
                "未支付");

            printf("  [√] 检查单已生成：类型=%s，部位=%s，单价=%s，数量/部位数=%d，总金额=%s。\n",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney);
        }

        // 记录创建时间
        getCurrentTimeStr(r4->createTime, sizeof(r4->createTime));

        // 头插法插入全局记录链表
        // 新记录会排在最前面，方便后续快速访问最近生成的记录
        r4->next = recordHead->next;
        recordHead->next = r4;

        // 是否继续开单
        printf("是否继续开单？(1.继续 0.结束，输入-1也可结束): ");
        while (1) {
            int cont = safeGetInt();
            if (cont == -1 || cont == 0) return 1;
            if (cont == 1) break;
            printf("  [!] 请输入 1、0 或 -1: ");
        }
    }

    return 1;
}

// 生成 Record 的唯一 ID
// 当前实现规则是：R2026XXXX
// 其中 XXXX 从现有记录中取最大值后 +1
void generateRecordID(char* buffer) {
    int maxId = 1999;

    // 遍历当前所有记录，找出已有的最大编号
    Record* r = recordHead->next;
    while (r != NULL) {
        int currentIdNum;

        // 从 recordId 中解析出后四位数字
        if (sscanf(r->recordId, "R2026%04d", &currentIdNum) == 1) {
            if (currentIdNum > maxId) maxId = currentIdNum;
        }
        r = r->next;
    }

    // 生成新的记录编号
    sprintf(buffer, "R2026%04d", maxId + 1);
}

// 医生叫号函数
// 功能：
// 1. 列出当前医生所有排班日期
// 2. 选中某天后显示候诊患者列表
// 3. 可自动广播叫号第一位已缴费患者
// 4. 叫号成功后，把 currentCallingPatientId 绑定到当前患者
void callPatient(const char* docId) {
    system("cls");
    printf("\n========== 智能排班溯源与叫号 =========="
        "\n");

    Schedule* s = scheduleList->next;
    int sCount = 0;
    char availableDates[20][15];

    // 收集当前医生所有排班日期（去重）
    while (s) {
        if (strcmp(s->doctor_id, docId) == 0) {
            int duplicate = 0;

            // 判断该日期是否已经加入过数组，避免重复显示
            for (int i = 0; i < sCount; i++) {
                if (strcmp(availableDates[i], s->date) == 0) duplicate = 1;
            }

            // 若不是重复日期，则收集到 availableDates 中
            if (!duplicate && sCount < 20) {
                strcpy(availableDates[sCount], s->date);
                sCount++;
            }
        }
        s = s->next;
    }

    // 没有排班日期，说明当前医生近期没有排班
    if (sCount == 0) {
        printf("  [!] 调度台回执：您在系统近期排班表中是真空状态。\n");
        return;
    }

    // 选择要查看的排班日期
    printf("  [授权排班日期选择]:\n");
    for (int i = 0; i < sCount; i++) printf("   [%d] %s\n", i + 1, availableDates[i]);
    printf("   [-1] 脱离排班中心\n----------------------------------------\n  选择检索索引号: ");

    // 读取用户选择的日期索引
    int dChoice = safeGetInt();

    // -1 表示退出
    if (dChoice == -1) return;

    // 输入越界则直接结束
    if (dChoice < 1 || dChoice > sCount) {
        printf("  [!] 输入越界：无此索引号，操作已终止。\n");
        return;
    }

    // 取出目标日期
    char* targetDate = availableDates[dChoice - 1];

    // 打印该日期的候诊患者列表
    printf("\n========== [%s] 候诊大厅 ==========\n", targetDate);
    printf(" %-8s | %-14s | %-12s | %-12s | %-10s\n", "系统位次", "挂号流转凭证", "患者全局ID", "档案署名", "当前候诊状态");
    printf("------------------------------------------------------------------------\n");

    int count = 0;
    Record* r = recordHead->next;

    // 遍历所有记录，找出当前医生在该日期的挂号记录
    while (r) {
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next;
            char pName[100] = "脱敏未知";

            // 根据 patientId 找患者姓名
            while (p) {
                if (strcmp(p->id, r->patientId) == 0) {
                    strcpy(pName, p->name);
                    break;
                }
                p = p->next;
            }

            // 根据挂号记录支付状态推断当前候诊状态
            char status[30];
            if (r->isPaid == 0) strcpy(status, "[ ！]未缴费");
            else if (r->isPaid == 1) strcpy(status, "[活跃]候诊");
            else strcpy(status, "[完结]已接诊");

            // 从挂号描述中提取排号号次
            int seqNum = 0;
            char* seqPtr = strstr(r->description, "排号:");
            if (!seqPtr) seqPtr = strstr(r->description, "队列号:");

            if (seqPtr) {
                if (strncmp(seqPtr, "排号:", strlen("排号:")) == 0) sscanf(seqPtr, "排号:%d", &seqNum);
                else sscanf(seqPtr, "队列号:%d", &seqNum);
            }

            // 输出候诊患者信息
            printf("  %-7d | %-14s | %-12s | %-12s | %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }

    // 没有候诊患者则直接返回
    if (count == 0) {
        printf("  今日无病患来访。\n");
        return;
    }

    printf("------------------------------------------------------------------------\n");

    // 是否自动广播叫号第一位合法患者
    printf("\n  [业务指令] 是否自动广播叫号，将第一顺位合法患者请入诊室？(1.同意 0.拒绝): ");
    {
        int autoCall;
        while (1) {
            autoCall = safeGetInt();
            if (autoCall == -1 || autoCall == 0 || autoCall == 1) break;
            printf("  [!] 请输入 1、0 或 -1: ");
        }
        if (autoCall == 1) {
            r = recordHead->next;
            int found = 0;

            // 再次遍历记录，找出当前日期下第一位已缴费且未接诊的挂号患者
            while (r) {
                if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1 && strstr(r->description, targetDate)) {
                    Patient* p = patientHead->next;
                    char pName[100] = "未知";

                    // 查患者姓名
                    while (p) {
                        if (strcmp(p->id, r->patientId) == 0) {
                            strcpy(pName, p->name);
                            break;
                        }
                        p = p->next;
                    }

                    // 广播叫号
                    printf("\n  【走廊大屏广播】 -> \"请 %s 号患者 [%s] 到 %s 诊室就医。\"\n", r->patientId, pName, docId);

                    // 标记该挂号记录为“已接诊”
                    r->isPaid = 2;
                    found = 1;

                    // 锁定当前接诊患者，后续诊断/开药/开检查都默认针对该患者
                    strcpy(currentCallingPatientId, r->patientId);
                    printf("  >>> HIS 引擎已强锁死当前看诊沙盒为: %s <<<\n", pName);
                    break;
                }
                r = r->next;
            }

            // 若没有找到可叫号患者，说明都已处理完
            if (!found) printf("  [提示] 您当日的预约病人已全部接诊完毕！\n");
        }
    }
}
// 医生录入诊断与辅助检查
// 流程：
// 1. 确定当前患者
// 2. 输入症状和诊断
// 3. 生成一条临床诊断记录
// 4. 询问是否继续开辅助检查
void diagnoseAndTest(const char* docId) {
    char pId[20];
    char symptoms[100], diag[100];

    // 若已有 currentCallingPatientId，则自动使用当前被叫号患者
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [系统自动提取当前病患ID]: %s\n", pId);
    }
    else {
        // 若当前没有绑定患者，则需要医生手动输入患者 ID，并校验患者是否存在
        if (!inputExistingPatientIdCommon(pId, sizeof(pId), "\n  [无关联病患] 请手动键入要下达诊断的病患ID (-1终止): ")) return;
    }

    // 输入症状，不能为空
    while (1) {
        printf("  请敲入宏观体征表象(如咳嗽发热): ");
        safeGetString(symptoms, sizeof(symptoms));
        if (strlen(symptoms) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 输入诊断结果，不能为空
    while (1) {
        printf("  请给出权威确切诊断学结论: ");
        safeGetString(diag, sizeof(diag));
        if (strlen(diag) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 创建诊断记录节点
    Record* r2 = (Record*)malloc(sizeof(Record));
    if (!r2) {
        printf("  [!] 内存分配失败。\n");
        return;
    }

    // 填写诊断记录基础字段
    generateRecordID(r2->recordId);
    r2->type = 2;  // 2 表示诊疗/诊断
    strcpy(r2->patientId, pId);
    strcpy(r2->staffId, docId);
    r2->cost = 20.0;   // 固定诊疗费
    r2->isPaid = 0;    // 初始状态为待支付

    // 把症状和诊断内容写入描述字段
    sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);

    // 记录创建时间并插入记录链表
    getCurrentTimeStr(r2->createTime, sizeof(r2->createTime));
    r2->next = recordHead->next;
    recordHead->next = r2;

    printf("  [√] 核心看诊数据入库封存。\n");

    // 询问是否继续开辅助检查
    printf("\n  需延伸生化/影像仪器的辅助检查吗？(1.开单 0.放弃，输入-1返回): ");
    while (1) {
        int choice = safeGetInt();
        if (choice == 1) break;
        if (choice == 0 || choice == -1) return;
        printf("  [!] 请输入 1、0 或 -1: ");
    }

    // 若选择继续，则直接进入门诊模式的辅助检查开单流程
    createAuxiliaryExamOrders(docId, pId, 0);
}

// 医生开药
// 流程：
// 1. 确定当前患者
// 2. 按关键字检索药品
// 3. 输出检索结果
// 4. 选择药品
// 5. 输入数量
// 6. 生成待支付药品记录
void prescribeMedicine(const char* docId) {
    char pId[20];

    // 若已有当前叫号患者，则自动挂载为处方对象
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [自动挂载处方对象]: %s\n", pId);
    }
    else {
        // 否则要求医生手动输入患者 ID，并校验患者是否存在
        if (!inputExistingPatientIdCommon(pId, sizeof(pId), "\n  请输入当前接诊患者ID (-1终止): ")) return;
    }

    // 外层循环允许连续检索和开多种药
    while (1) {
        char key[50];
        Drug* matched[100] = { NULL };  // 保存命中的药品指针
        int mCount;

        // 输入检索关键字
        printf("\n  请检索大药房物资库药品 (编号/药名，输-1退出配药): ");
        safeGetString(key, sizeof(key));

        // -1 表示结束开药
        if (strcmp(key, "-1") == 0) break;

        // 关键词不能为空
        if (strlen(key) == 0) {
            printf("  [!] 关键词不能为空。\n");
            continue;
        }

        // 先按药名模糊匹配
        mCount = findDrugsByNameFuzzy(key, matched, 100);

        // 若模糊匹配不到，再尝试把输入当作药品 ID 精确查找
        if (mCount == 0) {
            int id = 0;
            char* endptr = NULL;
            long tmp = strtol(key, &endptr, 10);

            // 必须是纯数字、正数，并且不能超过 int 范围
            if (endptr != key && *endptr == '\0' && tmp > 0 && tmp <= 2147483647L) {
                Drug* exact = findDrugById((int)tmp);
                if (exact) matched[mCount++] = exact;
            }
        }

        // 输出检索结果表
        printf("\n  ========== 药品储备库 ==========\n");
        printf("  %-6s | %-8s | %-30s | %-10s | %-8s\n", "映射号", "系统内码", "国家标准药名", "单价", "物理库存");
        printf("-----------------------------------------------------------------------\n");

        for (int i = 0; i < mCount; ++i) {
            char priceText[32];
            formatMoney(matched[i]->price, priceText, sizeof(priceText));

            // 左侧“映射”只是当前结果表中的序号，不是系统药品 ID
            printf("  [%-3d] | %-8d | %-30.20s | %-10s | %-8d\n", i + 1, matched[i]->id, matched[i]->name, priceText, matched[i]->stock);
        }

        // 没查到则提示后继续下一轮检索
        if (mCount == 0) {
            printf("  [!] 检索库中查无此药，请换词再试。\n");
            continue;
        }

        // 选择映射号
        printf("-----------------------------------------------------------------------\n");
        printf("  请选择映射号 (1-%d, 输入-1取消): ", mCount);

        // 这里读取的是“映射号”，只用于 matched[] 的下标定位
        int selectedIndex = safeGetInt();

        // -1 表示本轮取消选择，返回继续检索
        if (selectedIndex == -1) continue;

        // 越界则说明映射号不合法
        if (selectedIndex < 1 || selectedIndex > mCount) {
            printf("  [!] 定位脱靶，输入无法解析。\n");
            continue;
        }

        // 按映射号取出药品对象
        Drug* selectedMed = matched[selectedIndex - 1];

        // 输入数量并创建药品记录
        while (1) {
            int qty;

            printf("  提取 [%s] 物理装填数目 ([!] 单次不能超过10盒): ", selectedMed->name);

            // 使用统一安全函数读取药品数量
            qty = safeGetDrugQuantity();

            // -1 作为取消信号，直接结束当前药品的开立
            if (qty == -1) break;

            // 单次最多 10 盒
            if (qty > 10) {
                printf("  [越权预警] 系统默认指令：处方药单次过载(超10盒)！打回重填。\n");
                continue;
            }

            // 不允许超过当前库存
            if (qty > selectedMed->stock) {
                printf("  [缺货熔断] 中央仓库存不足！当前最高只能开: %d 盒\n", selectedMed->stock);
                continue;
            }

            // 计算本次药品总价
            double totalCost = qty * selectedMed->price;
            char priceText[32], totalText[32];
            formatMoney(selectedMed->price, priceText, sizeof(priceText));
            formatMoney(totalCost, totalText, sizeof(totalText));

            // 创建药品记录节点
            Record* r3 = (Record*)malloc(sizeof(Record));
            if (!r3) {
                printf("  [!] 内存分配失败。\n");
                return;
            }

            // 填写药品记录基础字段
            generateRecordID(r3->recordId);
            r3->type = 3;  // 3 表示药品
            strcpy(r3->patientId, pId);
            strcpy(r3->staffId, docId);
            r3->cost = totalCost;
            r3->isPaid = 0;  // 初始状态：待支付

            // 描述中保存药名、单价、数量、总价
            snprintf(r3->description, sizeof(r3->description), "药品:%s_单价:%s_数量:%d_总价:%s", selectedMed->name, priceText, qty, totalText);

            // 记录创建时间并插入全局记录链表
            getCurrentTimeStr(r3->createTime, sizeof(r3->createTime));
            r3->next = recordHead->next;
            recordHead->next = r3;

            // 注意：这里只是创建待支付药单，不在此处扣库存
            // 真正扣库存应当在患者支付成功后再执行
            printf("  [√] 云端处方下达指令送出，价值 %s。待患者支付成功后再正式扣减库存。\n", totalText);
            break;
        }
            }
    }

// 开具住院通知单
// 功能：
// 1. 确定患者
// 2. 选择普通/重症
// 3. 填写说明
// 4. 生成住院通知记录
void issueAdmissionNotice(const char* docId) {
    char pId[20];

    // 若当前已有叫号患者，则直接使用
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [接诊关联]: %s\n", pId);
    }
    else {
        // 否则手动输入患者 ID，并校验患者是否存在
        if (!inputExistingPatientIdCommon(pId, sizeof(pId), "\n  键入需强制收容的患者识别码 (-1返回): ")) return;
    }

    // 选择是否走重症优先通道
    printf("  是否开启危重高优先通道越级抢床位？(1.重症直达 0.普通常规): ");
    int isSevere;
    while (1) {
        isSevere = safeGetInt();
        if (isSevere == 0 || isSevere == 1) break;
        printf("  [!] 输入越界，请规范输入 1 或 0: ");
    }

    // 输入收治说明，不能为空
    char note[100];
    while (1) {
        printf("  输入初步临床诊断说明(严禁为空): ");
        safeGetString(note, sizeof(note));
        if (strlen(note) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 创建住院通知记录
    Record* r6 = (Record*)malloc(sizeof(Record));
    if (!r6) {
        printf("  [!] 内存分配失败。\n");
        return;
    }

    // 填写通知记录字段
    generateRecordID(r6->recordId);
    r6->type = 6;  // 6 表示住院通知/入院通知类记录
    strcpy(r6->patientId, pId);
    strcpy(r6->staffId, docId);
    r6->cost = 0.0;
    r6->isPaid = 0;  // 未处理状态

    // 描述中记录优先级和说明
    snprintf(r6->description, sizeof(r6->description), "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);

    // 记录创建时间并插入记录链表
    getCurrentTimeStr(r6->createTime, sizeof(r6->createTime));
    r6->next = recordHead->next;
    recordHead->next = r6;

    printf("  [√] 通知单越界发送至住院部。(打标属性: %s)\n", isSevere ? "重症" : "普通");
}