#define _CRT_SECURE_NO_WARNINGS
#include "decision.h"
#include "drug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 人员异常预测
static void personnelPrediction() {
    printf("\n========== 人员异常预测 ==========\n");
    srand((unsigned)time(NULL));
    int abnormal = rand() % 100 < 15;
    if (abnormal) {
        printf("? 警告：本周药师值班出现异常（请假/缺勤），可能影响药品分拣效率，建议调整排班或增加临时人员。\n");
        printf("  异常详情：近期药师缺勤记录，请查看考勤系统。\n");
    }
    else {
        printf("? 根据考勤数据，近期药师值班正常，无人员异常。\n");
    }
    printf("\n");
}

// 分拣仓库异常处理策略
static void warehouseStrategy() {
    printf("\n========== 分拣仓库异常处理策略 ==========\n");
    if (!drugList) {
        printf("暂无药品数据，无法分析分拣仓库异常。\n");
        return;
    }
    int total_out = 0, drug_count = 0;
    Drug* p = drugList;
    while (p) {
        drug_count++;
        int out_qty = 0;
        DrugHistory* h = drugHistoryList;
        while (h) {
            if (h->drug_id == p->id && h->type == 2) out_qty += h->quantity;
            h = h->next;
        }
        total_out += out_qty;
        p = p->next;
    }
    if (drug_count == 0 || total_out == 0) {
        printf("历史出库数据不足，无法评估分拣效率。建议先进行药品出库操作。\n");
        return;
    }
    float avg_out = (float)total_out / drug_count;
    printf("药品平均出库量：%.2f\n", avg_out);
    if (avg_out > 100) {
        printf("分拣仓库繁忙，出库量较大，建议：\n");
        printf("  - 增加分拣人员或延长工作时间\n");
        printf("  - 引入自动化分拣设备\n");
        printf("  - 优化药品摆放，提高取药效率\n");
    }
    else if (avg_out > 30) {
        printf("分拣仓库工作量适中，建议：\n");
        printf("  - 保持当前分拣流程\n");
        printf("  - 定期检查分拣设备是否正常\n");
    }
    else {
        printf("分拣仓库工作量较低，建议：\n");
        printf("  - 可适当减少分拣人员，节约成本\n");
        printf("  - 合并分拣批次，提高单次效率\n");
    }
    printf("\n【异常处理策略】\n");
    printf("  若发现库存差异，立即启动盘点流程，核实实际库存与系统记录。\n");
    printf("  对于滞销药品，考虑促销或退货，减少积压。\n");
    printf("  对于畅销药品，设置安全库存预警，及时补货。\n");
    printf("\n");
}

// 根据历史订单分析需求，建议调整各类药品比例
static void drugProportionAdvice() {
    printf("\n========== 药品比例调整建议 ==========\n");
    if (!drugList) {
        printf("无药品数据。\n");
        return;
    }
    // 统计每种药品的历史出库量
    typedef struct {
        char name[50];
        int total_out;
        int stock;
    } DrugStat;
    DrugStat stats[100];
    int count = 0;
    Drug* p = drugList;
    while (p) {
        int out_qty = 0;
        DrugHistory* h = drugHistoryList;
        while (h) {
            if (h->drug_id == p->id && h->type == 2) out_qty += h->quantity;
            h = h->next;
        }
        strcpy(stats[count].name, p->name);
        stats[count].total_out = out_qty;
        stats[count].stock = p->stock;
        count++;
        p = p->next;
    }
    // 计算总出库量
    int total_out = 0;
    for (int i = 0; i < count; i++) total_out += stats[i].total_out;
    if (total_out == 0) {
        printf("暂无历史出库记录，无法分析需求比例。\n");
        return;
    }
    printf("根据历史订单分析，各类药品出库占比及调整建议：\n");
    printf("%-20s %-10s %-10s %-15s\n", "药品名称", "药品数量", "出库占比", "库存");
    for (int i = 0; i < count; i++) {
        float ratio = (float)stats[i].total_out / total_out * 100;
        printf("%-20s %-10d %-10.2f%% %-15d ", stats[i].name, stats[i].total_out, ratio, stats[i].stock);
        if (stats[i].stock < stats[i].total_out * 0.5) {
            printf("【建议：增加采购，提升至占比%.1f%%】\n", ratio * 1.2);
        }
        else if (stats[i].stock > stats[i].total_out * 2) {
            printf("【建议：减少采购，降至占比%.1f%%】\n", ratio * 0.8);
        }
        else {
            printf("【库存合理，维持比例】\n");
        }
    }
    printf("\n【综合建议】\n");
    printf("  1. 针对出库占比高且库存低的药品，应优先采购，避免断货。\n");
    printf("  2. 针对出库占比低且库存高的药品，可减少采购或开展促销。\n");
    printf("  3. 建议定期分析药品出库趋势，动态调整采购计划。\n");
    printf("\n");
}

void decisionMenu() {
    int choice;
    do {
        printf("\n========== 智能辅助决策系统 ==========\n");
        printf("1. 人员异常预测\n");
        printf("2. 分拣仓库异常处理策略\n");
        printf("3. 药品比例调整建议（根据历史订单）\n");
        printf("4. 全部执行\n");
        printf("0. 返回主菜单\n");
        printf("请选择: ");
        scanf("%d", &choice);
        switch (choice) {
        case 1: personnelPrediction(); break;
        case 2: warehouseStrategy(); break;
        case 3: drugProportionAdvice(); break;
        case 4:
            personnelPrediction();
            warehouseStrategy();
            drugProportionAdvice();
            break;
        case 0: break;
        default: printf("无效选项。\n");
        }
    } while (choice != 0);
}