#define _CRT_SECURE_NO_WARNINGS
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "fileio.h"
#include "utils.h" // 【核心】引入安全输入工具库
#include "time_t.h"

// 独立维护的全局业务流水与财务链表指针
Transaction* transactionList;
// 人事报表链表头指针
PersonnelReport* personnelReportList;

// 人事报表加载函数
void parttimereport(char* start, char* end) {
    if (personnelReportList == NULL) return;
    if (recordHead->next == NULL) return;

    Record* r = recordHead->next;
    while (r) {
        // 时间范围判断 (只看日期前10位: YYYY-MM-DD)
        if (strncmp(r->createTime, start, 10) >= 0 && strncmp(r->createTime, end, 10) <= 0) {
            int flag = 0;
            PersonnelReport* pr = personnelReportList->next;
            while (pr) {
                if (strcmp(pr->doctor_id, r->staffId) == 0) {
                    pr->count++;
                    flag = 1;
                    break;
                }
                pr = pr->next;
            }

            if (flag == 0) {
                PersonnelReport* new_pr = (PersonnelReport*)malloc(sizeof(PersonnelReport));
                strcpy(new_pr->doctor_id, r->staffId);
                strcpy(new_pr->department, "未知科室");
                strcpy(new_pr->doctor_name, "未知医生");

                if (staffHead != NULL) {
                    Staff* s = staffHead->next;
                    while (s) {
                        if (strcmp(s->id, r->staffId) == 0) {
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

// ---------------------------------------------------------
// 业务一：输出财务统计报表
// ---------------------------------------------------------
static void showFinancialReport() {
    char start[20], end[20];
    double total_outpatient = 0, total_inpatient = 0, total_drug = 0;

    while (1) {
        printf("\n请输入财务统计起始日期 (YYYY-MM-DD, 输入0取消): ");
        judgetime(start);
        if (strcmp(start, "0") == 0) return;

        printf("请输入财务统计结束日期 (YYYY-MM-DD, 输入0取消): ");
        judgetime(end);
        if (strcmp(end, "0") == 0) return;

        if (strcmp(start, end) > 0) {
            printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
        }
        else {
            break;
        }
    }

    Transaction* t = transactionList->next;
    while (t) {
        if (strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
            if (t->type == 1) total_outpatient += t->amount;
            else if (t->type == 2) total_inpatient += t->amount;
            else if (t->type == 3) total_drug += t->amount;
        }
        t = t->next;
    }

    printf("\n========== 财务报表预览 (%s 至 %s) ==========\n", start, end);
    printf("  [1] 门诊业务总收入:  ￥ %.2f\n", total_outpatient);
    printf("  [2] 住院业务总收入:  ￥ %.2f\n", total_inpatient);
    printf("  [3] 药房调拨总收入:  ￥ %.2f\n", total_drug);
    printf("----------------------------------------------\n");
    printf("  全院累计营业总额:    ￥ %.2f\n", total_outpatient + total_inpatient + total_drug);
    printf("\n  (报表申明：以上数据实时提取自底层财务交割链表)\n");
    system("pause");
}

// ---------------------------------------------------------
// 业务二：人事接诊/开单报表展示
// ---------------------------------------------------------
static void showPersonnelReport() {
    char start[20], end[20];
    while (1) {
        printf("\n请输入人事考核起始日期 (YYYY-MM-DD, 输入0取消): ");
        judgetime(start);
        if (strcmp(start, "0") == 0) return;

        printf("请输入人事考核结束日期 (YYYY-MM-DD, 输入0取消): ");
        judgetime(end);
        if (strcmp(end, "0") == 0) return;

        if (strcmp(start, end) > 0) {
            printf("  [!] 逻辑错误：起始日期不能晚于结束日期，请重新输入。\n");
        }
        else {
            break;
        }
    }

    printf("\n========== 人事绩效报表 (%s 至 %s) ==========\n", start, end);
    printf("%-15s | %-15s | %-10s\n", "医生姓名", "科室", "累计接诊量");
    printf("----------------------------------------------------------\n");

    parttimereport(start, end);

    PersonnelReport* cr = personnelReportList->next;
    if (cr == NULL) {
        printf("  [提示] 该统计范围内暂无任何医生的接诊活动记录。\n");
    }
    else {
        while (cr) {
            printf("%-15s | %-15s | %-10d\n", cr->doctor_name, cr->department, cr->count);
            cr = cr->next;
        }
    }

    // 释放本次生成的临时报表链表，防止内存泄漏
    PersonnelReport* p = personnelReportList->next;
    while (p) {
        PersonnelReport* temp = p;
        p = p->next;
        free(temp);
    }
    personnelReportList->next = NULL;

    printf("\n  (统计申明：接诊量包含门诊挂号、查房医嘱及开药记录的总和)\n");
    system("pause");
}

// ---------------------------------------------------------
// 业务三：业务流水明细查询
// ---------------------------------------------------------
static void showBusinessReport() {
    int type;
    while (1) {
        system("cls");
        printf("\n========== 业务明细流水穿透查询 ==========\n");
        printf("  [1] 仅看门诊流水\n");
        printf("  [2] 仅看住院流水\n");
        printf("  [3] 仅看药品流水\n");
        printf("  [4] 全院业务混合流水\n");
        printf("  [0] 返回上级菜单\n");
        printf("------------------------------------------\n");
        printf("  请选择查询类型: ");

        while (1) {
            type = safeGetInt();
            if (type >= 0 && type <= 4) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n  请重新选择: ");
        }

        if (type == 0) return;

        char start[20], end[20];
        while (1) {
            printf("\n请输入流水查询起始日期 (YYYY-MM-DD): ");
            judgetime(start);
            printf("请输入流水查询结束日期 (YYYY-MM-DD): ");
            judgetime(end);
            if (strcmp(start, end) > 0) {
                printf("  [!] 逻辑错误：起始日期不能晚于结束日期。\n");
            }
            else {
                break;
            }
        }

        printf("\n========== 诊疗明细流水清单 (%s 至 %s) ==========\n", start, end);
        printf("%-20s | %-8s | %-10s | %s\n", "发生时间戳", "分类", "金额", "业务描述");
        printf("--------------------------------------------------------------------------------\n");

        Transaction* t = transactionList->next;
        int found = 0;
        while (t) {
            if (strncmp(t->time, start, 10) >= 0 && strncmp(t->time, end, 10) <= 0) {
                if (type == 4 || t->type == type) {
                    char typeStr[10];
                    if (t->type == 1) strcpy(typeStr, "门诊");
                    else if (t->type == 2) strcpy(typeStr, "住院");
                    else strcpy(typeStr, "药品");

                    printf("%-20s | %-8s | %-10.2f | %s\n", t->time, typeStr, t->amount, t->description);
                    found = 1;
                }
            }
            t = t->next;
        }
        if (!found) printf("  [数据空置] 检索范围内未发现匹配的财务流水记录。\n");
        printf("--------------------------------------------------------------------------------\n");
        system("pause");
    }
}

// ---------------------------------------------------------
// 统计报表中心子菜单
// ---------------------------------------------------------
void reportMenu() {
    int choice;
    do {
        system("cls");
        printf("\n========== 医院经营数据统计报表中心 ==========\n");
        printf("  [1] 财务报表 (全院收入分类汇总)\n");
        printf("  [2] 人事报表 (临床医师接诊量考核)\n");
        printf("  [3] 业务报表 (诊疗项目明细流水)\n");
        printf("  [0] 返回高管控制大厅\n");
        printf("----------------------------------------------\n");
        printf("  请下达提取指令: ");

        // 【核心拦截】：使用 safeGetInt 并拦截非法菜单项
        while (1) {
            choice = safeGetInt();
            if (choice >= 0 && choice <= 3) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n  请重新下达指令: ");
        }

        switch (choice) {
        case 1: showFinancialReport(); break;
        case 2: showPersonnelReport(); break;
        case 3: showBusinessReport(); break;
        case 0: break;
        }
    } while (choice != 0);
}