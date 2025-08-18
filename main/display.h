#ifndef DISPLAY_H
#define DISPLAY_H

// 初始化OLED
void display_init(void);

// 更新显示 (温度, 设定, 电池)
void display_update(float temp, float setpoint, float battery);

#endif