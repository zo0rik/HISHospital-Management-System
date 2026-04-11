#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h" // 强行引入安全输入防护组件

// 链表头指针
Drug* drugList;
DrugHistory* drugHistoryList;
//——————————————————————————————
// 辅助函数：检查药品ID是否已存在，防止重复添加
// ——————————————————————————————
static int isDrugIdExists(int id) {
    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) return 1;
        p = p->next;
    }
    return 0;
}
// ---------------------------------------------------------
//从文件加载药品数据到内存链表
// ---------------------------------------------------------
void loadDrugs() {
    FILE* fp = fopen("drugs.txt", "r");
    if (!fp) return;  // 无数据

    char line[512];
    Drug d;
    Drug* tail = NULL;
    // 用strtok函数按逗号分割每行数据，并依次赋值给药品结构体的字段
    while (fgets(line, sizeof(line), fp)) {
        // 除去行末的换行符，防止干扰字符串解析
        line[strcspn(line, "\n")] = 0;

        char* token = strtok(line, ",");
        if (token) d.id = atoi(token); else d.id = 0;//编号
        token = strtok(NULL, ",");
        if (token) strcpy(d.name, token); else d.name[0] = '\0';//名称
        token = strtok(NULL, ",");
        if (token) d.stock = atoi(token); else d.stock = 0;//库存
        token = strtok(NULL, ",");
        if (token) d.price = atof(token); else d.price = 0.0;//价格
        token = strtok(NULL, ",");
        if (token) strcpy(d.batch, token); else d.batch[0] = '\0';//批次
        token = strtok(NULL, ",");
        if (token) strcpy(d.expiry, token); else d.expiry[0] = '\0';//有效期
        token = strtok(NULL, ",");
        if (token) strcpy(d.last_in, token); else d.last_in[0] = '\0';//最新入库时间
        token = strtok(NULL, ",");
        if (token) strcpy(d.last_out, token); else d.last_out[0] = '\0';//最新出库时间

        // 写入链表,尾插法
        Drug* node = (Drug*)malloc(sizeof(Drug));
        *node = d;
        if (drugList->next == NULL) {
            drugList->next = node;
            tail = node;
        }
        else {
            tail->next = node;
            tail = node;
        }
    }
    tail->next = NULL;  // 确保链表末尾指针正确设置
    fclose(fp);
}
// ---------------------------------------------------------
// 保存链表中的药品数据到本地文件
// ---------------------------------------------------------
void saveDrugs() {
    FILE* fp = fopen("drugs.txt", "w");
    if (!fp) return;
    Drug* p = drugList->next;
    while (p) {
        fprintf(fp, "%d,%s,%d,%.2f,%s,%s,%s,%s\n",
            p->id, p->name, p->stock, p->price,
            p->batch, p->expiry, p->last_in, p->last_out);
        p = p->next;
    }
    fclose(fp);
}

// --------------------------------------------------------
//加载药品出入库历史记录到内存链表
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
        if (token) h.drug_id = atoi(token); else h.drug_id = 0;
        token = strtok(NULL, ",");
        if (token) h.type = atoi(token); else h.type = 0;

        token = strtok(NULL, ",");
        if (token) h.quantity = atoi(token); else h.quantity = 0;

        token = strtok(NULL, ",");
        if (token) strcpy(h.time, token); else h.time[0] = '\0';

        DrugHistory* node = (DrugHistory*)malloc(sizeof(DrugHistory));
        *node = h;
        node->next = NULL;
        if (drugHistoryList->next == NULL) {  // 判断链表有没有数据
            drugHistoryList->next = node;
            tail = node;
        }
        else {
            tail->next = node;
            tail = node;
        }
    }
    tail->next = NULL;  // 确保链表末尾指针正确设置
    fclose(fp);
}

// ---------------------------------------------------------
// 保存药品出入库历史记录到本地文件
// ---------------------------------------------------------
void saveDrugHistory() {
    FILE* fp = fopen("drug_history.txt", "w");
    if (!fp) return;
    DrugHistory* p = drugHistoryList->next;
    while (p) {
        fprintf(fp, "%d,%d,%d,%s\n", p->drug_id, p->type, p->quantity, p->time);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 展示所有药品库存信息
// ---------------------------------------------------------
static void displayAllDrugs() {
    if (!(drugList->next)) {
        printf("药品库为空。\n");
        return;
    }
    printf("\n--- 药品列表 ---\n");
    printf("%-5s %-20s %-10s %-8s %-12s %-12s %-20s %-20s\n", "ID", "名称", "库存", "价格", "批号", "有效期", "最近入库", "最近出库");
    Drug* p = drugList->next;
    while (p) {
        printf("%-5d %-20s %-10d %-8.2f %-12s %-12s %-20s %-30s\n",
            p->id, p->name, p->stock, p->price, p->batch, p->expiry,
            p->last_in, p->last_out);
        p = p->next;
    }
    printf("------------------------------------------------------------------------------------------------------------\n");
}

// ---------------------------------------------------------
// 药物库存查询模块：支持按ID、名称模糊搜索、或展示全部库存
// ---------------------------------------------------------
static void drugStockQuery() {
    int choice;
    printf("\n药品库存查询:\n1-按ID查询\n2-按名称模糊查询\n3-查看所有药品\n0-返回上一步\n请选择: ");
    if (scanf("%d", &choice) != 1)
        choice = -1;
    if (choice == 0)
        return;
    else if (choice == 1) {
        int id; printf("请输入药品ID: ");
        while (scanf("%d", &id) != 1 || choice < 0) {
            while (getchar() != '\n');
            printf("输入格式错误，请重新输入药品ID: ");
        }
        while (getchar() != '\n');//清空缓存区
        int found = 0;
        Drug* p = drugList->next;
        while (p != NULL) {
            if (p->id == id) {
                printf("ID:%d 名称:%s 库存:%d 价格:%.2f 批号:%s 有效期:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                found = 1;
                return;
            }
            p = p->next;
        }
        if (found == 0) {
            printf("未找到该药品。\n");
        }
    }
    else if (choice == 2) {
        char name[50]; printf("请输入药品名称关键字: "); scanf("%49s", name);while (getchar() != '\n');//清空缓存区
        int found = 0;
        Drug* p = drugList->next;
        while (p) {
            if (strstr(p->name, name)) {
                printf("ID:%d 名称:%s 库存:%d 价格:%.2f 批号:%s 有效期:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                found = 1;
            }
            p = p->next;
        }
        if (!found) printf("未找到匹配药品。\n");
    }
    else if (choice == 3) {
        displayAllDrugs();
    }
    else printf("无效选择。\n");
}
// ---------------------------------------------------------
// 查看药品出入库历史记录
// ---------------------------------------------------------
static void viewStockRecords() {
    printf("\n--- 库存变动记录 ---\n");
    DrugHistory* h = drugHistoryList->next;
    if (!h) { printf("暂无记录。\n"); return; }
    printf("%-8s %-6s %-8s %-30s\n", "药品ID", "类型", "数量", "时间");
    while (h) {
        printf("%-8d %-6s %-8d %-30s\n", h->drug_id,
            (h->type == 1) ? "入库" : "出库", h->quantity, h->time);
        h = h->next;
    }
    printf("----------------------------------------------------------------\n");
}

// ---------------------------------------------------------
// 药品入库模块：支持按ID入库(不使用名称以区分批次），并记录入库时间和数量变动历史
// ---------------------------------------------------------
static void drugIn() {
    int  id, quantity;
    printf("请输入药品ID: ");
    while (scanf("%d", &id) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入药品ID: ");
    }
    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) {
            printf("当前库存: %d\n", p->stock);
            printf("请输入入库数量: ");
            while (scanf("%d", &quantity) != 1) {
                while (getchar() != '\n');
                printf("输入格式错误，请重新输入入库数量: ");
            }
            if (quantity <= 0) { printf("数量必须为正。\n"); return; }
            p->stock += quantity;
            getCurrentTime(p->last_in, 30);
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            h->drug_id = id;
            h->type = 1;
            h->quantity = quantity;
            getCurrentTime(h->time, 30);
            h->next = drugHistoryList->next;
            drugHistoryList->next = h;
            printf("入库成功，新库存: %d\n", p->stock);
            saveDrugs();
            saveDrugHistory();
            return;
        }
        p = p->next;
    }
    printf("未找到该药品。\n");
}


// ---------------------------------------------------------
// 药品出库模块：支持按ID出库，并记录出库时间和数量变动历史，同时检查库存是否足够
// ---------------------------------------------------------
static void drugOut() {
    int id, quantity;
    printf("请输入药品ID: ");
    while (scanf("%d", &id) != 1) {
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入药品ID: ");
    }
    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) {
            printf("当前库存: %d\n", p->stock);
            printf("请输入出库数量: ");
            while (scanf("%d", &quantity) != 1) {
                while (getchar() != '\n');
                printf("输入格式错误，请重新输入出库数量: ");
            }
            if (quantity <= 0) { printf("数量必须为正。\n"); return; }
            if (p->stock < quantity) { printf("库存不足！\n"); return; }
            p->stock -= quantity;
            getCurrentTime(p->last_out, 15);
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            h->drug_id = id;
            h->type = 2;
            h->quantity = quantity;
            getCurrentTime(h->time, 15);
            h->next = drugHistoryList->next;
            drugHistoryList->next = h;
            printf("出库成功，现库存: %d\n", p->stock);
            saveDrugs();
            saveDrugHistory();
            return;
        }
        p = p->next;
    }
    printf("未找到该药品。\n");
}

// ---------------------------------------------------------
// 新增药物模块：支持输入药品基本信息并添加到链表，同时进行输入验证，确保数据完整性和合理性
//----------------------------------------------------------
void addDrug() {
    Drug* d = malloc(sizeof(Drug));
    printf("请输入药品ID: ");
    //id查重；
    while (1) {
        printf("请输入药品ID: ");
        while (scanf("%d", &d->id) != 1) {
            while (getchar() != '\n');
            printf("输入格式错误，请重新输入: ");
        }
        if (isDrugIdExists(d->id)) {
            printf("ID已存在，请重新输入！\n");
        }
        else {
            break;
        }
    }
    d->name[19] = '\0'; // 防止输入过长导致缓冲区溢出]='\0'
    printf("请输入药品名称: ");
    while (1) {
        scanf("%20s", d->name);
        if (d->name[19] == '\0')
            break;
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入药品名称: ");
    }
    d->stock = 0;//初始库存设为零；
    printf("请输入药品价格： ");
    while (1) {
        if (scanf("%f", &d->price) == 1)
            break;
        while (getchar() != '\n');
        printf("输入格式错误，请重新输入: ");
    }
    printf("请输入药品批次: ");
    scanf("%19s", d->batch);while (getchar() != '\n');//清空输入缓冲区
    printf("请输入药品有效期,输入格式为(XXXX-YY-ZZ): ");
    judgetime(d->expiry);// 输入格式判断
    getCurrentTime(d->last_in, 30); // 获取当前时间作为入库时间
    strcpy(d->last_out, ""); // 出库时间初始为空
    //加入药品链表
    d->next = NULL;
    Drug* p = drugList->next;

    if (p == NULL) {
        drugList->next = d;
    }
    else {
        while (p->next) p = p->next;
        p->next = d;
    }
    saveDrugs();
    printf("药品新增成功，如要补充数量，请进行入库操作\n");
}

// ---------------------------------------------------------
// 药品总路由菜单：提供药品库存查询、入库、出库、历史记录查看、新增药品等功能的入口，支持循环操作直到用户选择退出
// ---------------------------------------------------------
void drugMenu() {
    int choice;
    do {
        printf("\n========== 药房管理 ==========\n");
        printf("1. 药品库存查询\n");
        printf("2. 药品入库\n");
        printf("3. 药品出库\n");
        printf("4. 查看库存变动记录\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        if (scanf("%d", &choice) != 1) {
            choice = -1;
            while (getchar() != '\n');
        }
        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); break;
        case 3: drugOut(); break;
        case 4: viewStockRecords(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}