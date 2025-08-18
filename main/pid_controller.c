#include "pid_controller.h"
#include "esp_log.h"

static const char *TAG = "PID";

// 初始化PID参数
void pid_init(PID_t *pid, float kp, float ki, float kd, float setpoint) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->setpoint = setpoint;
    pid->integral = 0.0;
    pid->last_error = 0.0;
    pid->last_input = 0.0;
    ESP_LOGI(TAG, "PID initialized: Kp=%.2f, Ki=%.2f, Kd=%.2f, Setpoint=%.2f", kp, ki, kd, setpoint);
}

// 计算PID输出
float pid_compute(PID_t *pid, float input) {
    float error = pid->setpoint - input;
    float dInput = input - pid->last_input;

    // 积分项 (防风饱和: 限制积分)
    pid->integral += pid->Ki * error;
    if (pid->integral > 100.0) pid->integral = 100.0;
    if (pid->integral < 0.0) pid->integral = 0.0;

    // PID公式
    float output = pid->Kp * error + pid->integral - pid->Kd * dInput;

    // 限幅0-100%
    if (output > 100.0) output = 100.0;
    if (output < 0.0) output = 0.0;

    pid->last_error = error;
    pid->last_input = input;

    ESP_LOGD(TAG, "PID output: %.2f (Error: %.2f, Integral: %.2f, dInput: %.2f)", output, error, pid->integral, dInput);

    return output;
}