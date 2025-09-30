#ifndef BUZZER_H
#define BUZZER_H

// 初始化蜂鸣器（传入GPIO编号，例：7）
void buzzer_init(int gpio);

// 蜂鸣报警 (1s)
void buzzer_alarm(void);

#endif
