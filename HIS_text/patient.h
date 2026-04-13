#pragma once
#ifndef USER_H
#define USER_H

#include "models.h"

// 对 main 暴露的接口：注册与患者端主路由
void registerPatient();
void userTerminal(const char* currentId); // 接收 main.c 验证通过的 ID
Patient* findPatientById(const char* pid);

#endif