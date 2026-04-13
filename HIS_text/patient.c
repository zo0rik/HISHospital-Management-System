#define _CRT_SECURE_NO_WARNINGS  
#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>  
#include <time.h>    // time、localtime、strftime 等时间处理
#include "models.h"                  // 项目中的核心数据结构定义
#include "patient.h"                 // patient 模块头文件
#include "utils.h"                   // 工具函数，如安全输入、金额格式化、时间字符串等
#include "doctor.h"                  // 医生相关声明
#include "schedule.h"                // 排班相关声明
#include "transaction.h"             // 财务流水相关声明
#include "drug.h"                    // 药品相关声明
#include "inpatient_department.h"    // 住院部相关声明

// 外部函数：生成一条 Record 的唯一 ID
extern void generateRecordID(char* buffer);

// 外部全局变量：床位链表头
extern Bed* bedHead;

// 外部安全追加流水函数，成功返回真，失败返回假
extern int appendTransactionSafe(int type, double amount, const char* description);

// 获取当前年份。
// 如果 localtime 失败，则默认返回 2026。
static int getCurrentYearValue(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    return tm_info ? (tm_info->tm_year + 1900) : 2026;
}

// 从一个 ID 字符串末尾提取最后 4 位数字序号。
// 例如：P20260001 -> 1
// 如果字符串长度不足 4，或提取失败，则返回 -1。
static int extractTrailingSequence(const char* id) {
    int len = (int)strlen(id);
    if (len < 4) return -1;
    int seq = -1;
    if (sscanf(id + len - 4, "%4d", &seq) == 1) return seq;
    return -1;
}

// 从药品记录描述字符串中解析药品名称和数量。
// 仅支持 type == 3 的药品记录。
// 预期 description 格式类似：
// 药品:阿莫西林_单价:12.50元_数量:2
// 解析成功返回 1，失败返回 0。
static int parseDrugRecordInfo(const Record* rec, char* drugName, size_t drugNameSize, int* qtyOut) {
    if (!rec || rec->type != 3 || !drugName || drugNameSize == 0 || !qtyOut) return 0;
    drugName[0] = '\0';
    *qtyOut = 0;
    if (sscanf(rec->description, "药品:%49[^_]_单价:%*[^_]_数量:%d", drugName, qtyOut) == 2 && *qtyOut > 0) {
        return 1;
    }
    return 0;
}

// 根据药品名称精确查找药品。
// 找到则返回对应 Drug 节点指针，否则返回 NULL。
static Drug* findDrugByName(const char* name) {
    for (Drug* d = drugList->next; d != NULL; d = d->next) {
        if (strcmp(d->name, name) == 0) return d;
    }
    return NULL;
}

// 检查某条药品记录对应的库存是否足够。
// 若记录无法解析，则默认返回 1，不阻塞后续流程。
// 若能解析出药品名和数量，则必须保证药品存在且库存足够。
static int ensureDrugStockForRecord(const Record* rec) {
    char drugName[50];
    int qty = 0;
    if (!parseDrugRecordInfo(rec, drugName, sizeof(drugName), &qty)) return 1;
    Drug* drug = findDrugByName(drugName);
    return drug && drug->stock >= qty;
}

// 为某条药品记录创建一条“发药历史”节点。
// 同时把对应药品指针和数量通过输出参数返回。
// 返回值约定：
// 1. 正常成功：返回 DrugHistory* 节点
// 2. 解析失败：返回 NULL
// 3. 药品不存在 / 库存不足 / 内存分配失败：返回 (DrugHistory*)-1
static DrugHistory* createDrugDispenseHistoryForRecord(const Record* rec, Drug** drugOut, int* qtyOut) {
    char drugName[50];
    int qty = 0;
    if (!parseDrugRecordInfo(rec, drugName, sizeof(drugName), &qty)) {
        if (drugOut) *drugOut = NULL;
        if (qtyOut) *qtyOut = 0;
        return NULL;
    }

    Drug* drug = findDrugByName(drugName);
    if (!drug || drug->stock < qty) return (DrugHistory*)-1;

    DrugHistory* hist = (DrugHistory*)malloc(sizeof(DrugHistory));
    if (!hist) return (DrugHistory*)-1;

    hist->drug_id = drug->id;                         // 记录药品 ID
    hist->type = 2;                                  // 2 表示出库/发药
    hist->quantity = qty;                            // 记录本次发药数量
    getCurrentTimeStr(hist->time, sizeof(hist->time)); // 记录当前时间
    hist->next = NULL;

    if (drugOut) *drugOut = drug;
    if (qtyOut) *qtyOut = qty;
    return hist;
}

// 真正提交发药动作：
// 1. 扣减药品库存
// 2. 更新药品最近出库时间
// 3. 将历史记录挂到 drugHistoryList 链表头
static void commitDrugDispense(Drug* drug, int qty, DrugHistory* hist) {
    if (!drug || !hist || qty <= 0) return;
    drug->stock -= qty;
    strncpy(drug->last_out, hist->time, sizeof(drug->last_out) - 1);
    drug->last_out[sizeof(drug->last_out) - 1] = '\0';
    hist->next = drugHistoryList->next;
    drugHistoryList->next = hist;
}

// 生成患者 ID。
// 规则：P + 当前年份 + 四位递增序号
// 例如：P20260001
void generatePatientID(char* idBuffer) {
    int maxId = 999;
    int currentIdNum;
    int currentYear = getCurrentYearValue();
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        currentIdNum = extractTrailingSequence(p->id);
        if (currentIdNum > maxId) maxId = currentIdNum;
    }
    sprintf(idBuffer, "P%d%04d", currentYear, maxId + 1);
}

// 根据患者 ID 精确查找患者。
// 找到返回患者节点指针，找不到返回 NULL。
Patient* findPatientById(const char* pid) {
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (strcmp(p->id, pid) == 0) return p;
    }
    return NULL;
}

// 收集当前系统中所有不重复的科室名称。
// depts 由调用方提供，maxCount 表示最多收集多少个科室。
// 返回实际收集到的科室数量。
int collectHospitalDepartments(char depts[][50], int maxCount) {
    int dCount = 0;
    if (!depts || maxCount <= 0) return 0;

    for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
        int exists = 0;
        if (strlen(stf->department) == 0) continue;

        for (int i = 0; i < dCount; ++i) {
            if (strcmp(depts[i], stf->department) == 0) {
                exists = 1;
                break;
            }
        }

        if (!exists && dCount < maxCount) {
            strcpy(depts[dCount], stf->department);
            dCount++;
        }
    }
    return dCount;
}

// 判断某个科室名称是否存在于当前系统中。
int isKnownHospitalDepartment(const char* deptName) {
    char depts[50][50];
    int dCount = 0;
    if (!deptName || strlen(deptName) == 0) return 0;

    dCount = collectHospitalDepartments(depts, 50);
    for (int i = 0; i < dCount; ++i) {
        if (strcmp(depts[i], deptName) == 0) return 1;
    }
    return 0;
}

// 按指定分隔符拼接当前系统中的动态科室提示字符串。
void buildHospitalDeptPrompt(char* promptBuffer, size_t size, const char* separator) {
    char depts[50][50];
    int dCount = 0;
    size_t used = 0;

    if (!promptBuffer || size == 0) return;
    promptBuffer[0] = '\0';
    if (!separator) separator = "/";

    dCount = collectHospitalDepartments(depts, 50);
    for (int i = 0; i < dCount; ++i) {
        int written = snprintf(promptBuffer + used, size - used, "%s%s", (i == 0) ? "" : separator, depts[i]);
        if (written < 0 || (size_t)written >= size - used) {
            promptBuffer[size - 1] = '\0';
            return;
        }
        used += (size_t)written;
    }
}

// 统一读取并校验患者 ID 是否存在。
// 返回 1 表示拿到了有效患者 ID；返回 0 表示用户输入 -1 取消。
int inputExistingPatientIdCommon(char* pId, size_t size, const char* prompt) {
    while (1) {
        Patient* targetPat;
        printf("%s", prompt ? prompt : "请输入患者ID: ");
        safeGetString(pId, (int)size);
        if (strcmp(pId, "-1") == 0) return 0;
        targetPat = findPatientById(pId);
        if (targetPat) return 1;
        printf("  [!] 患者ID不存在，请重新输入。\n");
    }
}

// 判断某条记录是否是“住院欠费补交单”。
// 条件：
// 1. 记录存在
// 2. type == 5（住院类）
// 3. description 中包含 [住院欠费补交单]
static int isInpatientArrearsRecord(const Record* rec) {
    return rec &&
        rec->type == 5 &&
        strstr(rec->description, "[住院欠费补交单]") != NULL;
}

// 追加一条简单记录的函数声明
static void appendSimpleRecord(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description);

// 支付后自动检查是否满足出院条件的函数声明
static void checkAndAutoDischarge(Record* rec, const char* currentPatientId);

// 根据记录类型，映射到财务流水类型。
// 用于在支付时写入不同种类的财务收入。
static int determineTransactionTypeForRecord(const Record* rec) {
    if (!rec) return TRANS_OUTPATIENT_INCOME;
    if (rec->type == 3) return TRANS_DRUG_INCOME;
    if (rec->type == 4) return TRANS_EXAM_INCOME;
    if (rec->type == 5) return TRANS_INPATIENT_BED_INCOME;
    return TRANS_OUTPATIENT_INCOME;
}

// 结算单条记录。
// 主要功能：
// 1. 校验该记录是否可支付
// 2. 若是药品单，先校验库存并准备发药历史
// 3. 若是住院欠费补交单，则把普通余额转入住院押金
// 4. 若是普通业务，则正常记收入并扣普通余额
// 5. 若是药品单并支付成功，则提交出库
// 成功返回 1，失败返回 0。
static int settleSingleRecord(Patient* p, const char* currentPatientId, Record* rec) {
    if (!p || !currentPatientId || !rec || rec->isPaid != 0) return 0;
    if (p->balance < rec->cost) return 0;

    Drug* dispenseDrug = NULL;
    DrugHistory* dispenseHist = NULL;
    int dispenseQty = 0;

    // 若是药品单，先检查库存并构造出库历史
    if (rec->type == 3) {
        if (!ensureDrugStockForRecord(rec)) return 0;
        dispenseHist = createDrugDispenseHistoryForRecord(rec, &dispenseDrug, &dispenseQty);
        if (dispenseHist == (DrugHistory*)-1) return 0;
    }

    // 若是住院欠费补交单，则不是普通收入，而是给住院押金补钱
    if (isInpatientArrearsRecord(rec)) {
        if (!appendTransactionSafe(TRANS_INPATIENT_SUPPLEMENT, rec->cost, "住院欠费补交(不计收入)")) {
            if (dispenseHist && dispenseHist != (DrugHistory*)-1) free(dispenseHist);
            return 0;
        }
        p->balance -= rec->cost;           // 普通余额减少
        p->inpatientDeposit += rec->cost;  // 住院押金增加
        rec->isPaid = 2;                   // 标记补缴完成
        appendSimpleRecord(5, currentPatientId, "SYS", rec->cost, 1, "[住院明细][补缴] 住院欠费补交完成");
    }
    else {
        // 普通支付：记收入、扣余额、标记已支付
        int transType = determineTransactionTypeForRecord(rec);
        if (!appendTransactionSafe(transType, rec->cost, rec->description)) {
            if (dispenseHist && dispenseHist != (DrugHistory*)-1) free(dispenseHist);
            return 0;
        }
        p->balance -= rec->cost;
        rec->isPaid = 1;
        if (dispenseHist) {
            // 若是药品单，则在支付成功后再真正发药出库
            commitDrugDispense(dispenseDrug, dispenseQty, dispenseHist);
            dispenseHist = NULL;
        }
    }

    // 预留：支付后检查是否自动出院
    checkAndAutoDischarge(rec, currentPatientId);
    return 1;
}

// 追加一条简单 Record 记录到链表头。
// 常用于系统内部自动生成一条业务记录。
static void appendSimpleRecord(int type, const char* patientId, const char* staffId,
    double cost, int isPaid, const char* description) {
    Record* rec = (Record*)malloc(sizeof(Record));
    if (!rec) return;

    generateRecordID(rec->recordId);
    rec->type = type;
    strcpy(rec->patientId, patientId);
    strcpy(rec->staffId, staffId);
    rec->cost = cost;
    rec->isPaid = isPaid;
    strncpy(rec->description, description, sizeof(rec->description) - 1);
    rec->description[sizeof(rec->description) - 1] = '\0';
    getCurrentTimeStr(rec->createTime, 30);

    rec->next = recordHead->next;
    recordHead->next = rec;
}

// 支付后自动检查出院。
// 当前为空实现，仅作保留接口。
static void checkAndAutoDischarge(Record* rec, const char* currentPatientId) {
    (void)rec;
    (void)currentPatientId;
}

// 患者注册与建档。
// 支持：
// 1. 普通门诊
// 2. 急诊绿色通道
void registerPatient() {
    printf("\n========== 账户注册与建档 ==========\n");
    printf("请选择就诊类型 (1.普通门诊 2.急诊绿色通道 -1.取消返回): ");

    int type;
    while (1) {
        type = safeGetInt();
        if (type == -1) return;
        if (type == 1 || type == 2) break;
        printf("  [!] 输入有误：只能输入 1 或 2，请重新选择: ");
    }

    Patient* newPatient = (Patient*)malloc(sizeof(Patient));
    if (!newPatient) {
        printf("  [!] 内存分配失败，无法注册患者。\n");
        return;
    }
    newPatient->next = NULL;
    newPatient->isEmergency = (type == 2) ? 1 : 0;

    // 输入真实姓名
    while (1) {
        printf("请输入真实姓名 (输入-1取消): ");
        safeGetString(newPatient->name, 100);
        if (strcmp(newPatient->name, "-1") == 0) {
            free(newPatient);
            return;
        }
        if (strlen(newPatient->name) > 0) break;
        printf("  [!] 输入不能为空，请重新输入！\n");
    }

    // 设置密码
    printf("请设置登录密码 (至少6位，仅限数字或字母组合, 输入-1取消): ");
    safeGetPassword(newPatient->password, 50);
    if (strcmp(newPatient->password, "-1") == 0) {
        free(newPatient);
        return;
    }

    // 输入性别
    printf("请输入生理性别 (男/女, 输入-1取消): ");
    safeGetGender(newPatient->gender, 10);
    if (strcmp(newPatient->gender, "-1") == 0) {
        free(newPatient);
        return;
    }

    // 普通门诊患者需要输入年龄和过敏史
    if (!newPatient->isEmergency) {
        printf("请输入周岁年龄 (1-200, 输入-1取消): ");
        while (1) {
            newPatient->age = safeGetInt();
            if (newPatient->age == -1) {
                free(newPatient);
                return;
            }
            if (newPatient->age >= 1 && newPatient->age <= 200) break;
            printf("  [!] 输入异常：年龄必须在 1~200 之间，请重新输入 (输入-1取消): ");
        }

        while (1) {
            printf("请输入过敏史(未知则填未知，无则填无, 输入-1取消): ");
            safeGetString(newPatient->allergy, 100);
            if (strcmp(newPatient->allergy, "-1") == 0) {
                free(newPatient);
                return;
            }
            if (strlen(newPatient->allergy) > 0) break;
            printf("  [!] 输入不能为空！\n");
        }
        if (strlen(newPatient->allergy) == 0) strcpy(newPatient->allergy, "无");
    }
    else {
        // 急诊患者信息可简化
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    // 初始化患者信息
    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;
    newPatient->inpatientDeposit = 0.0;
    newPatient->isInpatient = 0;

    // 挂到患者链表尾部
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("\n  [√] 系统建档操作已完成。\n");
    printf("  ==========================================\n");
    printf("  您的终身就诊识别码为: 【 %s 】 (凭此号登录)\n", newPatient->id);
    printf("  ==========================================\n");
}

/*
 * 函数名：bookAppointment
 * ------------------------------------------------------------
 * 功能说明：
 *   患者在自助终端中进行“未来一周门诊预约挂号”。
 *
 *   该函数支持两种搜索方式：
 *   1. 按科室名称搜索未来一周的排班
 *   2. 按医生姓名或工号搜索未来一周的排班
 *
 *   当患者选定某个排班后，系统会进一步进行一系列业务规则校验：
 *   - 全院当天门诊量是否超限
 *   - 患者当天挂号次数是否超限
 *   - 患者是否已经在同一天挂过同科室号
 *   - 患者是否已经挂过同一天同一医生的号
 *   - 医生当天号源是否已满
 *
 *   如果校验通过，则创建一条“门诊挂号记录 Record”，
 *   该记录初始状态为“未支付”，后续需要患者去财务中心完成缴费。
 *
 * 参数说明：
 *   currentPatientId
 *      当前登录患者的唯一 ID，用于：
 *      1. 标识这条挂号记录属于哪个患者
 *      2. 在校验环节中统计该患者当天是否已挂号
 *
 * 返回值：
 *   无返回值（void）
 *
 * 核心流程：
 *   1. 计算“今天”和“未来一周截止日期”
 *   2. 让用户选择搜索方式（科室 / 医生）
 *   3. 根据关键字筛选符合条件的未来一周排班
 *   4. 展示排班表并让患者输入排班 ID
 *   5. 验证该排班是否属于当前搜索结果
 *   6. 执行业务规则检查
 *   7. 若通过，则创建挂号记录并提示成功
 *
 * 依赖的全局链表/外部资源：
 *   - scheduleList : 排班链表
 *   - staffHead    : 医生链表
 *   - recordHead   : 记录链表
 *
 * 注意事项：
 *   - 本函数只“生成挂号记录”，不会直接扣费
 *   - 费用会在后续财务中心中支付
 *   - 使用了 safeGetInt / safeGetString 等安全输入函数，避免非法输入
 */
void bookAppointment(const char* currentPatientId) {
    char today[11];
    char nextWeek[11];
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);
    t += 7 * 24 * 60 * 60;
    tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        system("cls");
        printf("\n========== 自助预约门诊挂号 ==========\n");
        printf("  [1] 按【科室名称】搜寻未来一周排班\n");
        printf("  [2] 按【医生姓名/工号】搜寻排班\n");
        printf(" [-1] 放弃挂号，返回上级菜单\n");
        printf("--------------------------------------\n");
        printf("  请选择搜寻引擎: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        /* keyword 用来保存用户输入的搜索关键字 */
        char keyword[50];

        /*
         * ========== 方式一：按科室搜索 ==========
         *
         * 这里会先从 staff 链表中收集所有已存在的科室
         *
         */
        if (choice == 1) {
            /* depts 用来保存系统中所有不重复的科室名称 */
            char depts[20][50];
            /* dCount 表示当前收集到的科室数量 */
            int dCount = collectHospitalDepartments(depts, 20);

            /* 打印当前系统中已有的门诊科室 */
            printf("\n  [系统当前已开设的门诊科室]: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);

            while (1) {
                printf("\n  请输入您要挂号的目标科室名称 (输入-1返回): ");
                safeGetString(keyword, 50);

                if (strcmp(keyword, "-1") == 0) break;

                if (strlen(keyword) == 0) {
                    printf("  [!] 输入不能为空！");
                    system("pause");
                    continue;
                }

                if (isKnownHospitalDepartment(keyword)) break;
                else printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！");
            }

            /* 如果用户输入 -1，返回外层菜单重新选搜索方式 */
            if (strcmp(keyword, "-1") == 0) continue;
        }

        /*
         * ========== 方式二：按医生姓名 / 工号搜索 ==========
         *
         * 支持两种匹配：
         * 1. 医生姓名包含 keyword
         * 2. 医生工号等于 keyword
         */
        else if (choice == 2) {
            printf("  请输入医生精确姓名或纯数字工号 (如:李四 / 1001, 输入-1返回): ");
            safeGetString(keyword, 50);
                                                                                                                           
            if (strcmp(keyword, "-1") == 0) continue;

            if (strlen(keyword) == 0) {
                printf("  [!] 输入不能为空！\n");
                system("pause");
                continue;
            }
        }

        else {
            printf("  [!] 无效的菜单选项，请正确输入菜单中提供的数字编号！\n");
            system("pause");
            continue;
        }

        /*
         * matchedSchIds:
         *   存放本轮搜索命中的所有排班 ID，
         *   后面用于验证用户输入的目标排班 ID 是否真的来自当前搜索结果。
         *
         * matchedCount:
         *   当前命中的排班数量。
         */
        int matchedSchIds[200];
        int matchedCount = 0;

        printf("\n========== 未来一周可预约排班总表 (%s 至 %s) ==========\n", today, nextWeek);
        printf("  %-8s | %-12s | %-8s | %-18s | %-10s | %-10s\n",
            "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
        printf("  ---------------------------------------------------------------------------\n");

        /*
         * found:
         *   表示是否找到了符合条件的排班
         *
         * maxRegId:
         *   用于生成新的挂号记录流水号尾号
         *   初始值从 4999 开始
         */
        int found = 0;
        int maxRegId = 4999;

        /*
         * 遍历已有记录，找当前最大挂号流水尾号，
         * 后面生成新挂号记录时会用 maxRegId + 1。
         */
        for (Record* r_temp = recordHead->next; r_temp != NULL; r_temp = r_temp->next) {
            int curReg;
            curReg = extractTrailingSequence(r_temp->recordId);
            if (curReg >= 0) {
                if (curReg > maxRegId) maxRegId = curReg;
            }
        }

        /*
         * 遍历排班链表 scheduleList，筛选未来一周内符合条件的排班：
         * 1. 日期在 today ~ nextWeek 范围内
         * 2. 能找到对应医生
         * 3. 满足当前搜索条件
         * 4. 班次不是“休息”
         */
        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) continue;

            /* 根据排班里的 doctor_id 找医生档案 */
            Staff* matchedDoc = NULL;
            for (Staff* d = staffHead->next; d != NULL; d = d->next) {
                if (strcmp(d->id, s->doctor_id) == 0) {
                    matchedDoc = d;
                    break;
                }
            }
            if (!matchedDoc) continue;

            int match = 0;

            /* 按科室搜索时：医生科室必须等于 keyword */
            if (choice == 1 && strcmp(matchedDoc->department, keyword) == 0) match = 1;

            /* 按医生搜索时：姓名包含 keyword，或工号精确等于 keyword */
            if (choice == 2) {
                if (strstr(matchedDoc->name, keyword) || strcmp(matchedDoc->id, keyword) == 0) match = 1;
            }

            /* 命中且不是休息班次才展示 */
            if (match && strcmp(s->shift, "休息") != 0) {
                char docDisp[130];
                snprintf(docDisp, sizeof(docDisp), "%s(%s)", matchedDoc->name, matchedDoc->id);

                printf("  [%-6d] | %-12s | %-8s | %-18s | %-10s | %-10s\n",
                    s->schedule_id, s->date, s->shift, docDisp,
                    matchedDoc->department, matchedDoc->level);

              
                if (matchedCount < 200) {
                    matchedSchIds[matchedCount++] = s->schedule_id;
                }
                found++;
            }
        }

        /* 若没有任何排班命中，则提示并重新搜索 */
        if (found == 0) {
            printf("\n  [!] 数据流反馈：未搜索到满足当前条件的排班资源。\n");
            system("pause");
            continue;
        }

        printf("  ---------------------------------------------------------------------------\n");
        printf("  请输入要确认选择的【排班ID】 (输入-1重新搜索): ");

        /*
         * 用户输入目标排班 ID。
         * 若输入 -1，则回到外层重新搜索。
         */
        int targetSchId = safeGetInt();
        if (targetSchId == -1) continue;

        /*
         * 校验该排班 ID 是否出现在本次搜索结果中。
         * 这样可以防止患者输入一个系统存在但当前搜索结果里没有的排班。
         */
        int idInResult = 0;
        for (int i = 0; i < matchedCount; i++) {
            if (matchedSchIds[i] == targetSchId) {
                idInResult = 1;
                break;
            }
        }
        if (!idInResult) {
            printf("  [!] 当前科室/医生无此排班，请重新输入！\n");
            system("pause");
            continue;
        }

        /*
         * 再次根据 targetSchId 在排班链表里定位目标排班对象。
         */
        Schedule* targetSch = NULL;
        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            if (s->schedule_id == targetSchId) {
                targetSch = s;
                break;
            }
        }
        if (!targetSch) {
            printf("  [!] 参数越界：排班ID不属于有效ID。\n");
            system("pause");
            continue;
        }

        /*
         * 再通过 targetSch->doctor_id 找到对应医生对象。
         */
        Staff* targetDoc = NULL;
        for (Staff* d = staffHead->next; d != NULL; d = d->next) {
            if (strcmp(d->id, targetSch->doctor_id) == 0) {
                targetDoc = d;
                break;
            }
        }
        if (!targetDoc) {
            printf("  [!] 底层数据异常：医生档案关联引用失败。\n");
            system("pause");
            continue;
        }

        /*
         * staffIdStr:
         *   当前目标医生 ID 的字符串副本
         *   方便后面在 record 中比较
         */
        char staffIdStr[22];
        snprintf(staffIdStr, sizeof(staffIdStr), "%s", targetDoc->id);

        /*
         * 以下变量用于执行业务规则校验：
         *
         * patientDailyActive:
         *   当前患者在 targetSch->date 这一天总共已有多少条挂号记录
         *
         * patientDeptDailyActive:
         *   当前患者在同一天、同科室已有多少条挂号记录
         *
         * sameDocSameDay:
         *   当前患者是否已经在同一天挂过该医生
         *
         * docDailyCount:
         *   该医生在该天已有多少个挂号号源
         *
         * hospitalDailyCount:
         *   全院在该天一共有多少条挂号记录
         */
        int patientDailyActive = 0, patientDeptDailyActive = 0, sameDocSameDay = 0, docDailyCount = 0, hospitalDailyCount = 0;

        /*
         * 遍历所有挂号记录（type == 1），统计业务限制所需数据。
         */
        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
            if (rec->type == 1 && strstr(rec->description, targetSch->date)) {
                hospitalDailyCount++;

                /* 统计该医生当天已有的挂号人数 */
                if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;

                /*
                 * 统计当前患者当天挂号情况。
                 * rec->isPaid != 2 表示还不是“已接诊完毕/已离室”的状态。
                 */
                if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                    patientDailyActive++;
                    /*
                     * 进一步判断：
                     * 1. 是否已经挂过同科室号
                     * 2. 是否已经挂过同医生号
                     */
                    for (Staff* recDoc = staffHead->next; recDoc != NULL; recDoc = recDoc->next) {
                        char tempDId[22];
                        snprintf(tempDId, sizeof(tempDId), "%s", recDoc->id);

                        if (strcmp(tempDId, rec->staffId) == 0) {
                            if (strcmp(recDoc->department, targetDoc->department) == 0) patientDeptDailyActive++;
                            if (strcmp(tempDId, staffIdStr) == 0) sameDocSameDay = 1;
                            break;
                        }
                    }
                }
            }
        }

        /* 全院日门诊量限制：最多 1000 */
        if (hospitalDailyCount >= 1000) {
            printf("\n  [系统过载] 全院日门诊量已达设定阈值。\n");
            system("pause");
            continue;
        }

        /* 患者单日挂号次数限制：最多 5 次 */
        if (patientDailyActive >= 5) {
            printf("\n  [策略约束] 患者单日预约次数已达上限。\n");
            system("pause");
            continue;
        }

        /* 同日同科室不可重复挂号 */
        if (patientDeptDailyActive >= 1) {
            printf("\n  [策略约束] 同日同科室不允许重复挂号。\n");
            system("pause");
            continue;
        }

        /* 同日同医生不可重复挂号 */
        if (sameDocSameDay) {
            printf("\n  [策略约束] 该日已存在相同医师的挂号记录。\n");
            system("pause");
            continue;
        }

        /*
         * 若该医生当天号源已满（50人），则不给继续挂号，
         * 并且推荐替代资源：
         * 1. 该医生其他时间
         * 2. 同日同科室其他医生
         */
        if (docDailyCount >= 50) {
            int recCount = 0;
            printf("\n  [ ！] %s 医生在 %s 的有效号源已全部分配完毕。\n", targetDoc->name, targetSch->date);
            printf("  >>> 调度系统为您推荐以下相似接诊资源 <<<\n");

            printf("\n  [分支一：该医师的其他接诊时段]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                if (strcmp(altS->doctor_id, targetDoc->id) == 0 &&
                    strcmp(altS->date, targetSch->date) != 0 &&
                    strcmp(altS->shift, "休息") != 0 &&
                    strcmp(altS->date, today) >= 0 &&
                    strcmp(altS->date, nextWeek) <= 0) {
                    printf("    -> 资源索引 [%d] 日期: %s 班次: %s\n", altS->schedule_id, altS->date, altS->shift);
                    recCount++;
                }
            }

            printf("\n  [分支二：同日同科室的出诊医师]\n");
            for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
                if (strcmp(altS->date, targetSch->date) == 0 &&
                    strcmp(altS->doctor_id, targetDoc->id) != 0 &&
                    strcmp(altS->shift, "休息") != 0) {
                    for (Staff* altD = staffHead->next; altD != NULL; altD = altD->next) {
                        if (strcmp(altD->id, altS->doctor_id) == 0 &&
                            strcmp(altD->department, targetDoc->department) == 0) {
                            printf("    -> 资源索引 [%d] 医生: %s 班次: %s\n", altS->schedule_id, altD->name, altS->shift);
                            recCount++;
                            break;
                        }
                    }
                }
            }

            if (recCount == 0) printf("  未命中相似属性资源，请重置过滤条件。\n");
            system("pause");
            continue;
        }

        /*
         * 通过前面统计出的 docDailyCount，
         * 给当前新挂号患者分配排号序号：
         * 第 docDailyCount + 1 位
         */
        int seqNum = docDailyCount + 1;

        /*
 * 根据医生职称确定挂号费：
 * 1. 主任医师：50 元
 * 2. 副主任医师：30 元
 * 3. 主治医师：15 元
 * 4. 其它默认：15 元
 */
        double regFee;
        if (strcmp(targetDoc->level, "主任医师") == 0) {
            regFee = 50.0;
        }
        else if (strcmp(targetDoc->level, "副主任医师") == 0) {
            regFee = 30.0;
        }
        else if (strcmp(targetDoc->level, "主治医师") == 0) {
            regFee = 15.0;
        }
        else {
            regFee = 15.0;
        }

        /*
         * 创建新的挂号记录：
         * type = 1 表示门诊挂号
         * isPaid = 0 表示待支付
         */
        Record* newRecord = (Record*)malloc(sizeof(Record));
        if (!newRecord) {
            printf("  [!] 内存分配失败。\n");
            return;
        }

        sprintf(newRecord->recordId, "REG%d%04d", getCurrentYearValue(), maxRegId + 1);
        newRecord->type = 1;  // 1 表示门诊挂号
        strcpy(newRecord->patientId, currentPatientId);
        strcpy(newRecord->staffId, staffIdStr);
        newRecord->cost = regFee;
        newRecord->isPaid = 0; // 待支付

        /*
         * description 中保存：
         * 1. 医生姓名
         * 2. 就诊日期
         * 3. 排号序号
         */
        sprintf(newRecord->description, "挂号:%s(%s)_排号:%d", targetDoc->name, targetSch->date, seqNum);

        getCurrentTimeStr(newRecord->createTime, 30);
        newRecord->next = NULL;

        /*
         * 将新挂号记录挂到 record 链表尾部。
         * 这里采用尾插。
         */
        Record* temp = recordHead;
        while (temp->next) temp = temp->next;
        temp->next = newRecord;

        /*
         * 提示挂号成功，但注意这里只是创建待支付记录，
         * 患者还需要去费用中心支付。
         */
        printf("\n  [√ 事务确认] %s 医师接诊号源申请成功！系统产生待支付流水 %.2f 元。\n", targetDoc->name, regFee);
        printf("  =======================================================\n");
        printf("  >>> 当日分诊系统的预计算序号为：【 第 %d 号 】 <<<\n", seqNum);
        printf("  =======================================================\n");
        printf("  (系统提示：请至费用中心结清账单)\n");
        system("pause");
        return;
    }
}


/*
 * 函数名：financeCenter
 * ------------------------------------------------------------
 * 功能说明：
 *   这是患者端的“个人财务结算中心”。
 *
 *   该函数主要负责两大功能：
 *   1. 账户充值
 *   2. 清算患者当前所有未支付账单
 *
 *   在这个函数中，系统会先显示当前患者的：
 *   - 普通账户余额
 *   - 住院押金余额
 *   - 当前住院状态
 *   - 若押金为负，则显示待补交金额
 *
 *   随后提供两个业务入口：
 *   [1] 在线充值
 *   [2] 待支付账单清算
 *
 * 参数说明：
 *   currentPatientId
 *      当前登录患者的唯一 ID。
 *      用于查找患者对象、筛选该患者名下的记录、完成扣费和充值。
 *
 * 返回值：
 *   无返回值（void）
 *
 * 主要依赖：
 *   - findPatientById()            根据患者 ID 找到患者对象
 *   - formatMoney()                把金额格式化成 xx.xx元 字符串
 *   - safeGetInt()                 安全读取整数菜单输入
 *   - safeGetMoneyInRange()        安全读取金额输入
 *   - ensureDrugStockForRecord()   检查药品单库存是否足够
 *   - settleSingleRecord()         真正执行单笔账单支付
 *   - isInpatientArrearsRecord()   判断记录是否为住院补缴单
 *   - appendTransaction()          写入财务流水
 *
 * 核心流程：
 *   1. 查找当前患者对象
 *   2. 显示账户余额和住院押金信息
 *   3. 让患者选择充值或支付
 *   4. 若充值，则增加余额并生成充值记录
 *   5. 若支付，则列出所有未支付账单
 *      - 可一键聚合支付
 *      - 也可按流水号单独核销
 *
 * 业务特点：
 *   - 药品支付前会再次检查库存，防止支付后库存不足
 *   - 住院欠费补交单有特殊处理逻辑
 *   - 聚合支付允许部分成功、部分失败
 */
void financeCenter(const char* currentPatientId) {
 
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentPatientId);
        if (!p) return;

        printf("\n========== 个人财务结算中心 ==========\n");
        char balanceText[32], depositText[32], arrearsText[32];

        /* 把数值金额格式化为统一的 xx.xx元 样式 */
        formatMoney(p->balance, balanceText, sizeof(balanceText));
        formatMoney(p->inpatientDeposit, depositText, sizeof(depositText));
        /* 显示当前账户资金情况 */
        printf("  [当前账户可用余额]:      %s\n", balanceText);
        printf("  [当前住院押金余额]:      %s\n", depositText);
        printf("  [当前住院状态]:          %s\n", p->isInpatient ? "住院中" : "非住院");

        /*
         * 如果住院押金为负，表示已经欠费，
         * 这里显示待补交金额（取绝对值）。
         */
        if (p->inpatientDeposit < 0) {
            formatMoney(-p->inpatientDeposit, arrearsText, sizeof(arrearsText));
            printf("  [当前待补交金额]:        %s\n", arrearsText);
        }

        /* 财务中心主菜单 */
        printf("--------------------------------------\n");
        printf("  [1] 在线网银充值 (预存备用金)\n");
        printf("  [2] 待处理账单清算 (聚合结算)\n");
        printf(" [-1] 返回主终端\n");
        printf("  业务指令: ");

        int choice = safeGetInt();
        if (choice == -1) return;

        //一、充值功能

        if (choice == 1) {
            printf("\n  请输入需充值的金额 (输入-1取消): ");

            double money = safeGetMoneyInRange(0.01, 10000.0);
            if (money == -1.0) continue;

            if (money > 0) {
                /* 给患者普通账户余额加钱 */
                p->balance += money;

                /*
                 * 生成一条充值记录 Record：
                 * type = 7 表示充值
                 * isPaid = 1 表示该记录已完成
                 */
                Record* r7 = (Record*)malloc(sizeof(Record));
                if (!r7) {
                    printf("  [!] 内存分配失败。\n");
                    return;
                }

                generateRecordID(r7->recordId);
                r7->type = 7;  // 7 表示充值
                strcpy(r7->patientId, currentPatientId);
                strcpy(r7->staffId, "SYS");   // 表示系统自动处理
                r7->cost = money;
                r7->isPaid = 1;
                sprintf(r7->description, "终端自助充值入账");
                getCurrentTimeStr(r7->createTime, 30);

                /* 头插法，把充值记录插入 record 链表 */
                r7->next = recordHead->next;
                recordHead->next = r7;

                /*
                 * 写入财务流水。
                 * 充值属于“患者资金注入”，不算医院业务收入，所以描述中写“不计收入”。
                 */
                appendTransaction(TRANS_RECHARGE, money, "终端自助充值(不计收入)");

                /* 再次格式化金额用于回显 */
                char moneyText[32];
                formatMoney(money, moneyText, sizeof(moneyText));

                printf("  [完成] 充值业务受理成功，金额: %s。\n", moneyText);
                system("pause");
            }
            
            else if (money != 0) {
                printf("  [异常] 充值数额校验失败，必须为正值参数。\n");
                system("pause");
            }
        }

         // 二、清算待支付账单
        
        else if (choice == 2) {
            
            while (1) {
                /*
                 * hasUnpaid:
                 *   当前患者是否存在未支付账单
                 * totalUnpaidCost:
                 *   当前患者所有未支付账单的总金额
                 */
                int hasUnpaid = 0;
                double totalUnpaidCost = 0.0;

                system("cls");
                printf("\n========== 待清算账单明细列表 ==========\n");

                /*
                 * 遍历所有记录，列出当前患者 isPaid == 0 的记录。
                 * 也就是“尚未支付”的账单。
                 */
                for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                    if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                        char typeName[20];

                        /*
                         * 根据记录类型 rec->type 映射成可读的业务名称：
                         * 1 -> 门诊挂号
                         * 2 -> 临床诊疗
                         * 3 -> 药事调拨
                         * 4 -> 辅助检验
                         * 5 -> 住院类（住院补缴 / 住院账目）
                         */
                        switch (rec->type) {
                        case 1: strcpy(typeName, "门诊挂号"); break;
                        case 2: strcpy(typeName, "临床诊疗"); break;
                        case 3: strcpy(typeName, "药事调拨"); break;
                        case 4: strcpy(typeName, "辅助检验"); break;
                        case 5:
                            if (isInpatientArrearsRecord(rec)) strcpy(typeName, "住院补缴");
                            else strcpy(typeName, "住院账目");
                            break;
                        default: strcpy(typeName, "其他"); break;
                        }

                        /*
                         * 打印账单明细：
                         * - 流水号
                         * - 业务类型
                         * - 金额
                         * - 描述信息
                         */
                        printf("  流水号: %-12s | 业务项: [%-10s] | 实收: %-8.2f | 备忘录: %s\n",
                            rec->recordId, typeName, rec->cost, rec->description);

                        totalUnpaidCost += rec->cost;
                        hasUnpaid = 1;
                    }
                }

                /* 若当前没有未支付账单，提示并退出清算循环 */
                if (!hasUnpaid) {
                    printf("  [核查反馈] 当前无未决的财务单据。\n");
                    system("pause");
                    break;
                }

                /*
                 * 输出待清算总额与当前普通余额的对比，
                 * 帮助患者判断资金是否足够。
                 */
                printf("----------------------------------------------------------------------\n");
                char unpaidText[32], balanceText2[32];
                formatMoney(totalUnpaidCost, unpaidText, sizeof(unpaidText));
                formatMoney(p->balance, balanceText2, sizeof(balanceText2));
                printf("  [资金比对] 待清算总额: %s | 普通余额: %s\n\n", unpaidText, balanceText2);

                /*
                 * 提供两种支付方式：
                 * 1. 一键聚合支付：尽量一次把所有未支付账单都结掉
                 * 2. 指定流水号单项核销：只支付某一条账单
                 * -1. 返回上一级
                 */
                printf("  1. 一键聚合支付\n");
                printf("  2. 指定流水号单项核销\n");
                printf(" -1. 返回上一级\n");
                printf("  请选择: ");

                int payChoice = safeGetInt();
                if (payChoice == -1) break;

                /*
                 * =========================
                 * 2.1 一键聚合支付
                 * =========================
                 */
                if (payChoice == 1) {
                    /*
                     * 先判断普通余额是否足够覆盖所有未支付账单总额。
                     * 若不足，则直接拒绝执行。
                     */
                    if (p->balance < totalUnpaidCost) {
                        char gapText[32];
                        formatMoney(totalUnpaidCost - p->balance, gapText, sizeof(gapText));
                        printf("  [拒绝执行] 账户流动资金不足。(缺口: %s)\n", gapText);
                        system("pause");
                    }
                    else {
                        /*
                         * pending:
                         *   用于临时保存所有待支付记录指针
                         *
                         * pendingCount:
                         *   当前收集到多少条待支付记录
                         *
                         * stockOk:
                         *   药品库存检查是否全部通过
                         */
                        Record* pending[512];
                        int pendingCount = 0;
                        int stockOk = 1;

                        /*
                         * 第一步：先收集所有待支付记录。
                         * 第二步：如果遇到药品单(type == 3)，先检查库存是否还足够。
                         * 因为药品库存可能在患者开单之后发生变化。
                         */
                        for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                            if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid == 0) {
                                if (pendingCount < (int)(sizeof(pending) / sizeof(pending[0]))) {
                                    pending[pendingCount++] = rec;
                                }

                                if (rec->type == 3 && !ensureDrugStockForRecord(rec)) {
                                    printf("  [拒绝执行] 药品库存已发生变化，单据 %s 暂无法支付，请联系药房刷新库存。\n", rec->recordId);
                                    stockOk = 0;
                                    break;
                                }
                            }
                        }

                        /* 若有任意药品单库存不足，则终止本次聚合支付 */
                        if (!stockOk) {
                            system("pause");
                            continue;
                        }

                        /*
                         * processed:
                         *   已成功处理的账单数量
                         *
                         * failed:
                         *   是否出现失败
                         *
                         * failedId:
                         *   保存失败那条账单的流水号，便于报错
                         */
                        int processed = 0;
                        int failed = 0;
                        char failedId[30] = "";

                        /*
                         * 逐条调用 settleSingleRecord() 完成真正的支付。
                         * 如果中途失败，则记录失败位置并终止。
                         */
                        for (int i = 0; i < pendingCount; ++i) {
                            if (!settleSingleRecord(p, currentPatientId, pending[i])) {
                                failed = 1;
                                strncpy(failedId, pending[i]->recordId, sizeof(failedId) - 1);
                                failedId[sizeof(failedId) - 1] = '\0';
                                break;
                            }
                            processed++;
                        }

                        /*
                         * 如果中途失败：
                         * - 前面已成功的账单保持成功状态
                         * - 后面未处理的账单继续保留待支付状态
                         */
                        if (failed) {
                            printf("  [异常] 聚合支付在单据 %s 处中止，前面已成功处理 %d 笔，其余账单保留待支付状态。\n", failedId, processed);
                            system("pause");
                            continue;
                        }

                        /* 全部成功 */
                        printf("  [处理成功] 所有挂起账单已完成聚合清算。\n");
                        system("pause");
                        break;
                    }
                }

                /*
                 * =========================
                 * 2.2 指定流水号单项核销
                 * =========================
                 */
                else if (payChoice == 2) {
                    printf("\n  请输入需独立清算的单据流水号 (输入-1取消): ");
                    char target[30];
                    safeGetString(target, 30);

                    if (strcmp(target, "-1") == 0) continue;
                    if (strlen(target) == 0) {
                        printf("  [!] 输入不能为空！\n");
                        system("pause");
                        continue;
                    }

                    /*
                     * 在所有记录中查找：
                     * 1. 流水号等于 target
                     * 2. 属于当前患者
                     * 3. 当前状态是未支付
                     */
                    Record* tRec = NULL;
                    for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                        if (strcmp(rec->recordId, target) == 0 &&
                            strcmp(rec->patientId, currentPatientId) == 0 &&
                            rec->isPaid == 0) {
                            tRec = rec;
                            break;
                        }
                    }

                    /* 没找到合法目标账单 */
                    if (!tRec) {
                        printf("  [!] 流水寻址失败。\n");
                        system("pause");
                        continue;
                    }

                    /* 判断余额是否足够支付这单 */
                    if (p->balance < tRec->cost) {
                        printf("  [拒绝执行] 余额不足。\n");
                        system("pause");
                    }
                    else {
                        /*
                         * 如果是药品单，再次检查库存。
                         * 因为药品库存是动态变化的。
                         */
                        if (tRec->type == 3 && !ensureDrugStockForRecord(tRec)) {
                            printf("  [拒绝执行] 药房库存不足，当前药单无法完成支付。\n");
                            system("pause");
                            continue;
                        }

                        /*
                         * 真正执行单笔支付。
                         * settleSingleRecord() 内部会：
                         * - 扣除余额
                         * - 更新记录支付状态
                         * - 必要时写财务流水
                         * - 若是药品单则完成出库
                         * - 若是住院补缴则更新住院押金
                         */
                        if (!settleSingleRecord(p, currentPatientId, tRec)) {
                            printf("  [异常] 本次支付未提交，请重试或联系管理员核查。\n");
                            system("pause");
                            continue;
                        }

                        /*
                         * 若是住院欠费补交单，给专门提示；
                         * 否则按普通账单支付成功提示。
                         */
                        if (isInpatientArrearsRecord(tRec)) {
                            printf("  [处理成功] 住院欠费补交完成，%.2f 元。\n", tRec->cost);
                        }
                        else {
                            printf("  [处理成功] 单据 %s 核销完毕，%.2f 元。\n", tRec->recordId, tRec->cost);
                        }
                        system("pause");
                    }
                }

                /* 清算菜单输入非法 */
                else {
                    printf("  [!] 无效选项！\n");
                    system("pause");
                }
            }
        }

        /* 财务中心主菜单输入非法 */
        else {
            printf("  [!] 无效选项！\n");
            system("pause");
        }
    }
}

// 个人医疗档案查询中心。
// 支持查看：
// 1. 门诊挂号
// 2. 临床诊疗
// 3. 门诊处方
// 4. 辅助检查
// 5. 住院与押金账单
// 6. 终身财务流水
void medicalRecords(const char* currentPatientId) {
    while (1) {
        system("cls");
        printf("\n========== 个人医疗时序档案检索平台 ==========\n");
        printf("  [1] 调取门诊与排号记录表\n");
        printf("  [2] 调取临床诊疗与医嘱报告\n");
        printf("  [3] 调取门诊处方扣费清单\n");
        printf("  [4] 调取辅助生化与影像开单记录\n");
        printf("  [5] 调取住院与押金账单记录\n");
        printf("  [6] 汇总终身财务进出流水总账\n");
        printf(" [-1] 退出检索平台\n");
        printf("----------------------------------------\n");
        printf("  请指派数据抽取卷宗号: ");

        int c = safeGetInt();
        if (c == -1) return;

        // 查询门诊类记录（1~4）
        if (c >= 1 && c <= 4) {
            int printed = 0;
            printf("\n--- 门诊业务类型查询结果反馈 ---\n");
            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0 &&
                    rec->type == c &&
                    rec->isPaid != 4 &&
                    !strstr(rec->description, "[住院记账]")) {
                    printf("  记录流水: %-12s | 快照: %s | 净值: %-6.2f | 详述: %s\n",
                        rec->recordId, rec->createTime, rec->cost, rec->description);
                    printed = 1;
                }
            }
            if (!printed) printf("  [数据空置] 无关联数据。\n");
            system("pause");
        }
        // 查询住院相关业务明细
        else if (c == 5) {
            int printed = 0;
            system("cls");
            printf("\n========== 住院业务数据聚合 ==========\n");

            Patient* pp = findPatientById(currentPatientId);
            if (pp) {
                printf("  当前住院状态: %s\n", pp->isInpatient ? "住院中" : "非住院");
                printf("  当前住院押金余额: %.2f\n", pp->inpatientDeposit);
                if (pp->inpatientDeposit < 0) {
                    printf("  当前待补交金额: %.2f\n", -pp->inpatientDeposit);
                }
            }

            printf("  %-20s | %-10s | %-10s | %-10s | %s\n", "时间", "类型", "金额", "状态", "说明");
            printf("  -----------------------------------------------------------------------------------------\n");

            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) != 0) continue;

                int isInpatientDetail = 0;
                char typeName[20] = "";
                char statusName[20] = "";

                // 通过 type + description 标签判断是否属于住院业务明细
                if (rec->type == 3 && strstr(rec->description, "[住院明细][药品]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "药品");
                }
                else if (rec->type == 4 && strstr(rec->description, "[住院明细][检查]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "检查");
                }
                else if (rec->type == 5 && strstr(rec->description, "[住院明细]")) {
                    isInpatientDetail = 1;
                    if (strstr(rec->description, "[床位费]")) strcpy(typeName, "床位费");
                    else if (strstr(rec->description, "[押金]")) strcpy(typeName, "押金");
                    else if (strstr(rec->description, "[补缴]")) strcpy(typeName, "补缴");
                    else if (strstr(rec->description, "[待补交]")) strcpy(typeName, "待补交");
                    else strcpy(typeName, "住院账务");
                }
                else if (isInpatientArrearsRecord(rec)) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "待补交");
                }
                else if (rec->type == 8 && strstr(rec->description, "[住院明细][退回]")) {
                    isInpatientDetail = 1;
                    strcpy(typeName, "退回");
                }

                if (!isInpatientDetail) continue;

                // 根据支付状态映射显示文案
                if (rec->isPaid == 0) strcpy(statusName, "待补交");
                else if (rec->isPaid == 1) strcpy(statusName, "已入账");
                else strcpy(statusName, "已完成");

                printf("  %-20s | %-10s | %-10.2f | %-10s | %s\n",
                    rec->createTime, typeName, rec->cost, statusName, rec->description);
                printed = 1;
            }

            if (!printed) printf("  [数据空置] 暂无住院明细。\n");
            printf("  -----------------------------------------------------------------------------------------\n");
            system("pause");
        }
        // 查询终身财务总账
        else if (c == 6) {
            double totalSpent = 0.0, totalRecharged = 0.0, totalRefunded = 0.0;
            int printed = 0;
            char typeName[30];
            printf("\n========== 终身资产流入流出审计报表 ==========\n");
            printf("  %-20s | %-10s | %-10s | %-40s\n", "登记时间", "变动", "金额", "特征");
            printf("  ---------------------------------------------------------------------------------\n");

            for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                if (strcmp(rec->patientId, currentPatientId) == 0) {
                    // 已支付的 1~5 类记录视为支出
                    if (rec->type >= 1 && rec->type <= 5 && rec->isPaid > 0 && rec->isPaid != 4) {
                        if (!(rec->type == 3 && strstr(rec->description, "[住院记账]"))) {
                            totalSpent += rec->cost;
                            strcpy(typeName, "耗损(-)");
                            printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                                rec->createTime, typeName, rec->cost, rec->description);
                            printed = 1;
                        }
                    }
                    // type == 7 表示充值
                    else if (rec->type == 7) {
                        totalRecharged += rec->cost;
                        strcpy(typeName, "汇入(+)");
                        printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                            rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                    // type == 8 表示退款
                    else if (rec->type == 8) {
                        totalRefunded += rec->cost;
                        strcpy(typeName, "清退(+)");
                        printf("  %-20s | %-10s | %-10.2f | %-40s\n",
                            rec->createTime, typeName, rec->cost, rec->description);
                        printed = 1;
                    }
                }
            }

            if (!printed) printf("  暂无财务活动数据。\n");
            printf("  ---------------------------------------------------------------------------------\n");
            printf("  【整合】注资: +%.2f | 返还: +%.2f | 核销: -%.2f\n", totalRecharged, totalRefunded, totalSpent);

            Patient* pp = findPatientById(currentPatientId);
            if (pp) printf("  >>> 当前余额: ￥ %.2f <<<\n", pp->balance);

            system("pause");
        }
        else {
            printf("  [!] 无效选项！\n");
            system("pause");
        }
    }
}

// 修改患者密码。
// 流程：
// 1. 校验旧密码
// 2. 输入新密码
// 3. 再输入一次确认
// 4. 一致则更新
void changePatientPassword(const char* currentId) {
    Patient* p = findPatientById(currentId);
    if (!p) return;

    char oldPwd[50] = { 0 }, newPwd[50] = { 0 }, confirmPwd[50] = { 0 };

    printf("\n========== 核心安全凭证更替操作 ==========\n");
    printf("请输入原始密码进行校验 (输入-1取消): ");
    safeGetString(oldPwd, 50);
    if (strcmp(oldPwd, "-1") == 0) return;

    if (strcmp(p->password, oldPwd) != 0) {
        printf("  [安全阻断] 特征匹配失败！\n");
        system("pause");
        return;
    }

    printf("请输入新密码 (至少6位，仅限数字或字母, 输入-1取消): ");
    safeGetPassword(newPwd, 50);
    if (strcmp(newPwd, "-1") == 0) return;

    printf("重复输入以完成校验 (输入-1取消): ");
    safeGetString(confirmPwd, 50);
    if (strcmp(confirmPwd, "-1") == 0) return;

    if (strcmp(newPwd, confirmPwd) != 0) {
        printf("  [异常] 二次校验不一致，操作取消。\n");
        system("pause");
        return;
    }

    strcpy(p->password, newPwd);
    printf("  [完成] 服务凭证已安全重置！\n");
    system("pause");
}

// 患者端主菜单。
// 统一调度患者的各项自助服务入口。
void userTerminal(const char* currentId) {
    while (1) {
        system("cls");
        Patient* p = findPatientById(currentId);

        printf("\n======================================================\n");
        printf("   患者专属自助数据终端   [ 登录实体: %s (%s) ]\n", p->name, p->id);
        printf("======================================================\n");
        printf("  [1] 门诊自助挂号系统(在线预约)\n");
        printf("  [2] 财务与费用中心 (账户缴费)\n");
        printf("  [3] 个人医疗档案库 (查询报告)\n");
        printf("  [4] 个人账号安全设置 (密码修改)\n");
        printf(" [-1] 结束当前操作，返回系统主菜单\n");
        printf("------------------------------------------------------\n");
        printf("  请选择: ");

        int option = safeGetInt();
        switch (option) {
        case 1: bookAppointment(currentId); break;
        case 2: financeCenter(currentId); break;
        case 3: medicalRecords(currentId); break;
        case 4: changePatientPassword(currentId); break;
        case -1: return;
        default:
            printf("  [!] 无效选项！\n");
            system("pause");
            break;
        }
    }
}