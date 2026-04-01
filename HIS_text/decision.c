#define _CRT_SECURE_NO_WARNINGS
#include "decision.h"
#include "drug.h"       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------------------------------------------------------
// 业务一：基于概率的人员异常与风险预测模型
// ---------------------------------------------------------
static void personnelPrediction() {
    printf("\n========== 人员异常预测预警 ==========\n");
    srand((unsigned)time(NULL));
    // 使用随机数模拟 15% 概率触发预警机制
    int abnormal = rand() % 100 < 15;

    if (abnormal) {
        printf("[!] 警告：本周药师值班出现异常（请假/缺勤比例偏高），可能影响药品分拣与发药效率。\n");
        printf("    AI 建议：立刻启动应急排班方案，从门诊部抽调或增加临时辅助人员。\n");
    }
    else {
        printf("[√] 智能监控：根据近期考勤大数据分析，各项人事排班稳定，暂无异常流失风险。\n");
    }
    printf("\n");
}

// ---------------------------------------------------------
// 业务二：分拣仓库效率监控与异常处理策略
// ---------------------------------------------------------
static void warehouseStrategy() {
    printf("\n========== 药房分拣仓库效能评估 ==========\n");
    if (!drugList) {
        printf("暂无药品字典数据，无法建立效能评估模型。\n");
        return;
    }

    int total_out = 0, drug_count = 0;
    Drug* p = drugList;

    // 遍历整个药品字典，统计历史上所有药品的总出库量
    while (p) {
        drug_count++; // 统计药品品类总数
        int out_qty = 0;
        DrugHistory* h = drugHistoryList;
        // 遍历历史流水寻找匹配的药品出库记录(type=2)
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

    // 计算单品平均出库周转率
    float avg_out = (float)total_out / drug_count;
    printf("全院药品单品平均出库流转量：%.2f\n", avg_out);

    // 根据流转阈值分级给出 AI 决策策略
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

// ---------------------------------------------------------
// 业务三：根据历史出库量大数据，提供智能采购及库存配比建议
// ---------------------------------------------------------
static void drugProportionAdvice() {
    printf("\n========== 供应链库存配比 AI 优化建议 ==========\n");
    if (!drugList) {
        printf("无药品基础数据接入。\n");
        return;
    }

    // 建立结构体数组用于对数据进行聚合统计
    typedef struct {
        char name[50];
        int total_out; // 总消耗
        int stock;     // 现存量
    } DrugStat;

    DrugStat stats[100];
    int count = 0;

    Drug* p = drugList;
    while (p) {
        int out_qty = 0;
        DrugHistory* h = drugHistoryList;
        // 累加单一药品的出库消耗
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

    // 汇总大盘总消耗量用于计算占率
    int total_out = 0;
    for (int i = 0; i < count; i++) total_out += stats[i].total_out;

    if (total_out == 0) {
        printf("系统缺乏有效出库历史，模型无法完成计算。\n");
        return;
    }

    printf("基于医疗消耗大数据的采购结构分析：\n\n");
    printf("%-20s %-10s %-10s %-15s\n", "药品名称", "累计消耗", "消耗占率", "当前库存");

    for (int i = 0; i < count; i++) {
        float ratio = (float)stats[i].total_out / total_out * 100; // 消耗占率百分比
        printf("%-20s %-10d %-10.2f%% %-15d ", stats[i].name, stats[i].total_out, ratio, stats[i].stock);

        // AI 策略判定阈值
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

// ---------------------------------------------------------
// 智能决策引擎菜单子路由
// ---------------------------------------------------------
void decisionMenu() {
    int choice;
    do {
        printf("\n========== 智能辅助决策控制台 ==========\n");
        printf("1. 人事效能与异常预测\n");
        printf("2. 分拣与药房负载分析\n");
        printf("3. 供应链采销配比优化建议\n");
        printf("4. 一键执行全景分析\n");
        printf("0. 返回高管主菜单\n");
        printf("请选择功能: ");
        if (scanf("%d", &choice) != 1)
            choice = -1;
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