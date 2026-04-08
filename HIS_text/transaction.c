#define _CRT_SECURE_NO_WARNINGS
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"

// 独立维护的全局业务流水与财务链表指针
Transaction* transactionList;
// 人事报表链表头指针
PersonnelReport* personnelReportList;
// 人事报表加载函数
void parttimereport(char* start, char* end) {
    // ==========先判断链表头是否存在，防止空指针崩溃 ==========
    if (personnelReportList==NULL) {
        return;
    }
    // ==========判断记录链表是否为空 ==========
    if (recordHead->next== NULL) {
        return;
    }

    Record* r = recordHead->next;
    while (r) {
        // 时间范围判断
        if (strncmp(r->createTime, start,10) >= 0 && strncmp(r->createTime, end,10) <= 0) {
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

                // 默认值先填空，防止乱码
                strcpy(new_pr->department, "未知科室");
                strcpy(new_pr->doctor_name, "未知医生");

                // ========== 修复 3：判断员工链表是否为空 ==========
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
// 加载本地财务交易记录
// ---------------------------------------------------------
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
        // type: 1=门诊收入, 2=住院收入, 3=药品收入
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

        // 尾插法挂载节点
        // 尾插法挂载节点
        if (transactionList->next == NULL) {
            transactionList->next = node;
            tail = node;
        }
        else {
            tail->next = node;
            tail = node;
        }
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 财务交易记录持久化保存
// ---------------------------------------------------------
void saveTransactions() {
    FILE* fp = fopen("transactions.txt", "w");
    if (!fp) return;
    Transaction* p = transactionList->next;
    while (p) {
        fprintf(fp, "%d,%d,%.2f,%s,%s\n", p->id, p->type, p->amount, p->time, p->description);
        p = p->next;
    }
    fclose(fp);
}

// ---------------------------------------------------------
// 业务一：输出财务统计报表 (按时间范围累加分类收入)
// ---------------------------------------------------------
static void showFinancialReport() {
    char start[11], end[11];
    float total_outpatient = 0, total_inpatient = 0, total_drug = 0;
    while (1) {
        printf("请输入统计起始日期 (YYYY-MM-DD): ");
        judgetime(start);// 验证日期格式;
        printf("请输入统计结束日期 (YYYY-MM-DD): ");
        judgetime(end);// 验证日期格式;
		if (strcmp(start, end) > 0) {
            printf("起始日期不能晚于结束日期，请重新输入。\n");
        }
        else {
            break;
        }
    }
    Transaction* t = transactionList->next;
    while (t) {
        // 利用字典序比较时间字符串，筛选在时间区间内的记录
        if (strncmp(t->time, start,10) >= 0 && strncmp(t->time, end,10) <= 0) {
            // 根据交易类型分别累加金额
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
    printf("总计营业额: %.2f\n", total_outpatient + total_inpatient + total_drug);
    printf("\n（可导出报表，此处仅为大屏预览）\n");
}

// ---------------------------------------------------------
// 业务二：人事接诊/开单报表展示 (占位模拟)
// ---------------------------------------------------------
static void showPersonnelReport() {
    char start[11], end[11];
    while (1) {
        printf("请输入统计起始日期 (YYYY-MM-DD): ");
        judgetime(start);
        printf("请输入统计结束日期 (YYYY-MM-DD): ");
        judgetime(end);
		if (strcmp(start, end) > 0) {
            printf("起始日期不能晚于结束日期，请重新输入。\n");
        }
        else {
            break;
        }
    }
    printf("\n========== 人事报表 (%s 至 %s) ==========\n", start, end);
    printf("%-15s %-12s %-10s\n", "医生姓名", "科室", "接诊量");

    // 生成报表链表
    parttimereport(start, end);

    PersonnelReport* cr = personnelReportList->next;
    if (cr == NULL) {
        printf("统计范围内无接诊记录。\n");
        return;
    }
    while (cr) {
        printf("%-15s %-12s %-10d\n", cr->doctor_name, cr->department, cr->count);
        cr = cr->next;
    }
    PersonnelReport* p = personnelReportList->next;
    while (p) {
        PersonnelReport* temp = p;
        p = p->next;
        free(temp);
    }
    personnelReportList->next = NULL; // 彻底清空

    printf("\n（可导出报表，此处仅为大屏预览）\n");
}
// ---------------------------------------------------------
// 业务三：业务流水明细查询
// ---------------------------------------------------------
static void showBusinessReport() {
    char start[11], end[11];
    int type;
    
    printf("请选择业务类型:\n1-门诊流水\n2-住院流水\n3-药品流水\n4-全部\n0-退出\n选择: ");
    if (scanf("%d", &type) != 1) {
		while (getchar() != '\n');
        type = -1;
    }
    while (type != 0) {
		if (type < 0 || type > 4) {
            printf("无效选项，请重新选择: ");
            if (scanf("%d", &type) != 1) {
                while (getchar() != '\n');
                type = -1;
            }
            continue;
        }
        while (1) {
            printf("请输入查询起始日期 (YYYY-MM-DD): ");
		    judgetime(start);// 验证日期格式
            printf("请输入查询结束日期 (YYYY-MM-DD): ");
		    judgetime(end);// 验证日期格式
			if (strcmp(start, end) > 0) {
                printf("起始日期不能晚于结束日期，请重新输入。\n");
            } else {
                break;
            }
        }
        printf("\n========== 诊疗明细流水 (%s 至 %s) ==========\n", start, end);
        Transaction* t = transactionList->next;
        int found = 0;
        while (t) {
            // 范围检索
            if (strncmp(t->time, start,10) >= 0 && strncmp(t->time, end,10) <= 0) {
                // 分类检索 (类型4代表无视类型直接输出全部)
                if (type == 4 || t->type == type) {
                    printf("%-20s %-12s %-10.2f %s\n", t->time,
                        (t->type == 1) ? "门诊" : (t->type == 2) ? "住院" : "药品",
                        t->amount, t->description);
                    found = 1;
                }
            }
            t = t->next;
        }
        if (!found) printf("检索范围内无相关记录。\n");
        while (scanf("%d", &type) != 1) {
			while (getchar() != '\n');
			printf("无效选项，请重新选择: ");
        }
    }
}

// ---------------------------------------------------------
// 统计报表中心子路由
// ---------------------------------------------------------
void reportMenu() {
    int choice;
    do {
        printf("\n========== 统计报表中心 ==========\n");
        printf("1. 财务报表 (收入统计)\n");
        printf("2. 人事报表 (接诊量统计)\n");
        printf("3. 业务报表 (明细流水)\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }
        switch (choice) {
        case 1: showFinancialReport(); break;
        case 2: showPersonnelReport(); break;
        case 3: showBusinessReport(); break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}