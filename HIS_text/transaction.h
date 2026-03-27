#pragma once
#ifndef TRANSACTION_H
#define TRANSACTION_H

// 补全队友缺失的财务流水结构体定义
typedef struct Transaction {
    int id;                 // 交易流水号
    int type;               // 交易类型: 1=门诊, 2=住院, 3=药品
    float amount;           // 交易金额
    char time[30];          // 交易时间 (如 2026-03-24)
    char description[200];  // 交易描述明细
    struct Transaction* next; // 指向下一条记录的指针
} Transaction;

// 声明全局链表供 .c 文件使用
extern Transaction* transactionList;

// 核心业务函数声明
void loadTransactions(void);
void saveTransactions(void);
void reportMenu(void);

#endif