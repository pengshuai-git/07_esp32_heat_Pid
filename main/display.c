#include "display.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "DISPLAY";

// ===== SSD1306 基本定义 =====
#define I2C_PORT            I2C_NUM_0
#define I2C_SDA_IO          8
#define I2C_SCL_IO          9
#define I2C_CLK_HZ          400000
#define OLED_I2C_ADDR       0x3C

#define OLED_CMD            0x00
#define OLED_DATA           0x40

static inline esp_err_t i2c_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {OLED_CMD, cmd};
    return i2c_master_write_to_device(I2C_PORT, OLED_I2C_ADDR, buf, sizeof(buf), 1000 / portTICK_PERIOD_MS);
}

static inline esp_err_t i2c_write_data(const uint8_t *data, size_t len) {
    // 逐块写入，保证前缀 0x40
    esp_err_t err = ESP_OK;
    uint8_t block[17];
    block[0] = OLED_DATA;
    while (len) {
        size_t n = len > 16 ? 16 : len;
        memcpy(&block[1], data, n);
        err = i2c_master_write_to_device(I2C_PORT, OLED_I2C_ADDR, block, n + 1, 1000 / portTICK_PERIOD_MS);
        if (err != ESP_OK) return err;
        data += n;
        len -= n;
    }
    return err;
}

static void oled_set_pos(uint8_t x, uint8_t page) {
    i2c_write_cmd(0xB0 | (page & 0x07));
    i2c_write_cmd(0x00 | (x & 0x0F));
    i2c_write_cmd(0x10 | ((x >> 4) & 0x0F));
}

static void oled_clear_all(void) {
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_pos(0, page);
        uint8_t zeros[128] = {0};
        i2c_write_data(zeros, sizeof(zeros));
    }
}

// 5x7 字模：仅覆盖当前界面所需字符，未知字符按空格处理
static const uint8_t* glyph5x7(char c) {
    // 数字
    static const uint8_t d0[5] = {0x3E,0x51,0x49,0x45,0x3E};
    static const uint8_t d1[5] = {0x00,0x42,0x7F,0x40,0x00};
    static const uint8_t d2[5] = {0x42,0x61,0x51,0x49,0x46};
    static const uint8_t d3[5] = {0x21,0x41,0x45,0x4B,0x31};
    static const uint8_t d4[5] = {0x18,0x14,0x12,0x7F,0x10};
    static const uint8_t d5[5] = {0x27,0x45,0x45,0x45,0x39};
    static const uint8_t d6[5] = {0x3C,0x4A,0x49,0x49,0x30};
    static const uint8_t d7[5] = {0x01,0x71,0x09,0x05,0x03};
    static const uint8_t d8[5] = {0x36,0x49,0x49,0x49,0x36};
    static const uint8_t d9[5] = {0x06,0x49,0x49,0x29,0x1E};
    // 符号/空白
    static const uint8_t sp[5] = {0x00,0x00,0x00,0x00,0x00};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t dot[5] = {0x00,0x40,0x60,0x00,0x00};
    static const uint8_t percent[5] = {0x62,0x64,0x08,0x13,0x23};
    // 大写
    static const uint8_t Cc[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t Tt[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t Ss[5] = {0x22,0x49,0x49,0x49,0x31};
    static const uint8_t Bb[5] = {0x7F,0x49,0x49,0x49,0x36};
    static const uint8_t Ee[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t Hh[5] = {0x7F,0x08,0x08,0x08,0x7F};
    static const uint8_t Ll[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t Oo[5] = {0x3E,0x41,0x41,0x41,0x3E};
    // 小写
    static const uint8_t aa[5] = {0x20,0x54,0x54,0x54,0x78};
    static const uint8_t ee[5] = {0x30,0x49,0x49,0x49,0x3A};
    static const uint8_t mm[5] = {0x7C,0x08,0x04,0x08,0x7C};
    static const uint8_t pp[5] = {0x7C,0x14,0x14,0x14,0x08};
    static const uint8_t tt[5] = {0x04,0x3F,0x44,0x40,0x20};

    switch (c) {
        case '0': return d0; case '1': return d1; case '2': return d2; case '3': return d3; case '4': return d4;
        case '5': return d5; case '6': return d6; case '7': return d7; case '8': return d8; case '9': return d9;
        case ' ': return sp; case ':': return colon; case '.': return dot; case '%': return percent;
        case 'C': return Cc; case 'T': return Tt; case 'S': return Ss; case 'B': return Bb;
        case 'E': return Ee; case 'H': return Hh; case 'L': return Ll; case 'O': return Oo;
        case 'a': return aa; case 'e': return ee; case 'm': return mm; case 'p': return pp; case 't': return tt;
        default:  return sp;
    }
}

static void draw_char5x7(uint8_t x, uint8_t y, char c) {
    const uint8_t *g = glyph5x7(c);
    uint8_t page = y >> 3;      // 每页 8 像素
    uint8_t offset = y & 0x07;  // 页内位移

    uint8_t lower[6];
    uint8_t upper[6];
    for (int i = 0; i < 5; i++) {
        uint8_t raw = g[i];
        lower[i] = raw << offset;
        upper[i] = (offset == 0) ? 0x00 : (raw >> (8 - offset));
    }
    lower[5] = 0x00;
    upper[5] = 0x00;

    oled_set_pos(x, page);
    i2c_write_data(lower, 6);
    if (offset && page < 7) {
        oled_set_pos(x, page + 1);
        i2c_write_data(upper, 6);
    }
}

static void draw_text(uint8_t x, uint8_t y, const char *s) {
    while (*s) {
        draw_char5x7(x, y, *s++);
        x += 6; // 5 列字形 + 1 列间距
        if (x > 122) break;
    }
}

void display_clear(void) {
    oled_clear_all();
}

void display_show_text(const char *line1, const char *line2, const char *line3) {
    oled_clear_all();
    if (line1) draw_text(0, 0, line1);
    if (line2) draw_text(0, 16, line2);
    if (line3) draw_text(0, 32, line3);
}

// 初始化I2C和OLED（SSD1306）
void display_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_CLK_HZ,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);

    // 发送 I2C START/STOP 探测设备是否在线（调试日志）
    uint8_t dummy = 0x00;
    esp_err_t probe = i2c_master_write_to_device(I2C_PORT, OLED_I2C_ADDR, &dummy, 0, 100 / portTICK_PERIOD_MS);
    if (probe != ESP_OK) {
        ESP_LOGW(TAG, "OLED 0x%02X 未响应，检查I2C连线/上拉/地址", OLED_I2C_ADDR);
    }

    // SSD1306 初始化序列（兼容 128x64 常见模块）
    i2c_write_cmd(0xAE);                    // display off
    i2c_write_cmd(0xD5); i2c_write_cmd(0x80); // display clock divide
    i2c_write_cmd(0xA8); i2c_write_cmd(0x3F); // multiplex ratio: 1/64
    i2c_write_cmd(0xD3); i2c_write_cmd(0x00); // display offset
    i2c_write_cmd(0x40);                    // start line = 0
    i2c_write_cmd(0x8D); i2c_write_cmd(0x14); // charge pump enable
    i2c_write_cmd(0x20); i2c_write_cmd(0x00); // memory mode: horizontal addressing
    i2c_write_cmd(0xA1);                    // segment remap (flip X)
    i2c_write_cmd(0xC8);                    // COM scan direction remap (flip Y)
    i2c_write_cmd(0xDA); i2c_write_cmd(0x12); // COM pins hardware config
    i2c_write_cmd(0x81); i2c_write_cmd(0x7F); // contrast
    i2c_write_cmd(0xD9); i2c_write_cmd(0xF1); // pre-charge
    i2c_write_cmd(0xDB); i2c_write_cmd(0x40); // VCOM detect
    i2c_write_cmd(0xA4);                    // resume to RAM content display
    i2c_write_cmd(0xA6);                    // normal display (not inverted)
    i2c_write_cmd(0xAF);                    // display ON

    oled_clear_all();
    ESP_LOGI(TAG, "OLED initialized (SSD1306 @0x3C)");
}

// 更新显示：三行基本文本
void display_update(float temp, float setpoint, float battery) {
    char line[32];
    oled_clear_all();
    snprintf(line, sizeof(line), "Temp: %.1fC", temp);
    draw_text(0, 0, line);
    snprintf(line, sizeof(line), "Set : %.1fC", setpoint);
    draw_text(0, 16, line);
    snprintf(line, sizeof(line), "Batt: %.0f%%", battery);
    draw_text(0, 32, line);
}