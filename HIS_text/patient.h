#ifndef PATIENT_H
#define PATIENT_H

#include <stddef.h>
#include "models.h"

/*
 * 患者模块（拆分版）
 * - patient1.c：注册、挂号、通用查询辅助
 * - patient2.c：缴费、档案查询、密码修改、患者端主菜单
 */

 /* 基础查询与工具 */
void generatePatientID(char* idBuffer);
Patient* findPatientById(const char* pid);
int collectHospitalDepartments(char depts[][50], int maxCount);
int isKnownHospitalDepartment(const char* deptName);
void buildHospitalDeptPrompt(char* promptBuffer, size_t size, const char* separator);
int inputExistingPatientIdCommon(char* pId, size_t size, const char* prompt);

/* 患者业务入口 */
void registerPatient(void);
void bookAppointment(const char* currentPatientId);
void financeCenter(const char* currentPatientId);
void medicalRecords(const char* currentPatientId);
void changePatientPassword(const char* currentId);
void userTerminal(const char* currentId);

#endif /* PATIENT_H */
