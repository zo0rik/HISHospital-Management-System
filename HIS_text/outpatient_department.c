#define _CRT_SECURE_NO_WARNINGS  // 关闭 VS 对传统 C 函数的安全警告
#include <stdio.h>   // 标准输入输出
#include <stdlib.h>  // malloc、free 等内存管理
#include <string.h>  // strcpy、strcmp、strstr、strlen 等字符串函数
#include "models.h"                  // 项目核心数据结构定义
#include "utils.h"                   // 通用工具函数，如安全输入、金额格式化、时间获取
#include "schedule.h"                // 排班相关声明
#include "doctor.h"                  // 医生相关声明
#include "outpatient_department.h"   // 当前模块头文件/相关声明
#include "patient.h"                 // 患者相关声明
#include "inpatient_department.h"    // 住院部相关声明
#include "drug.h"                    // 药品相关声明

// 当前被医生“叫到诊室”的患者 ID。
// 门诊看诊、开药、开检查时，如果这里有值，就默认对这个患者操作。
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
// 结果写入 buffer。
static void joinSelectedParts(const int* selected, int count, char* buffer, size_t size) {
    size_t used = 0;
    if (!buffer || size == 0) return;
    buffer[0] = '\0';
    for (int i = 0; i < count; ++i) {
        const char* part = examBodyParts[selected[i] - 1];
        int written = snprintf(buffer + used, size - used, "%s%s", (i == 0) ? "" : "、", part);
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
    strncpy(pId, patientId, sizeof(pId) - 1);
    pId[sizeof(pId) - 1] = '\0';

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
            "\n1. 抽血"
            "\n2. CT"
            "\n3. 核磁共振"
            "\n4. 彩超"
            "\n0. 结束开单"
            "\n请选择检查类型: ");
        examType = safeGetIntInRange(0, 4, "  [!] 请输入 0~4 之间的菜单编号: ");
        if (examType == 0) break;

        // 抽血类检查：进入二级菜单
        if (examType == 1) {
            int bloodChoice;
            printf("\n1. 血常规 (25.00元)\n2. 生化 (260.00元)\n0. 返回上一级\n请选择: ");
            bloodChoice = safeGetIntInRange(0, 2, "  [!] 请输入 0~2 之间的菜单编号: ");
            if (bloodChoice == 0) continue;

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

            if (examType == 2) { strcpy(examName, "CT"); unitPrice = 260.0; }
            else if (examType == 3) { strcpy(examName, "核磁共振"); unitPrice = 400.0; }
            else { strcpy(examName, "彩超"); unitPrice = 150.0; }

            // 打印部位列表
            printExamBodyParts();

            // 输入部位数量
            printf("请输入检查部位数量(1-15): ");
            partCount = safeGetIntInRange(1, 15, "  [!] 检查部位数量必须在 1~15 之间，请重新输入: ");

            // 逐个输入部位编号，并做重复校验
            for (int i = 0; i < partCount; ++i) {
                while (1) {
                    int duplicated = 0;
                    printf("请输入第%d个部位编号: ", i + 1);
                    selected[i] = safeGetIntInRange(1, 15, "  [!] 部位编号必须在 1~15 之间，请重新输入: ");

                    // 检查是否重复选择了同一个部位
                    for (int j = 0; j < i; ++j) {
                        if (selected[j] == selected[i]) {
                            duplicated = 1;
                            break;
                        }
                    }
                    if (duplicated) {
                        printf("  [!] 部位编号不能重复，请重新输入。\n");
                        continue;
                    }
                    break;
                }
            }

            // 把选择的部位拼接成文本
            joinSelectedParts(selected, partCount, examDetail, sizeof(examDetail));
            quantity = partCount;
        }

        // 计算总价
        totalCost = unitPrice * quantity;

        // 金额格式化成 xx.xx元
        formatMoney(unitPrice, unitMoney, sizeof(unitMoney));
        formatMoney(totalCost, totalMoney, sizeof(totalMoney));

        // 创建检查记录
        Record* r4 = (Record*)malloc(sizeof(Record));
        if (!r4) { printf("  [!] 内存分配失败。\n"); return 0; }
        generateRecordID(r4->recordId);
        r4->type = 4;  // 4 表示辅助检查
        strcpy(r4->patientId, pId);
        strcpy(r4->staffId, docId);
        r4->cost = totalCost;

        // 住院模式：直接从住院押金中扣费
        if (inpatientMode) {
            Patient* ip = findPatientById(pId);
            if (!ip || !ip->isInpatient) {
                free(r4);
                printf("  [!] 当前患者并非有效住院病人，无法走住院押金扣费流程。\n");
                return 0;
            }

            // 住院押金直接扣减
            ip->inpatientDeposit -= totalCost;
            r4->isPaid = 1;  // 住院模式直接记为已支付
            snprintf(r4->description, sizeof(r4->description),
                "[住院明细][检查] 检查类型:%.20s_检查部位:%.120s_单价:%.20s_数量:%d_总金额:%.20s_状态:%.24s",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney,
                "已从住院押金扣除");

            // 若扣完后押金为负，则同步更新住院欠费补交单
            syncInpatientArrearsBill(ip, docId);

            printf("  [√] 住院检查单已生成：类型=%s，部位=%s，单价=%s，数量/部位数=%d，总金额=%s。\n",
                examName,
                (strlen(examDetail) > 0 ? examDetail : "无"),
                unitMoney,
                quantity,
                totalMoney);
            printf("  [住院押金] 已直接扣费，当前押金余额：%.2f元\n", ip->inpatientDeposit);
            if (ip->inpatientDeposit < 0) {
                printf("  [资金预警] 当前已转为欠费状态，系统已同步更新待补交账单。\n");
            }
        }
        else {
            // 门诊模式：只生成待支付检查单，不立即扣费
            r4->isPaid = 0;
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

        // 记录开单时间并插入记录链表
        getCurrentTimeStr(r4->createTime, sizeof(r4->createTime));
        r4->next = recordHead->next;
        recordHead->next = r4;

        // 是否继续开单
        printf("是否继续开单？(1.继续 0.结束): ");
        if (safeGetIntInRange(0, 1, "  [!] 请输入 1 或 0: ") == 0) break;
    }
    return 1;
}

// 生成 Record 的唯一 ID
// 当前实现规则是：R2025XXXX
// 其中 XXXX 从现有记录中取最大值后 +1
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
            for (int i = 0; i < sCount; i++) {
                if (strcmp(availableDates[i], s->date) == 0) duplicate = 1;
            }
            if (!duplicate && sCount < 20) {
                strcpy(availableDates[sCount], s->date);
                sCount++;
            }
        }
        s = s->next;
    }

    if (sCount == 0) {
        printf("  [!] 调度台回执：您在系统近期排班表中是真空状态。\n");
        return;
    }

    // 选择要查看的排班日期
    printf("  [授权排班日期选择树]:\n");
    for (int i = 0; i < sCount; i++) printf("   [%d] %s\n", i + 1, availableDates[i]);
    printf("   [-1] 脱离排班中心\n----------------------------------------\n  选择检索索引号: ");

    int dChoice = safeGetInt();
    if (dChoice == -1) return;
    if (dChoice < 1 || dChoice > sCount) {
        printf("  [!] 输入越界：无此索引号，操作已终止。\n");
        return;
    }
    char* targetDate = availableDates[dChoice - 1];

    // 打印该日期的候诊患者列表
    printf("\n========== [%s] 候诊大厅 ==========\n", targetDate);
    printf(" %-8s | %-14s | %-12s | %-12s | %-10s\n", "系统位次", "挂号流转凭证", "患者全局ID", "档案署名", "当前生命周期");
    printf("------------------------------------------------------------------------\n");
    int count = 0;
    Record* r = recordHead->next;
    while (r) {
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && strstr(r->description, targetDate)) {
            Patient* p = patientHead->next;
            char pName[100] = "脱敏未知";

            // 找患者姓名
            while (p) {
                if (strcmp(p->id, r->patientId) == 0) {
                    strcpy(pName, p->name);
                    break;
                }
                p = p->next;
            }

            // 根据支付状态推断候诊生命周期
            char status[30];
            if (r->isPaid == 0) strcpy(status, "[卡死]缺费");
            else if (r->isPaid == 1) strcpy(status, "[活跃]候诊");
            else strcpy(status, "[完结]离室");

            // 从 description 中提取排号号次
            int seqNum = 0;
            char* seqPtr = strstr(r->description, "排号:");
            if (!seqPtr) seqPtr = strstr(r->description, "队列号:");
            if (seqPtr) {
                if (strncmp(seqPtr, "排号:", strlen("排号:")) == 0) sscanf(seqPtr, "排号:%d", &seqNum);
                else sscanf(seqPtr, "队列号:%d", &seqNum);
            }

            printf("  %-7d | %-14s | %-12s | %-12s | %-10s\n", seqNum, r->recordId, r->patientId, pName, status);
            count++;
        }
        r = r->next;
    }

    if (count == 0) {
        printf("  今日无病患来访。\n");
        return;
    }
    printf("------------------------------------------------------------------------\n");

    // 是否自动广播叫号第一位合法患者
    printf("\n  [业务指令] 是否自动广播叫号，将第一顺位合法患者请入诊室？(1.同意 0.拒绝): ");
    if (safeGetIntInRange(0, 1, "  [!] 请输入 1 或 0: ") == 1) {
        r = recordHead->next;
        int found = 0;
        while (r) {
            if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1 && strstr(r->description, targetDate)) {
                Patient* p = patientHead->next;
                char pName[100] = "未知";
                while (p) {
                    if (strcmp(p->id, r->patientId) == 0) {
                        strcpy(pName, p->name);
                        break;
                    }
                    p = p->next;
                }

                // 广播叫号
                printf("\n  【走廊大屏广播】 -> \"请 %s 号患者 [%s] 到 %s 诊室就医。\"\n", r->patientId, pName, docId);

                // 标记该挂号记录状态变化
                r->isPaid = 2;
                found = 1;

                // 锁定当前接诊患者
                strcpy(currentCallingPatientId, r->patientId);
                printf("  >>> HIS 引擎已强锁死当前看诊沙盒为: %s <<<\n", pName);
                break;
            }
            r = r->next;
        }
        if (!found) printf("  [提示] 您当日的预约病人已全部接诊完毕！\n");
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

    // 若已有 currentCallingPatientId，则自动使用
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [系统自动提取当前病患ID]: %s\n", pId);
    }
    else {
        // 否则手动输入患者 ID
        printf("\n  [无关联病患] 请手动键入要下达诊断的病患ID (-1终止): ");
        safeGetString(pId, sizeof(pId));
        if (strcmp(pId, "-1") == 0) return;
    }

    // 输入症状
    while (1) {
        printf("  请敲入宏观体征表象(如咳嗽发热): ");
        safeGetString(symptoms, sizeof(symptoms));
        if (strlen(symptoms) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 输入诊断结果
    while (1) {
        printf("  请给出权威确切诊断学结论: ");
        safeGetString(diag, sizeof(diag));
        if (strlen(diag) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 创建诊断记录
    Record* r2 = (Record*)malloc(sizeof(Record));
    if (!r2) { printf("  [!] 内存分配失败。\n"); return; }
    generateRecordID(r2->recordId);
    r2->type = 2;  // 2 表示诊疗/诊断
    strcpy(r2->patientId, pId);
    strcpy(r2->staffId, docId);
    r2->cost = 20.0;   // 固定诊疗费
    r2->isPaid = 0;    // 待支付
    sprintf(r2->description, "症状:%s_临床诊断:%s", symptoms, diag);
    getCurrentTimeStr(r2->createTime, sizeof(r2->createTime));
    r2->next = recordHead->next;
    recordHead->next = r2;
    printf("  [√] 核心看诊数据入库封存。\n");

    // 是否继续开辅助检查
    printf("\n  需延伸生化/影像仪器的辅助检查吗？(1.开单 0.放弃): ");
    if (safeGetIntInRange(0, 1, "  [!] 请输入 1 或 0: ") != 1) return;

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

    // 若已有当前叫号患者，则自动挂载
    if (strlen(currentCallingPatientId) > 0) {
        strcpy(pId, currentCallingPatientId);
        printf("\n  [自动挂载处方对象]: %s\n", pId);
    }
    else {
        printf("\n  请输入当前接诊患者ID (-1终止): ");
        safeGetString(pId, sizeof(pId));
        if (strcmp(pId, "-1") == 0) return;
    }

    while (1) {
        char key[50];
        Drug* matched[100] = { NULL };  // 保存命中的药品
        int mCount;

        // 输入检索关键字
        printf("\n  请检索大药房物资库字典 (拼音/编号/药名，输-1退出配药): ");
        safeGetString(key, sizeof(key));
        if (strcmp(key, "-1") == 0) break;
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
            if (endptr != key && *endptr == '\0' && tmp > 0 && tmp <= 2147483647L) {
                Drug* exact = findDrugById((int)tmp);
                if (exact) matched[mCount++] = exact;
            }
        }

        // 输出检索结果表
        printf("\n  ========== 药品储备库 ==========\n");
        printf("  %-4s | %-8s | %-20s | %-10s | %-8s\n", "映射", "系统内码", "国家标准药名", "单价", "物理库存");
        printf("-----------------------------------------------------------------------\n");
        for (int i = 0; i < mCount; ++i) {
            char priceText[32];
            formatMoney(matched[i]->price, priceText, sizeof(priceText));
            printf("  [%-2d] | %-8d | %-20.20s | %-10s | %-8d\n", i + 1, matched[i]->id, matched[i]->name, priceText, matched[i]->stock);
        }

        if (mCount == 0) {
            printf("  [!] 检索库中查无此药，请换词再试。\n");
            continue;
        }

        // 选择映射号
        printf("-----------------------------------------------------------------------\n");
        printf("  请选择映射号 (1-%d, 输入-1取消): ", mCount);
        int selectedIndex = safeGetInt();
        if (selectedIndex == -1) continue;
        if (selectedIndex < 1 || selectedIndex > mCount) {
            printf("  [!] 定位脱靶，输入无法解析。\n");
            continue;
        }
        Drug* selectedMed = matched[selectedIndex - 1];

        // 输入数量并创建药品记录
        while (1) {
            int qty;
            printf("  提取 [%s] 物理装填数目 (高红线拦截10盒): ", selectedMed->name);
            qty = safeGetDrugQuantity();
            if (qty == -1) break;

            // 单次最多 10 盒
            if (qty > 10) {
                printf("  [越权预警] 系统默认指令：处方药单次过载(超10盒)！打回重填。\n");
                continue;
            }

            // 不允许超过库存
            if (qty > selectedMed->stock) {
                printf("  [缺货熔断] 中央仓库存不足！当前最高只能开: %d 盒\n", selectedMed->stock);
                continue;
            }

            double totalCost = qty * selectedMed->price;
            char priceText[32], totalText[32];
            formatMoney(selectedMed->price, priceText, sizeof(priceText));
            formatMoney(totalCost, totalText, sizeof(totalText));

            // 创建药品记录
            Record* r3 = (Record*)malloc(sizeof(Record));
            if (!r3) { printf("  [!] 内存分配失败。\n"); return; }
            generateRecordID(r3->recordId);
            r3->type = 3;  // 3 表示药品
            strcpy(r3->patientId, pId);
            strcpy(r3->staffId, docId);
            r3->cost = totalCost;
            r3->isPaid = 0;  // 待支付
            snprintf(r3->description, sizeof(r3->description), "药品:%s_单价:%s_数量:%d_总价:%s", selectedMed->name, priceText, qty, totalText);
            getCurrentTimeStr(r3->createTime, sizeof(r3->createTime));
            r3->next = recordHead->next;
            recordHead->next = r3;

            // 注意：这里只生成待支付记录，不立刻扣库存
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
        printf("\n  键入需强制收容的患者识别码 (-1返回): ");
        safeGetString(pId, sizeof(pId));
        if (strcmp(pId, "-1") == 0) return;
    }

    // 是否走重症优先通道
    printf("  是否开启危重高优先通道越级抢床位？(1.重症直达 0.普通常规): ");
    int isSevere = safeGetIntInRange(0, 1, "  [!] 输入越界，请规范输入 1 或 0: ");

    // 输入收治说明
    char note[100];
    while (1) {
        printf("  填写拟定收治初步说明(严禁为空): ");
        safeGetString(note, sizeof(note));
        if (strlen(note) > 0) break;
        printf("  [!] 输入不能为空，请重新输入。\n");
    }

    // 创建住院通知记录
    Record* r6 = (Record*)malloc(sizeof(Record));
    if (!r6) { printf("  [!] 内存分配失败。\n"); return; }
    generateRecordID(r6->recordId);
    r6->type = 6;  // 6 表示住院通知/入院通知类记录
    strcpy(r6->patientId, pId);
    strcpy(r6->staffId, docId);
    r6->cost = 0.0;
    r6->isPaid = 0;  // 未处理状态
    snprintf(r6->description, sizeof(r6->description), "优先级:%s_说明:%s", isSevere ? "重症" : "普通", note);
    getCurrentTimeStr(r6->createTime, sizeof(r6->createTime));
    r6->next = recordHead->next;
    recordHead->next = r6;

    printf("  [√] 通知单越界发送至住院部。(打标属性: %s)\n", isSevere ? "重症" : "普通");
}