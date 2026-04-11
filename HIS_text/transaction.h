#pragma once
#ifndef TRANSACTION_H
#define TRANSACTION_H

// 补全队友缺失的财务流水结构体定义
typedef struct Transaction {
    int id;                 // 交易流水号
    int type;               // 交易类型: 1=门诊, 2=住院, 3=药品
    double amount;          // 交易金额（修复：float精度不足，改为double）
    char time[30];          // 交易时间 (如 2026-03-24)
    char description[200];  // 交易描述明细
    struct Transaction* next; // 指向下一条记录的指针
} Transaction;
//用于统计报表中人事接诊量的结构体定义
typedef struct PersonnelReport {
	char doctor_id[20];  // 医生ID
	char doctor_name[20];    // 医生姓名
    char department[30];    // 科室
    int count;             // 接诊量统计
    struct PersonnelReport* next; // 指向下一条记录的指针
} PersonnelReport;
// 声明全局链表供 .c 文件使用
extern Transaction* transactionList;
// 人事报表链表头指针
extern PersonnelReport* personnelReportList;
// 核心业务函数声明
void reportMenu(void);
void parttimereport(char* start,char* end);//专用于财务报表的输出格式化展示

#endif