#include "web_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <ctype.h>

#include "../Hardware/buzzer.h"
#include "../Hardware/rgb.h"
#include "../Hardware/display.h"
#include "../Hardware/relay.h"
#include "../Hardware/temperature.h"
#include "../Hardware/battery_monitor.h"
#include "pid_controller.h"

// 若未使用 EMBED_TXTFILES，改用 SPIFFS/LittleFS 或内置最小页面
static const char INDEX_FALLBACK[] = "<!doctype html><meta charset=utf-8><title>ESP32</title><p>前端未嵌入，请访问 /api 接口或开启嵌入文件</p>";

static const char *TAG = "WEB";
static httpd_handle_t s_server = NULL;
static PID_t s_pid;
static bool s_pid_running = false;
static TaskHandle_t s_pid_task = NULL;
static float s_pid_last_temp = 0.0f;
static float s_pid_last_output = 0.0f;
static float s_pid_max_temp = 80.0f; // 温度上限，超过则告警（红灯+蜂鸣）

static esp_err_t serve_text(httpd_req_t *req, const char *start, const char *end, const char *ctype){
    httpd_resp_set_type(req, ctype);
    return httpd_resp_send(req, start, end - start);
}

// CORS 支持
static inline void set_cors(httpd_req_t *req){
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    httpd_resp_set_hdr(req, "Connection", "close");
}
static esp_err_t api_options(httpd_req_t *req){ set_cors(req); return httpd_resp_sendstr(req, ""); }

// 静态文件
static esp_err_t on_index(httpd_req_t *req){ return httpd_resp_send(req, INDEX_FALLBACK, HTTPD_RESP_USE_STRLEN); }
static esp_err_t on_css(httpd_req_t *req){ httpd_resp_set_status(req, "404 Not Found"); return httpd_resp_sendstr(req, ""); }
static esp_err_t on_js(httpd_req_t *req){ httpd_resp_set_status(req, "404 Not Found"); return httpd_resp_sendstr(req, ""); }

// 简易JSON读取
static cJSON* read_json(httpd_req_t *req){
    int total = req->content_len;
    if (total < 0) total = 0;
    char *buf = (char*)malloc(total + 1);
    if (!buf) return NULL;
    int received = 0;
    while (received < total) {
        int need = total - received;
        int r = httpd_req_recv(req, buf + received, need);
        if (r == HTTPD_SOCK_ERR_TIMEOUT) continue; // 继续读
        if (r <= 0) { break; }
        received += r;
    }
    buf[received] = 0;
    cJSON *j = received>0 ? cJSON_Parse(buf) : cJSON_CreateObject();
    free(buf);
    if (!j) j = cJSON_CreateObject();
    return j;
}

// /api/beep
static esp_err_t api_beep(httpd_req_t *req){ set_cors(req); ESP_LOGI(TAG, "API /beep"); buzzer_alarm(); httpd_resp_sendstr(req, "{\"ok\":true}"); return ESP_OK; }

// /api/led
static esp_err_t api_led(httpd_req_t *req){
    set_cors(req);
    cJSON *j = read_json(req); if(!j){ httpd_resp_send_500(req); return ESP_FAIL; }
    cJSON *r=cJSON_GetObjectItem(j,"r"), *g=cJSON_GetObjectItem(j,"g");
    // 这里简单切换：若传入 toggle 就在 0/255 间切换；实际可存状态
    static int R=0,G=0; int oldR=R, oldG=G; if(r){ R = R?0:255; } if(g){ G = G?0:255; }
    if (R!=oldR || G!=oldG) {
        ESP_LOGI(TAG, "API /led R=%d G=%d", R, G);
    } else {
        ESP_LOGD(TAG, "API /led unchanged R=%d G=%d", R, G);
    }
    set_rgb(R,G,0);
    cJSON_Delete(j);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// /api/oled
static esp_err_t api_oled(httpd_req_t *req){
    set_cors(req);
    cJSON *j = read_json(req); if(!j){ httpd_resp_send_500(req); return ESP_FAIL; }
    const char *text = cJSON_GetStringValue(cJSON_GetObjectItem(j,"text"));
    if(!text) text = "";
    // 简单兼容：仅显示 ASCII，可见字符转大写，超过宽度自动分行
    char l1[32]={0}, l2[32]={0}, l3[32]={0};
    const int maxw = 21; // 128/6 ≈ 21 字符
    const char *p = text; int line=0, col=0;
    while (*p && line<3) {
        char ch = *p++;
        if (ch=='\r') continue;
        if (ch=='\n') { line++; col=0; continue; }
        if ((unsigned char)ch < 32 || (unsigned char)ch > 126) ch = ' ';
        ch = (char)toupper((unsigned char)ch);
        if (col>=maxw) { line++; col=0; if(line>=3) break; }
        if (line==0) l1[col++] = ch;
        else if (line==1) l2[col++] = ch;
        else l3[col++] = ch;
    }
    ESP_LOGI(TAG, "API /oled len=%d", (int)strlen(text));
    display_show_text(l1, l2, l3);
    cJSON_Delete(j);
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// /api/relay
static esp_err_t api_relay(httpd_req_t *req){
    set_cors(req);
    bool want_toggle = true; // 默认 toggle
    int want_on = -1;        // -1 表示未指定，0/1 表示强制关/开
    bool stop_pid = true;    // 手动控制时默认停止 PID，避免被覆盖

    // 读取 JSON 体
    cJSON *j = read_json(req);
    if (j) {
        if (cJSON_HasObjectItem(j, "toggle")) want_toggle = cJSON_IsTrue(cJSON_GetObjectItem(j, "toggle"));
        if (cJSON_HasObjectItem(j, "on")) want_on = cJSON_IsTrue(cJSON_GetObjectItem(j, "on")) ? 1 : 0;
        if (cJSON_HasObjectItem(j, "off")) want_on = cJSON_IsTrue(cJSON_GetObjectItem(j, "off")) ? 0 : want_on;
        if (cJSON_HasObjectItem(j, "manual")) stop_pid = cJSON_IsTrue(cJSON_GetObjectItem(j, "manual"));
    }
    // 兼容 GET ?on=1/off=1/toggle=1
    int qlen = httpd_req_get_url_query_len(req);
    if (qlen > 0) {
        char *q = (char*)malloc(qlen + 1);
        if (q) {
            if (httpd_req_get_url_query_str(req, q, qlen + 1) == ESP_OK) {
                char val[8];
                if (httpd_query_key_value(q, "on", val, sizeof(val)) == ESP_OK) want_on = atoi(val)!=0 ? 1 : 0;
                if (httpd_query_key_value(q, "off", val, sizeof(val)) == ESP_OK) want_on = atoi(val)!=0 ? 0 : want_on;
                if (httpd_query_key_value(q, "toggle", val, sizeof(val)) == ESP_OK) want_toggle = atoi(val)!=0;
                if (httpd_query_key_value(q, "manual", val, sizeof(val)) == ESP_OK) stop_pid = atoi(val)!=0;
            }
            free(q);
        }
    }

    if (stop_pid && s_pid_running) { s_pid_running = false; ESP_LOGI(TAG, "API /relay: stop PID for manual control"); }

    if (cJSON_HasObjectItem(j, "pwm")) {
        int pct = cJSON_GetObjectItem(j, "pwm")->valueint;
        relay_set_pwm_percent(pct);
    } else if (want_on == 1) { relay_set(true); }
    else if (want_on == 0) { relay_set(false); }
    else if (want_toggle) { relay_toggle(); }

    bool now = relay_get();
    ESP_LOGI(TAG, "API /relay -> %s", now?"ON":"OFF");
    httpd_resp_set_type(req, "application/json");
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"relay\":%s,\"pid_running\":%s}", now?"true":"false", s_pid_running?"true":"false");
    httpd_resp_sendstr(req, buf);
    if (j) cJSON_Delete(j);
    return ESP_OK;
}

// /api/battery
static esp_err_t api_battery(httpd_req_t *req){
    set_cors(req);
    float v = battery_read_voltage();
    float p = battery_voltage_to_percentage(v);
    char buf[64];
    snprintf(buf,sizeof(buf),"{\"voltage\":%.2f,\"percent\":%.0f}", v, p);
    httpd_resp_set_type(req, "application/json");
    static int last_pct = -1;
    int ipct = (int)(p + 0.5f);
    if (ipct != last_pct) {
        ESP_LOGI(TAG, "API /battery voltage=%.2f percent=%d", v, ipct);
        last_pct = ipct;
    } else {
        ESP_LOGD(TAG, "API /battery (unchanged) percent=%d", ipct);
    }
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

// /api/temp
static esp_err_t api_temp(httpd_req_t *req){
    set_cors(req);
    float t = temperature_read();
    char buf[64];
    snprintf(buf,sizeof(buf),"{\"value\":%.2f}", t);
    httpd_resp_set_type(req, "application/json");
    // 不在串口打印温度数据，避免刷屏
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

// ===== PID 控制 =====
static void pid_control_task(void *arg){
    TickType_t windowStart = xTaskGetTickCount();
    const TickType_t windowTicks = pdMS_TO_TICKS(WINDOW_SIZE);
    while (s_pid_running) {
        float current = temperature_read();
        float output = pid_compute(&s_pid, current); // 0~100
        s_pid_last_temp = current;
        s_pid_last_output = output;
        // 直接以 PID 输出映射 PWM 占空（0~100%）
        relay_set_pwm_percent((int)(output + 0.5f));

        // 超温告警：红灯+蜂鸣；未超温：绿灯，蜂鸣关闭（有源蜂鸣，静默即拉低，已在蜂鸣函数内部处理）
        static TickType_t last_beep = 0;
        if (current > s_pid_max_temp) {
            set_rgb(255, 0, 0);
            TickType_t now2 = xTaskGetTickCount();
            if (now2 - last_beep > pdMS_TO_TICKS(1000)) {
                last_beep = now2;
                // 简化处理：阻塞 1s 蜂鸣；如需非阻塞可改为定时器或独立任务
                buzzer_alarm();
            }
        } else {
            set_rgb(0, 255, 0);
        }

        // OLED 显示当前温度/设定与 PID 参数（两行参数避免过长）
        {
            char l1[28], l2[28], l3[28];
            snprintf(l1, sizeof(l1), "T:%.1f S:%.1f Max:%.1f", current, s_pid.setpoint, s_pid_max_temp);
            snprintf(l2, sizeof(l2), "KP:%.2f KI:%.3f", s_pid.Kp, s_pid.Ki);
            snprintf(l3, sizeof(l3), "KD:%.2f OUT:%3.0f%%", s_pid.Kd, output);
            display_show_text(l1, l2, l3);
        }

        ESP_LOGI(TAG, "PID loop: set=%.1f temp=%.1f out=%.0f%% (PWM)", s_pid.setpoint, current, output);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    relay_set(false);
    vTaskDelete(NULL);
}

static esp_err_t api_pid_params(httpd_req_t *req){
    set_cors(req);
    cJSON *j = read_json(req); if(!j){ httpd_resp_send_500(req); return ESP_FAIL; }
    double sp = cJSON_GetObjectItem(j, "setpoint") ? cJSON_GetObjectItem(j, "setpoint")->valuedouble : s_pid.setpoint;
    double kp = cJSON_GetObjectItem(j, "kp") ? cJSON_GetObjectItem(j, "kp")->valuedouble : s_pid.Kp;
    double ki = cJSON_GetObjectItem(j, "ki") ? cJSON_GetObjectItem(j, "ki")->valuedouble : s_pid.Ki;
    double kd = cJSON_GetObjectItem(j, "kd") ? cJSON_GetObjectItem(j, "kd")->valuedouble : s_pid.Kd;
    if (cJSON_HasObjectItem(j, "max")) s_pid_max_temp = (float)cJSON_GetObjectItem(j, "max")->valuedouble;
    s_pid.setpoint = (float)sp; s_pid.Kp=(float)kp; s_pid.Ki=(float)ki; s_pid.Kd=(float)kd;
    // 修改参数时重置积分/误差，避免历史影响
    s_pid.integral = 0.0f; s_pid.last_error = 0.0f; s_pid.last_input = s_pid_last_temp;
    ESP_LOGI(TAG, "API /pid/params sp=%.1f Kp=%.2f Ki=%.3f Kd=%.2f", s_pid.setpoint, s_pid.Kp, s_pid.Ki, s_pid.Kd);
    cJSON_Delete(j);
    httpd_resp_set_type(req, "application/json");
    char buf[160];
    snprintf(buf,sizeof(buf),"{\"ok\":true,\"setpoint\":%.1f,\"kp\":%.2f,\"ki\":%.3f,\"kd\":%.2f,\"max\":%.1f}", s_pid.setpoint, s_pid.Kp, s_pid.Ki, s_pid.Kd, s_pid_max_temp);
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

static esp_err_t api_pid_start(httpd_req_t *req){
    set_cors(req);
    if (!s_pid_running) {
        s_pid_running = true;
        if (s_pid.Kp==0 && s_pid.Ki==0 && s_pid.Kd==0) pid_init(&s_pid, 2.0f, 0.1f, 0.5f, 40.0f);
        xTaskCreate(pid_control_task, "pid_task", 4096, NULL, 5, &s_pid_task);
        ESP_LOGI(TAG, "API /pid/start -> started");
    }
    httpd_resp_set_type(req, "application/json");
    char buf[128];
    snprintf(buf,sizeof(buf),"{\"ok\":true,\"running\":true,\"setpoint\":%.1f,\"kp\":%.2f,\"ki\":%.3f,\"kd\":%.2f}", s_pid.setpoint, s_pid.Kp, s_pid.Ki, s_pid.Kd);
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

static esp_err_t api_pid_stop(httpd_req_t *req){
    set_cors(req);
    if (s_pid_running) { s_pid_running = false; ESP_LOGI(TAG, "API /pid/stop -> stopped"); }
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

static esp_err_t api_pid_status(httpd_req_t *req){
    set_cors(req);
    httpd_resp_set_type(req, "application/json");
    char buf[160];
    // 增加最近一次温度 ADC 原始与等效电压(mV)
    extern int temperature_get_last_raw(void);
    extern int temperature_get_last_mv(void);
    int last_raw = temperature_get_last_raw();
    int last_mv  = temperature_get_last_mv();
    extern int relay_get_pwm_percent(void);
    int pwm = relay_get_pwm_percent();
    snprintf(buf,sizeof(buf),"{\"running\":%s,\"setpoint\":%.1f,\"kp\":%.2f,\"ki\":%.3f,\"kd\":%.2f,\"max\":%.1f,\"temp\":%.2f,\"output\":%.0f,\"adc\":%d,\"mv\":%d,\"pwm\":%d}",
             s_pid_running?"true":"false", s_pid.setpoint, s_pid.Kp, s_pid.Ki, s_pid.Kd, s_pid_max_temp, s_pid_last_temp, s_pid_last_output, last_raw, last_mv, pwm);
    httpd_resp_sendstr(req, buf);
    return ESP_OK;
}

void web_server_start(void){
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    // 默认 max_uri_handlers=8，不足以注册当前所有 API，这里扩大容量
    cfg.max_uri_handlers = 32;
    if (httpd_start(&s_server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed");
        return;
    }
    httpd_uri_t u_index = { .uri="/", .method=HTTP_GET, .handler=on_index };
    httpd_uri_t u_css   = { .uri="/styles.css", .method=HTTP_GET, .handler=on_css };
    httpd_uri_t u_js    = { .uri="/app.js", .method=HTTP_GET, .handler=on_js };
    httpd_uri_t u_beep  = { .uri="/api/beep", .method=HTTP_POST, .handler=api_beep };
    httpd_uri_t u_led   = { .uri="/api/led", .method=HTTP_POST, .handler=api_led };
    httpd_uri_t g_led   = { .uri="/api/led", .method=HTTP_GET,  .handler=api_led };
    httpd_uri_t u_oled  = { .uri="/api/oled", .method=HTTP_POST, .handler=api_oled };
    httpd_uri_t g_oled  = { .uri="/api/oled", .method=HTTP_GET,  .handler=api_oled };
    httpd_uri_t u_relay = { .uri="/api/relay", .method=HTTP_POST, .handler=api_relay };
    httpd_uri_t g_relay = { .uri="/api/relay", .method=HTTP_GET,  .handler=api_relay };
    httpd_uri_t u_batt  = { .uri="/api/battery", .method=HTTP_POST, .handler=api_battery };
    httpd_uri_t u_temp  = { .uri="/api/temp", .method=HTTP_POST, .handler=api_temp };
    httpd_uri_t g_batt  = { .uri="/api/battery", .method=HTTP_GET,  .handler=api_battery };
    httpd_uri_t g_temp  = { .uri="/api/temp",    .method=HTTP_GET,  .handler=api_temp };
    httpd_uri_t u_pidp  = { .uri="/api/pid/params", .method=HTTP_POST, .handler=api_pid_params };
    httpd_uri_t u_pids  = { .uri="/api/pid/start",  .method=HTTP_POST, .handler=api_pid_start };
    httpd_uri_t u_pidx  = { .uri="/api/pid/stop",   .method=HTTP_POST, .handler=api_pid_stop };
    httpd_uri_t g_pids  = { .uri="/api/pid/status", .method=HTTP_GET,  .handler=api_pid_status };
    httpd_uri_t o_pidp  = { .uri="/api/pid/params", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_pids  = { .uri="/api/pid/start",  .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_pidx  = { .uri="/api/pid/stop",   .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_beep  = { .uri="/api/beep", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_led   = { .uri="/api/led", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_oled  = { .uri="/api/oled", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_relay = { .uri="/api/relay", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_batt  = { .uri="/api/battery", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_uri_t o_temp  = { .uri="/api/temp", .method=HTTP_OPTIONS, .handler=api_options };
    httpd_register_uri_handler(s_server, &u_index);
    httpd_register_uri_handler(s_server, &u_css);
    httpd_register_uri_handler(s_server, &u_js);
    httpd_register_uri_handler(s_server, &u_beep);
    httpd_register_uri_handler(s_server, &u_led);
    httpd_register_uri_handler(s_server, &g_led);
    httpd_register_uri_handler(s_server, &u_oled);
    httpd_register_uri_handler(s_server, &g_oled);
    httpd_register_uri_handler(s_server, &u_relay);
    httpd_register_uri_handler(s_server, &g_relay);
    httpd_register_uri_handler(s_server, &u_batt);
    httpd_register_uri_handler(s_server, &u_temp);
    httpd_register_uri_handler(s_server, &g_batt);
    httpd_register_uri_handler(s_server, &g_temp);
    httpd_register_uri_handler(s_server, &u_pidp);
    httpd_register_uri_handler(s_server, &u_pids);
    httpd_register_uri_handler(s_server, &u_pidx);
    httpd_register_uri_handler(s_server, &g_pids);
    httpd_register_uri_handler(s_server, &o_pidp);
    httpd_register_uri_handler(s_server, &o_pids);
    httpd_register_uri_handler(s_server, &o_pidx);
    httpd_register_uri_handler(s_server, &o_beep);
    httpd_register_uri_handler(s_server, &o_led);
    httpd_register_uri_handler(s_server, &o_oled);
    httpd_register_uri_handler(s_server, &o_relay);
    httpd_register_uri_handler(s_server, &o_batt);
    httpd_register_uri_handler(s_server, &o_temp);
    // 若 PID 参数尚未初始化，则给出设备端默认值，供前端首次读取
    if (s_pid.Kp==0 && s_pid.Ki==0 && s_pid.Kd==0 && s_pid.setpoint==0) {
        pid_init(&s_pid, 2.0f, 0.1f, 0.5f, 40.0f);
    }
    ESP_LOGI(TAG, "web server started");
}


