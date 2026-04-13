#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "models.h"
#include "patient.h"
#include "utils.h"
#include "doctor.h"
#include "schedule.h"
#include "transaction.h"
#include "drug.h"
#include "inpatient_department.h"

/* -------------------- patient1.c 内部辅助函数 -------------------- */

// 获取当前年份
/*
 * 函数名：getCurrentYearValue
 * 作用：获取当前系统年份，用于患者ID等业务编号的生成年份部分。
 * 返回值：
 *   - 成功获取本地时间时，返回当前年份，例如 2026。
 *   - 若 localtime 调用失败，则返回默认值 2026，避免编号生成中断。
 * 说明：
 *   这是 patient1.c 内部使用的辅助函数，不对模块外公开。
 */
static int getCurrentYearValue(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    return tm_info ? (tm_info->tm_year + 1900) : 2026;
}

// 获取班次排序优先级：上午/早班 -> 下午/晚班 -> 休息
/*
 * 函数名：getShiftPriorityForSchedule
 * 作用：把班次字符串转换成可比较的优先级数值，供排班排序使用。
 * 参数：
 *   shift - 班次名称，支持“上午/早班/下午/晚班/休息”。
 * 返回值：
 *   - 上午/早班：0
 *   - 下午/晚班：1
 *   - 休息：2
 *   - 其他未知值或空指针：9
 * 说明：
 *   数值越小，排序时越靠前。
 */
static int getShiftPriorityForSchedule(const char* shift) {
    if (!shift) return 9;
    if (strcmp(shift, "上午") == 0 || strcmp(shift, "早班") == 0) return 0;
    if (strcmp(shift, "下午") == 0 || strcmp(shift, "晚班") == 0) return 1;
    if (strcmp(shift, "休息") == 0) return 2;
    return 9;
}

// 排班排序：先按日期，再按班次，最后按排班ID
/*
 * 函数名：compareSchedulePtrByDateShiftId
 * 作用：作为 qsort 的比较函数，对排班指针数组进行统一排序。
 * 排序规则：
 *   1. 先按日期升序；
 *   2. 同一天按班次优先级排序；
 *   3. 日期和班次都相同时，按排班ID升序。
 * 参数：
 *   lhs、rhs - 指向 Schedule* 元素的指针。
 * 返回值：
 *   符合 qsort 约定的比较结果。
 */
static int compareSchedulePtrByDateShiftId(const void* lhs, const void* rhs) {
    const Schedule* a = *(const Schedule* const*)lhs;
    const Schedule* b = *(const Schedule* const*)rhs;
    int cmp = strcmp(a->date, b->date);
    if (cmp != 0) return cmp;
    cmp = getShiftPriorityForSchedule(a->shift) - getShiftPriorityForSchedule(b->shift);
    if (cmp != 0) return cmp;
    return a->schedule_id - b->schedule_id;
}

// 按医生ID查找医生
/*
 * 函数名：findStaffByIdLocal
 * 作用：在全局医生链表中按医生ID查找对应的 Staff 节点。
 * 参数：
 *   id - 目标医生工号。
 * 返回值：
 *   - 找到则返回对应 Staff 指针；
 *   - 找不到返回 NULL。
 * 说明：
 *   仅供 patient1.c 内部使用，主要用于挂号检索和推荐逻辑。
 */
static Staff* findStaffByIdLocal(const char* id) {
    for (Staff* d = staffHead->next; d != NULL; d = d->next) {
        if (strcmp(d->id, id) == 0) return d;
    }
    return NULL;
}

// 按排班ID查找排班
/*
 * 函数名：findScheduleByIdLocal
 * 作用：在全局排班链表中按排班ID查找具体排班节点。
 * 参数：
 *   scheduleId - 排班唯一编号。
 * 返回值：
 *   - 找到时返回 Schedule 指针；
 *   - 未找到时返回 NULL。
 */
static Schedule* findScheduleByIdLocal(int scheduleId) {
    for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
        if (s->schedule_id == scheduleId) return s;
    }
    return NULL;
}

// 统计某位医生在指定日期的挂号数量
/*
 * 函数名：countDoctorAppointmentsOnDate
 * 作用：统计某位医生在指定日期已有多少条挂号记录。
 * 参数：
 *   doctorId - 医生ID；
 *   date     - 目标日期字符串，格式为 YYYY-MM-DD。
 * 返回值：
 *   符合条件的挂号记录数量。
 * 说明：
 *   当前统计依据是记录类型为 1，且 description 中包含该日期。
 *   该结果用于判断医生是否达到单日 50 个号的上限。
 */
static int countDoctorAppointmentsOnDate(const char* doctorId, const char* date) {
    int count = 0;
    for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
        if (rec->type == 1 &&
            strcmp(rec->staffId, doctorId) == 0 &&
            strstr(rec->description, date) != NULL) {
            count++;
        }
    }
    return count;
}

// 判断某个排班ID是否已经加入推荐列表
/*
 * 函数名：isScheduleIdAlreadyCollected
 * 作用：判断某个排班ID是否已经被加入推荐结果数组，防止重复推荐。
 * 参数：
 *   arr        - 推荐结果数组；
 *   count      - 当前数组中有效元素数量；
 *   scheduleId - 待检查的排班ID。
 * 返回值：
 *   - 已存在返回 1；
 *   - 不存在返回 0。
 */
static int isScheduleIdAlreadyCollected(Schedule* const* arr, int count, int scheduleId) {
    for (int i = 0; i < count; ++i) {
        if (arr[i] && arr[i]->schedule_id == scheduleId) return 1;
    }
    return 0;
}

// 收集智能推荐的排班
/*
 * 函数名：collectRecommendedSchedules
 * 作用：当目标医生当天号源已满时，收集可供患者选择的推荐排班。
 * 推荐规则：
 *   1. 排除休息排班；
 *   2. 只保留未来一周内的排班；
 *   3. 排除当前患者原本选择的排班；
 *   4. 排除医生当日已满 50 号的排班；
 *   5. 优先收集同一医生的其他日期排班，或同一天同科室其他医生的排班。
 * 参数：
 *   targetSch - 原目标排班；
 *   targetDoc - 原目标排班对应医生；
 *   today、nextWeek - 推荐日期范围；
 *   out      - 输出数组；
 *   maxOut   - 输出数组最大容量。
 * 返回值：
 *   实际收集到的推荐排班数量。
 */
static int collectRecommendedSchedules(const Schedule* targetSch, const Staff* targetDoc,
    const char* today, const char* nextWeek, Schedule** out, int maxOut) {
    int count = 0;
    if (!targetSch || !targetDoc || !out || maxOut <= 0) return 0;

    for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
        int sameDoctorOtherDate = 0;
        int sameDateSameDept = 0;

        // 跳过休息排班
        if (strcmp(altS->shift, "休息") == 0) continue;

        // 只保留未来一周内的排班
        if (strcmp(altS->date, today) < 0 || strcmp(altS->date, nextWeek) > 0) continue;

        // 跳过当前目标排班本身
        if (altS->schedule_id == targetSch->schedule_id) continue;

        // 跳过已经满号的排班
        if (countDoctorAppointmentsOnDate(altS->doctor_id, altS->date) >= 50) continue;

        // 同一医生的其他日期
        sameDoctorOtherDate =
            strcmp(altS->doctor_id, targetDoc->id) == 0 &&
            strcmp(altS->date, targetSch->date) != 0;

        // 同一天同科室的其他医生
        if (strcmp(altS->date, targetSch->date) == 0 &&
            strcmp(altS->doctor_id, targetDoc->id) != 0) {
            Staff* altD = findStaffByIdLocal(altS->doctor_id);
            if (altD && strcmp(altD->department, targetDoc->department) == 0) {
                sameDateSameDept = 1;
            }
        }

        if (!sameDoctorOtherDate && !sameDateSameDept) continue;
        if (isScheduleIdAlreadyCollected(out, count, altS->schedule_id)) continue;

        out[count++] = altS;
        if (count >= maxOut) break;
    }

    // 推荐结果排序
    if (count > 1) {
        qsort(out, (size_t)count, sizeof(out[0]), compareSchedulePtrByDateShiftId);
    }
    return count;
}

// 提取ID最后4位数字
/*
 * 函数名：extractTrailingSequence
 * 作用：从编号字符串末尾提取最后四位数字序号。
 * 参数：
 *   id - 如 P20260001、REG20265000 这类编号字符串。
 * 返回值：
 *   - 提取成功返回末尾四位对应的整数；
 *   - 长度不足或格式不符时返回 -1。
 * 说明：
 *   主要用于生成新患者ID和新挂号记录ID时确定当前最大流水号。
 */
static int extractTrailingSequence(const char* id) {
    int len = (int)strlen(id);
    int seq = -1;
    if (len < 4) return -1;
    if (sscanf(id + len - 4, "%4d", &seq) == 1) return seq;
    return -1;
}

/* -------------------- 对外公开函数：基础与挂号 -------------------- */

// 生成患者ID：P + 年份 + 四位序号
/*
 * 函数名：generatePatientID
 * 作用：按照系统规则为新患者生成唯一患者ID。
 * 编号规则：
 *   P + 当前年份 + 四位递增序号，例如 P20260001。
 * 参数：
 *   idBuffer - 输出缓冲区，用于接收新生成的患者ID。
 * 说明：
 *   函数会遍历 patientHead 链表，找到当前最大尾号后再加 1。
 */
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

// 按患者ID查找患者
/*
 * 函数名：findPatientById
 * 作用：在患者链表中根据患者ID精确查找患者节点。
 * 参数：
 *   pid - 患者ID。
 * 返回值：
 *   - 找到则返回对应 Patient 指针；
 *   - 未找到则返回 NULL。
 * 说明：
 *   该函数是 patient 模块的重要基础接口，多个模块都会复用。
 */
Patient* findPatientById(const char* pid) {
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (strcmp(p->id, pid) == 0) return p;
    }
    return NULL;
}

// 收集系统中所有不重复的科室名称
/*
 * 函数名：collectHospitalDepartments
 * 作用：扫描医生链表，收集当前系统中所有不重复的科室名称。
 * 参数：
 *   depts    - 由调用者提供的二维字符数组，用于保存科室名；
 *   maxCount - 最多允许收集的科室数量。
 * 返回值：
 *   实际收集到的不重复科室数量。
 * 说明：
 *   此函数常用于生成科室提示列表和校验用户输入的科室名称。
 */
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

// 判断科室名称是否存在
/*
 * 函数名：isKnownHospitalDepartment
 * 作用：判断用户输入的科室名称是否存在于当前医院系统中。
 * 参数：
 *   deptName - 待校验的科室名。
 * 返回值：
 *   - 存在返回 1；
 *   - 不存在或参数无效返回 0。
 */
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

// 生成科室提示字符串
/*
 * 函数名：buildHospitalDeptPrompt
 * 作用：把系统中已有的科室名称拼接成一条提示字符串。
 * 参数：
 *   promptBuffer - 输出缓冲区；
 *   size         - 缓冲区大小；
 *   separator    - 科室之间的分隔符，若为 NULL 默认使用 "/"。
 * 说明：
 *   该函数不直接打印，而是由调用方决定如何展示拼接后的提示文本。
 */
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

// 统一输入并校验患者ID
/*
 * 函数名：inputExistingPatientIdCommon
 * 作用：统一读取患者ID并校验该患者是否真实存在。
 * 参数：
 *   pId    - 输出缓冲区；
 *   size   - 缓冲区大小；
 *   prompt - 自定义提示语，传 NULL 时使用默认提示。
 * 返回值：
 *   - 输入了有效患者ID返回 1；
 *   - 输入 -1 取消返回 0。
 * 说明：
 *   该函数把“读取 + 存在性校验”封装到一起，供门诊和住院端复用。
 */
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

// 注册患者
/*
 * 函数名：registerPatient
 * 作用：完成患者的首次注册建档。
 * 主要流程：
 *   1. 选择普通门诊或急诊绿色通道；
 *   2. 录入姓名、密码、性别；
 *   3. 普通门诊额外录入年龄和过敏史；
 *   4. 生成患者ID并初始化余额、押金、住院状态；
 *   5. 将新患者节点挂到患者链表尾部。
 * 说明：
 *   本函数只负责在内存中建立患者档案，最终持久化由系统统一保存。
 */
void registerPatient(void) {
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

    // 输入姓名
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

    // 输入密码
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

    // 普通门诊需补充年龄和过敏史
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
        // 急诊信息可简化
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    // 初始化患者信息
    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;
    newPatient->inpatientDeposit = 0.0;
    newPatient->isInpatient = 0;

    // 插入患者链表尾部
    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("\n  [√] 系统建档操作已完成。\n");
    printf("  ==========================================\n");
    printf("  您的终身就诊识别码为: 【 %s 】 (凭此号登录)\n", newPatient->id);
    printf("  ==========================================\n");
}

// 患者预约挂号
/*
 * 函数名：bookAppointment
 * 作用：提供患者端“预约门诊挂号”功能。
 * 参数：
 *   currentPatientId - 当前登录患者的ID，用于挂号约束判断和记录归属。
 * 主要流程：
 *   1. 计算今天到未来一周的日期范围；
 *   2. 支持按科室或按医生检索排班；
 *   3. 对命中的排班按日期、班次、排班ID排序展示；
 *   4. 校验患者输入的排班ID是否有效；
 *   5. 检查全院总量、患者单日次数、同日同科室、同医生重复挂号等限制；
 *   6. 若目标医生满 50 个号，给出智能推荐排班；
 *   7. 通过后生成一条待支付挂号记录。
 * 说明：
 *   函数只创建挂号单，不在此处扣费；实际支付由财务中心完成。
 */
void bookAppointment(const char* currentPatientId) {
    char today[11];
    char nextWeek[11];
    time_t t = time(NULL);
    struct tm* tm_info = localtime(&t);

    // 计算今天和未来一周结束日期
    strftime(today, sizeof(today), "%Y-%m-%d", tm_info);
    t += 7 * 24 * 60 * 60;
    tm_info = localtime(&t);
    strftime(nextWeek, sizeof(nextWeek), "%Y-%m-%d", tm_info);

    while (1) {
        char keyword[50];
        int choice;

        system("cls");
        printf("\n========== 自助预约门诊挂号 ==========\n");
        printf("  [1] 按【科室名称】搜寻未来一周排班\n");
        printf("  [2] 按【医生姓名/工号】搜寻排班\n");
        printf(" [-1] 放弃挂号，返回上级菜单\n");
        printf("--------------------------------------\n");
        printf("  请选择搜寻引擎: ");

        choice = safeGetInt();
        if (choice == -1) return;

        // 按科室搜索
        if (choice == 1) {
            char depts[20][50];
            int dCount = collectHospitalDepartments(depts, 20);

            printf("\n  [系统当前已开设的门诊科室]: ");
            for (int i = 0; i < dCount; i++) printf("[%s] ", depts[i]);

            while (1) {
                printf("\n  请输入您要挂号的目标科室名称 (输入-1返回): ");
                safeGetString(keyword, sizeof(keyword));

                if (strcmp(keyword, "-1") == 0) break;
                if (strlen(keyword) == 0) {
                    printf("  [!] 输入不能为空！");
                    system("pause");
                    continue;
                }
                if (isKnownHospitalDepartment(keyword)) break;
                printf("  [!] 输入的科室不存在，请从上方列表中选择并重新输入！");
            }

            if (strcmp(keyword, "-1") == 0) continue;
        }
        // 按医生搜索
        else if (choice == 2) {
            printf("  请输入医生精确姓名或纯数字工号 (如:李四 / 1001, 输入-1返回): ");
            safeGetString(keyword, sizeof(keyword));

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

        Schedule* matchedSchedules[200] = { NULL };
        int matchedCount = 0;
        int maxRegId = 4999;

        // 找当前最大的挂号记录序号
        for (Record* r_temp = recordHead->next; r_temp != NULL; r_temp = r_temp->next) {
            int curReg = extractTrailingSequence(r_temp->recordId);
            if (curReg > maxRegId) maxRegId = curReg;
        }

        // 筛选未来一周内符合条件的排班
        for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
            Staff* matchedDoc = NULL;
            int match = 0;

            if (strcmp(s->date, today) < 0 || strcmp(s->date, nextWeek) > 0) continue;
            if (strcmp(s->shift, "休息") == 0) continue;

            matchedDoc = findStaffByIdLocal(s->doctor_id);
            if (!matchedDoc) continue;

            if (choice == 1 && strcmp(matchedDoc->department, keyword) == 0) match = 1;
            if (choice == 2 &&
                (strstr(matchedDoc->name, keyword) != NULL || strcmp(matchedDoc->id, keyword) == 0)) {
                match = 1;
            }

            if (match && matchedCount < 200) {
                matchedSchedules[matchedCount++] = s;
            }
        }

        // 排序显示
        if (matchedCount > 1) {
            qsort(matchedSchedules, (size_t)matchedCount, sizeof(matchedSchedules[0]), compareSchedulePtrByDateShiftId);
        }

        printf("\n========== 未来一周可预约排班总表 (%s 至 %s) ==========\n", today, nextWeek);
        printf("  %-8s | %-12s | %-8s | %-18s | %-10s | %-10s\n",
            "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
        printf("  ---------------------------------------------------------------------------\n");

        for (int i = 0; i < matchedCount; ++i) {
            Staff* matchedDoc = findStaffByIdLocal(matchedSchedules[i]->doctor_id);
            char docDisp[130];

            if (!matchedDoc) continue;
            snprintf(docDisp, sizeof(docDisp), "%s(%s)", matchedDoc->name, matchedDoc->id);
            printf("  [%-6d] | %-12s | %-8s | %-18s | %-10s | %-10s\n",
                matchedSchedules[i]->schedule_id,
                matchedSchedules[i]->date,
                matchedSchedules[i]->shift,
                docDisp,
                matchedDoc->department,
                matchedDoc->level);
        }

        // 没有匹配排班
        if (matchedCount == 0) {
            printf("\n  [!] 数据流反馈：未搜索到满足当前条件的排班资源。\n");
            system("pause");
            continue;
        }

        printf("  ---------------------------------------------------------------------------\n");

        int targetSchId;
        while (1) {
            int idInResult = 0;

            printf("  请输入要确认选择的【排班ID】 (输入-1重新搜索): ");
            targetSchId = safeGetInt();

            if (targetSchId == -1) break;

            for (int i = 0; i < matchedCount; ++i) {
                if (matchedSchedules[i] && matchedSchedules[i]->schedule_id == targetSchId) {
                    idInResult = 1;
                    break;
                }
            }

            if (idInResult) break;

            printf("  [!] 当前科室/医生无此排班，请重新输入！\n");
        }

        if (targetSchId == -1) continue;

        {
            char recommendSourceSuffix[64] = "";

            while (1) {
                Schedule* targetSch = findScheduleByIdLocal(targetSchId);
                Staff* targetDoc = NULL;
                char staffIdStr[22];
                int patientDailyActive = 0;      // 患者当天挂号数
                int patientDeptDailyActive = 0;  // 患者当天同科室挂号数
                int sameDocSameDay = 0;          // 患者当天是否挂过同一医生
                int docDailyCount = 0;           // 医生当天挂号数
                int hospitalDailyCount = 0;      // 全院当天挂号数

                if (!targetSch) {
                    printf("  [!] 参数越界：排班ID不属于有效ID。\n");
                    system("pause");
                    break;
                }

                targetDoc = findStaffByIdLocal(targetSch->doctor_id);
                if (!targetDoc) {
                    printf("  [!] 底层数据异常：医生档案关联引用失败。\n");
                    system("pause");
                    break;
                }

                snprintf(staffIdStr, sizeof(staffIdStr), "%s", targetDoc->id);

                // 统计当天挂号相关数据
                for (Record* rec = recordHead->next; rec != NULL; rec = rec->next) {
                    if (rec->type == 1 && strstr(rec->description, targetSch->date) != NULL) {
                        hospitalDailyCount++;

                        if (strcmp(rec->staffId, staffIdStr) == 0) docDailyCount++;

                        if (strcmp(rec->patientId, currentPatientId) == 0 && rec->isPaid != 2) {
                            patientDailyActive++;

                            for (Staff* recDoc = staffHead->next; recDoc != NULL; recDoc = recDoc->next) {
                                if (strcmp(recDoc->id, rec->staffId) == 0) {
                                    if (strcmp(recDoc->department, targetDoc->department) == 0) {
                                        patientDeptDailyActive++;
                                    }
                                    if (strcmp(recDoc->id, staffIdStr) == 0) sameDocSameDay = 1;
                                    break;
                                }
                            }
                        }
                    }
                }

                // 各种挂号约束判断
                if (hospitalDailyCount >= 1000) {
                    printf("\n  [系统过载] 全院日门诊量已达设定阈值。\n");
                    system("pause");
                    break;
                }

                if (patientDailyActive >= 5) {
                    printf("\n  [策略约束] 患者单日预约次数已达上限。\n");
                    system("pause");
                    break;
                }

                if (patientDeptDailyActive >= 1) {
                    printf("\n  [策略约束] 同日同科室不允许重复挂号。\n");
                    system("pause");
                    break;
                }

                if (sameDocSameDay) {
                    printf("\n  [策略约束] 该日已存在相同医师的挂号记录。\n");
                    system("pause");
                    break;
                }

                // 医生当天满50个号后触发智能推荐
                if (docDailyCount >= 50) {
                    Schedule* recommendedSchedules[200] = { NULL };
                    int recCount = collectRecommendedSchedules(targetSch, targetDoc, today, nextWeek, recommendedSchedules, 200);

                    printf("\n  [ ！] %s 医生在 %s 的有效号源已全部分配完毕。\n", targetDoc->name, targetSch->date);
                    printf("  >>> 调度系统为您推荐以下相似接诊资源 <<<\n");

                    if (recCount == 0) {
                        printf("  未命中可预约的相似属性资源，请重置过滤条件。\n");
                        system("pause");
                        break;
                    }

                    printf("\n  %-8s | %-12s | %-8s | %-18s | %-10s | %-10s\n",
                        "排班ID", "出诊日期", "班次", "出诊医师(工号)", "科室", "职称");
                    printf("  ---------------------------------------------------------------------------\n");

                    for (int i = 0; i < recCount; ++i) {
                        Staff* altDoc = findStaffByIdLocal(recommendedSchedules[i]->doctor_id);
                        char docDisp[130];

                        if (!altDoc) continue;
                        snprintf(docDisp, sizeof(docDisp), "%s(%s)", altDoc->name, altDoc->id);
                        printf("  [%-6d] | %-12s | %-8s | %-18s | %-10s | %-10s\n",
                            recommendedSchedules[i]->schedule_id,
                            recommendedSchedules[i]->date,
                            recommendedSchedules[i]->shift,
                            docDisp,
                            altDoc->department,
                            altDoc->level);
                    }

                    printf("  ---------------------------------------------------------------------------\n");
                    printf("  是否要预约智能推荐的医生/排班？(1.是 0.否): ");

                    while (1) {
                        int useRecommended = safeGetInt();

                        if (useRecommended == 0 || useRecommended == -1) {
                            targetSchId = -1;
                            break;
                        }

                        if (useRecommended == 1) {
                            printf("  请输入要预约的推荐排班号 (输入-1重新搜索): ");

                            while (1) {
                                int nextSchId = safeGetInt();
                                int validRecId = 0;

                                if (nextSchId == -1) {
                                    targetSchId = -1;
                                    break;
                                }

                                for (int i = 0; i < recCount; ++i) {
                                    if (recommendedSchedules[i] && recommendedSchedules[i]->schedule_id == nextSchId) {
                                        validRecId = 1;
                                        break;
                                    }
                                }

                                if (validRecId) {
                                    snprintf(recommendSourceSuffix, sizeof(recommendSourceSuffix), "_推荐来源:%s满号", targetDoc->id);
                                    targetSchId = nextSchId;
                                    break;
                                }

                                printf("  [!] 请输入上方智能推荐列表中的排班号 (输入-1重新搜索): ");
                            }
                            break;
                        }

                        printf("  [!] 请输入 1、0 或 -1: ");
                    }

                    if (targetSchId == -1) break;
                    continue;
                }

                {
                    int seqNum = docDailyCount + 1;  // 当前排号
                    double regFee = 15.0;
                    Record* newRecord = NULL;
                    Record* temp = NULL;

                    // 根据医生职称计算挂号费
                    if (strcmp(targetDoc->level, "主任医师") == 0) regFee = 50.0;
                    else if (strcmp(targetDoc->level, "副主任医师") == 0) regFee = 30.0;
                    else if (strcmp(targetDoc->level, "主治医师") == 0) regFee = 15.0;

                    newRecord = (Record*)malloc(sizeof(Record));
                    if (!newRecord) {
                        printf("  [!] 内存分配失败。\n");
                        return;
                    }

                    // 生成挂号记录
                    sprintf(newRecord->recordId, "REG%d%04d", getCurrentYearValue(), maxRegId + 1);
                    newRecord->type = 1;
                    strcpy(newRecord->patientId, currentPatientId);
                    strcpy(newRecord->staffId, staffIdStr);
                    newRecord->cost = regFee;
                    newRecord->isPaid = 0;
                    snprintf(newRecord->description, sizeof(newRecord->description),
                        "挂号:%s(%s)_班次:%s_排号:%d_科室:%s%s",
                        targetDoc->name,
                        targetSch->date,
                        targetSch->shift,
                        seqNum,
                        targetDoc->department,
                        recommendSourceSuffix);
                    getCurrentTimeStr(newRecord->createTime, 30);
                    newRecord->next = NULL;

                    // 插入记录链表尾部
                    temp = recordHead;
                    while (temp->next) temp = temp->next;
                    temp->next = newRecord;

                    printf("\n  [√ 事务确认] %s 医师接诊号源申请成功！系统产生待支付流水 %.2f 元。\n", targetDoc->name, regFee);
                    printf("  =======================================================\n");
                    printf("  >>> 当日分诊系统的预计算序号为：【 第 %d 号 】 <<<\n", seqNum);
                    printf("  =======================================================\n");
                    printf("  (系统提示：请至费用中心结清账单)\n");
                    system("pause");
                    return;
                }
            }
        }
    }
}