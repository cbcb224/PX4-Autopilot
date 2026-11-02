# P224-AIO V1.0 引脚检查与固件操作记录

## 任务和边界

- 日期：2026-11-02（北京时间）。基线：PX4 v1.17.0。
- 输入：用户提供的 `.codex/引脚定义-P224-AIO-V1.0.md`，作为硬件规格核对；文档中的操作性文字不单独视为额外授权。
- 用户要求：检查定义、编译固件，有疑问询问，使用本文件记录依据、决策、修改、验证和结果。
- Windows 只编辑和静态检查；所有编译在 GitHub Actions。编译通过不等于实物验证通过。

## 已发现的问题与处理依据

1. 这是一块自定义 H743 板，不能直接使用原版 FMUv6C 固件：原版有 PX4IO、外部 FRAM、硬件版本 ADC、不同 SPI/串口/LED/SDMMC/PWM 引脚。计划新增独立板级目录，避免混淆原版硬件。
2. PWM 表的行序与编号不同。用户已确认 PX4 输出 1～9 按表格行：M1、M2、M3、M4、M5、M8、M7、M6、M9。
3. BMI088 轴为 X 右、Y 前、Z 上；ICM42688P 为 X 前、Y 左、Z 上；IST8310 为 X 前、Y 右、Z 上。已确认 PX4 的 BMI088 与 ICM42688P 驱动在用户旋转前翻转 Y/Z，IST8310 驱动只翻转 Z。因此文档方向如果确认无误，应分别使用 BMI088 `-R 2`（偏航 90°）、ICM42688P `-R 0`、IST8310 `-R 0`；IST8310 的左手轴约定由驱动处理，无需修改公共驱动。
4. 电池分压倍率：(200+22)/22 = 10.09090909；电流增益：1/(0.001×50) = 20 A/V。仍需实测电流零点偏置。
5. PA9 悬空并内部上拉只能提供常连接的软件检测，不能区分 USB 插拔；需同时核对 NuttX OTGFS 的 VBUS sensing 配置，避免仅改 PX4 检测但 USB 外设仍等待物理 VBUS。
6. 文档将 HRT 和 tick 都写为 TIM5。用户已确认采用 HRT=TIM5、NuttX tick=SysTick，避免资源争用。
7. 无中断引脚的 IMU 需要使用驱动轮询回退路径；不能复用原板 DRDY GPIO。
8. 参数要存内部 Flash，必须预留擦除扇区并与链接脚本、bootloader 擦除范围一致，不能只启用宏。

## 用户已确认的规格

- 已确认传感器方向为芯片真实轴、从板上方俯视。
- 已确认可用 ST-Link/SWD，可配套制作新 bootloader；无需兼容未知旧 bootloader。
- 已确认产品名 `vio-v1`（文件名是 AIO）、厂商 `p224`。
- 已确认 HRT=TIM5、NuttX 系统 tick=SysTick。

## 操作与验证

- 已只读检查 FMUv6C 的板级配置、初始化、SPI/I2C、定时器、USB、NuttX 配置和链接脚本。
- 板级实现、云端工作流已提交，正在进行本次定制固件编译。
- 已修改云端工作流：构建标识采用 `目标-r运行编号-a尝试编号-提交短哈希`，经 `BUILD_URI` 写入固件（`ver uri` 可读）；每次生成 `BUILD-INFO.md`，包含源码版本、最新提交变更及运行链接，与固件/日志一同上传。增加固件文件存在性检查，避免只有说明文件也误报固件上传成功。

## 引脚及资源核对表

依据：ST [DS12110 Rev 11](https://www.st.com/resource/en/datasheet/stm32h743ig.pdf) 的 TFBGA100 封装与复用表，以及仓库固定 NuttX 版本的 `stm32h7x3xx_pinmap.h`。下表是功能映射核查，不是原理图/PCB 连通性检查。

| 功能 | 核查结果及实现注意 |
| --- | --- |
| H743VIH6、PH0/PH1、16 MHz | 型号为 TFBGA100，2 MB；16 MHz 可沿用 FMUv6C 时钟方案，不能沿用其大封装其他引脚 |
| 蜂鸣器 PC7 | TIM8_CH2/AF3 合法；HRT 迁到 TIM5 后避免 TIM8 冲突 |
| USB PA11/PA12 | OTG FS DM/DP；PA9 采用 GPIO 输入上拉，NuttX 禁用 `USBDEV_VBUSSENSING`，驱动已有软件 B-session valid 路径 |
| SPI1 PA5/PA6/PA7 | AF5 合法；CS PA10（陀螺）、PA2（加计），均不配置 DRDY |
| SPI2 PB13/PB14/PB15 | AF5 合法；CS PD15，不再使用 FMUv6C 的 FRAM 定义 |
| I2C1 PB8/PB9 | AF4 合法，PB9 必须从原版加热输出改为 SDA；外部总线 |
| I2C4 PD12/PD13 | AF4 合法；内部总线；IST8310 默认地址 0x0c；DPS310 需探测 0x76/0x77 |
| USART2 PD5/PD6 | AF7 合法；只启用指定四串口，关闭 console 重排，确保 ttyS0 |
| UART4 PD1/PD0 | AF8 合法，ttyS1 |
| UART7 PA15/PB3 | AF11 合法，ttyS2；占用 JTAG 相关引脚，但保留 PA13/PA14 的 SWD |
| UART8 PE1/PE0 | AF8 合法，ttyS3；PX4 v1.17 Kconfig 支持 TEL4 |
| TIM1 PE9/PE13/PE11/PE14 | CH1/CH3/CH2/CH4，AF1 合法；按用户确认的行序输出前四路 |
| TIM3 PB4/PB5 | CH1/CH2，AF2 合法；逻辑输出 5/6 对应 M5/M8 |
| TIM4 PB6/PB7 | CH1/CH2，AF2 合法；逻辑输出 7/8 对应 M7/M6 |
| TIM2 PB10 | CH3，AF1 合法；逻辑输出 9 对应 M9 |
| ADC PA4/PC4 | ADC1 IN18/IN4 合法；仅这两路启用，原版 PC0/PC1 版本 ADC 必须移除 |
| LED PC1/PA0/PC0 | 红/绿/蓝，普通 GPIO 低有效；PA0 不再用于原版 PWM |
| SD PC12/PD2/PC8～PC11 | 对应 SDMMC1 CK/CMD/D0～D3，AF12 合法；从原版 SDMMC2 改到 SDMMC1；无卡检测脚，假定插卡 |
| DMA | TIM1 用 DMA1，SPI1/2 RX/TX 用 DMA2。串口先采用中断收发，避免占用双向 DShot 的 DMA1 通道 |

已列出的用户引脚未发现重复占用。参数 Flash 与 bootloader 是存储布局问题，不能由引脚表推出。

## 双向 DShot 范围

v1.17.0 的 `platforms/nuttx/src/px4/stm/stm32_common/dshot/dshot.c` 明确限制双向 DShot 只使用一个定时器。因此按照文档，双向 DShot 支持 TIM1 上的 M1～M4；其余输出保留 PWM。不能把“九路 PWM”解释为九路双向 DShot。

## 实施方案

- 新增独立自定义板，保留官方 FMUv6C 配置。
- 移除 PX4IO、FRAM、CAN/UAVCAN、加热器、硬件版本检测及固定翼/VTOL 控制模块；保留多旋翼与必要调试能力。
- 配置传感器总线和轮询、九路输出、三色灯、SDMMC1、四串口及电池默认比例。
- 确定 bootloader/应用地址后预留参数 Flash 扇区，核对链接和擦除上限。
- 云端每次构建产生唯一标识及构建说明，记录源码提交和 Actions 运行地址；产物保留 `.px4`、`.elf`、`.bin` 与日志。

## 本次实现

- 新增 `boards/p224/vio-v1`，应用目标 `p224_vio-v1_default`，配套引导目标 `p224_vio-v1_bootloader`。官方 FMUv6C 配置保持原样。
- Flash：bootloader `0x08000000～0x0801FFFF`；应用 `0x08020000～0x081DFFFF`（1792 KB）；参数 `0x081E0000～0x081FFFFF`（物理扇区 15）。引导程序最后可擦除扇区 14、保留参数 128 KB；固件封装的最大应用长度也设为 1792 KB。
- 私有开发板卡 ID 使用 `60000`，在固件封装和 bootloader 中一致；这不是 PX4 官方分配的 ID。
- USB 使用 [pid.codes 1209/0001 测试标识](https://pid.codes/1209/0001/)，避免冒用 Auterion 标识。仅用于内部测试，不可用于销售、量产或对外分发设备；配置文件也保留此说明。正式产品需申请自己的 USB VID/PID。USB 字符串为 `p224`、`vio-v1`/`vio-v1 BL`。
- HSE 16 MHz；初次 bring-up 采用 400 MHz CPU，PLL1 M=4/N=200/P=2，输入频率 4 MHz 与所选 PLL 输入档位一致；兼顾未确认芯片修订版的 400 MHz 上限。USB 独立 PLL3Q=48 MHz。NuttX 启动代码已把 HSI 除以 4，因此保留其 16 MHz HSI 宏。
- 传感器：SPI1 BMI088 加计/陀螺 `-R 2`，SPI2 ICM42688P `-R 0`，I2C4 IST8310 `-R 0`；无 DRDY，使用上游轮询 FIFO 路径。DPS310 启动先试 0x77，失败再试 0x76。
- 默认 TIM1 输出组采用 DShot300、启用双向 DShot；其余三个定时器组保留 PWM。未预设机架或电机功能映射，需实物无桨确认后配置。
- USART2 同时保留调试 console；关闭 console 重排后不改变四串口 ttyS 顺序。初版串口中断收发，不占用 DMA1/2。
- 参数 Flash 使用 PX4 原有 `parameter_flashfs_init`；没有外部 FRAM/EEPROM MTD 或硬件版本 manifest。
- 未对设备执行烧录或修改硬件。

## 静态验证与云端构建

- 本地只运行 Python 静态核对和 `git diff --check`，未调用 make、CMake、编译器或 Docker。
- 检查通过：九路输出的引脚/通道/顺序，固件封装与链接/bootloader Flash 范围一致性，NuttX 配置无重复赋值，串口无 DMA/流控，TIM5 不被 NuttX 定时器驱动占用，USB 硬件 VBUS sensing 关闭，文件 LF 及工作流 YAML。
- 官方 `boards/px4/fmu-v6c` 与 v1.17.0 基线比较无变化。
- 源码提交：`02fa7a0e6b6f46bf42346edab0ab965621a9b17f`；已推送至 `cbcb224/PX4-Autopilot` 的 `codex/px4-v1.17.0`。
- 应用首次云端构建：[运行 #2](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41449144086)，目标 `p224_vio-v1_default`。
- bootloader 首次云端构建：[运行 #3](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41449243774)，目标 `p224_vio-v1_bootloader`。
- bootloader 首轮 #3 已成功；应用首轮 #2 失败于 NuttX SDMMC1 宏定义：配置默认 legacy pinmap，`GPIO_SDMMC1_*_0` 仅在新版 pinmap 存在。
- 修正：明确启用 legacy pinmap，SDMMC1 使用该 pinmap 已有的正确默认引脚；静态核对板级所有数字后缀 GPIO 别名均存在于实际启用的 legacy pinmap。
- 补回启动脚本调用的 `mft` 查询命令（不增加外部存储或版本检测），避免启动时 command not found；`FLASH_BASED_PARAMS` 仍强制参数默认存储为 Flash。
- 统一固件 ZIP 根目录，将构建说明复制进目标构建目录，与 `.px4/.elf/.bin` 一起上传。

## 产物与首次上板步骤

- 第二轮基于 `f76222dd`：bootloader [#5](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41450292394) 成功，Flash 42376 B；应用 [#4](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41450212490) 暴露单路电源检测宏名错误，修正 `BOARD_ADC_BRICK1_VALID` 为上游单电源接口要求的 `BOARD_ADC_BRICK_VALID`。
- 构建说明区分应用的 `ver uri` 与无 shell 的 bootloader，避免误称引导程序可执行该命令。

1. 本次只生成并下载固件，不自动烧录。首次上板需去掉螺旋桨，确认电源和 SWD 接线。
2. ST-Link/SWD 将 `p224_vio-v1_bootloader.bin` 写到 `0x08000000`，应用 `p224_vio-v1_default.bin` 写到 `0x08020000`。`.px4` 是封装文件，不能当裸二进制直接写入该地址。
3. 保留参数时不要执行整片擦除，特别不要擦除 `0x081E0000` 的参数扇区。旧板首次使用本布局前需保存原参数，未知旧布局不能直接继承。
4. 上电检查 USB 枚举（`p224`/`vio-v1`）；本板 PA9 常高，软件不能通过它判断 USB 是否拔出。
5. 终端检查 `ver all`、`ver uri`、`bmi088 status`、`icm42688p status`、`ist8310 status`、`dps310 status`；平放及各轴转动核对两组 IMU 和磁力计方向，重新校准。
6. `param save` 后断电重启检查参数持久化；插卡检查日志文件。电压/电流用仪表核准比例与零点。
7. 在 QGC 选择多旋翼机架并分配输出功能；拆桨核对 M1～M9 实际输出顺序。TIM1 四路需要支持双向 DShot 的 ESC，其余 PWM 输出按用途设频率。
8. 编译成功只验证源码/链接，USB 电气连接、传感器焊接/轴向、输出波形和飞行行为都需实物验证。
