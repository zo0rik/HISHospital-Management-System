#define _CRT_SECURE_NO_WARNINGS  // 关闭 VS 对部分传统 C 函数的安全警告
#include <stdio.h>   // 标准输入输出
#include <stdlib.h>  // malloc、free、system 等
#include <string.h>  // strcpy、strcmp、strstr、strlen 等字符串处理
#include <time.h>    // time、localtime 等时间处理
#include "models.h"                  // 项目核心数据结构
#include "utils.h"                   // 工具函数，如安全输入、金额格式化、时间工具
#include "inpatient_department.h"    // 当前模块头文件
#include "transaction.h"             // 财务流水相关
#include "outpatient_department.h"   // 门诊相关，住院中会复用门诊检查功能
#include "drug.h"                    // 药品相关
#include "patient.h"                 // 患者相关

// 外部函数：生成记录 ID
extern void generateRecordID(char* buffer);
// 外部全局变量：床位链表头
extern Bed* bedHead;
// 外部安全财务流水写入函数
extern int appendTransactionSafe(int type, double amount, const char* description);

// 保存医院内已有科室名称的数组
static char hospitalDepts[50][50];
// 当前已收集到的科室数量
static int hospitalDeptCount = 0;
// 是否已完成科室初始化
static int isDeptsInitialized = 0;

// 判断一条记录是否为“住院欠费补交单”
// 条件：
// 1. 记录存在
// 2. 记录类型为 5（住院类）
// 3. 描述中包含 [住院欠费补交单]
int isInpatientArrearsBill(const Record* rec) {
    return rec &&
        rec->type == 5 &&
        strstr(rec->description, "[住院欠费补交单]") != NULL;
}

// 同步住院欠费补交单
// 作用：
// 1. 若患者住院押金为负，则创建或更新一张“待补交账单”
// 2. 若患者押金已不欠费，则把旧欠费单标记为已补清
// 3. 若存在多张未支付欠费单，只保留一张有效的，其余作废
void syncInpatientArrearsBill(Patient* patient, const char* staffId) {
    if (!patient) return;

    Record* bill = NULL;

    // 先遍历所有记录，查找该患者是否已有住院欠费补交单
    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (strcmp(r->patientId, patient->id) == 0 && isInpatientArrearsBill(r)) {
            if (r->isPaid == 0 && !bill) {
                // 第一张未支付欠费单作为当前有效账单
                bill = r;
            }
            else if (r->isPaid == 0) {
                // 多余的未支付欠费单作废
                r->isPaid = 2;
                strcpy(r->description, "[住院欠费补交单] 已被更新账单替代");
                getCurrentTimeStr(r->createTime, 30);
            }
        }
    }

    // 若当前住院押金小于 0，说明仍然欠费
    if (patient->inpatientDeposit < 0) {
        double arrears = -patient->inpatientDeposit;

        if (!bill) {
            // 若没有现成账单，则新建一张欠费补交单
            bill = (Record*)malloc(sizeof(Record));
            if (!bill) return;

            generateRecordID(bill->recordId);
            bill->type = 5;
            strcpy(bill->patientId, patient->id);
            strcpy(bill->staffId, staffId ? staffId : "SYS");
            bill->cost = arrears;
            bill->isPaid = 0;
            sprintf(bill->description, "[住院欠费补交单] 当前待补交金额: %.2f", arrears);
            getCurrentTimeStr(bill->createTime, 30);
            bill->next = recordHead->next;
            recordHead->next = bill;
        }
        else {
            // 若已有账单，则更新金额和说明
            bill->cost = arrears;
            sprintf(bill->description, "[住院欠费补交单] 当前待补交金额: %.2f", arrears);
            getCurrentTimeStr(bill->createTime, 30);
        }
    }
    else {
        // 若押金不再为负，则说明欠费已清
        if (bill) {
            bill->isPaid = 2;
            strcpy(bill->description, "[住院欠费补交单] 已补清");
            getCurrentTimeStr(bill->createTime, 30);
        }
    }
}

// 创建一条“住院明细记录”节点，但不立刻插入链表
// 参数含义：
// type: 记录类型
// patientId: 患者 ID
// staffId: 操作人员 ID
// cost: 金额
// isPaid: 支付状态
// description: 描述
static Record* createInpatientDetailRecordNode(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description) {
    Record* rec = (Record*)malloc(sizeof(Record));
    if (!rec) return NULL;

    generateRecordID(rec->recordId);
    rec->type = type;
    strncpy(rec->patientId, patientId ? patientId : "", sizeof(rec->patientId) - 1);
    rec->patientId[sizeof(rec->patientId) - 1] = '\0';
    strncpy(rec->staffId, staffId ? staffId : "", sizeof(rec->staffId) - 1);
    rec->staffId[sizeof(rec->staffId) - 1] = '\0';
    rec->cost = cost;
    rec->isPaid = isPaid;
    strncpy(rec->description, description ? description : "", sizeof(rec->description) - 1);
    rec->description[sizeof(rec->description) - 1] = '\0';
    getCurrentTimeStr(rec->createTime, 30);
    rec->next = NULL;
    return rec;
}

// 将已经准备好的记录节点插入到记录链表头部
static void appendPreparedRecord(Record* rec) {
    if (!rec) return;
    rec->next = recordHead->next;
    recordHead->next = rec;
}

// 直接创建并插入一条住院明细记录
static void appendInpatientDetailRecord(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description) {
    Record* rec = createInpatientDetailRecordNode(type, patientId, staffId, cost, isPaid, description);
    appendPreparedRecord(rec);
}

// 初始化医院科室列表
// 从 staff 链表中提取不重复的科室名称，保存到 hospitalDepts 中
static void initHospitalDepts() {
    if (isDeptsInitialized) return;

    hospitalDeptCount = 0;
    for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
        if (strlen(stf->department) == 0) continue;

        int exists = 0;
        for (int i = 0; i < hospitalDeptCount; i++) {
            if (strcmp(hospitalDepts[i], stf->department) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && hospitalDeptCount < 50) {
            strcpy(hospitalDepts[hospitalDeptCount], stf->department);
            hospitalDeptCount++;
        }
    }

    // 若没有读取到任何科室，则给一个默认值
    if (hospitalDeptCount == 0) {
        strcpy(hospitalDepts[0], "综合科");
        hospitalDeptCount = 1;
    }
    isDeptsInitialized = 1;
}

// 获取某个患者当前的负责科室
// 做法：
// 1. 从记录链表中找到 type == 6 的住院通知/住院相关记录
// 2. 取出其中的 staffId
// 3. 再到 staff 链表中找对应医生，从而得到其 department
void getResponsibleDept(const char* patientId, char* deptBuffer) {
    char staffId[20] = "";
    strcpy(deptBuffer, "未知科室");

    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (r->type == 6 && strcmp(r->patientId, patientId) == 0) {
            strcpy(staffId, r->staffId);
        }
    }

    if (strlen(staffId) > 0) {
        for (Staff* s = staffHead->next; s != NULL; s = s->next) {
            if (strcmp(s->id, staffId) == 0) {
                strcpy(deptBuffer, s->department);
                break;
            }
        }
    }
}

// 如果床位链表为空，则自动初始化全院病床
// 规则：
// 每个科室生成 5 类房型
// 每类房型 4 张床
void initBedsIfEmpty() {
    if (bedHead->next) return;

    initHospitalDepts();

    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };

    Bed* tail = bedHead;
    int roomNum = 1;

    for (int d = 0; d < hospitalDeptCount; d++) {
        for (int i = 0; i < 5; i++) {
            for (int j = 1; j <= 4; j++) {
                Bed* b = (Bed*)malloc(sizeof(Bed));
                sprintf(b->bedId, "%d-%d", roomNum, j);
                b->isOccupied = 0;
                strcpy(b->patientId, "");
                strcpy(b->bedType, types[i]);
                b->price = prices[i];
                strcpy(b->wardType, (i >= 3) ? wards[1] : wards[0]);
                b->isRoundsDone = 0;
                b->next = NULL;
                tail->next = b;
                tail = b;
            }
            roomNum++;
        }
    }
}

// 根据床位号推断该床位所属科室
// 规则：一个科室对应连续 5 个房间号
const char* getRoomDepartment(const char* bedId) {
    initHospitalDepts();

    int roomNum;
    if (sscanf(bedId, "%d-", &roomNum) != 1) return "未知科室";

    int deptIdx = (roomNum - 1) / 5;
    if (deptIdx >= 0 && deptIdx < hospitalDeptCount) {
        return hospitalDepts[deptIdx];
    }
    return "未知科室";
}

// 检查某个床位号是否已存在
static int bedIdExists(const char* bedId) {
    if (!bedId) return 0;
    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (strcmp(b->bedId, bedId) == 0) return 1;
    }
    return 0;
}

// 检查并调整某科室床位紧张程度
// 若该科室空床率低于 20%，则尝试把“单人陪护疗养病房”拆分成双人病房扩容
void checkAndAdjustBedTension(const char* targetDept) {
    int total = 0, empty = 0;

    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
            total++;
            if (b->isOccupied == 0) empty++;
        }
    }

    if (total > 0 && ((double)empty / total) < 0.2) {
        printf("\n  [系统动态调度] %s 专属空床率不足 20%%！触发紧急扩容机制...\n", targetDept);
        printf("  >>> 正在将【%s】内的单人陪护疗养病房临时调整为双人病房 <<<\n", targetDept);

        int converted = 0;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (b->isOccupied == 0 &&
                strcmp(b->bedType, "单人陪护疗养病房") == 0 &&
                strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {

                // 把原床位改成双人病房
                strcpy(b->bedType, "双人病房");
                b->price = 150.0;

                // 复制出一个额外床位
                Bed* extra = (Bed*)malloc(sizeof(Bed));
                if (!extra) continue;
                *extra = *b;

                // 给复制床位一个新 ID，例如原本 3-1，复制后可能变成 3-A
                strncpy(extra->bedId, b->bedId, sizeof(extra->bedId) - 2);
                extra->bedId[sizeof(extra->bedId) - 2] = '\0';
                size_t blen = strlen(extra->bedId);
                if (blen + 1 >= sizeof(extra->bedId)) {
                    free(extra);
                    continue;
                }
                extra->bedId[blen] = 'A';
                extra->bedId[blen + 1] = '\0';

                // 防止床位号重复
                if (bedIdExists(extra->bedId)) {
                    free(extra);
                    continue;
                }

                // 头插到床位链表
                extra->next = bedHead->next;
                bedHead->next = extra;
                converted++;
            }
        }

        if (converted > 0) {
            printf("  [扩容成功] 已为 %s 增加了 %d 张可用双人床位！\n", targetDept, converted);
        }
        else {
            printf("  [扩容失败] %s 当前已无空余的单人疗养病房可供拆分。\n", targetDept);
        }
    }
}

// 获取动态科室提示字符串
// 例如：内科/外科/骨科
void getDynamicDeptPrompt(char* promptBuffer) {
    char depts[20][50];
    int dCount = 0;

    for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
        int exists = 0;
        for (int i = 0; i < dCount; i++) {
            if (strcmp(depts[i], stf->department) == 0) {
                exists = 1;
                break;
            }
        }
        if (!exists && strlen(stf->department) > 0) {
            strcpy(depts[dCount], stf->department);
            dCount++;
        }
    }

    strcpy(promptBuffer, "");
    for (int i = 0; i < dCount; i++) {
        strcat(promptBuffer, depts[i]);
        if (i < dCount - 1) strcat(promptBuffer, "/");
    }
}

// 查看全院床位图
// 先按科室筛选，再打印该科室下的所有床位信息
void viewAllBeds() {
    initBedsIfEmpty();
    while (1) {
        char deptStr[200];
        char targetDept[50];

        system("cls");
        printf("\n========================================================================================\n");
        printf("                            全院病房与床位实时使用雷达图谱                               \n");
        printf("========================================================================================\n");

        getDynamicDeptPrompt(deptStr);
        printf("当前活跃科室有: (%s)\n请输入要查看的科室 (输-1返回): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "-1") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) {
                    isValidDept = 1;
                    break;
                }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n请输入要查看的科室 (输-1返回): ");
        }

        printf("\n>>> 【%s】 专属住院病区动态 <<<\n", targetDept);
        printf("%-10s %-12s %-18s %-8s %-10s %-15s %-15s\n", "房号-床位", "病区等级", "房型", "单价", "目前状态", "收治大夫", "入住患者");
        printf("----------------------------------------------------------------------------------------\n");

        int bedCount = 0;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                char patName[100] = "-";
                char docName[50] = "-";

                if (b->isOccupied) {
                    // 找患者姓名
                    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                        if (strcmp(p->id, b->patientId) == 0) {
                            strcpy(patName, p->name);
                            break;
                        }
                    }

                    // 找负责医生姓名
                    for (Record* r = recordHead->next; r != NULL; r = r->next) {
                        if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                            for (Staff* s = staffHead->next; s != NULL; s = s->next) {
                                if (strcmp(s->id, r->staffId) == 0) {
                                    strcpy(docName, s->name);
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }

                printf("%-10s %-12s %-18s %-8.2f %-10s %-15s %s(%s)\n",
                    b->bedId, b->wardType, b->bedType, b->price,
                    b->isOccupied ? "[有客]" : "[闲置]", docName,
                    b->isOccupied ? patName : "-", b->isOccupied ? b->patientId : "-");
                bedCount++;
            }
        }

        if (bedCount == 0) printf("  (未找到该科室的专属病床数据)\n");
        printf("----------------------------------------------------------------------------------------\n");
        system("pause");
    }
}

// 办理患者入院
// 流程：
// 1. 查看未处理住院通知单
// 2. 选择患者
// 3. 定位负责科室
// 4. 分配床位
// 5. 计算住院押金
// 6. 从普通账户扣押金并转入住院押金
// 7. 激活住院状态
void admitPatient(const char* docId) {
    initBedsIfEmpty();
    int noticeCount = 0;

    printf("\n========== 门诊下发《待入院通知单》队列 ==========\n");

    // 先显示重症队列
    printf("【重症优先通道】\n");
    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "重症")) {
            char pName[100] = "未知";
            for (Patient* pt = patientHead->next; pt != NULL; pt = pt->next) {
                if (strcmp(pt->id, r->patientId) == 0) {
                    strcpy(pName, pt->name);
                    break;
                }
            }
            char deptName[50];
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [紧急] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n",
                r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
    }

    // 再显示普通入院队列
    printf("\n【普通入院队列】\n");
    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "普通")) {
            char pName[100] = "未知";
            for (Patient* pt = patientHead->next; pt != NULL; pt = pt->next) {
                if (strcmp(pt->id, r->patientId) == 0) {
                    strcpy(pName, pt->name);
                    break;
                }
            }
            char deptName[50];
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [常规] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n",
                r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
    }

    if (noticeCount == 0) {
        printf("  [提示] 当前暂无门诊下发的未处理住院通知单。\n");
        return;
    }

    char pId[20];
    Record* targetNotice = NULL;
    Patient* targetPat = NULL;

    // 输入患者 ID，并确保有未处理通知单，且患者未住院
    while (1) {
        printf("\n请输入需办理入院的患者ID (输-1返回): ");
        safeGetString(pId, 20);
        if (strcmp(pId, "-1") == 0) return;

        targetNotice = NULL;
        for (Record* r = recordHead->next; r != NULL; r = r->next) {
            if (r->type == 6 && strcmp(r->patientId, pId) == 0 && r->isPaid == 0) {
                targetNotice = r;
                break;
            }
        }

        if (!targetNotice) {
            int alreadyIn = 0;
            for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                if (strcmp(p->id, pId) == 0 && p->isInpatient == 1) {
                    alreadyIn = 1;
                    break;
                }
            }
            if (alreadyIn) printf("  [!] 拦截警告：该患者当前已处于住院收治状态，请重新输入！\n");
            else printf("  [!] 拦截警告：缺乏门诊前端开具的合规通知单，无法受理收治，请重新输入！\n");
            continue;
        }

        targetPat = NULL;
        for (Patient* p = patientHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, pId) == 0) {
                targetPat = p;
                break;
            }
        }

        if (!targetPat) {
            printf("  [!] 患者基础信息不存在，请重新输入！\n");
            continue;
        }
        if (targetPat->isInpatient == 1) {
            printf("  [!] 该患者当前已处于住院状态，不能重复办理入院！\n");
            continue;
        }
        break;
    }

    char requiredDept[50];
    getResponsibleDept(pId, requiredDept);
    printf("\n>>> 锁定通知单：该患者由【%s】下发，系统正检索 %s 的专属空余床位...\n", requiredDept, requiredDept);

    // 检查目标科室床位是否紧张，必要时自动扩容
    checkAndAdjustBedTension(requiredDept);

    int hasLocalEmpty = 0;
    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
            hasLocalEmpty = 1;
            break;
        }
    }

    int isCrossDept = 0;

    // 若本科室已无空床，则启用跨科室调度
    if (!hasLocalEmpty) {
        printf("\n  [警告] %s 专属床位资源枯竭（扩容后仍无空余）！\n", requiredDept);
        printf("  >>> 触发系统应急预案：开启跨科室床位全局调度 <<<\n");
        isCrossDept = 1;

        int hasAnyEmpty = 0;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (!b->isOccupied) {
                hasAnyEmpty = 1;
                break;
            }
        }

        if (!hasAnyEmpty) {
            printf("  [极其抱歉] 全院整体床位网络满载，暂无法提供物理空间进行收治转运。\n");
            return;
        }

        printf("\n【全院可用空闲病床列表 (跨域收治)】:\n");
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (!b->isOccupied) {
                printf("  [%s] 所属: %-6s | %s - %s (基准日租 ￥%.2f)\n",
                    b->bedId, getRoomDepartment(b->bedId), b->wardType, b->bedType, b->price);
            }
        }
    }
    else {
        printf("\n【%s】可用空闲病床列表:\n", requiredDept);
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
                printf("  [%s] %s - %s (基准日租 ￥%.2f)\n", b->bedId, b->wardType, b->bedType, b->price);
            }
        }
    }

    char selectBed[20];
    Bed* finalBed = NULL;

    // 输入并校验最终选择的床位号
    while (1) {
        printf("\n请输入系统拟分配的床位号 (如 1-3) (输入-1取消): ");
        safeGetString(selectBed, 20);
        if (strcmp(selectBed, "-1") == 0) return;

        finalBed = NULL;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (strcmp(b->bedId, selectBed) == 0 && !b->isOccupied) {
                if (isCrossDept || strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) {
                    finalBed = b;
                    break;
                }
            }
        }

        if (finalBed) break;

        if (!isCrossDept) printf("  [!] 无效选择，资源已被占用或该床位不存在/越界，请重新输入！\n");
        else printf("  [!] 无效选择，资源已被占用或该床位不存在，请重新输入！\n");
    }

    printf("\n  [√] 资源 [%s] 分配锁定成功！\n", finalBed->bedId);
    printf("请输入拟定住院周期(天): ");
    int days = safeGetPositiveInt();
    if (days == -1) return;

    // 押金规则：200 * 天数，最低 1000，向上取整到 100 元
    int baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    int finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("  系统通过风控模型核算，需缴纳初始住院统筹押金: %d 元。\n", finalDeposit);

    // 普通余额不足则无法正式办理入院
    if (targetPat->balance < finalDeposit) {
        printf("  [提示] 账户实时结余不足，住院押金未完成缴纳，暂不办理正式入院。\n");
        printf("  [提示] 当前普通账户余额: %.2f 元，押金要求: %d 元。\n", targetPat->balance, finalDeposit);
        system("pause");
        return;
    }

    Record* depositDetail = NULL;
    {
        // 创建住院押金明细记录
        char adminTime[30];
        char detailDesc[300];
        getCurrentTimeStr(adminTime, 30);
        sprintf(detailDesc,
            "[住院明细][押金] 病房:%s_床位:%s_入院日期:%s_预期天数:%d_押金:%d",
            finalBed->wardType, finalBed->bedId, adminTime, days, finalDeposit);
        depositDetail = createInpatientDetailRecordNode(5, pId, docId, finalDeposit, 1, detailDesc);
    }
    if (!depositDetail) {
        printf("  [异常] 住院明细记录创建失败，本次入院未提交。\n");
        system("pause");
        return;
    }

    {
        // 写财务流水：住院押金收取，但不计收入
        char transDesc[200];
        sprintf(transDesc, "住院押金收取(患者:%s, 不计收入)", targetPat->name);
        if (!appendTransactionSafe(TRANS_INPATIENT_DEPOSIT, (double)finalDeposit, transDesc)) {
            free(depositDetail);
            printf("  [异常] 财务流水写入失败，本次入院未提交。\n");
            system("pause");
            return;
        }
    }

    // 更新患者状态和资金
    targetPat->balance -= finalDeposit;
    targetPat->inpatientDeposit += finalDeposit;
    targetPat->isInpatient = 1;

    printf("  [√] 账户资金划扣完毕，病区收治状态已激活。\n");

    // 更新床位状态
    finalBed->isOccupied = 1;
    strcpy(finalBed->patientId, pId);
    finalBed->isRoundsDone = 0;

    // 通知单处理完成
    targetNotice->isPaid = 1;

    // 插入住院押金明细
    appendPreparedRecord(depositDetail);

    if (isCrossDept) {
        printf("\n  [成功] 跨模块调度机制完成，已将病患安置于 [%s] 的 %s 节点。\n",
            getRoomDepartment(finalBed->bedId), finalBed->bedId);
    }
    else {
        printf("\n  [成功] 基础收治流程完结，已分配至 %s 的 %s 节点。\n",
            requiredDept, finalBed->bedId);
    }

    system("pause");
}

// 查看在院病人中住院押金低于 300 元的名单
void viewLowDepositInpatients(void) {
    int found = 0;
    char depBuf[32];

    printf("\n========== 在院病人住院押金预警列表 (低于300元) ==========\n");
    printf("%-15s %-10s %-12s %-12s\n", "患者ID", "床位", "姓名", "押金余额");
    printf("--------------------------------------------------------------\n");

    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (!b->isOccupied) continue;
        for (Patient* p = patientHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, b->patientId) == 0 && p->isInpatient && p->inpatientDeposit < 300.0) {
                formatMoney(p->inpatientDeposit, depBuf, sizeof(depBuf));
                printf("%-15s %-10s %-12s %-12s\n", p->id, b->bedId, p->name, depBuf);
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        printf("当前无住院押金低于300元的在院病人。\n");
    }
}

// 日常查房
// 功能：
// 1. 按科室查看在院病人
// 2. 录入基础医嘱
// 3. 调用药房配药
// 4. 开辅助检查并直接从住院押金扣费
void wardRounds(const char* docId) {
    while (1) {
        char deptStr[200];
        char targetDept[50];

        system("cls");
        printf("\n========== 住院病区日常巡检与查房 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("调取目标科室域 (%s, 输入-1返回): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "-1") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) {
                    isValidDept = 1;
                    break;
                }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n调取目标科室域 (输-1返回): ");
        }

        printf("\n--- 【%s】 当前住院受管名单 ---\n", targetDept);
        printf("%-10s %-15s %-10s %-10s %-10s\n", "房-床号", "患者ID", "姓名", "评估级别", "查房状态");
        printf("----------------------------------------------------------\n");

        int pCount = 0;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (b->isOccupied && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                char pName[100] = "未知";
                char severity[10] = "普通";

                for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                    if (strcmp(p->id, b->patientId) == 0) {
                        strcpy(pName, p->name);
                        break;
                    }
                }

                // 根据 type == 6 的记录描述判断是否为重症
                for (Record* r = recordHead->next; r != NULL; r = r->next) {
                    if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                        if (strstr(r->description, "重症")) strcpy(severity, "重症");
                    }
                }

                printf("%-10s %-15s %-10s %-10s %-10s\n",
                    b->bedId, b->patientId, pName, severity, b->isRoundsDone ? "[已查房]" : "[待查房]");
                pCount++;
            }
        }

        if (pCount == 0) {
            printf("  (目标病区当前无处于活跃周期的在院人员)\n");
            system("pause");
            continue;
        }

        char pId[20];
        Bed* targetBed = NULL;

        // 选择要查房的患者
        while (1) {
            printf("\n检索需建立查房会话的患者ID (输入-1退回科室层级): ");
            safeGetString(pId, 20);
            if (strcmp(pId, "-1") == 0) break;

            targetBed = NULL;
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (b->isOccupied &&
                    strcmp(b->patientId, pId) == 0 &&
                    strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                    targetBed = b;
                    break;
                }
            }
            if (targetBed) break;
            printf("  [!] 定位失败：未在 %s 的管辖范畴内检索到对应人员，请重新输入！\n", targetDept);
        }
        if (strcmp(pId, "-1") == 0) continue;

        // 进入该患者的查房面板
        while (1) {
            system("cls");
            printf("\n========== 针对患者实体 [%s] 的查房干预面板 ==========\n", pId);
            printf("  [1] 录入基础观察医嘱\n");
            printf("  [2] 调用内部药房系统配发治疗耗材\n");
            printf("  [3] 开立辅助检查并直接从住院押金扣费\n");
            printf("  [-1] 结束当前查房会话\n");
            printf("---------------------------------------\n");
            printf("下达操作指令: ");

            int choice;
            while (1) {
                choice = safeGetInt();
                if (choice == -1 || (choice >= 1 && choice <= 3)) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n下达操作指令: ");
            }
            if (choice == -1) break;

            // 录入基础医嘱
            if (choice == 1) {
                char note[200];
                printf("输入实时医嘱指令内容(避免非法空格切断字符, 输入-1取消): ");
                safeGetString(note, 200);
                if (strcmp(note, "-1") == 0) continue;

                Record* rx = (Record*)malloc(sizeof(Record));
                generateRecordID(rx->recordId);
                rx->type = 2;
                strcpy(rx->patientId, pId);
                strcpy(rx->staffId, docId);
                rx->cost = 0;
                rx->isPaid = 1;
                sprintf(rx->description, "住院查房医嘱:%s", note);
                getCurrentTimeStr(rx->createTime, 30);
                rx->next = recordHead->next;
                recordHead->next = rx;

                printf("  [√] 会话操作确认，医嘱项已持久化记录。\n");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
            // 调用门诊处方开药逻辑
            else if (choice == 2) {
                extern char currentCallingPatientId[20];
                strcpy(currentCallingPatientId, pId);

                extern void prescribeMedicine(const char* docId);
                prescribeMedicine(docId);

                strcpy(currentCallingPatientId, "");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
            // 调用门诊辅助检查逻辑，但以住院模式扣费
            else if (choice == 3) {
                extern char currentCallingPatientId[20];
                strcpy(currentCallingPatientId, pId);
                createAuxiliaryExamOrders(docId, pId, 1);
                strcpy(currentCallingPatientId, "");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
        }
    }
}

// 办理出院
// 流程：
// 1. 按科室查看待出院患者
// 2. 选择患者
// 3. 输入实际住院天数
// 4. 结算床位费
// 5. 若押金有剩余则退回普通余额
// 6. 若押金不足则生成欠费补交单
// 7. 释放床位，取消住院状态
void dischargePatient() {
    while (1) {
        char deptStr[200];
        char targetDept[50];

        system("cls");
        printf("\n========== 离院综合清算办理控制台 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("锁定业务作用域科室 (%s, 输入-1放弃办理): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "-1") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) {
                    isValidDept = 1;
                    break;
                }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n锁定业务作用域科室 (输-1放弃办理): ");
        }

        printf("\n--- 【%s】 等待出院审批名册 ---\n", targetDept);
        printf("%-10s %-12s %-15s %-10s\n", "房号-床位", "属性", "患者ID", "姓名");
        printf("-----------------------------------------------------\n");

        int count = 0;
        for (Bed* b_list = bedHead->next; b_list != NULL; b_list = b_list->next) {
            if (b_list->isOccupied && strcmp(getRoomDepartment(b_list->bedId), targetDept) == 0) {
                char pName[100] = "未知";
                for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                    if (strcmp(p->id, b_list->patientId) == 0) {
                        strcpy(pName, p->name);
                        break;
                    }
                }
                printf("%-10s %-12s %-15s %-10s\n", b_list->bedId, b_list->wardType, b_list->patientId, pName);
                count++;
            }
        }

        if (count == 0) {
            printf("  (当前无可流转的出院对象)\n");
            system("pause");
            continue;
        }

        char pId[20];
        Bed* targetBed = NULL;

        // 选择出院患者
        while (1) {
            printf("\n确立清算实体的患者ID (输入-1终止当前操作): ");
            safeGetString(pId, 20);
            if (strcmp(pId, "-1") == 0) break;

            targetBed = NULL;
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (b->isOccupied &&
                    strcmp(b->patientId, pId) == 0 &&
                    strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                    targetBed = b;
                    break;
                }
            }
            if (targetBed) break;
            printf("  [!] 校验异常：所选区域中无此患者实体记录，请重新输入！\n");
        }
        if (strcmp(pId, "-1") == 0) continue;

        time_t t = time(NULL);
        struct tm* tm_info = localtime(&t);
        int currentHour = tm_info->tm_hour;

        printf("\n键入供财务审核的最终驻留计费天数: ");
        int actualDays = safeGetPositiveInt();
        if (actualDays == -1) continue;

        int billableDays = actualDays;

        // 若在 08:00 前出院，触发优惠：最后一天床位费不计
        if (currentHour >= 0 && currentHour < 8) {
            printf("  [时间优惠策略触发] 08:00 阈值前申请办理，系统已自动扣减终端最后一天的驻留费用计提！\n");
            billableDays = (actualDays > 1) ? actualDays - 1 : 0;
        }

        double totalBedFee = billableDays * targetBed->price;

        Patient* targetPat = findPatientById(pId);
        if (!targetPat) {
            printf("  [!] 未找到患者信息。\n");
            system("pause");
            return;
        }

        {
            char desc[300];
            Record* bedFeeDetail = NULL;
            Record* refundDetail = NULL;
            double depositAfterFee = targetPat->inpatientDeposit - totalBedFee;

            // 创建床位费明细
            sprintf(desc, "[住院明细][床位费] 床位:%s_计费天数:%d_床位单价:%.2f",
                targetBed->bedId, billableDays, targetBed->price);
            bedFeeDetail = createInpatientDetailRecordNode(5, pId, "SYS", totalBedFee, 1, desc);
            if (!bedFeeDetail) {
                printf("\n  [异常] 床位费明细创建失败，本次出院结算未完成。\n");
                system("pause");
                return;
            }

            // 写床位费收入流水
            if (!appendTransactionSafe(TRANS_INPATIENT_BED_INCOME, totalBedFee, "出院结算床位费")) {
                free(bedFeeDetail);
                printf("\n  [异常] 财务流水写入失败，本次出院结算未完成，请稍后重试。\n");
                system("pause");
                return;
            }

            // 若押金结余大于 0，则准备退款明细
            if (depositAfterFee > 0) {
                refundDetail = createInpatientDetailRecordNode(
                    8, pId, "SYS", depositAfterFee, 1,
                    "[住院明细][退回] 出院后住院押金结余退回普通账户"
                );
                if (!refundDetail) {
                    printf("\n  [异常] 退款明细创建失败，请联系财务核查。\n");
                    system("pause");
                    return;
                }
                if (!appendTransactionSafe(TRANS_INPATIENT_REFUND, depositAfterFee, "住院押金结余退回(不计收入)")) {
                    free(refundDetail);
                    printf("\n  [异常] 退款流水写入失败，请联系财务核查。\n");
                    system("pause");
                    return;
                }
            }

            // 更新住院押金余额
            targetPat->inpatientDeposit = depositAfterFee;
            appendPreparedRecord(bedFeeDetail);

            if (depositAfterFee > 0) {
                // 有结余则退回普通余额
                double refund = depositAfterFee;
                targetPat->balance += refund;
                targetPat->inpatientDeposit = 0;
                appendPreparedRecord(refundDetail);
                printf("\n  [资金清算] 住院押金结余 %.2f 元，已退回普通账户余额。\n", refund);
            }
            else if (depositAfterFee < 0) {
                // 不够支付床位费，则生成欠费补交单
                syncInpatientArrearsBill(targetPat, "SYS");
                printf("\n  [资金预警] 出院结算后仍欠费 %.2f 元，已生成待补交账单。\n", -targetPat->inpatientDeposit);
            }
            else {
                // 刚好结清
                printf("\n  [资金清算] 出院结算完成，住院押金已刚好结清。\n");
            }
        }

        // 释放床位
        targetBed->isOccupied = 0;
        strcpy(targetBed->patientId, "");
        targetBed->isRoundsDone = 0;

        // 取消住院状态
        targetPat->isInpatient = 0;

        printf("  [解除挂载] 系统解绑指令送达，该物理床位资源已恢复闲置状态。\n");
        system("pause");
        return;
    }
}

// 住院管理主菜单
void inpatientMenu(const char* docId) {
    while (1) {
        system("cls");
        printf("\n==================================================\n");
        printf("                 住院管理调度中心                 \n");
        printf("==================================================\n");
        printf("  [1] 查看全院病房实时图谱\n");
        printf("  [2] 办理患者入院 (核算押金/床位分配)\n");
        printf("  [3] 查看在院病人中住院押金低于300元名单\n");
        printf("  [4] 日常查房/医嘱/药房/辅助检查\n");
        printf("  [5] 办理患者出院 (特惠与统账流转)\n");
        printf("  [-1] 返回上级医生大厅\n");
        printf("--------------------------------------------------\n");
        printf("调拨系统功能执行器: ");

        int choice;
        while (1) {
            choice = safeGetInt();
            if (choice == -1 || (choice >= 1 && choice <= 5)) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
        }

        switch (choice) {
        case 1:
            viewAllBeds();
            break;
        case 2:
            admitPatient(docId);
            system("pause");
            break;
        case 3:
            viewLowDepositInpatients();
            system("pause");
            break;
        case 4:
            wardRounds(docId);
            break;
        case 5:
            dischargePatient();
            break;
        case -1:
            return;
        }
    }
}