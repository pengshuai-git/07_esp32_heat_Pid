#include "relay.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

static int s_relay_gpio = -1;
static bool s_relay_on = false;
static bool s_pwm_mode = false;
static int s_pwm_percent = 0;

void relay_init(int gpio) {
    s_relay_gpio = gpio;
    s_pwm_mode = false;
    gpio_config_t io = {
        .pin_bit_mask = (1ULL<<s_relay_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    relay_set(false);
}

void relay_set(bool on) {
    s_relay_on = on;
    if (!s_pwm_mode) {
        gpio_set_level(s_relay_gpio, on ? 1 : 0);
    } else {
        // PWM 模式下，on/off 可作为 100%/0% 的快捷设置
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, on ? 255 : 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
        s_pwm_percent = on ? 100 : 0;
    }
}

void relay_toggle(void) { relay_set(!s_relay_on); }

bool relay_get(void) { return s_relay_on; }

void relay_init_pwm(int gpio, int freq_hz) {
    s_relay_gpio = gpio;
    s_pwm_mode = true;
    ledc_timer_config_t t = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = freq_hz > 0 ? freq_hz : 1000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
    ledc_channel_config_t ch = {
        .gpio_num = s_relay_gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_3,
        .timer_sel = LEDC_TIMER_1,
        .duty = 0,
        .intr_type = LEDC_INTR_DISABLE,
    };
    ledc_channel_config(&ch);
}

void relay_set_pwm_percent(int percent) {
    if (!s_pwm_mode) {
        // 未开启 PWM，退化为阈值开关
        relay_set(percent >= 50);
        return;
    }
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }
    uint32_t duty = (uint32_t)(percent * 255 / 100);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
    s_relay_on = (percent > 0);
    s_pwm_percent = percent;
}

int relay_get_pwm_percent(void) { return s_pwm_percent; }


