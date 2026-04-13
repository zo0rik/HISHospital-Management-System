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

// 交易流水链表头节点
Transaction* transactionList = NULL;
// 人员报表链表头节点
PersonnelReport* personnelReportList = NULL;

// 安全添加交易记录（自动生成ID，防止重复）
int appendTransactionSafe(int type, double amount, const char* description) {
    Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
    if (!newTrans) return 0;

    // 遍历获取最大ID，保证新记录ID唯一
    int maxId = 0;
    for (Transaction* curr = transactionList ? transactionList->next : NULL; curr != NULL; curr = curr->next) {
        if (curr->id > maxId) maxId = curr->id;
    }

    // 赋值交易基本信息
    newTrans->id = maxId + 1;
    newTrans->type = type;
    newTrans->amount = amount;
    getCurrentTimeStr(newTrans->time, 30);

    // 处理备注信息，安全复制
    if (description) {
        strncpy(newTrans->description, description, sizeof(newTrans->description) - 1);
        newTrans->description[sizeof(newTrans->description) - 1] = '\0';
    }
    else {
        newTrans->description[0] = '\0';
    }

    newTrans->next = NULL;

    // 头节点不存在则先创建
    if (!transactionList) {
        transactionList = (Transaction*)calloc(1, sizeof(Transaction));
        if (!transactionList) {
            free(newTrans);
            return 0;
        }
    }

    // 尾插法添加到链表
    Transaction* curr = transactionList;
    while (curr->next) curr = curr->next;
    curr->next = newTrans;
    return 1;
}

// 添加交易记录（对外简易接口）
void appendTransaction(int type, double amount, const char* description) {
    (void)appendTransactionSafe(type, amount, description);
}

// 获取交易类型中文名称
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

// 判断是否为营业收入类型（押金/充值不算营收）
static int isIncomeTransactionType(int type) {
    return type == TRANS_OUTPATIENT_INCOME ||
        type == TRANS_INPATIENT_BED_INCOME ||
        type == TRANS_DRUG_INCOME ||
        type == TRANS_EXAM_INCOME;
}

// 显示所有交易流水
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

    // 遍历打印所有流水
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

// 按时间段生成财务统计报表
static void showFinancialReport(void) {
    char start[20], end[20];
    double total_outpatient = 0.0;
    double total_bed = 0.0;
    double total_drug = 0.0;
    double total_exam = 0.0;

    // 输入并校验时间区间
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

    // 按类型统计营收
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

    // 输出统计结果
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

// 生成医生接诊量统计报表
void parttimereport(char* start, char* end) {
    if (personnelReportList == NULL) {
        return;
    }
    // 就诊记录为空直接返回
    if (recordHead->next == NULL) {
        return;
    }

    Record* r = recordHead->next;
    while (r) {
        // 只统计在时间范围内的记录
        if (strncmp(r->createTime, start, 10) >= 0 && strncmp(r->createTime, end, 10) <= 0) {
            int flag = 0;
            PersonnelReport* pr = personnelReportList->next;
            // 判断该医生是否已在报表中
            while (pr) {
                if (strcmp(pr->doctor_id, r->staffId) == 0) {
                    pr->count++;
                    flag = 1;
                    break;
                }
                pr = pr->next;
            }

            // 不存在则新建报表项
            if (flag == 0) {
                PersonnelReport* new_pr = (PersonnelReport*)malloc(sizeof(PersonnelReport));
                strcpy(new_pr->doctor_id, r->staffId);

                // 赋默认值，防止乱码
                strcpy(new_pr->department, "未知科室");
                strcpy(new_pr->doctor_name, "未知医生");

                // 从职工链表匹配医生信息
                if (staffHead != NULL && staffHead->next != NULL) {
                    Staff* s = staffHead->next;
                    while (s) {
                        if (strcmp(s->id, r->staffId) == 0|| strcmp(s->id + 1, r->staffId) == 0) {
                            strcpy(new_pr->department, s->department);
                            strcpy(new_pr->doctor_name, s->name);
                            break;
                        }
                        s = s->next;
                    }
                }

                new_pr->count = 1;
                new_pr->next = personnelReportList->next;
                personnelReportList->next = new_pr;
            }
        }
        r = r->next;
    }
}

// 只显示药品收入流水
static void showDrugOnlyTransactions(void) {
    system("cls");
    printf("请选择查看方式：1.查看所有药物流水 2.按时间筛选药物流水\n输入-1返回上级菜单\n请选择: ");
    int choice = safeGetInt();
    if (choice == -1) return;
    // 输入合法性校验
    while (1) {
        if (choice == 1 || choice == 2) break;
        else {
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请选择查看方式：1.查看所有流水 2.按时间筛选流水\n输入-1返回上级菜单\n请选择: ");
            choice = safeGetInt();
            if (choice == -1) return;
        }
    }

    int found = 0;
    if (choice == 2) {
        char start[20], end[20];
        // 时间区间输入与校验
        while (1) {
            // 起始日期不能晚于结束日期
            printf("\n请输入起始日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(start);
            if (strcmp(start, "-1") == 0) return;
            printf("请输入结束日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(end);
            if (strcmp(end, "-1") == 0) return;
            if (strcmp(start, end) > 0) {
                printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
            }
            else {
                break;
            }
        }
        printf("\n========== 药物类业务流水 ==========（从 %s 到 %s）\n", start, end);
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        // 筛选并打印药品流水
        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_DRUG_INCOME && strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
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
            printf("该时间段内暂无药物类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }

    else {
        // 查看全部药品流水
        printf("\n========== 药物类业务流水 ==========\n");
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_DRUG_INCOME) {
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
            printf("暂无药物类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    system("pause");
}

// 只显示检查类流水
static void showExamOnlyTransactions(void) {
    system("cls");
    printf("请选择查看方式：1.查看所有检查流水 2.按时间筛选检查流水\n输入-1返回上级菜单\n请选择: ");
    int choice = safeGetInt();
    if (choice == -1) return;
    while (1) {
        if (choice == 1 || choice == 2) break;
        else {
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请选择查看方式：1.查看所有流水 2.按时间筛选流水\n输入-1返回上级菜单\n请选择: ");
            choice = safeGetInt();
            if (choice == -1) return;
        }
    }

    int found = 0;
    if (choice == 2) {
        char start[20], end[20];
        while (1) {
            printf("\n请输入起始日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(start);
            if (strcmp(start, "-1") == 0) return;
            printf("请输入结束日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(end);
            if (strcmp(end, "-1") == 0) return;
            if (strcmp(start, end) > 0) {
                printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
            }
            else {
                break;
            }
        }
        printf("\n========== 检查类业务流水 ==========（从 %s 到 %s）\n", start, end);
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_EXAM_INCOME && strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
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
            printf("该时间段内暂无检查类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }

    else {
        printf("\n========== 检查类业务流水 ==========\n");
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_EXAM_INCOME) {
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
            printf("暂无检查类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    system("pause");
}

// 只显示床位费流水
static void showBedOnlyTransactions(void) {
    system("cls");
    printf("请选择查看方式：1.查看所有床位流水 2.按时间筛选床位流水\n输入-1返回上级菜单\n请选择: ");
    int choice = safeGetInt();
    if (choice == -1) return;
    while (1) {
        if (choice == 1 || choice == 2) break;
        else {
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请选择查看方式：1.查看所有流水 2.按时间筛选流水\n输入-1返回上级菜单\n请选择: ");
            choice = safeGetInt();
            if (choice == -1) return;
        }
    }

    int found = 0;
    if (choice == 2) {
        char start[20], end[20];
        while (1) {

            printf("\n请输入起始日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(start);
            if (strcmp(start, "-1") == 0) return;
            printf("请输入结束日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(end);
            if (strcmp(end, "-1") == 0) return;
            if (strcmp(start, end) > 0) {
                printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
            }
            else {
                break;
            }
        }
        printf("\n========== 住院床位费业务流水 ==========（从 %s 到 %s）\n", start, end);
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_INPATIENT_BED_INCOME && strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
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
            printf("该时间段内暂无床位类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    else {
        printf("\n========== 住院床位费业务流水 ==========\n");
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_INPATIENT_BED_INCOME) {
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
            printf("暂无床位类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    system("pause");
}

// 只显示门诊流水
static void showDoorOnlyTransactions(void) {
    system("cls");
    printf("请选择查看方式：1.查看所有门诊流水 2.按时间筛选门诊流水\n输入-1返回上级菜单\n请选择: ");
    int choice = safeGetInt();
    if (choice == -1) return;
    while (1) {
        if (choice == 1 || choice == 2) break;
        else {
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请选择查看方式：1.查看所有流水 2.按时间筛选流水\n输入-1返回上级菜单\n请选择: ");
            choice = safeGetInt();
            if (choice == -1) return;
        }
    }

    int found = 0;
    if (choice == 2) {
        char start[20], end[20];
        while (1) {
            printf("\n请输入起始日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(start);
            if (strcmp(start, "-1") == 0) return;
            printf("请输入结束日期 (YYYY-MM-DD, 输入-1取消): ");
            judgetime(end);
            if (strcmp(end, "-1") == 0) return;
            if (strcmp(start, end) > 0) {
                printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
            }
            else {
                break;
            }
        }
        printf("\n========== 门诊业务流水 ==========（从 %s 到 %s）\n", start, end);
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_OUTPATIENT_INCOME && strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
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
            printf("该时间段内暂无门诊类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    else {
        printf("\n========== 门诊业务流水 ==========\n");
        printf("%-8s %-14s %-12s %-12s %s\n", "ID", "类型", "金额", "时间", "说明");
        printf("-------------------------------------------------------------------------------\n");

        for (Transaction* t = transactionList ? transactionList->next : NULL; t != NULL; t = t->next) {
            if (t->type == TRANS_OUTPATIENT_INCOME) {
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
            printf("暂无门诊类流水记录。\n");
        }

        printf("-------------------------------------------------------------------------------\n");
    }
    system("pause");
}

// 财务报表菜单主入口
void reportMenu(void) {
    while (1) {
        system("cls");
        printf("\n========== 财务报表与流水中心 ==========\n");
        printf("  [1] 查看全部业务流水\n");
        printf("  [2] 查看药物类流水\n");
        printf("  [3] 查看检查类流水\n");
        printf("  [4] 查看住院床位费流水\n");
        printf("  [5] 查看门诊业务流水\n");
        printf("  [6] 查看区间财务报表\n");
        printf("  [-1] 返回上级菜单\n");
        printf("----------------------------------------\n");
        printf("请选择: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        // 菜单分发
        switch (choice) {
        case 1:
            showAllTransactions();
            break;
        case 2:
            showDrugOnlyTransactions();
            break;
        case 3:
            showExamOnlyTransactions();
            break;
        case 4:
            showBedOnlyTransactions();
            break;
        case 5:
            showDoorOnlyTransactions();
            break;
        case 6:
            showFinancialReport();
            break;
        default:
            printf("  [!] 无效选项！\n");
            system("pause");
            break;
        }
    }
}