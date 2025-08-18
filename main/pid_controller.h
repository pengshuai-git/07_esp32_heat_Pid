#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>

// PID结构
typedef struct {
    float Kp;          // 比例增益
    float Ki;          // 积分增益
    float Kd;          // 微分增益
    float setpoint;    // 设定温度
    float integral;    // 积分项
    float last_error;  // 上次误差
    float last_input;  // 上次输入
} PID_t;

// 初始化PID
void pid_init(PID_t *pid, float kp, float ki, float kd, float setpoint);

// 计算PID输出 (0-100%)
float pid_compute(PID_t *pid, float input);

// 时间比例控制窗口 (s)
#define WINDOW_SIZE 5000  // 5秒窗口

#endif