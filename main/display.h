#ifndef DISPLAY_H
#define DISPLAY_H

// 初始化OLED
void display_init(void);

// 更新显示 (温度, 设定, 电池)
void display_update(float temp, float setpoint, float battery);

// 可选：清屏
void display_clear(void);

// 显示三行任意文本（超长自动截断到屏宽）
void display_show_text(const char *line1, const char *line2, const char *line3);

#endif