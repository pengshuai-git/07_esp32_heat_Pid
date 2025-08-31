# ESP32 Heat PID 项目

## 项目简介
本项目是基于 ESP32 的智能热量控制系统，使用 PID 控制算法实现精确的温度调节。

## 文件结构
```
├── main/                   # 主程序代码
│   ├── main.c             # 主入口文件
│   ├── pid_controller.c   # PID 控制器实现
│   ├── display.c          # 显示模块
│   ├── uart.c             # UART 通信模块
│   └── ...                # 其他模块
├── build/                  # 构建输出目录（已忽略）
├── sdkconfig               # SDK 配置文件
├── CMakeLists.txt          # CMake 构建配置
├── .gitignore              # Git 忽略规则
└── README.md               # 项目说明文件
```

## 环境配置
1. 安装 [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)。
2. 配置环境变量：
   ```sh
   set IDF_PATH=E:/espressif/v5.0.8/esp-idf
   ```
3. 安装工具链并确保 `riscv32-esp-elf-gcc` 在 PATH 中。

## 构建与烧录
1. 清理构建目录：
   ```sh
   rd /s /q build
   ```
2. 构建项目：
   ```sh
   idf.py build
   ```
3. 烧录固件：
   ```sh
   idf.py -p [PORT] flash
   ```

## 功能模块
- **PID 控制器**：实现温度的精确控制。
- **显示模块**：通过屏幕显示当前温度和设定值。
- **通信模块**：通过 UART 接收和发送数据。

## 贡献
欢迎提交 Issue 和 Pull Request 来改进本项目。

## 许可证
本项目基于 MIT 许可证开源。
