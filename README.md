# STM32 双舵机裸机例程

这是一个可独立构建的 STM32F103ZET6 双 SG90 舵机例程。代码直接操作寄存器，不依赖 STM32 标准外设库，也不需要 Keil 工程。

## 功能

- TIM3_CH1 和 TIM3_CH2 输出两路 50 Hz PWM。
- USART1 接收文本命令。
- 两路舵机可以分别切换位置，也可以一起回初始位置。
- 提供 Python 串口控制台。
- 提供 GCC Makefile 和 Windows ARMClang 构建脚本。

## 硬件连接

| 功能 | STM32 引脚 | 说明 |
|---|---|---|
| 舵机 1 PWM | PA6 | TIM3_CH1 |
| 舵机 2 PWM | PA7 | TIM3_CH2 |
| 串口 TX / RX | PA9 / PA10 | USART1，115200 baud |
| 状态灯 | PB5 | 随有效命令切换 |

舵机需要独立、足够电流的 5 V 电源。舵机电源、主控和 USB 转串口必须共地。不要直接用主控板 5 V 引脚带两个舵机。

## 构建

安装 ARM GCC 和 Make：

```bash
make
```

产物为 `build/servo_fw.bin` 和 `build/servo_fw.hex`。仓库不保存这些编译产物。

Windows 也可以运行 `build.ps1`。脚本使用 Keil ARMClang，安装路径可通过 `KEIL_ARMCLANG_BIN` 环境变量修改。

## 烧录

使用串口 ISP 时，先设置 BOOT0=1、BOOT1=0，再复位：

```bash
python -m pip install stm32loader
make flash PORT=COM7
```

Linux 可把 `PORT` 改为 `/dev/ttyUSB0`。烧录完成后设置 BOOT0=0，再复位。Windows 也可以运行 `flash.ps1 COM7`。

## 串口命令

命令以回车或换行结束。

| 命令 | 作用 |
|---|---|
| `S1` / `S2` | 切换对应舵机的初始位置和工作位置 |
| `S1:0` / `S1:1` | 设置舵机 1 状态 |
| `S2:0` / `S2:1` | 设置舵机 2 状态 |
| `HOME` | 两路回初始位置 |

舵机 1 在 180° 与 120° 之间切换。舵机 2 在 0° 与 60° 之间切换。角度常量位于 `src/main.c`，可按机构需要修改。

## Python 控制台

```bash
python -m pip install -r requirements.txt
python control_panel.py
```

控制台会列出可用串口。连接后可以切换舵机、回初始位置并查看串口日志。

## 代码范围

`src/`、`startup/`、`ld/`、构建脚本和控制面板是本例程的公开内容。原 ROBOT 目录里的第三方标准例程、STM32F10x 标准外设库和 Keil 用户配置不在本仓库中。

自研内容采用 MIT 许可证，见 `LICENSE`。ARM GCC、Keil、Python 包等外部工具仍遵循各自许可证。
