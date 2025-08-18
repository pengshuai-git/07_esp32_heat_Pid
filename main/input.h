#ifndef INPUT_H
#define INPUT_H

// 初始化按键
void input_init(void);

// 获取设定温度变化 (返回delta)
float input_get_delta(void);

#endif