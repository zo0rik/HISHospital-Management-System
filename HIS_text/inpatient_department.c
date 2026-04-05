#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "utils.h"
#include "inpatient_department.h"
#include "transaction.h"
#include "outpatient_department.h" 
#include "drug.h"

/* 将所有外部依赖的函数与变量声明提取至文件顶部，避免 C89 语法冲突 */
extern void generateRecordID(char* buffer);
extern char currentCallingPatientId[20];
extern void prescribeMedicine(const char* docId);

// ---------------------------------------------------------
// 辅助函数：通过患者ID回溯其首诊负责科室
// 设计思路：在办理入院时，仅凭患者ID无法得知其应归属哪个病区。
// 此函数通过遍历全局流水，找到该患者最近的一张入院通知单（Type 6），
// 并根据开单医师的工号反向查询其所属科室，从而实现科室的自动匹配。
// ---------------------------------------------------------
void getResponsibleDept(const char* patientId, char* deptBuffer) {
    Record* r;
    char staffId[20];
    Staff* s;

    strcpy(staffId, "");
    strcpy(deptBuffer, "未知科室");
    r = recordHead->next;

    while (r) {
        if (r->type == 6 && strcmp(r->patientId, patientId) == 0) {
            strcpy(staffId, r->staffId);
        }
        r = r->next;
    }
    if (strlen(staffId) > 0) {
        s = staffHead->next;
        while (s) {
            if (strcmp(s->id, staffId) == 0) {
                strcpy(deptBuffer, s->department);
                break;
            }
            s = s->next;
        }
    }
}

// ---------------------------------------------------------
// 初始化模块：构建全院初始物理床位数据
// 设计思路：系统首次运行或未读取到本地文件时，自动生成基础数据。
// 包含5大科室，每科室5个房间，每房间4张床，共计100张床位。
// ---------------------------------------------------------
void initBedsIfEmpty() {
    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };
    Bed* tail;
    int roomNum = 1;
    int d, i, j;
    Bed* b;

    if (bedHead->next) return;

    tail = bedHead;

    for (d = 0; d < 5; d++) {
        for (i = 0; i < 5; i++) {
            for (j = 1; j <= 4; j++) {
                b = (Bed*)malloc(sizeof(Bed));
                sprintf(b->bedId, "%d-%d", roomNum, j);
                b->isOccupied = 0; strcpy(b->patientId, ""); strcpy(b->bedType, types[i]);
                b->price = prices[i]; strcpy(b->wardType, (i >= 3) ? wards[1] : wards[0]);
                b->isRoundsDone = 0;
                b->next = NULL; tail->next = b; tail = b;
            }
            roomNum++;
        }
    }
}

// ---------------------------------------------------------
// 辅助函数：根据床位编号映射归属科室
// ---------------------------------------------------------
const char* getRoomDepartment(const char* bedId) {
    int roomNum;
    int deptIdx;
    sscanf(bedId, "%d-", &roomNum);
    deptIdx = (roomNum - 1) / 5;
    if (deptIdx == 0) return "内科";
    if (deptIdx == 1) return "外科";
    if (deptIdx == 2) return "儿科";
    if (deptIdx == 3) return "妇产科";
    if (deptIdx == 4) return "急诊科";
    return "未知科室";
}

// ---------------------------------------------------------
// 资源调度模块：监测科室负载并执行动态扩容
// 设计思路：在办理入院前，系统会自动检查目标科室的床位空闲率。
// 如果空闲率低于 20%，触发应急机制，将科室内的“单人疗养病房”
// 动态拆分为两张“双人病房”床位，以增加收治容量。
// ---------------------------------------------------------
void checkAndAdjustBedTension(const char* targetDept) {
    int total = 0, empty = 0;
    Bed* b;
    int converted = 0;
    Bed* extra;

    b = bedHead->next;
    while (b) {
        if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
            total++;
            if (b->isOccupied == 0) empty++;
        }
        b = b->next;
    }

    if (total > 0 && ((float)empty / total) < 0.2f) {
        printf("\n  [系统动态调度] %s 专属空床率不足 20%%！触发紧急扩容机制...\n", targetDept);
        printf("  >>> 正在将【%s】内的单人陪护疗养病房临时调整为双人病房 <<<\n", targetDept);

        b = bedHead->next;
        while (b) {
            if (b->isOccupied == 0 &&
                strcmp(b->bedType, "单人陪护疗养病房") == 0 &&
                strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {

                strcpy(b->bedType, "双人病房");
                b->price = 150.0;

                extra = (Bed*)malloc(sizeof(Bed));
                *extra = *b;
                sprintf(extra->bedId, "%sA", b->bedId);
                extra->next = bedHead->next; bedHead->next = extra;
                converted++;
            }
            b = b->next;
        }
        if (converted > 0) {
            printf("  [扩容成功] 已为 %s 增加了 %d 张可用双人床位！\n", targetDept, converted);
        }
        else {
            printf("  [扩容失败] %s 当前已无空余的单人疗养病房可供拆分。\n", targetDept);
        }
    }
}

// 获取当前存在的科室名称列表，用于菜单提示
void getDynamicDeptPrompt(char* promptBuffer) {
    char depts[20][50];
    int dCount = 0;
    Staff* stf;
    int exists, i;

    stf = staffHead->next;
    while (stf) {
        exists = 0;
        for (i = 0; i < dCount; i++) { if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; } }
        if (!exists && strlen(stf->department) > 0) { strcpy(depts[dCount], stf->department); dCount++; }
        stf = stf->next;
    }

    strcpy(promptBuffer, "");
    for (i = 0; i < dCount; i++) {
        strcat(promptBuffer, depts[i]);
        if (i < dCount - 1) strcat(promptBuffer, "/");
    }
}

// ---------------------------------------------------------
// 1. 全院床位实时使用图谱查询功能
// ---------------------------------------------------------
void viewAllBeds() {
    char deptStr[200];
    char targetDept[50];
    Bed* b;
    int bedCount;
    char patName[100];
    char docName[50];
    Patient* p;
    Record* r;
    Staff* s;

    initBedsIfEmpty();

    while (1) {
        bedCount = 0;
        system("cls");
        printf("\n========================================================================================\n");
        printf("                           全院病房与床位实时使用雷达图谱                               \n");
        printf("========================================================================================\n");

        getDynamicDeptPrompt(deptStr);
        printf("当前活跃科室有: (%s)\n请输入要查看的科室 (输0返回): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n>>> 【%s】 专属住院病区动态 <<<\n", targetDept);
        // 使用定长占位符控制输出格式，确保表格列对齐
        printf("%-10s %-12s %-18s %-8s %-10s %-15s %-15s\n", "房号-床位", "病区等级", "房型", "单价", "目前状态", "收治大夫", "入住患者");
        printf("----------------------------------------------------------------------------------------\n");

        b = bedHead->next;
        while (b) {
            if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                strcpy(patName, "-");
                strcpy(docName, "-");

                // 如果床位被占用，关联查询对应的患者姓名及负责医生姓名
                if (b->isOccupied) {
                    p = patientHead->next;
                    while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(patName, p->name); break; } p = p->next; }

                    r = recordHead->next;
                    while (r) {
                        if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                            s = staffHead->next;
                            while (s) { if (strcmp(s->id, r->staffId) == 0) { strcpy(docName, s->name); break; } s = s->next; }
                            break;
                        }
                        r = r->next;
                    }
                }

                printf("%-10s %-12s %-18s %-8.2f %-10s %-15s %s(%s)\n",
                    b->bedId, b->wardType, b->bedType, b->price,
                    b->isOccupied ? "[有客]" : "[闲置]", docName,
                    b->isOccupied ? patName : "-", b->isOccupied ? b->patientId : "-");
                bedCount++;
            }
            b = b->next;
        }
        if (bedCount == 0) printf("  (未找到该科室的专属病床数据)\n");
        printf("----------------------------------------------------------------------------------------\n");
        system("pause");
    }
}

// ---------------------------------------------------------
// 2. 入院办理与床位分配系统
// ---------------------------------------------------------
void admitPatient(const char* docId) {
    Record* r;
    int noticeCount;
    char pId[20];
    Record* targetNotice;
    Patient* p;
    Patient* targetPat;
    char requiredDept[50];
    int hasLocalEmpty;
    Bed* b;
    int isCrossDept;
    int hasAnyEmpty;
    char selectBed[20];
    Bed* finalBed;
    int days;
    int baseDeposit;
    int finalDeposit;
    int isPaid;
    Record* r5;
    char adminTime[30];
    Patient* pt;
    char pName[100];
    char deptName[50];
    int alreadyIn;
    Transaction* newTrans;
    int maxId;
    Transaction* curr;

    noticeCount = 0;
    targetNotice = NULL;
    targetPat = NULL;
    hasLocalEmpty = 0;
    isCrossDept = 0;
    hasAnyEmpty = 0;
    finalBed = NULL;
    isPaid = 0;
    maxId = 0;

    initBedsIfEmpty();

    printf("\n========== 门诊下发《待入院通知单》队列 ==========\n");
    r = recordHead->next;

    // 分类显示待处理队列，重症患者优先展示，便于调度
    printf("【重症优先通道】\n");
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "重症")) {
            pt = patientHead->next; strcpy(pName, "未知");
            while (pt) { if (strcmp(pt->id, r->patientId) == 0) { strcpy(pName, pt->name); break; } pt = pt->next; }
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [紧急] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    printf("\n【普通入院队列】\n");
    r = recordHead->next;
    while (r) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "普通")) {
            pt = patientHead->next; strcpy(pName, "未知");
            while (pt) { if (strcmp(pt->id, r->patientId) == 0) { strcpy(pName, pt->name); break; } pt = pt->next; }
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [常规] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
        r = r->next;
    }

    if (noticeCount == 0) { printf("  [提示] 当前暂无门诊下发的未处理住院通知单。\n"); return; }

    printf("\n请输入需办理入院的患者ID (输0返回): "); safeGetString(pId, 20);
    if (strcmp(pId, "0") == 0) return;

    // 校验通知单有效性，防止越权或重复收治
    r = recordHead->next;
    while (r) { if (r->type == 6 && strcmp(r->patientId, pId) == 0 && r->isPaid == 0) { targetNotice = r; break; } r = r->next; }

    if (!targetNotice) {
        alreadyIn = 0;
        b = bedHead->next;
        while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0) { alreadyIn = 1; break; } b = b->next; }
        if (alreadyIn) printf("  [拦截警告] 业务异常：该患者当前已处于住院收治状态。\n");
        else printf("  [拦截警告] 业务阻断：缺乏门诊前端开具的合规通知单，无法受理收治。\n");
        return;
    }

    p = patientHead->next;
    while (p) { if (strcmp(p->id, pId) == 0) { targetPat = p; break; } p = p->next; }

    getResponsibleDept(pId, requiredDept);
    printf("\n>>> 锁定通知单：该患者由【%s】下发，系统正检索 %s 的专属空余床位...\n", requiredDept, requiredDept);

    checkAndAdjustBedTension(requiredDept);

    b = bedHead->next;
    while (b) {
        if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) { hasLocalEmpty = 1; break; }
        b = b->next;
    }

    // 跨科室资源协调机制：当目标科室即使扩容后仍然满载时，允许跨科室提取空床资源
    if (!hasLocalEmpty) {
        printf("\n  [警告] %s 专属床位资源枯竭（扩容后仍无空余）！\n", requiredDept);
        printf("  >>> 触发系统应急预案：开启跨科室床位全局调度 <<<\n");
        isCrossDept = 1;

        b = bedHead->next;
        while (b) { if (!b->isOccupied) { hasAnyEmpty = 1; break; } b = b->next; }

        if (!hasAnyEmpty) {
            printf("  [极其抱歉] 全院整体床位网络满载，暂无法提供物理空间进行收治转运。\n");
            return;
        }

        printf("\n【全院可用空闲病床列表 (跨域收治)】:\n");
        b = bedHead->next;
        while (b) {
            if (!b->isOccupied) {
                printf("  [%s] 所属: %-6s | %s - %s (基准日租 ￥%.2f)\n", b->bedId, getRoomDepartment(b->bedId), b->wardType, b->bedType, b->price);
            }
            b = b->next;
        }
    }
    else {
        printf("\n【%s】可用空闲病床列表:\n", requiredDept);
        b = bedHead->next;
        while (b) {
            if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
                printf("  [%s] %s - %s (基准日租 ￥%.2f)\n", b->bedId, b->wardType, b->bedType, b->price);
            }
            b = b->next;
        }
    }

    printf("\n请输入系统拟分配的床位号 (如 1-3) (输入0取消): "); safeGetString(selectBed, 20);
    if (strcmp(selectBed, "0") == 0) return;

    b = bedHead->next;
    while (b) {
        if (strcmp(b->bedId, selectBed) == 0 && !b->isOccupied) {
            if (!isCrossDept && strcmp(getRoomDepartment(b->bedId), requiredDept) != 0) {
                // 非跨科室模式下，阻断分配其他科室的床位
            }
            else {
                finalBed = b; break;
            }
        }
        b = b->next;
    }

    if (!finalBed) {
        if (!isCrossDept) printf("  [!] 无效选择，该床位不存在或越界提取了非本科室床位。\n");
        else printf("  [!] 资源已被占用或该床位不存在。\n");
        return;
    }

    printf("\n  [√] 资源 [%s] 分配锁定成功！\n", finalBed->bedId);
    printf("请输入拟定住院周期(天): ");

    // 使用输入包装函数拦截非法输入字符
    days = safeGetPositiveInt();

    // 动态押金计算模型：保障账户资金充足，最低押金限额 1000 元，按百位向上取整
    baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("  系统通过风控模型核算，需缴纳初始住院统筹押金: %d 元。\n", finalDeposit);

    // 资金划扣处理
    if (targetPat->balance >= finalDeposit) {
        targetPat->balance -= finalDeposit; isPaid = 1;
        printf("  [√] 账户资金划扣完毕，病区收治状态已激活。\n");

        newTrans = (Transaction*)malloc(sizeof(Transaction));
        curr = transactionList;
        while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
        newTrans->id = maxId + 1; newTrans->type = 2; newTrans->amount = finalDeposit;
        getCurrentTimeStr(newTrans->time, 30); sprintf(newTrans->description, "住院押金收取(患者:%s)", targetPat->name);
        newTrans->next = NULL;

        if (!transactionList) transactionList = newTrans;
        else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }
    }
    else {
        printf("  [提示] 账户实时结余不足，系统已挂起床位并生成待处理押金账单。\n");
    }

    // 写入占用状态及记录
    finalBed->isOccupied = 1; strcpy(finalBed->patientId, pId);
    finalBed->isRoundsDone = 0;
    targetNotice->isPaid = 1;

    r5 = (Record*)malloc(sizeof(Record));
    generateRecordID(r5->recordId);
    r5->type = 5; strcpy(r5->patientId, pId); strcpy(r5->staffId, docId);
    r5->cost = finalDeposit; r5->isPaid = isPaid;

    getCurrentTimeStr(adminTime, 30);
    sprintf(r5->description, "病房:%s_床位:%s_入院日期:%s_预期天数:%d_押金:%d_出院日期:无_总费用:0",
        finalBed->wardType, finalBed->bedId, adminTime, days, finalDeposit);
    getCurrentTimeStr(r5->createTime, 30);
    r5->next = recordHead->next; recordHead->next = r5;

    if (isPaid) {
        if (isCrossDept) printf("\n  [成功] 跨模块调度机制完成，已将病患安置于 [%s] 的 %s 节点。\n", getRoomDepartment(finalBed->bedId), finalBed->bedId);
        else printf("\n  [成功] 基础收治流程完结，已分配至 %s 的 %s 节点。\n", requiredDept, finalBed->bedId);
    }
    else {
        printf("\n  [待确认] 空间分配记录已生成，待财务端确认资金流入后释放节点使用权。\n");
    }
}

// ---------------------------------------------------------
// 3. 模拟定时计费任务
// ---------------------------------------------------------
// 该功能模拟每日早晨对在院患者进行床位费用的自动划扣，
// 同步检测患者余额是否不足，重置日常查房标记。
void dailyDeductionSimulation() {
    Bed* b;
    int count = 0;
    Patient* p;

    printf("\n========== 执行批量床位核销日结算 (模拟08:00) ==========\n");
    b = bedHead->next;
    while (b) {
        if (b->isOccupied) {
            p = patientHead->next;
            while (p) {
                if (strcmp(p->id, b->patientId) == 0) {
                    if (p->balance < 1000) printf("  [预警] 检测到在院实体 %s 资金池跌破1000元红线 (结余: %.2f)！\n", p->name, p->balance);
                    printf("  -> 实体 %s (挂载床位:%s) 日度消耗计提: %.2f 元。\n", p->name, b->bedId, b->price);
                    count++; break;
                }
                p = p->next;
            }
            b->isRoundsDone = 0;
        }
        b = b->next;
    }
    printf("--------------------------------------------------\n");
    printf("批处理作业完毕。累计扫描并计算 %d 个活跃在院单元，病床日内查房状态已复位。\n", count);
}

// ---------------------------------------------------------
// 4. 病区查房与医嘱下达模块
// ---------------------------------------------------------
void wardRounds(const char* docId) {
    char deptStr[200];
    char targetDept[50];
    Bed* b;
    int pCount;
    char pId[20];
    Bed* targetBed;
    Patient* p;
    char pName[100];
    char severity[10];
    Record* r;
    int choice;
    char note[200];
    Record* rx;
    Record* cur;
    char tempDesc[300];

    while (1) {
        pCount = 0;
        targetBed = NULL;

        system("cls");
        printf("\n========== 住院病区日常巡检与查房 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("调取目标科室域 (%s, 0返回): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n--- 【%s】 当前住院受管名单 ---\n", targetDept);
        printf("%-10s %-15s %-10s %-10s %-10s\n", "房-床号", "患者ID", "姓名", "评估级别", "查房状态");
        printf("----------------------------------------------------------\n");

        b = bedHead->next;
        while (b) {
            if (b->isOccupied && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                p = patientHead->next;
                strcpy(pName, "未知");
                strcpy(severity, "普通");

                while (p) { if (strcmp(p->id, b->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }

                r = recordHead->next;
                while (r) {
                    if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                        if (strstr(r->description, "重症")) strcpy(severity, "重症");
                    }
                    r = r->next;
                }

                printf("%-10s %-15s %-10s %-10s %-10s\n",
                    b->bedId, b->patientId, pName, severity, b->isRoundsDone ? "[已查房]" : "[待查房]");
                pCount++;
            }
            b = b->next;
        }

        if (pCount == 0) { printf("  (目标病区当前无处于活跃周期的在院人员)\n"); system("pause"); continue; }

        printf("\n检索需建立查房会话的患者ID (0退回科室层级): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) continue;

        b = bedHead->next;
        while (b) {
            if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                targetBed = b; break;
            }
            b = b->next;
        }
        if (!targetBed) { printf("  [!] 定位失败：未在 %s 的管辖范畴内检索到对应人员。\n", targetDept); system("pause"); continue; }

        while (1) {
            system("cls");
            printf("\n========== 针对患者实体 [%s] 的查房干预面板 ==========\n", pId);
            printf("  [1] 录入基础观察医嘱\n  [2] 调用内部药房系统配发治疗耗材\n  [0] 结束当前查房会话\n---------------------------------------\n下达操作指令: ");

            choice = safeGetInt();
            if (choice == 0) break;

            if (choice == 1) {
                printf("输入实时医嘱指令内容(避免非法空格切断字符): ");
                safeGetString(note, 200);
                rx = (Record*)malloc(sizeof(Record));

                generateRecordID(rx->recordId);
                rx->type = 2; strcpy(rx->patientId, pId); strcpy(rx->staffId, docId);
                rx->cost = 0; rx->isPaid = 1;
                sprintf(rx->description, "住院查房医嘱:%s", note);
                getCurrentTimeStr(rx->createTime, 30);
                rx->next = recordHead->next; recordHead->next = rx;
                printf("  [√] 会话操作确认，医嘱项已持久化记录。\n");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
            else if (choice == 2) {
                strcpy(currentCallingPatientId, pId);

                // 调用门诊共用的药房接口，实现跨模块联动
                prescribeMedicine(docId);

                // 费用状态隔离与挂账延期处理：
                // 将住院查房阶段产生的药材消耗强制设定为后台记账状态（isPaid = 4）。
                // 该设计避免了账单立刻暴露至患者财务前台引发计费重叠，而是等待出院时作为统筹结算的一部分由押金进行对冲。
                cur = recordHead->next;
                while (cur) {
                    if (cur->type == 3 && strcmp(cur->patientId, pId) == 0 && cur->isPaid == 0) {
                        cur->isPaid = 4;
                        sprintf(tempDesc, "[住院记账]%s", cur->description);
                        strcpy(cur->description, tempDesc);
                    }
                    cur = cur->next;
                }

                strcpy(currentCallingPatientId, "");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
        }
    }
}

// ---------------------------------------------------------
// 5. 离院结算与状态流转解绑
// ---------------------------------------------------------
void dischargePatient() {
    char deptStr[200];
    char targetDept[50];
    Bed* b_list;
    int count;
    char pId[20];
    Bed* b;
    Bed* targetBed;
    Record* r_check;
    time_t t;
    struct tm* tm_info;
    int currentHour;
    int actualDays;
    int billableDays;
    double totalBedFee;
    double totalDrugFee;
    Record* r;
    double totalHospitalCost;
    double total_deposit;
    Patient* pt;
    double refund;
    Record* r8;
    Transaction* newTrans;
    int maxId;
    Transaction* curr;
    char summary[200];
    double arrears;
    Record* r_arrears;
    Record* r_link;
    Record* r_drug;
    int hasDrug;
    Patient* p;
    char pName[100];

    while (1) {
        count = 0;
        targetBed = NULL;
        totalDrugFee = 0.0;
        total_deposit = 0.0;
        maxId = 0;

        system("cls");
        printf("\n========== 离院综合清算办理控制台 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("锁定业务作用域科室 (%s, 0放弃办理): ", deptStr);
        safeGetString(targetDept, 50);
        if (strcmp(targetDept, "0") == 0) return;

        printf("\n--- 【%s】 等待出院审批名册 ---\n", targetDept);
        printf("%-10s %-12s %-15s %-10s\n", "房号-床位", "属性", "患者ID", "姓名");
        printf("-----------------------------------------------------\n");

        b_list = bedHead->next;
        while (b_list) {
            if (b_list->isOccupied && strcmp(getRoomDepartment(b_list->bedId), targetDept) == 0) {
                p = patientHead->next;
                strcpy(pName, "未知");
                while (p) { if (strcmp(p->id, b_list->patientId) == 0) { strcpy(pName, p->name); break; } p = p->next; }
                printf("%-10s %-12s %-15s %-10s\n", b_list->bedId, b_list->wardType, b_list->patientId, pName);
                count++;
            }
            b_list = b_list->next;
        }
        if (count == 0) { printf("  (当前无可流转的出院对象)\n"); system("pause"); continue; }

        printf("\n确立清算实体的患者ID (0终止当前操作): "); safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) continue;

        b = bedHead->next;
        while (b) { if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) { targetBed = b; break; } b = b->next; }
        if (!targetBed) { printf("  [!] 校验异常：所选区域中无此实体记录。\n"); system("pause"); continue; }

        // 业务状态锁：系统级别并发请求过滤
        // 如果底层流水库中已经存在针对该患者挂起的欠款追缴单，代表结算流程已启动并被阻塞，
        // 需强制打断当前操作，防止重复核算造成的数据冗余和多次计费。
        r_check = recordHead->next;
        while (r_check) {
            if (strcmp(r_check->patientId, pId) == 0 && r_check->type == 5 && r_check->isPaid == 0 && strstr(r_check->description, "出院清算_补缴欠费差额")) {
                printf("\n  [系统资源互斥] 底层检测到该出院流水线正在挂起，存在待处理的资金缺口！\n");
                printf("  >>> 操作指引：需告知责任患者前往前端终端支付尾款。款项入账后引擎将利用异步回调自行释放空间资源，此处拒绝重复建单操作！\n");
                system("pause");
                return;
            }
            r_check = r_check->next;
        }

        t = time(NULL);
        tm_info = localtime(&t);
        currentHour = tm_info->tm_hour;
        printf("\n键入供财务审核的最终驻留计费天数: ");
        actualDays = safeGetPositiveInt(); // 提供正整数过滤保护

        billableDays = actualDays;
        if (currentHour >= 0 && currentHour < 8) {
            printf("  [时间优惠策略触发] 08:00 阈值前申请办理，系统已自动扣减终端最后一天的驻留费用计提！\n"); billableDays = (actualDays > 1) ? actualDays - 1 : 0;
        }

        totalBedFee = billableDays * targetBed->price;
        r = recordHead->next;
        while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 4) totalDrugFee += r->cost; r = r->next; }

        totalHospitalCost = totalBedFee + totalDrugFee;
        r = recordHead->next;
        while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) total_deposit += r->cost; r = r->next; }

        // ====== 分支域 A：押金全覆盖，直接解绑资源 ======
        if (total_deposit >= totalHospitalCost) {
            r = recordHead->next;
            while (r) { if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 4) r->isPaid = 1; r = r->next; }

            refund = total_deposit - totalHospitalCost;
            if (refund > 0) {
                pt = patientHead->next;
                while (pt) { if (strcmp(pt->id, pId) == 0) { pt->balance += refund; break; } pt = pt->next; }
                printf("\n  [资金清算] 提取前期统筹押金 %.2f 元，溢出资产 %.2f 元，已执行原路资金清退。\n", total_deposit, refund);

                r8 = (Record*)malloc(sizeof(Record));
                generateRecordID(r8->recordId); r8->type = 8; strcpy(r8->patientId, pId); strcpy(r8->staffId, "SYS");
                r8->cost = refund; r8->isPaid = 1; sprintf(r8->description, "出院清算_押金结余退回");
                getCurrentTimeStr(r8->createTime, 30); r8->next = recordHead->next; recordHead->next = r8;

                newTrans = (Transaction*)malloc(sizeof(Transaction));
                curr = transactionList;
                while (curr) { if (curr->id > maxId) maxId = curr->id; curr = curr->next; }
                newTrans->id = maxId + 1; newTrans->type = 2; newTrans->amount = -refund;
                getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, "出院清算_押金结余退回");
                newTrans->next = NULL;
                if (!transactionList) transactionList = newTrans; else { curr = transactionList; while (curr->next) curr = curr->next; curr->next = newTrans; }
            }

            r = recordHead->next;
            while (r) {
                if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) {
                    r->isPaid = 2;
                    sprintf(summary, " [出院结算:床费%.2f 药费%.2f 总消费%.2f]", totalBedFee, totalDrugFee, totalHospitalCost);
                    strcat(r->description, summary);
                }
                r = r->next;
            }
            targetBed->isOccupied = 0; strcpy(targetBed->patientId, "");
            printf("\n  [解除挂载] 系统解绑指令送达，该物理床位资源已恢复闲置状态。\n");
            system("pause"); return;
        }
        // ====== 分支域 B：资金断层，建立状态锁存凭据等待外部介入 ======
        else {
            arrears = totalHospitalCost - total_deposit;
            r_arrears = (Record*)malloc(sizeof(Record));

            generateRecordID(r_arrears->recordId); r_arrears->type = 5; strcpy(r_arrears->patientId, pId); strcpy(r_arrears->staffId, "SYS");
            r_arrears->cost = arrears; r_arrears->isPaid = 0;

            // 跨模块异步通信与参数包装机制：
            // 通过格式化字符串（序列化）将床位ID、当前计费金额等关键执行参数隐写在账单描述中。
            // 当外部接口（如患者自助端）处理完这一笔待缴费订单后，可以反向利用正则捕获（反序列化）读取这些参数，
            // 从而跨越进程与权限壁垒，全自动地实现释放病床、核销账目的行为，极大减轻了人工运维难度。
            sprintf(r_arrears->description, "出院清算_补缴欠费差额_床位:%s_床费:%.2f_药费:%.2f", targetBed->bedId, totalBedFee, totalDrugFee);
            getCurrentTimeStr(r_arrears->createTime, 30); r_arrears->next = recordHead->next; recordHead->next = r_arrears;

            printf("\n  [资金拦截] 收支不平衡预警：实发总额 %.2f，储备结余 %.2f。\n", totalHospitalCost, total_deposit);
            printf("  >>> 资产缺口认定: %.2f 元。数据指针已抛出至前端财务缴费中心。 <<<\n", arrears);
            printf("  后续动作移交异步处理：患者完成终端资产确认后，底层引擎将代理执行资源的清退与解绑，当前会话完毕。\n");
            system("pause"); return;
        }
    }
}

// ---------------------------------------------------------
// 住院部核心总线控制菜单
// ---------------------------------------------------------
void inpatientMenu(const char* docId) {
    int choice;
    while (1) {
        system("cls");
        printf("\n==================================================\n");
        printf("                 住院管理调度中心                 \n");
        printf("==================================================\n");
        printf("  [1] 查看全院病房实时图谱\n");
        printf("  [2] 办理患者入院 (核算押金/床位分配)\n");
        printf("  [3] 执行每日早8点查床扣费\n");
        printf("  [4] 日常查房与下达医嘱记录\n");
        printf("  [5] 办理患者出院 (特惠与统账流转)\n");
        printf("  [0] 返回上级医生大厅\n");
        printf("--------------------------------------------------\n");
        printf("调拨系统功能执行器: ");

        // 顶层输入流接管：阻断各类非法字符与非预期越界类型导致的异常流转
        choice = safeGetInt();
        switch (choice) {
        case 1: viewAllBeds(); break;
        case 2: admitPatient(docId); system("pause"); break;
        case 3: dailyDeductionSimulation(); system("pause"); break;
        case 4: wardRounds(docId); break;
        case 5: dischargePatient(); break;
        case 0: return;
        }
    }
}