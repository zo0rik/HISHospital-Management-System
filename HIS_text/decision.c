#define _CRT_SECURE_NO_WARNINGS
#include "decision.h"
#include "drug.h"       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "time_t.h"
#include "utils.h"

static void personnelPrediction() {
    printf("\n========== 人员异常预测预警 ==========\n");
    char start[30], currentTime[30];
    getCurrentDate(currentTime, 30);
    getPastDateAccurate(currentTime, start, 14);
    parttimereport(start, currentTime);
    if (!personnelReportList || !(personnelReportList->next)) {
        printf(" 在指定时间范围内无接诊数据，无法进行分析。\n");
        return;
    }

    int totalDoctors = 0;
    int highVolumeDoctors = 0;
    int lowVolumeDoctors = 0;

    PersonnelReport* current = personnelReportList->next;
    while (current != NULL) {
        totalDoctors++;

        if (current->count > 100) {
            highVolumeDoctors++;
        }
        else if (current->count < 20) {
            lowVolumeDoctors++;
        }
        current = current->next;
    }

    printf("统计完成：共 %d 名医生\n", totalDoctors);
    printf("  高接诊量医生(>100): %d 名\n", highVolumeDoctors);
    printf("  低接诊量医生(<20): %d 名\n", lowVolumeDoctors);

    printf("\n--- 医生接诊情况分析 ---\n");
    printf("%-10s %-20s %-15s %-8s %s\n", "医生ID", "姓名", "科室", "接诊量", "风险状态");
    printf("------------------------------------------------------------\n");

    current = personnelReportList->next;
    while (current != NULL) {
        char riskStatus[20] = "正常";
        float riskProbability = 0.0f;

        if (current->count > 120) {
            strcpy(riskStatus, "过劳风险");
            riskProbability = 0.8f;
        }
        else if (current->count > 100) {
            strcpy(riskStatus, "负荷较高");
            riskProbability = 0.6f;
        }
        else if (current->count < 10) {
            strcpy(riskStatus, "工作不足");
            riskProbability = 0.4f;
        }
        else if (current->count < 20) {
            strcpy(riskStatus, "关注对象");
            riskProbability = 0.3f;
        }

        printf("%-10s %-20s %-15s %-8d %s(%.1f%%)\n",
            current->doctor_id, current->doctor_name, current->department,
            current->count, riskStatus, riskProbability * 100);

        current = current->next;
    }

    printf("\n--- 预警名单 ---\n");
    int warningCount = 0;
    current = personnelReportList->next;

    while (current != NULL) {
        if (current->count > 100 || current->count < 20) {
            if (warningCount == 0) {
                printf("%-10s %-20s %-15s %-8s %s\n", "医生ID", "姓名", "科室", "接诊量", "预警原因");
                printf("----------------------------------------------------------------\n");
            }

            char reason[50];
            if (current->count > 100) {
                snprintf(reason, sizeof(reason), "接诊量过高(%d)，可能过劳", current->count);
            }
            else {
                snprintf(reason, sizeof(reason), "接诊量过低(%d)，工作状态异常", current->count);
            }

            printf("%-10s %-20s %-15s %-8d %s\n",
                current->doctor_id, current->doctor_name, current->department,
                current->count, reason);

            warningCount++;
        }
        current = current->next;
    }

    if (warningCount == 0) {
        printf("  暂无需要预警的医生\n");
    }
    else {
        printf("  共 %d 名医生需要关注\n", warningCount);
    }

    printf("\n--- 管理建议 ---\n");
    if (highVolumeDoctors > 0) {
        printf(" 对高接诊量医生(%d名)：适当减少排班，防止过劳\n", highVolumeDoctors);
    }
    if (lowVolumeDoctors > 0) {
        printf(" 对低接诊量医生(%d名)：了解原因，必要时调整岗位\n", lowVolumeDoctors);
    }
    if (highVolumeDoctors == 0 && lowVolumeDoctors == 0) {
        printf(" 所有医生接诊量在正常范围，继续保持\n");
    }

    printf("\n分析完成！\n");
    PersonnelReport* p = personnelReportList->next;
    while (p) {
        PersonnelReport* tmp = p;
        p = p->next;
        free(tmp);
    }
    personnelReportList->next = NULL;
}

static void warehouseStrategy() {
    printf("\n========== 药房分拣仓库效能评估 ==========\n");
    if (!(drugList->next)) {
        printf("暂无药品字典数据，无法建立效能评估模型。\n");
        return;
    }

    int total_out = 0, drug_count = 0;
    Drug* p = drugList->next;

    while (p) {
        drug_count++;
        int out_qty = 0;
        DrugHistory* h = drugHistoryList->next;
        while (h) {
            if (h->drug_id == p->id && h->type == 2) out_qty += h->quantity;
            h = h->next;
        }
        total_out += out_qty;
        p = p->next;
    }
    if (drug_count == 0 || total_out == 0) {
        printf("药房历史出库数据积累不足，无法完成效能评估。建议累积业务数据后再试。\n");
        return;
    }

    double avg_out = (double)total_out / drug_count;
    printf("全院药品单品平均出库流转量：%.2f\n", avg_out);

    if (avg_out > 100) {
        printf("【诊断】分拣仓库极度繁忙，属于高负载运作，建议：\n");
        printf("  - 增加药房夜班排班或延长自动化设备作业时间\n");
        printf("  - 考虑升级采购自动化发药机系统\n");
        printf("  - 将高频使用的药品调配至出货最前端\n");
    }
    else if (avg_out > 30) {
        printf("【诊断】分拣仓库工作量处于健康水位，建议：\n");
        printf("  - 保持当前的SOP分拣操作流\n");
        printf("  - 定期安排设备巡检以防宕机\n");
    }
    else {
        printf("【诊断】分拣仓库负荷较低，存在人力浪费，建议：\n");
        printf("  - 可优化排班，减少低峰期闲置人员以控制成本\n");
        printf("  - 实行批处理操作，集中时段进行药品分拣\n");
    }

    printf("\n[标准异常处理规范提示]\n");
    printf(" > 账实不符：立刻冻结该批次，启动二级复盘点流程。\n");
    printf(" > 滞销积压：对接财务与供应商实行折价或调换退货。\n");
    printf("\n");
}

static void drugProportionAdvice() {
    printf("\n========== 供应链库存配比 AI 优化建议 ==========\n");
    if (!(drugList->next)) {
        printf("无药品基础数据接入。\n");
        return;
    }

    typedef struct {
        char name[50];
        int total_out;
        int stock;
    } DrugStat;

    DrugStat stats[100];
    int count = 0;

    Drug* p = drugList->next;
    while (p) {
        int out_qty = 0;
        DrugHistory* h = drugHistoryList->next;
        while (h) {
            if (h->drug_id == p->id && h->type == 2) out_qty += h->quantity;
            h = h->next;
        }
        if (count >= 100) { printf("  [!] 药品种类超过统计上限(100)，部分数据已截断。\n"); break; }
        strcpy(stats[count].name, p->name);
        stats[count].total_out = out_qty;
        stats[count].stock = p->stock;
        count++;
        p = p->next;
    }

    int total_out = 0;
    for (int i = 0; i < count; i++) total_out += stats[i].total_out;

    if (total_out == 0) {
        printf("系统缺乏有效出库历史，模型无法完成计算。\n");
        return;
    }

    printf("基于医疗消耗大数据的采购结构分析：\n\n");
    printf("%-20s %-10s %-10s %-15s\n", "药品名称", "累计消耗", "消耗占率", "当前库存");

    for (int i = 0; i < count; i++) {
        double ratio = (double)stats[i].total_out / total_out * 100;
        printf("%-20s %-10d %-10.2f%% %-15d ", stats[i].name, stats[i].total_out, ratio, stats[i].stock);

        if (stats[i].stock < stats[i].total_out * 0.5) {
            printf("【高危短缺】应紧急采购补充，建议将常态占率拉升至 %.1f%%\n", ratio * 1.2);
        }
        else if (stats[i].stock > stats[i].total_out * 2) {
            printf("【资金占用】存在积压滞销风险，建议缩减采购配额至 %.1f%%\n", ratio * 0.8);
        }
        else {
            printf("【状态健康】供需模型匹配正常\n");
        }
    }

    printf("\n【模型输出总结】\n");
    printf("  1. 优先将资金向高消耗、低库存的长尾刚需药品倾斜。\n");
    printf("  2. 对于消耗占率极低的品种，采用按单订货模式（JIT）代替海量囤货。\n");
    printf("\n");
}

void decisionMenu() {
    int choice;
    do {
        system("cls");
        printf("\n========== 智能辅助决策控制台 ==========\n");
        printf("1. 人事效能与异常预测\n");
        printf("2. 分拣与药房负载分析\n");
        printf("3. 供应链采销配比优化建议\n");
        printf("4. 一键执行全景分析\n");
        printf("-1. 返回高管主菜单\n"); /* 【规则A】0→-1 */
        printf("请选择功能: ");

        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 4)) break; /* 【规则B】0→-1 */
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择功能: ");
        }

        switch (choice) {
        case 1:
            personnelPrediction();
            system("pause");
            break;
        case 2:
            warehouseStrategy();
            system("pause");
            break;
        case 3:
            drugProportionAdvice();
            system("pause");
            break;
        case 4:
            personnelPrediction();
            warehouseStrategy();
            drugProportionAdvice();
            system("pause");
            break;
        case -1: /* 【规则B】 */
            break;
        }
    } while (choice != -1); /* 【规则B】 */
}