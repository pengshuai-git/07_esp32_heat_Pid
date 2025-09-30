# 主应用组件源文件列表（排除 simple_battery_test.c 以避免旧ADC告警）
idf_component_register(SRCS 
    "main.c"
    "display.c"
    "input.c"
    "rgb.c"
    "buzzer.c"
    "uart.c"
    "pid_controller.c"
    "temperature.c"
    "battery_monitor.c"
    INCLUDE_DIRS ".")