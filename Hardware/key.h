#ifndef KEY_H
#define KEY_H

// 初始化按键（传入三个GPIO：加、减、确认）
void key_init(int btn_inc_gpio, int btn_dec_gpio, int btn_ok_gpio);

// 获取设定温度变化 (返回delta)
float key_get_delta(void);

#endif
