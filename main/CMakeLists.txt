# 主应用组件源文件列表（排除 simple_battery_test.c 以避免旧ADC告警）
idf_component_register(SRCS 
    "main.c"
    "pid_controller.c"
    "web_server.c"
    "../Hardware/display.c"
    "../Hardware/key.c"
    "../Hardware/rgb.c"
    "../Hardware/buzzer.c"
    "../Hardware/uart.c"
    "../Hardware/adc_shared.c"
    "../Hardware/relay.c"
    "../Hardware/temperature.c"
    "../Hardware/battery_monitor.c"
    "../Test/hardware_test.c"
    INCLUDE_DIRS "." "../Hardware" "../Test")