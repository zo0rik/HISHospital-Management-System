#pragma once
#ifndef INPATIENT_DEPARTMENT_H
#define INPATIENT_DEPARTMENT_H

// 住院部网关路由
#include "models.h"

void inpatientMenu(const char* docId);
void syncInpatientArrearsBill(Patient* patient, const char* staffId);
int isInpatientArrearsBill(const Record* rec);

#endif
