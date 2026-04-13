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

static int getCurrentYearValue(void) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    return tm_info ? (tm_info->tm_year + 1900) : 2026;
}

static int getShiftPriorityForSchedule(const char* shift) {
    if (!shift) return 9;
    if (strcmp(shift, "上午") == 0 || strcmp(shift, "早班") == 0) return 0;
    if (strcmp(shift, "下午") == 0 || strcmp(shift, "晚班") == 0) return 1;
    if (strcmp(shift, "休息") == 0) return 2;
    return 9;
}

static int compareSchedulePtrByDateShiftId(const void* lhs, const void* rhs) {
    const Schedule* a = *(const Schedule* const*)lhs;
    const Schedule* b = *(const Schedule* const*)rhs;
    int cmp = strcmp(a->date, b->date);
    if (cmp != 0) return cmp;
    cmp = getShiftPriorityForSchedule(a->shift) - getShiftPriorityForSchedule(b->shift);
    if (cmp != 0) return cmp;
    return a->schedule_id - b->schedule_id;
}

static Staff* findStaffByIdLocal(const char* id) {
    for (Staff* d = staffHead->next; d != NULL; d = d->next) {
        if (strcmp(d->id, id) == 0) return d;
    }
    return NULL;
}

static Schedule* findScheduleByIdLocal(int scheduleId) {
    for (Schedule* s = scheduleList->next; s != NULL; s = s->next) {
        if (s->schedule_id == scheduleId) return s;
    }
    return NULL;
}

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

static int isScheduleIdAlreadyCollected(Schedule* const* arr, int count, int scheduleId) {
    for (int i = 0; i < count; ++i) {
        if (arr[i] && arr[i]->schedule_id == scheduleId) return 1;
    }
    return 0;
}

static int collectRecommendedSchedules(const Schedule* targetSch, const Staff* targetDoc,
    const char* today, const char* nextWeek, Schedule** out, int maxOut) {
    int count = 0;
    if (!targetSch || !targetDoc || !out || maxOut <= 0) return 0;

    for (Schedule* altS = scheduleList->next; altS != NULL; altS = altS->next) {
        int sameDoctorOtherDate = 0;
        int sameDateSameDept = 0;

        if (strcmp(altS->shift, "休息") == 0) continue;
        if (strcmp(altS->date, today) < 0 || strcmp(altS->date, nextWeek) > 0) continue;
        if (altS->schedule_id == targetSch->schedule_id) continue;
        if (countDoctorAppointmentsOnDate(altS->doctor_id, altS->date) >= 50) continue;

        sameDoctorOtherDate =
            strcmp(altS->doctor_id, targetDoc->id) == 0 &&
            strcmp(altS->date, targetSch->date) != 0;

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

    if (count > 1) {
        qsort(out, (size_t)count, sizeof(out[0]), compareSchedulePtrByDateShiftId);
    }
    return count;
}

static int extractTrailingSequence(const char* id) {
    int len = (int)strlen(id);
    int seq = -1;
    if (len < 4) return -1;
    if (sscanf(id + len - 4, "%4d", &seq) == 1) return seq;
    return -1;
}

/* -------------------- 对外公开函数：基础与挂号 -------------------- */

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

Patient* findPatientById(const char* pid) {
    for (Patient* p = patientHead->next; p != NULL; p = p->next) {
        if (strcmp(p->id, pid) == 0) return p;
    }
    return NULL;
}

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

    printf("请设置登录密码 (至少6位，仅限数字或字母组合, 输入-1取消): ");
    safeGetPassword(newPatient->password, 50);
    if (strcmp(newPatient->password, "-1") == 0) {
        free(newPatient);
        return;
    }

    printf("请输入生理性别 (男/女, 输入-1取消): ");
    safeGetGender(newPatient->gender, 10);
    if (strcmp(newPatient->gender, "-1") == 0) {
        free(newPatient);
        return;
    }

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
        newPatient->age = -1;
        strcpy(newPatient->allergy, "急诊未知");
    }

    generatePatientID(newPatient->id);
    newPatient->balance = 0.0;
    newPatient->inpatientDeposit = 0.0;
    newPatient->isInpatient = 0;

    Patient* temp = patientHead;
    while (temp->next) temp = temp->next;
    temp->next = newPatient;

    printf("\n  [√] 系统建档操作已完成。\n");
    printf("  ==========================================\n");
    printf("  您的终身就诊识别码为: 【 %s 】 (凭此号登录)\n", newPatient->id);
    printf("  ==========================================\n");
}

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

        for (Record* r_temp = recordHead->next; r_temp != NULL; r_temp = r_temp->next) {
            int curReg = extractTrailingSequence(r_temp->recordId);
            if (curReg > maxRegId) maxRegId = curReg;
        }

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

        if (matchedCount == 0) {
            printf("\n  [!] 数据流反馈：未搜索到满足当前条件的排班资源。\n");
            system("pause");
            continue;
        }

        printf("  ---------------------------------------------------------------------------\n");
        printf("  请输入要确认选择的【排班ID】 (输入-1重新搜索): ");

        int targetSchId = safeGetInt();
        if (targetSchId == -1) continue;

        {
            int idInResult = 0;
            for (int i = 0; i < matchedCount; ++i) {
                if (matchedSchedules[i] && matchedSchedules[i]->schedule_id == targetSchId) {
                    idInResult = 1;
                    break;
                }
            }
            if (!idInResult) {
                printf("  [!] 当前科室/医生无此排班，请重新输入！\n");
                system("pause");
                continue;
            }
        }

        {
            char recommendSourceSuffix[64] = "";

            while (1) {
                Schedule* targetSch = findScheduleByIdLocal(targetSchId);
                Staff* targetDoc = NULL;
                char staffIdStr[22];
                int patientDailyActive = 0;
                int patientDeptDailyActive = 0;
                int sameDocSameDay = 0;
                int docDailyCount = 0;
                int hospitalDailyCount = 0;

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
                    int seqNum = docDailyCount + 1;
                    double regFee = 15.0;
                    Record* newRecord = NULL;
                    Record* temp = NULL;

                    if (strcmp(targetDoc->level, "主任医师") == 0) regFee = 50.0;
                    else if (strcmp(targetDoc->level, "副主任医师") == 0) regFee = 30.0;
                    else if (strcmp(targetDoc->level, "主治医师") == 0) regFee = 15.0;

                    newRecord = (Record*)malloc(sizeof(Record));
                    if (!newRecord) {
                        printf("  [!] 内存分配失败。\n");
                        return;
                    }

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
