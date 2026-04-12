#define _CRT_SECURE_NO_WARNINGS
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "utils.h"
#include "fileio.h"
#include "time_t.h"

Drug* drugList;
DrugHistory* drugHistoryList;

static void printDrugTableHeader(void) {
    printf("\n---------------------------------------------------------------------------------------------------------------\n");
    printf("%-6s %-20s %-10s %-12s %-14s %-14s %-20s %-20s\n",
        "编号", "药品名称", "库存", "单价", "批号", "有效期", "最近入库时间", "最近出库时间");
    printf("---------------------------------------------------------------------------------------------------------------\n");
}

static void printDrugRow(const Drug* d) {
    char priceText[32];
    if (!d) return;
    formatMoney(d->price, priceText, sizeof(priceText));
    printf("%-6d %-20.20s %-10d %-12s %-14.14s %-14.14s %-20.20s %-20.20s\n",
        d->id, d->name, d->stock, priceText, d->batch, d->expiry, d->last_in, d->last_out);
}

static void printDrugHistoryHeader(void) {
    printf("\n--------------------------------------------------------------\n");
    printf("%-10s %-10s %-10s %-24s\n", "药品编号", "变动类型", "数量", "时间");
    printf("--------------------------------------------------------------\n");
}

Drug* findDrugById(int id) {
    Drug* p = drugList ? drugList->next : NULL;
    while (p) {
        if (p->id == id) return p;
        p = p->next;
    }
    return NULL;
}

Drug* findDrugByNameExact(const char* name) {
    Drug* p;
    if (!name || !drugList) return NULL;
    p = drugList->next;
    while (p) {
        if (strcmp(p->name, name) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int findDrugsByNameFuzzy(const char* keyword, Drug** results, int maxResults) {
    int count = 0;
    Drug* p;
    if (!keyword || !results || maxResults <= 0 || !drugList) return 0;
    p = drugList->next;
    while (p) {
        if (strstr(p->name, keyword)) {
            results[count++] = p;
            if (count >= maxResults) break;
        }
        p = p->next;
    }
    return count;
}

static int isDrugIdExists(int id) {
    return findDrugById(id) != NULL;
}

static void displayAllDrugs(void) {
    Drug* p;
    if (!drugList || !(drugList->next)) {
        printf("药品库为空。\n");
        return;
    }
    printf("\n--- 药品列表 ---\n");
    printDrugTableHeader();
    p = drugList->next;
    while (p) {
        printDrugRow(p);
        p = p->next;
    }
    printf("---------------------------------------------------------------------------------------------------------------\n");
}

static void printSingleDrug(const Drug* p) {
    if (!p) return;
    printDrugTableHeader();
    printDrugRow(p);
    printf("---------------------------------------------------------------------------------------------------------------\n");
}

static void drugStockQuery(void) {
    int choice;
    printf("\n药品库存查询:\n1-按ID精确查询\n2-按名称精确查询\n3-按名称模糊查询\n4-查看所有药品\n-1-返回上一步\n请选择: ");

    while (1) {
        choice = safeGetInt();
        if (choice == -1 || (choice >= 1 && choice <= 4)) break;
        printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
    }

    if (choice == -1) return;

    if (choice == 1) {
        int id;
        Drug* p;
        printf("请输入药品ID (输入-1取消): ");
        id = safeGetPositiveInt();
        if (id == -1) return;
        p = findDrugById(id);
        if (p) printSingleDrug(p);
        else printf("  [!] 未找到该药品。\n");
    }
    else if (choice == 2) {
        char name[50];
        Drug* p;
        printf("请输入药品名称: ");
        safeGetString(name, sizeof(name));
        if (strlen(name) == 0) {
            printf("  [!] 输入不能为空。\n");
        }
        else {
            p = findDrugByNameExact(name);
            if (p) printSingleDrug(p);
            else printf("  [!] 未找到该药品。\n");
        }
    }
    else if (choice == 3) {
        char name[50];
        Drug* results[100];
        int found;
        printf("请输入药品名称关键字: ");
        safeGetString(name, sizeof(name));
        if (strlen(name) == 0) {
            printf("  [!] 输入不能为空。\n");
        }
        else {
            found = findDrugsByNameFuzzy(name, results, 100);
            if (!found) {
                printf("  [!] 未找到匹配药品。\n");
            }
            else {
                printDrugTableHeader();
                for (int i = 0; i < found; ++i) printDrugRow(results[i]);
                printf("---------------------------------------------------------------------------------------------------------------\n");
            }
        }
    }
    else if (choice == 4) {
        displayAllDrugs();
    }
    system("pause");
}

static void viewStockRecords(void) {
    DrugHistory* h = drugHistoryList ? drugHistoryList->next : NULL;
    printf("\n--- 库存变动记录 ---\n");
    if (!h) { printf("暂无记录。\n"); system("pause"); return; }
    printDrugHistoryHeader();
    while (h) {
        printf("%-10d %-10s %-10d %-24s\n", h->drug_id, (h->type == 1) ? "入库" : "出库", h->quantity, h->time);
        h = h->next;
    }
    printf("--------------------------------------------------------------\n");
    system("pause");
}

static void drugIn(void) {
    int id;
    Drug* p;
    printf("请输入药品ID (输入-1取消): ");
    id = safeGetPositiveInt();
    if (id == -1) return;

    p = findDrugById(id);
    if (!p) {
        printf("  [!] 未找到该药品。\n");
        system("pause");
        return;
    }

    printf("当前库存: %d\n", p->stock);
    printf("请输入入库数量: ");
    while (1) {
        int quantity = safeGetDrugQuantity();
        if (quantity == -1) return;
        if (p->stock > INT_MAX - quantity) {
            printf("  [!] 入库后库存将超过允许范围，请重新输入更小的数量: ");
            continue;
        }
        p->stock += quantity;
        getCurrentTimeStr(p->last_in, sizeof(p->last_in));
        DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
        if (!h) {
            printf("  [!] 内存分配失败。\n");
            system("pause");
            return;
        }
        h->drug_id = id; h->type = 1; h->quantity = quantity;
        getCurrentTimeStr(h->time, sizeof(h->time));
        h->next = drugHistoryList->next; drugHistoryList->next = h;
        printf("  [√] 入库成功，新库存: %d\n", p->stock);
        saveDrugs();
        saveDrugHistory();
        system("pause");
        return;
    }
}

static void drugOut(void) {
    int id;
    Drug* p;
    printf("请输入药品ID (输入-1取消): ");
    id = safeGetPositiveInt();
    if (id == -1) return;

    p = findDrugById(id);
    if (!p) {
        printf("  [!] 未找到该药品。\n");
        system("pause");
        return;
    }

    printf("当前库存: %d\n", p->stock);
    printf("请输入出库数量: ");
    while (1) {
        int quantity = safeGetDrugQuantity();
        if (quantity == -1) return;
        if (p->stock < quantity) {
            printf("  [!] 库存不足，请重新输入不超过当前库存的数量: ");
            continue;
        }
        p->stock -= quantity;
        getCurrentTimeStr(p->last_out, sizeof(p->last_out));
        DrugHistory* h = (DrugHistory*)malloc(sizeof(DrugHistory));
        if (!h) {
            printf("  [!] 内存分配失败。\n");
            system("pause");
            return;
        }
        h->drug_id = id; h->type = 2; h->quantity = quantity;
        getCurrentTimeStr(h->time, sizeof(h->time));
        h->next = drugHistoryList->next; drugHistoryList->next = h;
        printf("  [√] 出库成功，现库存: %d\n", p->stock);
        saveDrugs();
        saveDrugHistory();
        system("pause");
        return;
    }
}

void addDrug(void) {
    Drug* d = (Drug*)malloc(sizeof(Drug));
    if (!d) { printf("  [!] 内存分配失败，无法新增药品。\n"); return; }

    while (1) {
        printf("请输入新药品ID (输入-1取消): ");
        d->id = safeGetPositiveInt();
        if (d->id == -1) { free(d); return; }

        if (isDrugIdExists(d->id)) {
            printf("  [!] ID已存在，请重新输入！\n");
        }
        else {
            break;
        }
    }

    while (1) {
        printf("请输入药品名称: ");
        safeGetString(d->name, sizeof(d->name));
        if (strlen(d->name) > 0) break;
        printf("  [!] 药品名称不能为空，请重新输入。\n");
    }
    d->stock = 0;

    printf("请输入药品单价: ");
    d->price = safeGetMoneyInRange(0.0, 1000000.0);
    if (d->price == -1.0) { free(d); return; }

    while (1) {
        printf("请输入药品批次: ");
        safeGetString(d->batch, sizeof(d->batch));
        if (strlen(d->batch) > 0) break;
        printf("  [!] 药品批次不能为空，请重新输入。\n");
    }

    printf("请输入药品有效期,输入格式为(XXXX-YY-ZZ): ");
    judgetime(d->expiry);
    if (strcmp(d->expiry, "-1") == 0) { free(d); return; }

    getCurrentTimeStr(d->last_in, sizeof(d->last_in));
    strcpy(d->last_out, "无");

    d->next = NULL;
    if (drugList->next == NULL) {
        drugList->next = d;
    }
    else {
        Drug* p = drugList->next;
        while (p->next) p = p->next;
        p->next = d;
    }

    saveDrugs();
    printf("  [√] 药品新增成功，如要补充数量，请进行入库操作。\n");
    system("pause");
}

void drugMenu(void) {
    int choice;
    do {
        system("cls");
        printf("\n========== 药房管理 ==========\n");
        printf("1. 药品库存查询\n");
        printf("2. 药品入库\n");
        printf("3. 药品出库\n");
        printf("4. 查看库存变动记录\n");
        printf("5. 新增药品种类\n");
        printf("-1. 返回主菜单\n");
        printf("请选择: ");

        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
        }

        switch (choice) {
        case 1: drugStockQuery(); break;
        case 2: drugIn(); break;
        case 3: drugOut(); break;
        case 4: viewStockRecords(); break;
        case 5: addDrug(); break;
        case -1: break;
        }
    } while (choice != -1);
}
