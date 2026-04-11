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

extern void generateRecordID(char* buffer);
extern Bed* bedHead;

// =========================================================================
// 【新增】动态科室全局缓存池
// =========================================================================
static char hospitalDepts[50][50];
static int hospitalDeptCount = 0;
static int isDeptsInitialized = 0;

// 【新增】从医生档案中实时抓取并生成全院科室清单
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

    // 容错处理：如果当前没有任何医生档案，给个默认保底
    if (hospitalDeptCount == 0) {
        strcpy(hospitalDepts[0], "综合科");
        hospitalDeptCount = 1;
    }
    isDeptsInitialized = 1;
}

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

// 【修改】动态按需生成病床
void initBedsIfEmpty() {
    if (bedHead->next) return;

    // 初始化前先动态抓取全院科室数量
    initHospitalDepts();

    char types[5][50] = { "单人病房", "双人病房", "三人病房", "单人陪护病房", "单人陪护疗养病房" };
    double prices[5] = { 200.0, 150.0, 100.0, 300.0, 500.0 };
    char wards[2][50] = { "普通病房", "特殊病房" };

    Bed* tail = bedHead;
    int roomNum = 1;

    // 【核心变化】不再固定5个科室，而是根据动态科室数量(hospitalDeptCount)循环分配，每个科室分5个房间
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

// 【修改】废除硬编码，动态映射科室归属
const char* getRoomDepartment(const char* bedId) {
    initHospitalDepts(); // 确保科室映射表已加载

    int roomNum;
    if (sscanf(bedId, "%d-", &roomNum) != 1) return "未知科室";

    // 算法：每5个房间属于一个科室。房号26 -> (26-1)/5 = 5 -> 指向数组第6个科室
    int deptIdx = (roomNum - 1) / 5;

    if (deptIdx >= 0 && deptIdx < hospitalDeptCount) {
        return hospitalDepts[deptIdx];
    }
    return "未知科室";
}

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

                strcpy(b->bedType, "双人病房");
                b->price = 150.0;

                Bed* extra = (Bed*)malloc(sizeof(Bed));
                *extra = *b;
                /* 安全拼接：先复制最多N-2个字符，再追加'A'，保证不溢出 */
                strncpy(extra->bedId, b->bedId, sizeof(extra->bedId) - 2);
                extra->bedId[sizeof(extra->bedId) - 2] = '\0';
                size_t blen = strlen(extra->bedId);
                extra->bedId[blen] = 'A';
                extra->bedId[blen + 1] = '\0';
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

void getDynamicDeptPrompt(char* promptBuffer) {
    char depts[20][50];
    int dCount = 0;

    for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
        int exists = 0;
        for (int i = 0; i < dCount; i++) {
            if (strcmp(depts[i], stf->department) == 0) { exists = 1; break; }
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
        printf("当前活跃科室有: (%s)\n请输入要查看的科室 (输0返回): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "0") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) { isValidDept = 1; break; }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n请输入要查看的科室 (输0返回): ");
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
                    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                        if (strcmp(p->id, b->patientId) == 0) { strcpy(patName, p->name); break; }
                    }

                    for (Record* r = recordHead->next; r != NULL; r = r->next) {
                        if (r->type == 6 && strcmp(r->patientId, b->patientId) == 0) {
                            for (Staff* s = staffHead->next; s != NULL; s = s->next) {
                                if (strcmp(s->id, r->staffId) == 0) { strcpy(docName, s->name); break; }
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

void admitPatient(const char* docId) {
    initBedsIfEmpty();
    int noticeCount = 0;

    printf("\n========== 门诊下发《待入院通知单》队列 ==========\n");

    printf("【重症优先通道】\n");
    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "重症")) {
            char pName[100] = "未知";
            for (Patient* pt = patientHead->next; pt != NULL; pt = pt->next) {
                if (strcmp(pt->id, r->patientId) == 0) { strcpy(pName, pt->name); break; }
            }
            char deptName[50];
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [紧急] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
    }

    printf("\n【普通入院队列】\n");
    for (Record* r = recordHead->next; r != NULL; r = r->next) {
        if (r->type == 6 && r->isPaid == 0 && strstr(r->description, "普通")) {
            char pName[100] = "未知";
            for (Patient* pt = patientHead->next; pt != NULL; pt = pt->next) {
                if (strcmp(pt->id, r->patientId) == 0) { strcpy(pName, pt->name); break; }
            }
            char deptName[50];
            getResponsibleDept(r->patientId, deptName);
            printf(" -> [常规] 患者ID: %s | 姓名: %s | 负责科室: %s | 说明: %s\n", r->patientId, pName, deptName, r->description);
            noticeCount++;
        }
    }

    if (noticeCount == 0) { printf("  [提示] 当前暂无门诊下发的未处理住院通知单。\n"); return; }

    char pId[20];
    Record* targetNotice = NULL;
    Patient* targetPat = NULL;

    while (1) {
        printf("\n请输入需办理入院的患者ID (输0返回): ");
        safeGetString(pId, 20);
        if (strcmp(pId, "0") == 0) return;

        targetNotice = NULL;
        for (Record* r = recordHead->next; r != NULL; r = r->next) {
            if (r->type == 6 && strcmp(r->patientId, pId) == 0 && r->isPaid == 0) { targetNotice = r; break; }
        }

        if (!targetNotice) {
            int alreadyIn = 0;
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (b->isOccupied && strcmp(b->patientId, pId) == 0) { alreadyIn = 1; break; }
            }
            if (alreadyIn) printf("  [!] 拦截警告：该患者当前已处于住院收治状态，请重新输入！\n");
            else printf("  [!] 拦截警告：缺乏门诊前端开具的合规通知单，无法受理收治，请重新输入！\n");
            continue;
        }

        targetPat = NULL;
        for (Patient* p = patientHead->next; p != NULL; p = p->next) {
            if (strcmp(p->id, pId) == 0) { targetPat = p; break; }
        }
        if (targetPat) break;
    }

    char requiredDept[50];
    getResponsibleDept(pId, requiredDept);
    printf("\n>>> 锁定通知单：该患者由【%s】下发，系统正检索 %s 的专属空余床位...\n", requiredDept, requiredDept);

    checkAndAdjustBedTension(requiredDept);

    int hasLocalEmpty = 0;
    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (!b->isOccupied && strcmp(getRoomDepartment(b->bedId), requiredDept) == 0) { hasLocalEmpty = 1; break; }
    }

    int isCrossDept = 0;
    if (!hasLocalEmpty) {
        printf("\n  [警告] %s 专属床位资源枯竭（扩容后仍无空余）！\n", requiredDept);
        printf("  >>> 触发系统应急预案：开启跨科室床位全局调度 <<<\n");
        isCrossDept = 1;

        int hasAnyEmpty = 0;
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (!b->isOccupied) { hasAnyEmpty = 1; break; }
        }

        if (!hasAnyEmpty) {
            printf("  [极其抱歉] 全院整体床位网络满载，暂无法提供物理空间进行收治转运。\n");
            return;
        }

        printf("\n【全院可用空闲病床列表 (跨域收治)】:\n");
        for (Bed* b = bedHead->next; b != NULL; b = b->next) {
            if (!b->isOccupied) {
                printf("  [%s] 所属: %-6s | %s - %s (基准日租 ￥%.2f)\n", b->bedId, getRoomDepartment(b->bedId), b->wardType, b->bedType, b->price);
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

    while (1) {
        printf("\n请输入系统拟分配的床位号 (如 1-3) (输入0取消): ");
        safeGetString(selectBed, 20);
        if (strcmp(selectBed, "0") == 0) return;

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

    int baseDeposit = 200 * days;
    if (baseDeposit < 1000) baseDeposit = 1000;
    int finalDeposit = ((baseDeposit + 99) / 100) * 100;

    printf("  系统通过风控模型核算，需缴纳初始住院统筹押金: %d 元。\n", finalDeposit);

    int isPaid = 0;
    if (targetPat->balance >= finalDeposit) {
        targetPat->balance -= finalDeposit;
        isPaid = 1;
        printf("  [√] 账户资金划扣完毕，病区收治状态已激活。\n");

        Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
        int maxId = 0;
        for (Transaction* curr = transactionList; curr != NULL; curr = curr->next) {
            if (curr->id > maxId) maxId = curr->id;
        }

        newTrans->id = maxId + 1;
        newTrans->type = 2;
        newTrans->amount = (double)finalDeposit;
        getCurrentTimeStr(newTrans->time, 30);
        sprintf(newTrans->description, "住院押金收取(患者:%s)", targetPat->name);
        newTrans->next = NULL;

        if (!transactionList) transactionList = newTrans;
        else {
            Transaction* curr = transactionList;
            while (curr->next) curr = curr->next;
            curr->next = newTrans;
        }
    }
    else {
        printf("  [提示] 账户实时结余不足，系统已挂起床位并生成待处理押金账单。\n");
    }

    finalBed->isOccupied = 1;
    strcpy(finalBed->patientId, pId);
    finalBed->isRoundsDone = 0;
    targetNotice->isPaid = 1;

    Record* r5 = (Record*)malloc(sizeof(Record));
    generateRecordID(r5->recordId);
    r5->type = 5;
    strcpy(r5->patientId, pId);
    strcpy(r5->staffId, docId);
    r5->cost = finalDeposit;
    r5->isPaid = isPaid;

    char adminTime[30];
    getCurrentTimeStr(adminTime, 30);
    sprintf(r5->description, "病房:%s_床位:%s_入院日期:%s_预期天数:%d_押金:%d_出院日期:无_总费用:0",
        finalBed->wardType, finalBed->bedId, adminTime, days, finalDeposit);
    getCurrentTimeStr(r5->createTime, 30);
    r5->next = recordHead->next;
    recordHead->next = r5;

    if (isPaid) {
        if (isCrossDept) printf("\n  [成功] 跨模块调度机制完成，已将病患安置于 [%s] 的 %s 节点。\n", getRoomDepartment(finalBed->bedId), finalBed->bedId);
        else printf("\n  [成功] 基础收治流程完结，已分配至 %s 的 %s 节点。\n", requiredDept, finalBed->bedId);
    }
    else {
        printf("\n  [待确认] 空间分配记录已生成，待财务端确认资金流入后释放节点使用权。\n");
    }
}

void dailyDeductionSimulation() {
    int count = 0;
    printf("\n========== 执行批量床位核销日结算 (模拟08:00) ==========\n");

    for (Bed* b = bedHead->next; b != NULL; b = b->next) {
        if (b->isOccupied) {
            for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                if (strcmp(p->id, b->patientId) == 0) {
                    if (p->balance < 1000) {
                        printf("  [预警] 检测到在院实体 %s 资金池跌破1000元红线 (结余: %.2f)！\n", p->name, p->balance);
                    }
                    printf("  -> 实体 %s (挂载床位:%s) 日度消耗计提: %.2f 元。\n", p->name, b->bedId, b->price);
                    count++;
                    break;
                }
            }
            b->isRoundsDone = 0;
        }
    }
    printf("--------------------------------------------------\n");
    printf("批处理作业完毕。累计扫描并计算 %d 个活跃在院单元，病床日内查房状态已复位。\n", count);
}

void wardRounds(const char* docId) {
    while (1) {
        char deptStr[200];
        char targetDept[50];

        system("cls");
        printf("\n========== 住院病区日常巡检与查房 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("调取目标科室域 (%s, 输入0返回): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "0") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) { isValidDept = 1; break; }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n调取目标科室域 (输0返回): ");
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
                    if (strcmp(p->id, b->patientId) == 0) { strcpy(pName, p->name); break; }
                }

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

        if (pCount == 0) { printf("  (目标病区当前无处于活跃周期的在院人员)\n"); system("pause"); continue; }

        char pId[20];
        Bed* targetBed = NULL;

        while (1) {
            printf("\n检索需建立查房会话的患者ID (输入0退回科室层级): ");
            safeGetString(pId, 20);
            if (strcmp(pId, "0") == 0) break;

            targetBed = NULL;
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                    targetBed = b;
                    break;
                }
            }
            if (targetBed) break;
            printf("  [!] 定位失败：未在 %s 的管辖范畴内检索到对应人员，请重新输入！\n", targetDept);
        }
        if (strcmp(pId, "0") == 0) continue;

        while (1) {
            system("cls");
            printf("\n========== 针对患者实体 [%s] 的查房干预面板 ==========\n", pId);
            printf("  [1] 录入基础观察医嘱\n  [2] 调用内部药房系统配发治疗耗材\n  [0] 结束当前查房会话\n---------------------------------------\n下达操作指令: ");

            int choice;
            while (1) {
                choice = safeGetInt();
                if (choice >= 0 && choice <= 2) break;
                printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n下达操作指令: ");
            }
            if (choice == 0) break;

            if (choice == 1) {
                char note[200];
                printf("输入实时医嘱指令内容(避免非法空格切断字符, 输入0取消): ");
                safeGetString(note, 200);
                if (strcmp(note, "0") == 0) continue;

                Record* rx = (Record*)malloc(sizeof(Record));
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
                extern char currentCallingPatientId[20];
                strcpy(currentCallingPatientId, pId);

                extern void prescribeMedicine(const char* docId);
                prescribeMedicine(docId);

                for (Record* cur = recordHead->next; cur != NULL; cur = cur->next) {
                    if (cur->type == 3 && strcmp(cur->patientId, pId) == 0 && cur->isPaid == 0) {
                        char tempDesc[320];
                        cur->isPaid = 4;
                        snprintf(tempDesc, sizeof(tempDesc), "[住院记账]%s", cur->description);
                        strncpy(cur->description, tempDesc, sizeof(cur->description) - 1);
                        cur->description[sizeof(cur->description) - 1] = '\0';
                    }
                }

                strcpy(currentCallingPatientId, "");
                targetBed->isRoundsDone = 1;
                system("pause");
            }
        }
    }
}

void dischargePatient() {
    while (1) {
        char deptStr[200];
        char targetDept[50];

        system("cls");
        printf("\n========== 离院综合清算办理控制台 ==========\n");

        getDynamicDeptPrompt(deptStr);
        printf("锁定业务作用域科室 (%s, 输入0放弃办理): ", deptStr);

        while (1) {
            safeGetString(targetDept, 50);
            if (strcmp(targetDept, "0") == 0) return;

            int isValidDept = 0;
            for (Staff* stf = staffHead->next; stf != NULL; stf = stf->next) {
                if (strcmp(stf->department, targetDept) == 0) { isValidDept = 1; break; }
            }
            if (isValidDept) break;
            printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！\n锁定业务作用域科室 (输0放弃办理): ");
        }

        printf("\n--- 【%s】 等待出院审批名册 ---\n", targetDept);
        printf("%-10s %-12s %-15s %-10s\n", "房号-床位", "属性", "患者ID", "姓名");
        printf("-----------------------------------------------------\n");

        int count = 0;
        for (Bed* b_list = bedHead->next; b_list != NULL; b_list = b_list->next) {
            if (b_list->isOccupied && strcmp(getRoomDepartment(b_list->bedId), targetDept) == 0) {
                char pName[100] = "未知";
                for (Patient* p = patientHead->next; p != NULL; p = p->next) {
                    if (strcmp(p->id, b_list->patientId) == 0) { strcpy(pName, p->name); break; }
                }
                printf("%-10s %-12s %-15s %-10s\n", b_list->bedId, b_list->wardType, b_list->patientId, pName);
                count++;
            }
        }
        if (count == 0) { printf("  (当前无可流转的出院对象)\n"); system("pause"); continue; }

        char pId[20];
        Bed* targetBed = NULL;

        while (1) {
            printf("\n确立清算实体的患者ID (输入0终止当前操作): ");
            safeGetString(pId, 20);
            if (strcmp(pId, "0") == 0) break;

            targetBed = NULL;
            for (Bed* b = bedHead->next; b != NULL; b = b->next) {
                if (b->isOccupied && strcmp(b->patientId, pId) == 0 && strcmp(getRoomDepartment(b->bedId), targetDept) == 0) {
                    targetBed = b;
                    break;
                }
            }
            if (targetBed) break;
            printf("  [!] 校验异常：所选区域中无此患者实体记录，请重新输入！\n");
        }
        if (strcmp(pId, "0") == 0) continue;

        int pendingPayment = 0;
        for (Record* r_check = recordHead->next; r_check != NULL; r_check = r_check->next) {
            if (strcmp(r_check->patientId, pId) == 0 && r_check->type == 5 && r_check->isPaid == 0 && strstr(r_check->description, "出院清算_补缴欠费差额")) {
                printf("\n  [系统资源互斥] 底层检测到该出院流水线正在挂起，存在待处理的资金缺口！\n");
                printf("  >>> 操作指引：需告知责任患者前往前端终端支付尾款。款项入账后引擎将利用异步回调自行释放空间资源，此处拒绝重复建单操作！\n");
                system("pause");
                pendingPayment = 1;
                break;
            }
        }
        if (pendingPayment) return;

        time_t t = time(NULL);
        struct tm* tm_info = localtime(&t);
        int currentHour = tm_info->tm_hour;

        printf("\n键入供财务审核的最终驻留计费天数: ");
        int actualDays = safeGetPositiveInt();

        int billableDays = actualDays;
        if (currentHour >= 0 && currentHour < 8) {
            printf("  [时间优惠策略触发] 08:00 阈值前申请办理，系统已自动扣减终端最后一天的驻留费用计提！\n");
            billableDays = (actualDays > 1) ? actualDays - 1 : 0;
        }

        double totalBedFee = billableDays * targetBed->price;
        double totalDrugFee = 0;
        double total_deposit = 0;

        for (Record* r = recordHead->next; r != NULL; r = r->next) {
            if (strcmp(r->patientId, pId) == 0) {
                if (r->type == 3 && r->isPaid == 4) totalDrugFee += r->cost;
                else if (r->type == 5 && r->isPaid == 1) total_deposit += r->cost;
            }
        }

        double totalHospitalCost = totalBedFee + totalDrugFee;

        if (total_deposit >= totalHospitalCost) {
            for (Record* r = recordHead->next; r != NULL; r = r->next) {
                if (strcmp(r->patientId, pId) == 0 && r->type == 3 && r->isPaid == 4) r->isPaid = 1;
            }

            double refund = total_deposit - totalHospitalCost;
            if (refund > 0) {
                for (Patient* pt = patientHead->next; pt != NULL; pt = pt->next) {
                    if (strcmp(pt->id, pId) == 0) { pt->balance += refund; break; }
                }
                printf("\n  [资金清算] 提取前期统筹押金 %.2f 元，溢出资产 %.2f 元，已执行原路资金清退。\n", total_deposit, refund);

                Record* r8 = (Record*)malloc(sizeof(Record));
                extern void generateRecordID(char* buffer);
                generateRecordID(r8->recordId);
                r8->type = 8; strcpy(r8->patientId, pId); strcpy(r8->staffId, "SYS");
                r8->cost = refund; r8->isPaid = 1; sprintf(r8->description, "出院清算_押金结余退回");
                getCurrentTimeStr(r8->createTime, 30);
                r8->next = recordHead->next; recordHead->next = r8;

                Transaction* newTrans = (Transaction*)malloc(sizeof(Transaction));
                int maxId = 0;
                for (Transaction* curr = transactionList; curr != NULL; curr = curr->next) {
                    if (curr->id > maxId) maxId = curr->id;
                }
                newTrans->id = maxId + 1; newTrans->type = 2; newTrans->amount = -refund;
                getCurrentTimeStr(newTrans->time, 30); strcpy(newTrans->description, "出院清算_押金结余退回");
                newTrans->next = NULL;
                if (!transactionList) transactionList = newTrans;
                else {
                    Transaction* curr = transactionList;
                    while (curr->next) curr = curr->next;
                    curr->next = newTrans;
                }
            }

            for (Record* r = recordHead->next; r != NULL; r = r->next) {
                if (strcmp(r->patientId, pId) == 0 && r->type == 5 && r->isPaid == 1) {
                    r->isPaid = 2;
                    char summary[200];
                    sprintf(summary, " [出院结算:床费%.2f 药费%.2f 总消费%.2f]", totalBedFee, totalDrugFee, totalHospitalCost);
                    /* 修复Bug：原strcat可能溢出description[300]，改用strncat限制追加长度 */
                    strncat(r->description, summary, sizeof(r->description) - strlen(r->description) - 1);
                }
            }
            targetBed->isOccupied = 0;
            strcpy(targetBed->patientId, "");
            printf("\n  [解除挂载] 系统解绑指令送达，该物理床位资源已恢复闲置状态。\n");
            system("pause"); return;
        }
        else {
            double arrears = totalHospitalCost - total_deposit;
            Record* r_arrears = (Record*)malloc(sizeof(Record));
            extern void generateRecordID(char* buffer);

            generateRecordID(r_arrears->recordId);
            r_arrears->type = 5;
            strcpy(r_arrears->patientId, pId);
            strcpy(r_arrears->staffId, "SYS");
            r_arrears->cost = arrears;
            r_arrears->isPaid = 0;

            sprintf(r_arrears->description, "出院清算_补缴欠费差额_床位:%s_床费:%.2f_药费:%.2f", targetBed->bedId, totalBedFee, totalDrugFee);
            getCurrentTimeStr(r_arrears->createTime, 30);
            r_arrears->next = recordHead->next;
            recordHead->next = r_arrears;

            printf("\n  [资金拦截] 收支不平衡预警：实发总额 %.2f，储备结余 %.2f。\n", totalHospitalCost, total_deposit);
            printf("  >>> 资产缺口认定: %.2f 元。数据指针已抛出至前端财务缴费中心。 <<<\n", arrears);
            printf("  后续动作移交异步处理：患者完成终端资产确认后，底层引擎将代理执行资源的清退与解绑，当前会话完毕。\n");
            system("pause"); return;
        }
    }
}

void inpatientMenu(const char* docId) {
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

        int choice;
        while (1) {
            choice = safeGetInt();
            if (choice >= 0 && choice <= 5) break;
            printf("  [!] 输入格式不合法，请正确输入菜单中提供的数字编号！\n请重新选择: ");
        }

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