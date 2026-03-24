#pragma once
#ifndef USER_H
#define USER_H

// 对 main 暴露的接口：注册与患者端主路由
void registerPatient();
void userTerminal(const char* currentId); // 接收 main.c 验证通过的 ID

#endif