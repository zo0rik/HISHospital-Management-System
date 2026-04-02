#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h" // 强行引入安全输入防护组件

// 全局药品库与流水记录单链表
Drug* drugList = NULL;
DrugHistory* drugHistoryList = NULL;

// ---------------------------------------------------------
// 底层数据引擎：从本地库反序列化药品数据
// ---------------------------------------------------------
void loadDrugs() {
    FILE* fp = fopen("drugs.txt", "r");
    if (!fp) return;

    char line[512];
    Drug d;
    Drug* tail = NULL;

    // 采用逗号分隔符解析协议，适配管理端的高级数据结构
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0; // 剥离回车符

        char* token = strtok(line, ",");
        if (token) strcpy(d.id, token); else d.id[0] = '\0';

        token = strtok(NULL, ",");
        if (token) strcpy(d.name, token); else d.name[0] = '\0';

        token = strtok(NULL, ",");
        if (token) d.stock = atoi(token); else d.stock = 0;

        token = strtok(NULL, ",");
        if (token) d.price = atof(token); else d.price = 0.0;

        token = strtok(NULL, ",");
        if (token) strcpy(d.batch, token); else d.batch[0] = '\0';

        token = strtok(NULL, ",");
        if (token) strcpy(d.expiry, token); else d.expiry[0] = '\0';

        token = strtok(NULL, ",");
        if (token) strcpy(d.last_in, token); else d.last_in[0] = '\0';

        token = strtok(NULL, ",");
        if (token) strcpy(d.last_out, token); else d.last_out[0] = '\0';

        // 尾插法构建全局物资树
        Drug* node = (Drug*)malloc(sizeof(Drug));
        *node = d;
        node->next = NULL;
        if (!drugList) drugList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 数据持久化：内存数据同步至物理磁盘
// ---------------------------------------------------------
void saveDrugs() {
    FILE* fp = fopen("drugs.txt", "w");
    if (!fp) return;
    Drug* p = drugList;
    while (p) {
        fprintf(fp, "%s,%s,%d,%.2f,%s,%s,%s,%s\n",
            p->id, p->name, p->stock, p->price,
            p->batch, p->expiry, p->last_in, p->last_out);
        p = p->next;
    }
    fclose(fp);
}

// --------------------------------------------------------
// 物资流转溯源：加载出入库历史记录
// --------------------------------------------------------
void loadDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "r");
    if (!fp) return;

    char line[512];
    DrugHistory h;
    DrugHistory* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) strcpy(h.drug_id, token); else h.drug_id[0] = '\0';

        token = strtok(NULL, ",");
        if (token) h.type = atoi(token); else h.type = 0;

        token = strtok(NULL, ",");
        if (token) h.quantity = atoi(token); else h.quantity = 0;

        token = strtok(NULL, ",");
        if (token) strcpy(h.time, token); else h.time[0] = '\0';

        DrugHistory* node = (DrugHistory*)malloc(sizeof(DrugHistory));
        *node = h;
        node->next = NULL;
        if (!drugHistoryList) drugHistoryList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 物资流转溯源：保存历史记录
// ---------------------------------------------------------
void saveDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "w");
    if (!fp) return;
    DrugHistory* p = drugHistoryList;
    while (p) {
        fprintf(fp, "%s,%d,%d,%s\n", p->drug_id, p->type, p->quantity, p->time);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 报表输出：全景物资清单视图
// ---------------------------------------------------------
static void displayAllDrugs() {
    if (!drugList) {
        printf("  [系统提示] 当前中央物资储备库为空。\n");
        return;
    }
    printf("\n============================================================================================================\n");
    printf("                                         全院物资储存储备清单视图                                           \n");
    printf("============================================================================================================\n");
    printf(" %-10s | %-18s | %-8s | %-8s | %-12s | %-12s | %-20s\n", "系统内码", "国家标准药名", "物理库存", "统一零售", "批号", "有效期阈值", "末次入库时间");
    printf("------------------------------------------------------------------------------------------------------------\n");
    Drug* p = drugList;
    while (p) {
        printf(" %-10s | %-18s | %-8d | %-8.2f | %-12s | %-12s | %-20s\n",
            p->id, p->name, p->stock, p->price, p->batch, p->expiry, p->last_in);
        p = p->next;
    }
    printf("------------------------------------------------------------------------------------------------------------\n");
}

// ---------------------------------------------------------
// 业务一：药品多维检索矩阵
// ---------------------------------------------------------
static void drugStockQuery() {
    while (1) {
        system("cls");
        printf("\n========== 药房物资检索引擎 ==========\n");
        printf("  [1] 基于唯一内码 (ID) 精确检索\n");
        printf("  [2] 基于药品名称模糊匹配\n");
        printf("  [3] 提取全库物资清单报表\n");
        printf("  [0] 返回主调度界面\n");
        printf("--------------------------------------\n");
        printf("  选择检索模式: ");

        int choice = safeGetInt(); // 引入边界拦截组件，防止异常输入拖垮系统
        if (choice == 0) return;

        if (choice == 1) {
            char id[30];
            printf("\n  请输入待检索的药品精确内码 (ID): ");
            safeGetString(id, 30);

            Drug* p = drugList; int found = 0;
            while (p) {
                if (strcmp(p->id, id) == 0) {
                    printf("\n  [检索命中] 资产详情:\n");
                    printf("  内码: %s | 名称: %s | 库存量: %d 盒 | 零售价: %.2f 元\n", p->id, p->name, p->stock, p->price);
                    printf("  批号: %s | 有效期: %s | 变动记录: %s\n", p->batch, p->expiry, p->last_in);
                    found = 1; break;
                }
                p = p->next;
            }
            if (!found) printf("  [!] 检索库穿透：未寻址到与该内码匹配的物资实体。\n");
            system("pause");
        }
        else if (choice == 2) {
            char name[50];
            printf("\n  键入药品名称关键字片段: ");
            safeGetString(name, 50);

            int found = 0; Drug* p = drugList;
            printf("\n--- 模糊匹配反馈结果 ---\n");
            while (p) {
                if (strstr(p->name, name)) {
                    printf("  命中 -> ID:%s | 名称:%s | 库存:%d | 定价:%.2f\n", p->id, p->name, p->stock, p->price);
                    found = 1;
                }
                p = p->next;
            }
            if (!found) printf("  [!] 算法反馈：特征字段未在现有库中命中任何记录。\n");
            system("pause");
        }
        else if (choice == 3) {
            displayAllDrugs();
            system("pause");
        }
    }
}

// ---------------------------------------------------------
// 业务二：全局物资流转审计报表
// ---------------------------------------------------------
static void viewStockRecords() {
    printf("\n========== 仓储空间异动流水审计 ==========\n");
    DrugHistory* h = drugHistoryList;
    if (!h) { printf("  [报告空置] 系统暂未监控到任何物理库存的干预与流转行为。\n"); return; }

    printf(" %-15s | %-8s | %-10s | %-20s\n", "变动关联内码", "动作属性", "吞吐基数", "时间戳");
    printf("----------------------------------------------------------------\n");
    while (h) {
        printf(" %-15s | %-8s | %-10d | %-20s\n",
            h->drug_id, (h->type == 1) ? "[入库 +]" : "[出库 -]", h->quantity, h->time);
        h = h->next;
    }
    printf("----------------------------------------------------------------\n");
}

// ---------------------------------------------------------
// 业务三：药房物资增补入库 (写入物理库并产生审计流水)
// ---------------------------------------------------------
static void drugIn() {
    char id[30];
    printf("\n  扫描入库物资对应的系统内码(ID): ");
    safeGetString(id, 30);

    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            printf("  [目标确认] 当前资产存量为: %d 盒\n", p->stock);
            printf("  键入本次物资补给装填量: ");

            // 安全过滤：严格限制只能输入正整数，阻断恶意虚假账目
            int quantity = safeGetPositiveInt();

            p->stock += quantity;
            extern void getCurrentTimeStr(char* buffer, int size);
            getCurrentTimeStr(p->last_in, 30); // 更新最近入库快照

            // 头插法同步构建流转审计日志
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id, id);
            h->type = 1;
            h->quantity = quantity;
            getCurrentTimeStr(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;

            printf("  [√] 底层区块写入成功，该物资最新持有量刷新为: %d 盒\n", p->stock);
            return;
        }
        p = p->next;
    }
    printf("  [拦截] 系统抛出异常：无法锁定该 ID，请确认物资是否已在名录中建档。\n");
}

// ---------------------------------------------------------
// 业务四：库房人工强制调拨出库
// ---------------------------------------------------------
static void drugOut() {
    char id[30];
    printf("\n  核对需执行调拨的物资内码(ID): ");
    safeGetString(id, 30);

    Drug* p = drugList;
    while (p) {
        if (strcmp(p->id, id) == 0) {
            printf("  [数据镜像] 该资产可供调拨量为: %d 盒\n", p->stock);
            printf("  键入出库损耗配额: ");

            int quantity = safeGetPositiveInt();
            if (p->stock < quantity) {
                printf("  [熔断预警] 物理储备量无法覆盖本次调拨请求，操作已回滚！\n");
                return;
            }

            p->stock -= quantity;
            extern void getCurrentTimeStr(char* buffer, int size);
            getCurrentTimeStr(p->last_out, 30);

            // 建立追溯凭证
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            strcpy(h->drug_id, id);
            h->type = 2; // 状态2映射出库动作
            h->quantity = quantity;
            getCurrentTimeStr(h->time, 30);
            h->next = drugHistoryList;
            drugHistoryList = h;

            printf("  [√] 账目平衡扣除成功，核心账本库存结余: %d 盒\n", p->stock);
            return;
        }
        p = p->next;
    }
    printf("  [拦截] 字典树中不存在此物料实体。\n");
}

// ---------------------------------------------------------
// 业务五：新品种物资入网建档
// ---------------------------------------------------------
void addDrug() {
    Drug d;
    printf("\n========== 新型医疗物资准入登记 ==========\n");
    printf("  赋予新物资唯一识别内码(ID): ");
    safeGetString(d.id, 15);

    // 前置状态锁：阻断主键重复写入
    Drug* check = drugList;
    while (check) {
        if (strcmp(check->id, d.id) == 0) {
            printf("  [违规操作] 系统底座已存在占用该 ID 的活跃数据，拒绝建档！\n");
            return;
        }
        check = check->next;
    }

    printf("  键入国家药监局备案名称: ");
    while (1) { safeGetString(d.name, 50); if (strlen(d.name) > 0) break; }

    printf("  初始化入网物理装填量: ");
    d.stock = safeGetInt(); // 允许为0初始库存
    if (d.stock < 0) d.stock = 0;

    printf("  设定系统核算基准单价: ");
    d.price = safeGetDouble();
    if (d.price < 0) d.price = 0.0;

    printf("  追溯批次核准代码: ");
    while (1) { safeGetString(d.batch, 20); if (strlen(d.batch) > 0) break; }

    printf("  限定生命周期失效阀值 (如2026-12-31): ");
    while (1) { safeGetString(d.expiry, 15); if (strlen(d.expiry) > 0) break; }

    extern void getCurrentTimeStr(char* buffer, int size);
    getCurrentTimeStr(d.last_in, 30); // 注入首次活跃时间戳
    strcpy(d.last_out, "-");

    Drug* node = (Drug*)malloc(sizeof(Drug));
    *node = d;
    node->next = NULL;

    if (!drugList) drugList = node;
    else {
        Drug* p = drugList;
        while (p->next) p = p->next;
        p->next = node;
    }
    printf("  [√] 物资拓扑图节点生成成功，已纳入全院监控网络。\n");
}

// ---------------------------------------------------------
// 高管管理端：药房核心业务集线器
// ---------------------------------------------------------
void drugMenu() {
    while (1) {
        system("cls");
        printf("\n==================================================\n");
        printf("                 全院药剂耗材管理总机                 \n");
        printf("==================================================\n");
        printf("  [1] 药品库存拓扑检索\n");
        printf("  [2] 院外物资采购入库\n");
        printf("  [3] 院内耗材强制调拨\n");
        printf("  [4] 提取出入库异动审计流水\n");
        printf("  [5] 新品种物资准入建档\n");
        printf("  [0] 断开链路，返回上级枢纽\n");
        printf("--------------------------------------------------\n");
        printf("  传输执行动作编码: ");

        int choice = safeGetInt(); // 防控无效字母与溢出操作
        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); system("pause"); break;
        case 3: drugOut(); system("pause"); break;
        case 4: viewStockRecords(); system("pause"); break;
        case 5: addDrug(); system("pause"); break;
        case 0: return;
        }
    }
}