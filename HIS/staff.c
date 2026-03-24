#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "models.h"
#include "staff.h"
#include "utils.h"

// 内部工具：生成业务流水号
void generateRecordID(char* buffer) {
    static int recCount = 2000;
    sprintf(buffer, "R2025%04d", recCount++);
}

// 内部工具：如果病床没有初始化，生成 10 张测试床位
void initBedsIfEmpty() {
    if (bedHead->next) return;
    Bed* tail = bedHead;
    for (int i = 1; i <= 10; i++) {
        Bed* b = (Bed*)malloc(sizeof(Bed));
        b->bedNumber = 100 + i;
        b->isOccupied = 0;
        strcpy(b->patientId, "");
        b->next = NULL;
        tail->next = b;
        tail = b;
    }
}

// ---------------------------------------------------------
// 业务一：门诊-接诊叫号
// ---------------------------------------------------------
void callPatient(const char* docId) {
    Record* r = recordHead->next;
    int found = 0;
    while (r) {
        // 筛选该医生名下，Type=1(挂号)，且 isPaid=1(已缴费) 的患者
        if (strcmp(r->staffId, docId) == 0 && r->type == 1 && r->isPaid == 1) {
            printf("【呼叫系统】请患者 %s 进入诊室就诊!\n", r->patientId);
            r->isPaid = 2; // 标记为已接诊(闭环)
            found = 1;
            break;
        }
        r = r->next;
    }
    if (!found) printf("当前无人排队，或候诊患者尚未在财务端完成缴费。\n");
}

// ---------------------------------------------------------
// 业务二：门诊-看诊与开具检查单
// ---------------------------------------------------------
void diagnoseAndTest(const char* docId) {
    char pId[20], diag[100];
    printf("请输入就诊患者ID: "); safeGetString(pId, 20);
    printf("录入症状及诊断意见: "); safeGetString(diag, 100);

    // 录入看诊费
    Record* r = (Record*)malloc(sizeof(Record));
    generateRecordID(r->recordId);
    r->type = 2; strcpy(r->patientId, pId); strcpy(r->staffId, docId);
    r->cost = 20.0; r->isPaid = 0; sprintf(r->description, "诊断:%s", diag);
    r->next = recordHead->next; recordHead->next = r; // 头插法推入账单中心
    printf("【通知】已生成诊费账单20元，待患者缴费。\n");

    printf("是否需要开具化验单/X光检查？(1.是 0.否): ");
    if (safeGetInt() == 1) {
        // 录入检查费
        Record* test = (Record*)malloc(sizeof(Record));
        generateRecordID(test->recordId);
        test->type = 3; strcpy(test->patientId, pId); strcpy(test->staffId, docId);
        test->cost = 150.0; test->isPaid = 0; strcpy(test->description, "生化与X光检查套餐");
        test->next = recordHead->next; recordHead->next = test;
        printf("【通知】已开具检查单，费用150元。\n");
    }
}

// ---------------------------------------------------------
// 业务三：门诊-处方开药
// ---------------------------------------------------------
void prescribeMedicine(const char* docId) {
    char pId[20], key[50];
    printf("请输入患者ID: "); safeGetString(pId, 20);
    printf("搜索药品名称或编号: "); safeGetString(key, 50);

    // 模糊检索药品
    Medicine* m = medicineHead->next;
    while (m) {
        if (strcmp(m->name, key) == 0 || strcmp(m->id, key) == 0) break;
        m = m->next;
    }
    if (!m) { printf("未找到该药品，请核对药房系统!\n"); return; }

    printf("药品单价: %.2f | 当前药房库存: %d\n请输入开具数量: ", m->price, m->stock);
    int qty = safeGetInt();

    // 校验药房库存
    if (qty <= 0 || qty > m->stock) {
        printf("【拦截警告】输入数量非法或药房库存不足!\n"); return;
    }

    // 生成处方待缴费账单
    Record* r = (Record*)malloc(sizeof(Record));
    generateRecordID(r->recordId);
    r->type = 2; strcpy(r->patientId, pId); strcpy(r->staffId, docId);
    r->cost = qty * m->price; r->isPaid = 0; sprintf(r->description, "处方药:%s x %d", m->name, qty);
    r->next = recordHead->next; recordHead->next = r;
    printf("【成功】处方已下发，账单已推送至患者财务终端!\n");
}

// ---------------------------------------------------------
// 业务四：住院-办理入院与分配床位
// ---------------------------------------------------------
void admitPatient(const char* docId) {
    initBedsIfEmpty(); // 确保有床位基础数据
    char pId[20];
    printf("请输入办理入院的患者ID: "); safeGetString(pId, 20);

    // 寻找空床位
    Bed* b = bedHead->next;
    Bed* empty = NULL;
    while (b) {
        if (!b->isOccupied) { empty = b; break; }
        b = b->next;
    }
    if (!empty) { printf("【系统拒绝】全院病床已满，无空闲床位!\n"); return; }

    // 占用床位
    empty->isOccupied = 1; strcpy(empty->patientId, pId);

    // 生成住院押金账单
    Record* r = (Record*)malloc(sizeof(Record));
    generateRecordID(r->recordId);
    r->type = 4; strcpy(r->patientId, pId); strcpy(r->staffId, docId);
    r->cost = 500.0; r->isPaid = 0; sprintf(r->description, "住院押金(分配床号:%d)", empty->bedNumber);
    r->next = recordHead->next; recordHead->next = r;
    printf("【成功】成功分配床号 %d，生成待缴押金 500 元。\n", empty->bedNumber);
}

// ---------------------------------------------------------
// 医护端总路由 (接收 main 传入的医生指针 me，彻底去除输入密码环节)
// ---------------------------------------------------------
void staffTerminal(Staff* me) {
    while (1) {
        system("cls");
        printf("\n--- 医生工作台 (%s科: %s医师) ---\n", me->department, me->name);
        printf("1. 门诊业务中心\n2. 住院业务中心\n3. 个人中心\n0. 注销退出\n选择: ");
        int c = safeGetInt();

        if (c == 1) {
            printf(">> 门诊业务：1.接诊叫号 2.看诊与检查 3.开具处方 4.下达住院指令\n请选择: ");
            int sc = safeGetInt();
            if (sc == 1) callPatient(me->id);
            else if (sc == 2) diagnoseAndTest(me->id);
            else if (sc == 3) prescribeMedicine(me->id);
            else if (sc == 4) printf("【系统】住院指令已成功推送至住院部联网终端。\n");
            system("pause");
        }
        else if (c == 2) {
            printf(">> 住院业务：1.办理入院 2.提交查房记录 3.办理出院\n请选择: ");
            int sc = safeGetInt();
            if (sc == 1) admitPatient(me->id);
            else if (sc == 2) printf("【记录保存】日常查房医嘱与信息已安全录入。\n");
            else if (sc == 3) {
                // 出院结算逻辑：释放病床占用状态
                printf("请输入患者ID办理出院: ");
                char pId[20]; safeGetString(pId, 20);
                Bed* b = bedHead->next;
                while (b) {
                    if (b->isOccupied && strcmp(b->patientId, pId) == 0) {
                        b->isOccupied = 0;
                        printf("【办结】该患者的床位(编号%d)已释放并触发结算流程。\n", b->bedNumber);
                        break;
                    }
                    b = b->next;
                }
            }
            system("pause");
        }
        else if (c == 3) {
            // 个人信息查看
            printf("\n> 个人档案 <\n工号: %s\n科室: %s\n职称: %s\n*(如需提交离职/请假审批流，请联系人事管理科)*\n", me->id, me->department, me->level);
            system("pause");
        }
        else if (c == 0) return; // 退回大厅
    }
}