#define _CRT_SECURE_NO_WARNINGS
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "fileio.h"
#include "utils.h"
#include "time_t.h"

Transaction* transactionList = NULL;
PersonnelReport* personnelReportList = NULL;

int appendTransactionSafe(int type, double amount, const char* description) {
    Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
    if (!newTrans) return 0;

    int maxId = 0;
    for (Transaction* curr = transactionList ? transactionList->next : NULL; curr != NULL; curr = curr->next) {
        if (curr->id > maxId) maxId = curr->id;
    }

    newTrans->id = maxId + 1;
    newTrans->type = type;
    newTrans->amount = amount;
    getCurrentTimeStr(newTrans->time, 30);

    if (description) {
        strncpy(newTrans->description, description, sizeof(newTrans->description) - 1);
        newTrans->description[sizeof(newTrans->description) - 1] = '\0';
    }
    else {
        newTrans->description[0] = '\0';
    }

    newTrans->next = NULL;

    if (!transactionList) {
        transactionList = (Transaction*)calloc(1, sizeof(Transaction));
        if (!transactionList) {
            free(newTrans);
            return 0;
        }
    }

    Transaction* curr = transactionList;
    while (curr->next) curr = curr->next;
    curr->next = newTrans;
    return 1;
}

void appendTransaction(int type, double amount, const char* description) {
    (void)appendTransactionSafe(type, amount, description);
}

static const char* getTransactionTypeName(int type) {
    switch (type) {
    case TRANS_OUTPATIENT_INCOME:    return "门诊收入";
    case TRANS_INPATIENT_BED_INCOME: return "床位费收入";
    case TRANS_DRUG_INCOME:          return "药品收入";
    case TRANS_EXAM_INCOME:          return "检查收入";
    case TRANS_RECHARGE:             return "账户充值";
    case TRANS_INPATIENT_DEPOSIT:    return "住院押金";
    case TRANS_INPATIENT_SUPPLEMENT: return "住院补缴";
    case TRANS_INPATIENT_REFUND:     return "押金退回";
    default:                         return "其他";
    }
}

static int isIncomeTransactionType(int type) {
    return type == TRANS_OUTPATIENT_INCOME ||
           type == TRANS_INPATIENT_BED_INCOME ||
           type == TRANS_DRUG_INCOME ||
           type == TRANS_EXAM_INCOME;
}

static void showAllTransactions(void) {
    system("cls");
    printf("\n========== 全部业务流水 ==========\n");
    printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
    printf("-------------------------------------------------------------------------------\n");

    if (!transactionList || !transactionList->next) {
        printf("暂无业务流水记录。\n");
        system("pause");
        return;
    }

    for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
        printf("%-8d %-14s %-12.2f %-12.12s %s\n",
            t->id,
            getTransactionTypeName(t->type),
            t->amount,
            t->time,
            t->description);
    }

    printf("-------------------------------------------------------------------------------\n");
    system("pause");
}

static void showFinancialReport(void) {
    char start[20], end[20];
    double total_outpatient = 0.0;
    double total_bed = 0.0;
    double total_drug = 0.0;
    double total_exam = 0.0;

    while (1) {
        printf("\n请输入财务统计起始日期 (YYYY-MM-DD, 输入-1取消): ");
        judgetime(start);
        if (strcmp(start, "-1") == 0) return;

        printf("请输入财务统计结束日期 (YYYY-MM-DD, 输入-1取消): ");
        judgetime(end);
        if (strcmp(end, "-1") == 0) return;

        if (strcmp(start, end) > 0) {
            printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
        }
        else {
            break;
        }
    }

    for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
        if (strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
            switch (t->type) {
            case TRANS_OUTPATIENT_INCOME:
                total_outpatient += t->amount;
                break;
            case TRANS_INPATIENT_BED_INCOME:
                total_bed += t->amount;
                break;
            case TRANS_DRUG_INCOME:
                total_drug += t->amount;
                break;
            case TRANS_EXAM_INCOME:
                total_exam += t->amount;
                break;
            default:
                break;
            }
        }
    }

    system("cls");
    printf("\n========== 财务报表预览 (%s 至 %s) ==========\n", start, end);
    printf("  [1] 门诊业务总收入:      ￥ %.2f\n", total_outpatient);
    printf("  [2] 住院床位费总收入:    ￥ %.2f\n", total_bed);
    printf("  [3] 药品总收入:          ￥ %.2f\n", total_drug);
    printf("  [4] 检查总收入:          ￥ %.2f\n", total_exam);
    printf("--------------------------------------------------\n");
    printf("  全院累计营业总额:        ￥ %.2f\n",
        total_outpatient + total_bed + total_drug + total_exam);
    printf("\n  (住院押金、补交欠费、退款均不计入营业收入)\n");
    system("pause");
}

void parttimereport(char* start, char* end) {
    double total_outpatient = 0.0;
    double total_bed = 0.0;
    double total_drug = 0.0;
    double total_exam = 0.0;

    for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
        if (strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
            switch (t->type) {
            case TRANS_OUTPATIENT_INCOME:
                total_outpatient += t->amount;
                break;
            case TRANS_INPATIENT_BED_INCOME:
                total_bed += t->amount;
                break;
            case TRANS_DRUG_INCOME:
                total_drug += t->amount;
                break;
            case TRANS_EXAM_INCOME:
                total_exam += t->amount;
                break;
            default:
                break;
            }
        }
    }

    printf("\n========== 分时段财务统计 (%s 至 %s) ==========\n", start, end);
    printf("门诊收入: %.2f\n", total_outpatient);
    printf("床位费收入: %.2f\n", total_bed);
    printf("药品收入: %.2f\n", total_drug);
    printf("检查收入: %.2f\n", total_exam);
    printf("总收入: %.2f\n", total_outpatient + total_bed + total_drug + total_exam);
}

static void showIncomeOnlyTransactions(void) {
    system("cls");
    printf("\n========== 收入类业务流水 ==========\n");
    printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
    printf("-------------------------------------------------------------------------------\n");

    int found = 0;
    for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
        if (isIncomeTransactionType(t->type)) {
            printf("%-8d %-14s %-12.2f %-12.12s %s\n",
                t->id,
                getTransactionTypeName(t->type),
                t->amount,
                t->time,
                t->description);
            found = 1;
        }
    }

    if (!found) {
        printf("暂无收入类流水记录。\n");
    }

    printf("-------------------------------------------------------------------------------\n");
    system("pause");
}

void reportMenu(void) {
    while (1) {
        system("cls");
        printf("\n========== 财务报表与流水中心 ==========\n");
        printf("  [1] 查看全部业务流水\n");
        printf("  [2] 查看收入类流水\n");
        printf("  [3] 查看区间财务报表\n");
        printf("  [-1] 返回上级菜单\n");
        printf("----------------------------------------\n");
        printf("请选择: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        switch (choice) {
        case 1:
            showAllTransactions();
            break;
        case 2:
            showIncomeOnlyTransactions();
            break;
        case 3:
            showFinancialReport();
            break;
        default:
            printf("  [!] 无效选项！\n");
            system("pause");
            break;
        }
    }
}