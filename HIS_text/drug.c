#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h" 
#include "fileio.h"
#include "time_t.h"

Drug* drugList;
DrugHistory* drugHistoryList;

static int isDrugIdExists(int id) {
    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) return 1;
        p = p->next;
    }
    return 0;
}

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

static void drugStockQuery() {
    int choice;
    printf("\n药品库存查询:\n1-按ID查询\n2-按名称模糊查询\n3-查看所有药品\n0-返回上一步\n请选择: ");

    while (1) {
        choice = safeGetInt();
        if (choice >= 0 && choice <= 3) break;
        printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
    }

    if (choice == 0) return;

    if (choice == 1) {
        printf("请输入药品ID: ");
        int id = safeGetPositiveInt();
        int found = 0;
        Drug* p = drugList->next;
        while (p != NULL) {
            if (p->id == id) {
                printf("ID:%d 名称:%s 库存:%d 价格:%.2f 批号:%s 有效期:%s\n",
                    p->id, p->name, p->stock, p->price, p->batch, p->expiry);
                found = 1;
                break;
            }
            p = p->next;
        }
        if (found == 0) printf("  [!] 未找到该药品。\n");
    }
    else if (choice == 2) {
        char name[50];
        printf("请输入药品名称关键字: ");
        safeGetString(name, 50);
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
        if (!found) printf("  [!] 未找到匹配药品。\n");
    }
    else if (choice == 3) {
        displayAllDrugs();
    }
    system("pause");
}

static void viewStockRecords() {
    printf("\n--- 库存变动记录 ---\n");
    DrugHistory* h = drugHistoryList->next;
    if (!h) { printf("暂无记录。\n"); system("pause"); return; }
    printf("%-8s %-6s %-8s %-30s\n", "药品ID", "类型", "数量", "时间");
    while (h) {
        printf("%-8d %-6s %-8d %-30s\n", h->drug_id, (h->type == 1) ? "入库" : "出库", h->quantity, h->time);
        h = h->next;
    }
    printf("----------------------------------------------------------------\n");
    system("pause");
}

static void drugIn() {
    printf("请输入药品ID (输入0取消): ");
    int id = safeGetPositiveInt();
    if (id == 0) return;

    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) {
            printf("当前库存: %d\n", p->stock);
            printf("请输入入库数量: ");
            int quantity = safeGetPositiveInt();
            if (quantity <= 0) return;

            p->stock += quantity;
            getCurrentTimeStr(p->last_in, 30);
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            h->drug_id = id; h->type = 1; h->quantity = quantity;
            getCurrentTimeStr(h->time, 30);
            h->next = drugHistoryList->next; drugHistoryList->next = h;
            printf("  [√] 入库成功，新库存: %d\n", p->stock);
            saveDrugs();
            saveDrugHistory();
            system("pause");
            return;
        }
        p = p->next;
    }
    printf("  [!] 未找到该药品。\n");
    system("pause");
}

static void drugOut() {
    printf("请输入药品ID (输入0取消): ");
    int id = safeGetPositiveInt();
    if (id == 0) return;

    Drug* p = drugList->next;
    while (p) {
        if (p->id == id) {
            printf("当前库存: %d\n", p->stock);
            printf("请输入出库数量: ");
            int quantity = safeGetPositiveInt();
            if (quantity <= 0) return;

            if (p->stock < quantity) { printf("  [!] 库存不足！\n"); system("pause"); return; }
            p->stock -= quantity;
            getCurrentTimeStr(p->last_out, 30);
            DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
            h->drug_id = id; h->type = 2; h->quantity = quantity;
            getCurrentTimeStr(h->time, 30);
            h->next = drugHistoryList->next; drugHistoryList->next = h;
            printf("  [√] 出库成功，现库存: %d\n", p->stock);
            saveDrugs();
            saveDrugHistory();
            system("pause");
            return;
        }
        p = p->next;
    }
    printf("  [!] 未找到该药品。\n");
    system("pause");
}

void addDrug() {
    Drug* d = malloc(sizeof(Drug));
    /* 修复Bug：malloc失败时d为NULL，直接解引用会崩溃 */
    if (!d) { printf("  [!] 内存分配失败，无法新增药品。\n"); return; }

    while (1) {
        printf("请输入新药品ID (输入0取消): ");
        d->id = safeGetPositiveInt();
        if (d->id == 0) { free(d); return; }

        if (isDrugIdExists(d->id)) {
            printf("  [!] ID已存在，请重新输入！\n");
        }
        else {
            break;
        }
    }

    printf("请输入药品名称: "); safeGetString(d->name, 100);
    d->stock = 0;

    printf("请输入药品单价: "); d->price = safeGetDouble();

    printf("请输入药品批次: "); safeGetString(d->batch, 50);

    printf("请输入药品有效期,输入格式为(XXXX-YY-ZZ): "); judgetime(d->expiry);

    getCurrentTimeStr(d->last_in, 30);
    strcpy(d->last_out, "无"); // 修复写入空字符串引发塌方的Bug

    d->next = NULL;
    Drug* p = drugList->next;
    if (p == NULL) { drugList->next = d; }
    else { while (p->next) p = p->next; p->next = d; }

    saveDrugs();
    printf("  [√] 药品新增成功，如要补充数量，请进行入库操作。\n");
    system("pause");
}

void drugMenu() {
    int choice;
    do {
        system("cls");
        printf("\n========== 药房管理 ==========\n");
        printf("1. 药品库存查询\n");
        printf("2. 药品入库\n");
        printf("3. 药品出库\n");
        printf("4. 查看库存变动记录\n");
        printf("5. 新增药品种类\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice >= 0 && choice <= 5) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
        }

        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); break;
        case 3: drugOut(); break;
        case 4: viewStockRecords(); break;
        case 5: addDrug(); break;
        case 0: break;
        }
    } while (choice != 0);
}