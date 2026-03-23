#define _CRT_SECURE_NO_WARNINGS
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Transaction* transactionList = NULL;

void loadTransactions() {
    FILE* fp = fopen("transactions.txt", "r");
    if (!fp) return;

    char line[512];
    Transaction t;
    Transaction* tail = NULL;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* token = strtok(line, ",");
        if (token) t.id = atoi(token); else t.id = 0;
        token = strtok(NULL, ",");
        if (token) t.type = atoi(token); else t.type = 0;
        token = strtok(NULL, ",");
        if (token) t.amount = atof(token); else t.amount = 0.0;
        token = strtok(NULL, ",");
        if (token) strcpy(t.time, token); else t.time[0] = '\0';
        token = strtok(NULL, ",");
        if (token) strcpy(t.description, token); else t.description[0] = '\0';

        Transaction* node = (Transaction*)malloc(sizeof(Transaction));
        *node = t;
        node->next = NULL;
        if (!transactionList) transactionList = tail = node;
        else { tail->next = node; tail = node; }
    }
    fclose(fp);
}
void saveTransactions() {
    FILE* fp = fopen("transactions.txt", "w");
    if (!fp) return;
    Transaction* p = transactionList;
    while (p) {
        fprintf(fp, "%d,%d,%.2f,%s,%s\n", p->id, p->type, p->amount, p->time, p->description);
        p = p->next;
    }
    fclose(fp);
}

static void showFinancialReport() {
    char start[30], end[30];
    float total_outpatient = 0, total_inpatient = 0, total_drug = 0;
    printf("请输入统计起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入统计结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    Transaction* t = transactionList;
    while (t) {
        if (strcmp(t->time, start) >= 0 && strcmp(t->time, end) <= 0) {
            if (t->type == 1) total_outpatient += t->amount;
            else if (t->type == 2) total_inpatient += t->amount;
            else if (t->type == 3) total_drug += t->amount;
        }
        t = t->next;
    }
    printf("\n========== 财务报表 (%s 至 %s) ==========\n", start, end);
    printf("门诊收入: %.2f\n", total_outpatient);
    printf("住院收入: %.2f\n", total_inpatient);
    printf("药品收入: %.2f\n", total_drug);
    printf("总计: %.2f\n", total_outpatient + total_inpatient + total_drug);
    printf("\n（可导出报表，此处仅为预览）\n");
}

static void showPersonnelReport() {
    char start[30], end[30];
    printf("请输入统计起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入统计结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    printf("\n========== 人事报表 (%s 至 %s) ==========\n", start, end);
    printf("%-15s %-12s %-12s %-10s\n", "医生姓名", "科室", "接诊量", "开单量");
    // 这里简单模拟，实际应从交易记录中统计
    printf("（此处为演示，实际应根据交易记录统计）\n");
    printf("\n（可导出报表，此处仅为预览）\n");
}

static void showBusinessReport() {
    char start[30], end[30];
    int type;
    printf("请输入查询起始日期 (YYYY-MM-DD): "); scanf("%s", start);
    printf("请输入查询结束日期 (YYYY-MM-DD): "); scanf("%s", end);
    printf("请选择业务类型:\n1-门诊流水\n2-住院流水\n3-药品流水\n4-全部\n");
    scanf("%d", &type);
    printf("\n========== 诊疗流水 (%s 至 %s) ==========\n", start, end);
    Transaction* t = transactionList;
    int found = 0;
    while (t) {
        if (strcmp(t->time, start) >= 0 && strcmp(t->time, end) <= 0) {
            if (type == 4 || t->type == type) {
                printf("%-20s %-12s %-10.2f %s\n", t->time,
                    (t->type == 1) ? "门诊" : (t->type == 2) ? "住院" : "药品",
                    t->amount, t->description);
                found = 1;
            }
        }
        t = t->next;
    }
    if (!found) printf("无相关记录。\n");
    printf("\n（可导出明细，此处仅为预览）\n");
}

void reportMenu() {
    int choice;
    do {
        printf("\n========== 统计报表中心 ==========\n");
        printf("1. 财务报表\n");
        printf("2. 人事报表\n");
        printf("3. 业务报表\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: showFinancialReport(); break;
        case 2: showPersonnelReport(); break;
        case 3: showBusinessReport(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}