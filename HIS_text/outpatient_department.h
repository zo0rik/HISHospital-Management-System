#pragma once
#ifndef OUTPATIENT_DEPARTMENT_H
#define OUTPATIENT_DEPARTMENT_H
#include "patient.h"
#include "inpatient_department.h"
#include "transaction.h"
// 暴露全局状态，供住院部查房开药时借用
extern char currentCallingPatientId[20];

void generateRecordID(char* buffer);
void callPatient(const char* docId);
void diagnoseAndTest(const char* docId);
void prescribeMedicine(const char* docId);
void issueAdmissionNotice(const char* docId);

#endif