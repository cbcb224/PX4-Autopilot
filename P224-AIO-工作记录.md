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
| I2C4 PD12/PD13 | AF4 合法；内部总线；IST8310 默认地址 0x0e（首次上板修正）；DPS310 需探测 0x76/0x77 |
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

### 最终构建与下载核验（2026-11-02）

- 构建源码：`fa4fc3170ab924b8fe6abc91da56884758b22c71`，基于 PX4 v1.17.0；随后仅更新本记录，不改变固件源码。
- 应用 [Actions #8：成功](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41451943797)，目标 `p224_vio-v1_default`，Flash 1482896 B / 1792 KB（80.81%）。唯一构建标识 `p224_vio-v1_default-r8-a1-fa4fc317` 已在下载的裸二进制中检出。
- 引导程序 [Actions #9：成功](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41451992973)，目标 `p224_vio-v1_bootloader`，二进制 42376 B / 128 KB；构建标识在随包 BUILD-INFO.md 中记录。
- 最终文件位于工程目录 `P224-VIO-v1-固件/application/` 和 `P224-VIO-v1-固件/bootloader-final/`，各含 `.bin`、`.elf`、`.px4` 和云端 BUILD-INFO.md。较早下载的 `bootloader/` 与 `p224-bootloader-f76222dd.zip` 仅为历史产物，首次上板使用 `bootloader-final/`。
- ZIP SHA256 与 GitHub artifact digest 一致：应用 `e5d982aff7ea7da5e0409ec1218c6d4acac4b1a9f8e3184b89c0116271bdd412`；引导 `93253094dd7ed4ccf93dccc817f7eb47ca725fa02a1950c7fc910a4e05370756`。
- 裸二进制 SHA256：应用 `a2d2546c1a9d67883e403da44e09ef23d9f87350415eb02b6098acacf0b6e9d6`；引导 `9e937b212393895a071c9f8e1b35547221f3a6778082b0cbb07962b8ef599553`。
- 下载后验证通过：两包 board_id=60000，解压 `.px4` image 与 `.bin` 一致，应用/引导 reset vector 分别落入 `0x08020000` / `0x08000000` 对应镜像范围，BUILD-INFO 源码提交相符。
- 编译日志没有编译/链接错误；有上游 Kconfig 的 ARCH_CHIP_UNSET 被实际板型覆盖提示及 Actions Node 弃用提示。构建成功不代替实物验收。
- 全程未在本机编译，也未烧录设备。源码和本记录已推送；下载的二进制保留本地，不提交到源码仓库。

- 第二轮基于 `f76222dd`：bootloader [#5](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41450292394) 成功，Flash 42376 B；应用 [#4](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41450212490) 暴露单路电源检测宏名错误，修正 `BOARD_ADC_BRICK1_VALID` 为上游单电源接口要求的 `BOARD_ADC_BRICK_VALID`。
- 构建说明区分应用的 `ver uri` 与无 shell 的 bootloader，避免误称引导程序可执行该命令。
- 第三轮：同提交 `35561218` 的 bootloader [#7](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41451176355) 成功；应用 [#6](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41451131914) 继续发现同组单电源 ADC 通道宏仍沿用双路命名。完整核对 `board_common.h` 的单电源分支后，把电压/电流通道一起改为 `ADC_BATTERY_VOLTAGE_CHANNEL` / `ADC_BATTERY_CURRENT_CHANNEL`，通道值仍是 PA4/18、PC4/4。

1. 本次只生成并下载固件，不自动烧录。首次上板需去掉螺旋桨，确认电源和 SWD 接线。
2. ST-Link/SWD 将 `p224_vio-v1_bootloader.bin` 写到 `0x08000000`，应用 `p224_vio-v1_default.bin` 写到 `0x08020000`。`.px4` 是封装文件，不能当裸二进制直接写入该地址。
3. 保留参数时不要执行整片擦除，特别不要擦除 `0x081E0000` 的参数扇区。旧板首次使用本布局前需保存原参数，未知旧布局不能直接继承。
4. 上电检查 USB 枚举（`p224`/`vio-v1`）；本板 PA9 常高，软件不能通过它判断 USB 是否拔出。
5. 终端检查 `ver all`、`ver uri`、`bmi088 status`、`icm42688p status`、`ist8310 status`、`dps310 status`；平放及各轴转动核对两组 IMU 和磁力计方向，重新校准。
6. `param save` 后断电重启检查参数持久化；插卡检查日志文件。电压/电流用仪表核准比例与零点。
7. 在 QGC 选择多旋翼机架并分配输出功能；拆桨核对 M1～M9 实际输出顺序。TIM1 四路需要支持双向 DShot 的 ESC，其余 PWM 输出按用途设频率。
8. 编译成功只验证源码/链接，USB 电气连接、传感器焊接/轴向、输出波形和飞行行为都需实物验证。


## COM53 首次上板只读诊断（2026-11-02）

USB VID:PID=1209:0001；已收到 PX4 MAVLink 心跳，未解锁，未修改参数或触发输出。以下为命令响应。

```text

### ver all

NuttShell (NSH) NuttX-12.12.0
nsh>
nsh> ver all
HW arch: P224_VIO_V1
PX4 git-hash: fa4fc3170ab924b8fe6abc91da56884758b22c71
PX4 version: Release 0.0.0 (255)
PX4 git-branch: codex/px4-v1.17.0
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: fb2fadf6f599c1406f052db013efd00a2518e72c
Build datetime: Nov  2 2026 13:22:53
Build uri: p224_vio-v1_default-r8-a1-fa4fc317
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
nsh>
### ver uri
ver uri
Build uri: p224_vio-v1_default-r8-a1-fa4fc317
nsh>
### uptime
uptime
nsh: uptime: command not found
nsh>
### commander status
commander status
INFO  [commander] Disarmed
INFO  [commander] navigation mode: Hold
INFO  [commander] user intended navigation mode: Hold
INFO  [commander] in failsafe: no
commander: cycle: 5864 events, 382411us elapsed, 65.21us avg, min 37us max 2412us 74.439us rms
commander: preflight check: 597 events, 93664us elapsed, 156.89us avg, min 78us max 2369us 149.939us rms
nsh>
### bmi088 status
bmi088 status
Usage: bmi088 <command> [arguments...]
 Commands:

   start
     [-A]        Accel
     [-G]        Gyro
     [-s]        Internal SPI bus(es)
     [-S]        External SPI bus
     [-b <val>]  board-specific bus (default=all) (external SPI: n-th bus (default=1))
     [-c <val>]  chip-select pin (for internal SPI) or index (for external SPI)
     [-m <val>]  SPI mode
     [-f <val>]  bus frequency in kHz
     [-q]        quiet startup (no message if no device found)
     [-R <val>]  Rotation
                 default: 0

   stop

   status        print status info
nsh>
### icm42688p status
icm42688p status
INFO  [SPI_I2C] Running on SPI Bus 2
INFO  [icm42688p] FIFO empty interval: 1250 us (800.0 Hz)
INFO  [icm42688p] Clock input: disabled
icm42688p: bad register: 0 events
icm42688p: bad transfer: 0 events
icm42688p: FIFO empty: 0 events
icm42688p: FIFO overflow: 0 events
icm42688p: FIFO reset: 1 events
nsh>
### ist8310 status
ist8310 status
INFO  [SPI_I2C] Not running
nsh>
### dps310 status
dps310 status
INFO  [SPI_I2C] Running on I2C Bus 4, Address 0x76
dps310: read: 4677 events, 3062383us elapsed, 654.78us avg, min 402us max 1329us 391.100us rms
dps310: comm errors: 0 events
nsh>
### sensors status
sensors status
selected gyro: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -0.0009, lp:  -0.0009 mean dev:   0.0001 RMS:   0.0047 conf:   1.0000
	val:   0.0017, lp:   0.0059 mean dev:   0.0000 RMS:   0.0036 conf:   1.0000
	val:   0.0038, lp:   0.0005 mean dev:   0.0001 RMS:   0.0037 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:  -0.0005, lp:  -0.0001 mean dev:   0.0001 RMS:   0.0016 conf:   1.0000
	val:   0.0008, lp:   0.0010 mean dev:   0.0000 RMS:   0.0035 conf:   1.0000
	val:   0.0009, lp:   0.0004 mean dev:  -0.0001 RMS:   0.0019 conf:   1.0000

selected accel: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -0.1443, lp:  -0.1411 mean dev:  -0.0005 RMS:   0.0237 conf:   1.0000
	val:  -0.2181, lp:  -0.2166 mean dev:  -0.0008 RMS:   0.0351 conf:   1.0000
	val:  -9.7885, lp:  -9.8312 mean dev:  -0.0460 RMS:   0.3336 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:  -0.1199, lp:  -0.1200 mean dev:  -0.0003 RMS:   0.0143 conf:   1.0000
	val:  -0.3008, lp:  -0.2970 mean dev:  -0.0008 RMS:   0.0285 conf:   1.0000
	val:  -9.5268, lp:  -9.5248 mean dev:  -0.0323 RMS:   0.2840 conf:   1.0000

validator: best: -1, prev best: -1, failsafe: NO (0 events)

[vehicle_air_data] selected BARO: 6845985 (0)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val: 102954.0703, lp: 102957.8828 mean dev: -32.2119 RMS:  78.3588 conf:   1.0000
	val:  64.5542, lp:  64.4842 mean dev:  -3.1105 RMS:   7.6728 conf:   1.0000
	val: -134.7530, lp: -135.0614 mean dev:   2.6359 RMS:   6.4295 conf:   1.0000
BARO 6845985 EN: 1, offset: 0.000, Internal

Airspeed status:
	no data

[vehicle_acceleration] selected sensor: 2490386, rate: 799.5 Hz, estimated bias: [0.0035 0.0082 0.2838]
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal

[vehicle_angular_velocity] selected sensor: 2490386, rate: 8043.6 Hz FIFO, estimated bias: [-0.00008 0.00098 0.00033]
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
vehicle_angular_velocity: gyro filter: 60044 events, 926161us elapsed, 15.42us avg, min 9us max 203us 1.235us rms
vehicle_angular_velocity: gyro filter reset: 3 events
vehicle_angular_velocity: gyro selection changed: 3 events

[vehicle_gps_position] selected GPS: 0


[vehicle_imu] 0 - Accel: 6946826, interval: 1250.0 us (SD 97.1 us), Gyro: 6684682, interval: 1500.0 us (SD 127.6 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 6946826 EN: 1, offset: [-0.003 0.014 0.054], scale: [1.000 1.000 1.000], Internal
GYRO 6684682 EN: 1, offset: [0.003 -0.002 -0.002], Internal

[vehicle_imu] 1 - Accel: 2490386, interval: 1250.0 us (SD 30.9 us), Gyro: 2490386, interval: 1250.0 us (SD 30.9 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
nsh>
### ls /fs/microsd
ls /fs/microsd
/fs/microsd:
 param_import_fail.txt
 dataman
 log/
 ufw/
 parameters_backup.bson
 uavcan.db/
 mavlink/
 params
nsh>
### dmesg
dmesg
ut
ERROR [board_adc] sample timeout
[上述同一错误连续重复 124 行，合并显示]
Ensh>
```

```text

### bmi088 -A status

NuttShell (NSH) NuttX-12.12.0
nsh> bmi088 -A status
INFO  [SPI_I2C] Running on SPI Bus 1
INFO  [bmi088] FIFO empty interval: 1250 us (800.0 Hz)
bmi088_accel: bad register: 0 events
bmi088_accel: bad transfer: 0 events
bmi088_accel: FIFO empty: 0 events
bmi088_accel: FIFO overflow: 0 events
bmi088_accel: FIFO reset: 1 events
nsh>
### bmi088 -G status
bmi088 -G status
INFO  [SPI_I2C] Running on SPI Bus 1
INFO  [bmi088] FIFO empty interval: 1500 us (666.7 Hz)
bmi088_gyro: bad register: 0 events
bmi088_gyro: bad transfer: 0 events
bmi088_gyro: FIFO empty: 0 events
bmi088_gyro: FIFO overflow: 0 events
bmi088_gyro: FIFO reset: 1 events
nsh>
### board_adc status
board_adc status
INFO  [board_adc] running
nsh>
### listener adc_report -n 1
listener adc_report -n 1

TOPIC: adc_report
 adc_report
    timestamp: 113587794 (0.001948 seconds ago)
    device_id: 4294967295 (Type: 0xFF, UNKNOWN:31 (0xFF))
    raw_data: [0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    resolution: 65536
    v_ref: 3.30000
    channel_id: [4, 18, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1]

nsh>
### listener sensor_mag -n 1
listener sensor_mag -n 1
never published
nsh>
### param show SYS_AUTOSTART
param show SYS_AUTOSTART
Symbols: x = used, + = saved, * = unsaved
x + SYS_AUTOSTART [883,1245] : 4001

 920/1537 parameters used.
nsh>
### param show CAL_MAG*
param show CAL_MAG*
Symbols: x = used, + = saved, * = unsaved
x + CAL_MAG0_ID [56,122] : 396833
x   CAL_MAG0_PITCH [57,123] : 0.0000
x + CAL_MAG0_PRIO [58,124] : 50
x   CAL_MAG0_ROLL [59,125] : 0.0000
x   CAL_MAG0_ROT [60,126] : -1
x   CAL_MAG0_XCOMP [61,127] : 0.0000
x   CAL_MAG0_XODIAG [62,128] : 0.0000
x   CAL_MAG0_XOFF [63,129] : -0.1947
x   CAL_MAG0_XSCALE [64,130] : 1.0000
x   CAL_MAG0_YAW [65,131] : 0.0000
x   CAL_MAG0_YCOMP [66,132] : 0.0000
x   CAL_MAG0_YODIAG [67,133] : 0.0000
x   CAL_MAG0_YOFF [68,134] : -0.1922
x   CAL_MAG0_YSCALE [69,135] : 1.0000
x   CAL_MAG0_ZCOMP [70,136] : 0.0000
x   CAL_MAG0_ZODIAG [71,137] : 0.0000
x   CAL_MAG0_ZOFF [72,138] : -0.1665
x   CAL_MAG0_ZSCALE [73,139] : 1.0000
x   CAL_MAG1_ID [74,140] : 0
x   CAL_MAG1_ROT [75,144] : -1
x   CAL_MAG2_ID [76,158] : 0
x   CAL_MAG2_ROT [77,162] : -1
x   CAL_MAG3_ID [78,176] : 0
x   CAL_MAG3_ROT [79,180] : -1
x   CAL_MAG_COMP_TYP [80,194] : 0
x   CAL_MAG_SIDES [81,195] : 63

 920/1537 parameters used.
nsh>
### cat /fs/microsd/param_import_fail.txt
cat /fs/microsd/param_import_fail.txt
HW arch: MICOAIR_H743_AIO
PX4 git-hash: 54f0455ffcd755534539a7cf33a09a20bf71d29d
PX4 version: Release 1.16.2 (17826559)
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: 886acbbdb4f061e5c0ce1a76afbcfa7cb7df9849
Build datetime: Oct 30 2026 19:50:27
Build uri: localhost
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
New /fs/mtd_caldata size is:
ERROR [param] importing failed (-1)
ERROR [init] param import failed
INFO  [parameters] summary: 0/1825 (used/total)
INFO  [parameters] storage array: 0/1825 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 0/1825 elements (256 bytes total)
INFO  [parameters] auto save: on
param: export: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
param: find: 0 events
param: get: 0 events
param: set: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
dmesg [30:100]
INFO  [param] selected parameter backup file /fs/microsd/parameters_backup.bson
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
Loading airframe: /etc/init.d/airframes/4001_quad_x
HW arch: P224_P224_AIO_V1
PX4 git-hash: 527dfc114fc21b18c988a7c520534fb25e5470c0
PX4 version: 1.16.2 0 (17826304)
PX4 git-branch: codex/px4-1.16.2-custom
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: 886acbbdb4f061e5c0ce1a76afbcfa7cb7df9849
Build datetime: Nov  2 2026 04:06:17
Build uri: localhost
Build variant: default
Toolchain: GNU GCC, 9.3.1 20200408 (release)
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
New /fs/mtd_caldata size is:
ERROR [param] importing failed (-1)
ERROR [init] param import failed
[init] importing from parameter backup
INFO  [param] importing from '/fs/microsd/parameters_backup.bson'
INFO  [parameters] BSON document size 5 bytes, decoded 5 bytes (INT32:0, FLOAT:0)
INFO  [parameters] summary: 0/1823 (used/total)
INFO  [parameters] storage array: 0/1823 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 0/1823 elements (256 bytes total)
INFO  [parameters] auto save: on
param: export: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
param: find: 0 events
param: get: 0 events
param: set: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
dmesg [31:100]
INFO  [param] selected parameter backup file /fs/microsd/parameters_backup.bson
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
Loading airframe: /etc/init.d/airframes/4001_quad_x
HW arch: P224_P224_AIO_V1
PX4 git-hash: 527dfc114fc21b18c988a7c520534fb25e5470c0
PX4 version: 1.16.2 0 (17826304)
PX4 git-branch: codex/px4-1.16.2-custom
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: 886acbbdb4f061e5c0ce1a76afbcfa7cb7df9849
Build datetime: Nov  2 2026 04:06:17
Build uri: localhost
Build variant: default
Toolchain: GNU GCC, 9.3.1 20200408 (release)
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
New /fs/mtd_caldata size is:
ERROR [param] importing failed (-1)
ERROR [init] param import failed
[init] importing from parameter backup
INFO  [param] importing from '/fs/microsd/parameters_backup.bson'
INFO  [parameters] BSON document size 5 bytes, decoded 5 bytes (INT32:0, FLOAT:0)
INFO  [parameters] summary: 0/1823 (used/total)
INFO  [parameters] storage array: 0/1823 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 0/1823 elements (256 bytes total)
INFO  [parameters] auto save: on
param: export: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
param: find: 0 events
param: get: 0 events
param: set: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
dmesg [31:100]
INFO  [param] selected parameter backup file /fs/microsd/parameters_backup.bson
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
HW arch: P224_VIO_V10
PX4 git-hash: d6f12ad1c4f70ad3230afd7d86e971421e02fef4
PX4 vnsh: cat: read failed: Invalid argument
nsh>
### param status
param status
INFO  [parameters] summary: 920/1537 (used/total)
INFO  [parameters] storage array: 30/1537 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 24/1537 elements (256 bytes total)
INFO  [parameters] auto save: on
INFO  [parameters] last auto save: 120.746 seconds ago
param: export: 1 events, 218us elapsed, 218.00us avg, min 218us max 218us   nanus rms
param: find: 16037 events
param: get: 748238 events
param: set: 30 events, 247us elapsed, 8.23us avg, min 6us max 11us 1.382us rms
nsh>
### logger status
logger status
INFO  [logger] Running in mode: all
INFO  [logger] Number of subscriptions: 178 (5696 bytes)
INFO  [logger] Not logging
nsh>
```

临时使用上游正确默认地址 0x0e 探测并启动磁力计；不修改持久参数，重启后仍需修正固件启动脚本。

```text

### ist8310 -I -b 4 -a 0x0e -R 0 start

NuttShell (NSH) NuttX-12.12.0
nsh> ist8310 -I -b 4 -a 0x0e -R 0 start
ist8310 #0 on I2C bus 4 address 0xE
nsh>
### ist8310 status
ist8310 status
INFO  [SPI_I2C] Running on I2C Bus 4, Address 0x0E
ist8310: reset: 1 events
ist8310: bad register: 0 events
ist8310: bad transfer: 0 events
nsh>
### listener sensor_mag -n 1
listener sensor_mag -n 1

TOPIC: sensor_mag
 sensor_mag
    timestamp: 176857428 (0.004072 seconds ago)
    timestamp_sample: 176856480 (948 us before timestamp)
    device_id: 396833 (Type: 0x06, I2C:4 (0x0E))
    x: -0.00909
    y: -0.52651
    z: 0.18863
    temperature: nan
    error_count: 0
nsh>
### listener sensor_baro -n 1
listener sensor_baro -n 1

TOPIC: sensor_baro
 sensor_baro
    timestamp: 178666033 (0.005179 seconds ago)
    timestamp_sample: 178665176 (857 us before timestamp)
    device_id: 6845985 (Type: 0x68, I2C:4 (0x76))
    pressure: 102940.16406
    temperature: 65.33628
    error_count: 0
nsh>
### listener battery_status -n 1
listener battery_status -n 1

TOPIC: battery_status
 battery_status
    timestamp: 180477959 (0.002603 seconds ago)
    voltage_v: 0.00000
    current_a: 0.12789
    current_average_a: 15.00000
    discharged_mah: 1.42326
    remaining: -1.00000
    scale: 1.00000
    time_remaining_s: nan
    temperature: nan
    voltage_cell_v: [0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000, 0.00000]
    max_cell_voltage_delta: 0.00000
    full_charge_capacity_wh: 0.00000
    remaining_capacity_wh: 0.00000
    nominal_voltage: 0.00000
    internal_resistance_estimate: 0.00500
    ocv_estimate: 0.00000
    ocv_estimate_filtered: 0.00000
    volt_based_soc_estimate: -1.00000
    voltage_prediction: 0.00000
    prediction_error: 0.00000
    estimation_covariance_norm: 0.00000
    capacity: 0
    cycle_count: 0
    average_time_to_empty: 0
    manufacture_date: 0
    state_of_health: 0
    max_error: 0
    interface_error: 0
    faults: 0
    over_discharge_count: 0
    connected: False
    cell_count: 0
    source: 0
    priority: 0
    id: 1
    is_powering_off: False
    is_required: False
    warning: 0

nsh>
```

## 首次上板结论与修正

- COM53 识别为 VID:PID 1209:0001，实际运行 commit fa4fc317，与 #8 构建标识一致；MCU silicon revision V。设备未解锁，CPU 负载约 38%。
- BMI088 加计/陀螺均在 SPI1，ICM42688P 在 SPI2，bad register/transfer/overflow 均为 0；两组加计/陀螺 validator 均 OK。静止读数不能验证全部轴向，仍需手动转动实测。
- DPS310 在 I2C4/0x76，通信错误 0；温度约 65°C，此为芯片自报值，需结合实际板温与安装位置进一步确认，不能当环境温度。
- SDMMC1 已挂载且可读文件；logger 已运行，当前未记录（未解锁）。卡上的 param_import_fail.txt 包含旧 MICOAIR/P224 固件记录，不能认作本次启动失败；本次 param status 显示自动保存开启且已有一次 export，但未断电验证。
- IST8310 起初未运行，原因是启动脚本误写 0x0c。上游驱动默认地址为 0x0e；临时执行 `ist8310 -I -b 4 -a 0x0e -R 0 start` 后启动成功、sensor_mag 发布、错误为 0。已修正源码启动地址；该临时启动不修改参数，重启前有效。
- ADC 报告中通道 4/18 能返回 0，内部温度通道 20 返回 -1，日志持续 sample timeout。上游 H7 ADC 驱动无条件添加 ADC3 内部温度采样，RCC 仅在 CONFIG_STM32H7_ADC3=y 时开启时钟；当前自定义配置漏开 ADC3。已补回 ADC3，不增加外部 ADC3 引脚。
- 电池电压显示 0、未连接；用户随后确认只接 USB，电池未连接与当前供电情况相符；仍不能据此验收主电池采样硬件。未进行电机、PWM、DShot、串口外设和断电参数保持测试。
- ver all 显示 Release 0.0.0，但 hash 确认源码来自本次构建；原因为 fork 云端缺少版本 tag，准备同步本地已有官方 v1.17.0 tag，使后续构建版本号正确。
- 为上述确定的固件问题生成新版应用；不自动烧录、不重启设备、不修改用户参数。配套 bootloader 逻辑和地址不变，继续使用已验证 #9。

### COM53 诊断修正版交付

- 用户确认本次仅 USB 供电。
- 修正版应用 [Actions #10 成功](https://github.com/cbcb224/PX4-Autopilot/actions/runs/41458476853)，源码 `af0c6eb375d332ce1efe7328b67417e26ec926ee`，版本 tag `v1.17.0-9-gaf0c6eb375`，构建标识 `p224_vio-v1_default-r10-a1-af0c6eb3`。
- 本地目录 `P224-VIO-v1-固件/application-r10/`，应用裸二进制 1482920 B，继续写入 `0x08020000`。bootloader 不需更新，继续使用 #9；不要整片擦除参数区。
- ZIP SHA256 `44d6805d3c0f3d3b37b78e47e35044e760e1a7b76fe2ff38eea44666ab48b845` 与 GitHub digest 一致；BIN SHA256 `802c0d35f977aaaae8582d3b4e6d21132208a49c594ec7a2c8054fbac190e0a9`。
- 校验通过：board_id=60000，px4 image 与 bin 一致，镜像内构建标识和 v1.17.0 版本 tag 正确，reset vector 为 0x08020305。
- 仍未自动烧录；新版需要用户更新应用后再检查磁力计自动启动和 ADC 内部温度采样。没有对电机/输出、参数值、机架配置执行更改。为诊断临时启动了 IST8310，其他状态读取命令均为只读。

## 重刷 #10 后 COM53 复查（2026-11-02）

仅执行状态读取，没有手动启动传感器、修改参数或触发输出。

```text

### ver all

NuttShell (NSH) NuttX-12.12.0
nsh> ver all
HW arch: P224_VIO_V1
PX4 git-hash: af0c6eb375d332ce1efe7328b67417e26ec926ee
PX4 version: 1.17.0 0 (17891328)
PX4 git-branch: codex/px4-v1.17.0
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: fb2fadf6f599c1406f052db013efd00a2518e72c
Build datetime: Nov  2 2026 14:10:11
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
nsh>

### ist8310 status
ist8310 status
INFO  [SPI_I2C] Running on I2C Bus 4, Address 0x0E
ist8310: reset: 1 events
ist8310: bad register: 0 events
ist8310: bad transfer: 0 events
nsh>

### listener sensor_mag -n 1
listener sensor_mag -n 1

TOPIC: sensor_mag
 sensor_mag
    timestamp: 63117040 (0.007980 seconds ago)
    timestamp_sample: 63116092 (948 us before timestamp)
    device_id: 396833 (Type: 0x06, I2C:4 (0x0E))
    x: -0.02651
    y: -0.52121
    z: 0.16060
    temperature: nan
    error_count: 0
nsh>

### board_adc status
board_adc status
INFO  [board_adc] running
nsh>

### listener adc_report -n 1
listener adc_report -n 1

TOPIC: adc_report
 adc_report
    timestamp: 66746789 (0.006468 seconds ago)
    device_id: 4294967295 (Type: 0xFF, UNKNOWN:31 (0xFF))
    raw_data: [0, 0, 10112, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    resolution: 65536
    v_ref: 3.30000
    channel_id: [4, 18, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1]

nsh>

### bmi088 -A status
bmi088 -A status
INFO  [SPI_I2C] Running on SPI Bus 1
INFO  [bmi088] FIFO empty interval: 1250 us (800.0 Hz)
bmi088_accel: bad register: 0 events
bmi088_accel: bad transfer: 0 events
bmi088_accel: FIFO empty: 0 events
bmi088_accel: FIFO overflow: 0 events
bmi088_accel: FIFO reset: 1 events
nsh>

### bmi088 -G status
bmi088 -G status
INFO  [SPI_I2C] Running on SPI Bus 1
INFO  [bmi088] FIFO empty interval: 1500 us (666.7 Hz)
bmi088_gyro: bad register: 0 events
bmi088_gyro: bad transfer: 0 events
bmi088_gyro: FIFO empty: 0 events
bmi088_gyro: FIFO overflow: 0 events
bmi088_gyro: FIFO reset: 1 events
nsh>

### icm42688p status
icm42688p status
INFO  [SPI_I2C] Running on SPI Bus 2
INFO  [icm42688p] FIFO empty interval: 1250 us (800.0 Hz)
INFO  [icm42688p] Clock input: disabled
icm42688p: bad register: 0 events
icm42688p: bad transfer: 0 events
icm42688p: FIFO empty: 0 events
icm42688p: FIFO overflow: 0 events
icm42688p: FIFO reset: 1 events
nsh>

### dps310 status
dps310 status
INFO  [SPI_I2C] Running on I2C Bus 4, Address 0x76
dps310: read: 4665 events, 3060915us elapsed, 656.14us avg, min 402us max 1380us 391.795us rms
dps310: comm errors: 0 events
nsh>

### commander status
commander status
INFO  [commander] Disarmed
INFO  [commander] navigation mode: Hold
INFO  [commander] user intended navigation mode: Hold
INFO  [commander] in failsafe: no
commander: cycle: 6717 events, 470473us elapsed, 70.04us avg, min 37us max 1033us 56.988us rms
commander: preflight check: 711 events, 113556us elapsed, 159.71us avg, min 105us max 673us 43.269us rms
nsh>

### dmesg
dmesg
HW arch: P224_VIO_V1
PX4 git-hash: af0c6eb375d332ce1efe7328b67417e26ec926ee
PX4 version: 1.17.0 0 (17891328)
PX4 git-branch: codex/px4-v1.17.0
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: fb2fadf6f599c1406f052db013efd00a2518e72c
Build datetime: Nov  2 2026 14:10:11
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
New /fs/mtd_caldata size is:
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
INFO  [param] selected parameter backup file /fs/microsd/parameters_backup.bson
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
Loading airframe: /etc/init.d/airframes/4001_quad_x
INFO  [dataman] data manager file '/fs/microsd/dataman' size is 128528 bytes
Board sensors: /etc/init.d/rc.board_sensors
bmi088_accel #0 on SPI bus 1 rotation 2
bmi088_gyro #0 on SPI bus 1 rotation 2
icm42688p #0 on SPI bus 2
ist8310 #0 on I2C bus 4 address 0xE
ERROR [dps310] Product_ID mismatch
ERROR [SPI_I2C] dps310: no instance started (no device on bus?)
dps310 #0 on I2C bus 4 address 0x76
ekf2 [493:237]
INFO  [arch_dshot] Bidirectional DShot enabled, only one timer will be used
INFO  [arch_dshot] Allocated DMA UP Timer Index 0
INFO  [arch_dshot] Freed DMA UP Timer Index 0
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
Starting MAVLink on /dev/ttyS0
INFO  [mavlink] mode: Normal, data rate: 1200 B/s on /dev/ttyS0 @ 57600B
INFO  [cdcacm_autostart] Starting CDC/ACM autostart
INFO  [logger] logger started (mode=all)

NuttShell (NSH) NuttX-12.12.0
nsh> sercon: Registering CDC/ACM serial driver
sercon: Successfully registered the CDC/ACM serial driver
ERROR [tinybson] file write error -1, errno:22 (Invalid argument)
ERROR [tinybson] killed: file write error
ERROR [tinybson] killed: write error on BSON_INT32
ERROR [parameters] BSON append failed for 'CAL_GYRO0_ID'
ERROR [parameters] backup parameter export to /fs/microsd/parameters_backup.bson failed (-1)
INFO  [cdcacm_autostart] Starting mavlink on /dev/ttyACM0 (SYS_USB_AUTO=2)
INFO  [mavlink] mode: Onboard, data rate: 100000 B/s on /dev/ttyACM0 @ 2000000B
INFO  [mavlink] Setting system clock from SYSTEM_TIME sent by 255/190
ERROR [dataman] file read lseek failed 22
ERROR [dataman] file read lseek failed 22
ERROR [dataman_client] readSync failed! status=3, item=5, index=0, length=8
INFO  [commander] Connection to ground station lost
INFO  [mavlink] Starting mavlink shell
nsh>

```

### 重刷后存储与传感器补查

```text

### mount

NuttShell (NSH) NuttX-12.12.0
nsh> mount
  /bin type binfs
  /etc type cromfs
  /fs/microsd type vfat
  /proc type procfs
nsh>

### df
df
  Block    Number
  Size     Blocks       Used   Available Mounted on
     0          0          0           0 /bin
   512        293        293           0 /etc
  4096     122636         43      122593 /fs/microsd
     0          0          0           0 /proc
nsh>

### ls -l /fs/microsd
ls -l /fs/microsd
/fs/microsd:
 -rw-rw-rw-    5471 param_import_fail.txt
 -rw-rw-rw-  128528 dataman
 drw-rw-rw-       0 log/
 drw-rw-rw-       0 ufw/
 -rw-rw-rw-       0 parameters_backup.bson
 drw-rw-rw-       0 uavcan.db/
 drw-rw-rw-       0 mavlink/
 -rw-rw-rw-     349 params
nsh>

### param status
param status
INFO  [parameters] summary: 920/1537 (used/total)
INFO  [parameters] storage array: 30/1537 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 24/1537 elements (256 bytes total)
INFO  [parameters] auto save: on
INFO  [parameters] last auto save: 112.553 seconds ago
param: export: 1 events, 197us elapsed, 197.00us avg, min 197us max 197us   nanus rms
param: find: 19012 events
param: get: 700773 events
param: set: 30 events, 246us elapsed, 8.20us avg, min 6us max 12us 1.375us rms
nsh>

### param show SYS_AUTOSTART
param show SYS_AUTOSTART
Symbols: x = used, + = saved, * = unsaved
x + SYS_AUTOSTART [883,1245] : 4001

 920/1537 parameters used.
nsh>

### logger status
logger status
INFO  [logger] Running in mode: all
INFO  [logger] Number of subscriptions: 178 (5696 bytes)
INFO  [logger] Not logging
nsh>

### listener system_power -n 1
listener system_power -n 1

TOPIC: system_power
 system_power
    timestamp: 119546795 (0.003538 seconds ago)
    voltage5v_v: 0.00000
    voltage_payload_v: 0.00000
    sensors3v3: [0.00000, 0.00000, 0.00000, 0.00000]
    sensors3v3_valid: 0
    usb_connected: 1
    brick_valid: 1
    usb_valid: 1
    servo_valid: 1
    periph_5v_oc: 0
    hipower_5v_oc: 0
    comp_5v_valid: 0
    can1_gps1_5v_valid: 0
    payload_v_valid: 0

nsh>

### listener sensor_accel -n 1
listener sensor_accel -n 1

TOPIC: sensor_accel 2 instances

Instance 0:
 sensor_accel
    timestamp: 121185662 (0.000660 seconds ago)
    timestamp_sample: 121184982 (680 us before timestamp)
    device_id: 6946826 (Type: 0x6A, SPI:1 (0x00))
    x: -0.10235
    y: -0.22804
    z: -9.79887
    temperature: 34.00000
    error_count: 0
    clip_counter: [0, 0, 0]
    samples: 2


Instance 1:
 sensor_accel
    timestamp: 121200984 (0.000329 seconds ago)
    timestamp_sample: 121200696 (288 us before timestamp)
    device_id: 2490386 (Type: 0x26, SPI:2 (0x00))
    x: -0.08068
    y: -0.32872
    z: -9.89787
    temperature: 38.03140
    error_count: 0
    clip_counter: [0, 0, 0]
    samples: 10

nsh>

### sensors status
sensors status
selected gyro: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:   0.0002, lp:  -0.0013 mean dev:  -0.0000 RMS:   0.0979 conf:   1.0000
	val:   0.0088, lp:   0.0058 mean dev:   0.0000 RMS:   0.0960 conf:   1.0000
	val:  -0.0023, lp:   0.0004 mean dev:   0.0000 RMS:   0.0914 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:  -0.0013, lp:  -0.0009 mean dev:   0.0001 RMS:   0.0922 conf:   1.0000
	val:   0.0013, lp:   0.0015 mean dev:   0.0000 RMS:   0.0948 conf:   1.0000
	val:   0.0011, lp:   0.0010 mean dev:  -0.0000 RMS:   0.0856 conf:   1.0000

selected accel: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -0.1246, lp:  -0.1152 mean dev:  -0.0005 RMS:   0.2592 conf:   1.0000
	val:  -0.2632, lp:  -0.2415 mean dev:  -0.0005 RMS:   0.1980 conf:   1.0000
	val:  -9.8262, lp:  -9.8294 mean dev:  -0.0295 RMS:   0.3512 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:  -0.0879, lp:  -0.0890 mean dev:  -0.0001 RMS:   0.2426 conf:   1.0000
	val:  -0.3282, lp:  -0.3278 mean dev:  -0.0005 RMS:   0.1849 conf:   1.0000
	val:  -9.5351, lp:  -9.5373 mean dev:  -0.0199 RMS:   0.3023 conf:   1.0000

[vehicle_magnetometer] selected MAG: 396833 (0)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:   0.2801, lp:   0.2805 mean dev:   0.0018 RMS:   0.0137 conf:   1.0000
	val:  -0.2017, lp:  -0.2026 mean dev:   0.0016 RMS:   0.0126 conf:   1.0000
	val:   0.1950, lp:   0.1934 mean dev:  -0.0025 RMS:   0.0148 conf:   1.0000
MAG 396833 EN: 1, offset: [-0.194 -0.192 -0.166], scale: [1.000 1.000 1.000], Internal

[vehicle_air_data] selected BARO: 6845985 (0)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val: 102952.3203, lp: 102953.2109 mean dev: -18.7881 RMS:  61.8554 conf:   1.0000
	val:  64.9746, lp:  64.9606 mean dev:  -1.8986 RMS:   6.0579 conf:   1.0000
	val: -134.6103, lp: -134.6777 mean dev:   1.5343 RMS:   5.0764 conf:   1.0000
BARO 6845985 EN: 1, offset: 0.000, Internal

Airspeed status:
	no data

[vehicle_acceleration] selected sensor: 2490386, rate: 799.9 Hz, estimated bias: [-0.2571 0.3648 0.2574]
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal

[vehicle_angular_velocity] selected sensor: 2490386, rate: 8042.8 Hz FIFO, estimated bias: [-0.00158 0.00084 -0.00199]
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
vehicle_angular_velocity: gyro filter: 97347 events, 1527106us elapsed, 15.69us avg, min 11us max 203us 1.275us rms
vehicle_angular_velocity: gyro filter reset: 3 events
vehicle_angular_velocity: gyro selection changed: 3 events

[vehicle_gps_position] selected GPS: 0


[vehicle_imu] 0 - Accel: 6946826, interval: 1250.0 us (SD 95.9 us), Gyro: 6684682, interval: 1500.0 us (SD 114.6 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 6946826 EN: 1, offset: [-0.003 0.014 0.054], scale: [1.000 1.000 1.000], Internal
GYRO 6684682 EN: 1, offset: [0.003 -0.002 -0.002], Internal

[vehicle_imu] 1 - Accel: 2490386, interval: 1250.0 us (SD 30.8 us), Gyro: 2490386, interval: 1250.0 us (SD 30.8 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
nsh>

```

### 重刷 #10 后复查结论

- 运行版本确认：PX4 1.17.0，commit af0c6eb375，Build URI p224_vio-v1_default-r10-a1-af0c6eb3。
- 两项修正已在实机验证：IST8310 自动在 I2C4/0x0e 启动，sensor_mag error_count=0；ADC 内部温度通道 20 raw_data=10112，非此前 -1，完整启动日志未见 board_adc sample timeout。
- BMI088 加计/陀螺、ICM42688P bad register/transfer/FIFO overflow 均 0；DPS310 通信错误 0；sensors validator 中两组 IMU、磁力计、气压计均 OK。首次启动 FIFO reset=1 与 vehicle_imu gap=1 未见持续增长证据，不能当持续故障。
- 启动先试 DPS310 0x77 报 Product_ID mismatch，随后 0x76 成功，属于当前地址回退探测过程，不能当实际运行的气压计掉线。
- /fs/mtd_caldata 不存在由上游 rcS 无条件 bsondump 探测引起；本板使用 Flash 参数而未配置该 MTD，不等同于本板参数 Flash 损坏。
- 仍有真实存储异常：参数备份导出到 SD 报 errno 22，parameters_backup.bson 为 0 B；dataman lseek 亦报 errno 22。SD 挂载为 vfat，容量约 479 MiB，空闲 122593/122636 个 4096 B 块，不是空间耗尽。只读检查不能区分文件系统、卡或 SDMMC 驱动/硬件原因，暂不判定 SD 卡正常。
- 启动还记录三条 parameters get: param 65535 invalid，尚未定位调用来源。未为此修改参数或将其断言为存储损坏。
- SYS_AUTOSTART=4001 等已保存设置仍可读取，但不代替完整 Flash 参数断电持久性测试。logger 进程运行，当前未记录（Disarmed）。
- 本次未解锁、未启动电机、未改参数、未格式化或删除 SD 文件；串口检查已结束并释放 COM53。建议先备份/检查 SD 文件系统或换一张已知正常的 FAT32 卡进行对照复查，避免现在依赖日志及 SD 参数备份。

## SD 存储继续排查（2026-11-03）

```text

### ver uri

NuttShell (NSH) NuttX-12.12.0
nsh> ver uri
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
nsh>

### ls /dev
ls /dev
/dev:
 console
 console_buf
 led0
 mmcsd0
 null
 ttyACM0
 ttyS0
 ttyS1
 ttyS2
 ttyS3
nsh>

### dmesg
dmesg
HW arch: P224_VIO_V1
PX4 git-hash: af0c6eb375d332ce1efe7328b67417e26ec926ee
PX4 version: 1.17.0 0 (17891328)
PX4 git-branch: codex/px4-v1.17.0
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: fb2fadf6f599c1406f052db013efd00a2518e72c
Build datetime: Nov  2 2026 14:10:11
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
INFO [init] formatting /dev/mmcsd0
ERROR [init] format failed
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
New /fs/mtd_caldata size is:
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
Loading airframe: /etc/init.d/airframes/4001_quad_x
WARN  [dataman] Could not open data manager file /fs/microsd/dataman
ERROR [dataman] dataman start failed
Board sensors: /etc/init.d/rc.board_sensors
bmi088_accel #0 on SPI bus 1 rotation 2
bmi088_gyro #0 on SPI bus 1 rotation 2
icm42688p #0 on SPI bus 2
ist8310 #0 on I2C bus 4 address 0xE
ERROR [dps310] Product_ID mismatch
ERROR [SPI_I2C] dps310: no instance started (no device on bus?)
dps310 #0 on I2C bus 4 address 0x76
ekf2 [447:237]
INFO  [arch_dshot] Bidirectional DShot enabled, only one timer will be used
INFO  [arch_dshot] Allocated DMA UP Timer Index 0
INFO  [arch_dshot] Freed DMA UP Timer Index 0
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
Starting MAVLink on /dev/ttyS0
INFO  [cdcacm_autostart] Starting CDC/ACM autostart
INFO  [logger] logger started (mode=all)
INFO  [logger] log root dir created: /fs/microsd/log

NuttShell (NSH) NuttX-12.12.0
nsh> sercon: Registering CDC/ACM serial driver
sercon: Successfully registered the CDC/ACM serial driver
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
INFO  [cdcacm_autostart] Starting mavlink on /dev/ttyACM0 (SYS_USB_AUTO=2)
WARN  [health_and_arming_checks] Preflight Fail: Missing FMU SD Card
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
INFO  [mavlink] mode: Onboard, data rate: 100000 B/s on /dev/ttyACM0 @ 2000000B
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
WARN  [health_and_arming_checks] Preflight Fail: Attitude failure (roll)
WARN  [health_and_arming_checks] Preflight Fail: Missing FMU SD Card
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
ERROR [dataman_client] timeout after 1000 ms!
ERROR [dataman_client] Failed to get client ID!
ERROR [dataman_client] timeout after 5000 ms!
WARN  [mavlink] offboard mission init failed
INFO  [mavlink] mode: Normal, data rate: 1200 B/s on /dev/ttyS0 @ 57600B
INFO  [mavlink] Starting mavlink shell
nsh>

### param status
param status
INFO  [parameters] summary: 920/1537 (used/total)
INFO  [parameters] storage array: 30/1537 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 24/1537 elements (256 bytes total)
INFO  [parameters] auto save: on
INFO  [parameters] last auto save: 100.163 seconds ago
param: export: 1 events, 605us elapsed, 605.00us avg, min 605us max 605us   nanus rms
param: find: 16764 events
param: get: 630964 events
param: set: 30 events, 248us elapsed, 8.27us avg, min 6us max 12us 1.461us rms
nsh>

### ls -l /fs/microsd
ls -l /fs/microsd
/fs/microsd:
 dr--r--r--       0 log/
nsh>

```

用户确认上一次未插 SD 卡，故上次启动挂载失败不用于判断有卡时状态；用户插卡后恢复检查。

## 插回 SD 后复查（2026-11-03）

```text

### ver uri

NuttShell (NSH) NuttX-12.12.0
nsh> ver uri
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
nsh>

### mount
mount
  /bin type binfs
  /etc type cromfs
  /fs/microsd type vfat
  /proc type procfs
nsh>

### df
df
  Block    Number
  Size     Blocks       Used   Available Mounted on
     0          0          0           0 /bin
   512        293        293           0 /etc
  4096     122636         43      122593 /fs/microsd
     0          0          0           0 /proc
nsh>

### ls -l /fs/microsd
ls -l /fs/microsd
/fs/microsd:
 -rw-rw-rw-    5471 param_import_fail.txt
 -rw-rw-rw-  128528 dataman
 drw-rw-rw-       0 log/
 drw-rw-rw-       0 ufw/
 -rw-rw-rw-       0 parameters_backup.bson
 drw-rw-rw-       0 uavcan.db/
 drw-rw-rw-       0 mavlink/
 -rw-rw-rw-     349 params
nsh>

### param status
param status
INFO  [parameters] summary: 920/1537 (used/total)
INFO  [parameters] storage array: 30/1537 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 24/1537 elements (256 bytes total)
INFO  [parameters] auto save: on
INFO  [parameters] last auto save: 22.949 seconds ago
param: export: 1 events, 217us elapsed, 217.00us avg, min 217us max 217us   infus rms
param: find: 5235 events
param: get: 146341 events
param: set: 30 events, 248us elapsed, 8.27us avg, min 6us max 11us 1.311us rms
nsh>

### dataman status
dataman status
INFO  [dataman] Writes   0
INFO  [dataman] Reads    5
INFO  [dataman] Clears   0
dataman: read: 5 events, 7133us elapsed, 1426.60us avg, min 812us max 2775us 874.445us rms
dataman: write: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
nsh>

### logger status
logger status
INFO  [logger] Running in mode: all
INFO  [logger] Number of subscriptions: 178 (5696 bytes)
INFO  [logger] Not logging
nsh>

### dmesg
dmesg
HW arch: P224_VIO_V1
PX4 git-hash: af0c6eb375d332ce1efe7328b67417e26ec926ee
PX4 version: 1.17.0 0 (17891328)
PX4 git-branch: codex/px4-v1.17.0
OS: NuttX
OS version: Release 12.12.0 (202113279)
OS git-hash: fb2fadf6f599c1406f052db013efd00a2518e72c
Build datetime: Nov  2 2026 14:10:11
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
Build variant: default
Toolchain: GNU GCC, 13.2.1 20231009
PX4GUID: 000600000000363335343034510d00470038
MCU: STM32H7[4|5]xxx, rev. V
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
New /fs/mtd_caldata size is:
ERROR [bsondump] open '/fs/mtd_caldata' failed (2)
INFO  [param] selected parameter backup file /fs/microsd/parameters_backup.bson
Board architecture defaults: /etc/init.d/rc.board_arch_defaults
Board defaults: /etc/init.d/rc.board_defaults
Loading airframe: /etc/init.d/airframes/4001_quad_x
INFO  [dataman] data manager file '/fs/microsd/dataman' size is 128528 bytes
Board sensors: /etc/init.d/rc.board_sensors
bmi088_accel #0 on SPI bus 1 rotation 2
bmi088_gyro #0 on SPI bus 1 rotation 2
icm42688p #0 on SPI bus 2
ist8310 #0 on I2C bus 4 address 0xE
ERROR [dps310] Product_ID mismatch
ERROR [SPI_I2C] dps310: no instance started (no device on bus?)
dps310 #0 on I2C bus 4 address 0x76
ekf2 [493:237]
INFO  [arch_dshot] Bidirectional DShot enabled, only one timer will be used
INFO  [arch_dshot] Allocated DMA UP Timer Index 0
INFO  [arch_dshot] Freed DMA UP Timer Index 0
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
ERROR [parameters] get: param 65535 invalid
Starting MAVLink on /dev/ttyS0
INFO  [mavlink] mode: Normal, data rate: 1200 B/s on /dev/ttyS0 @ 57600B
INFO  [cdcacm_autostart] Starting CDC/ACM autostart
INFO  [logger] logger started (mode=all)

NuttShell (NSH) NuttX-12.12.0
nsh> sercon: Registering CDC/ACM serial driver
sercon: Successfully registered the CDC/ACM serial driver
ERROR [tinybson] file write error -1, errno:22 (Invalid argument)
ERROR [tinybson] killed: file write error
ERROR [tinybson] killed: write error on BSON_INT32
ERROR [parameters] BSON append failed for 'CAL_GYRO0_ID'
ERROR [parameters] backup parameter export to /fs/microsd/parameters_backup.bson failed (-1)
INFO  [cdcacm_autostart] Starting mavlink on /dev/ttyACM0 (SYS_USB_AUTO=2)
INFO  [mavlink] mode: Onboard, data rate: 100000 B/s on /dev/ttyACM0 @ 2000000B
WARN  [health_and_arming_checks] Preflight Fail: Attitude failure (roll)
INFO  [mavlink] Starting mavlink shell
nsh>

```

### 独立新文件读写诊断

只在确认路径不存在后创建两份诊断文件，不覆盖旧参数备份、不改参数值。

```text

### ls /fs/microsd/p224_diag_20261103.txt

NuttShell (NSH) NuttX-12.12.0
nsh> ls /fs/microsd/p224_diag_20261103.txt
nsh: ls: stat failed: No such file or directory
nsh>

### ls /fs/microsd/p224_diag_20261103.bson
ls /fs/microsd/p224_diag_20261103.bson
nsh: ls: stat failed: No such file or directory
nsh>

### echo P224_SD_DIAGNOSTIC_20261103 > /fs/microsd/p224_diag_20261103.txt
echo P224_SD_DIAGNOSTIC_20261103 > /fs/microsd/p224_diag_20261103.txt
nsh>

### cat /fs/microsd/p224_diag_20261103.txt
cat /fs/microsd/p224_diag_20261103.txt
nsh: cat: read failed: Invalid argument
nsh>

### param export /fs/microsd/p224_diag_20261103.bson
param export /fs/microsd/p224_diag_20261103.bson
Usage: param <command> [arguments...]
 Commands:

   load          Load params from a file (overwrite all)
     [<file>   ] File name (use default if not given)

   import        Import params from a file
     [<file>   ] File name (use default if not given)

   save          Save params to a file
     [<file>   ] File name (use default if not given)

   select        Select default file
     [<file>   ] File name

   select-backup Select default file
     [<file>   ] File name

   show          Show parameter values
     [-a]        Show all parameters (not just used)
     [-c]        Show only changed params (unused too)
     [-q]        quiet mode, print only param value (name needs to be exact)
     [<filter> ] Filter by param name (wildcard at end allowed, eg. sys_*)

   show-for-airframe Show changed params for airframe config

   status        Print status of parameter system

   set           Set parameter to a value
     <param_name> <value> Parameter name and value to set
     [fail     ] If provided, let the command fail if param is not found

   set-default   Set parameter default to a value
     [-s]        If provided, silent errors if parameter doesn't exists
     <param_name> <value> Parameter name and value to set
     [fail     ] If provided, let the command fail if param is not found

   compare       Compare a param with a value. Command will succeed if equal
     [-s]        If provided, silent errors if parameter doesn't exists
     <param_name> <value> Parameter name and value to compare

   greater       Compare a param with a value. Command will succeed if param is greater than the value
     [-s]        If provided, silent errors if parameter doesn't exists
     <param_name> <value> Parameter name and value to compare
     <param_name> <value> Parameter name and value to compare

   touch         Mark a parameter as used
     [<param_name1> [<param_name2>]] Parameter name (one or more)

   reset         Reset only specified params to default
     [<param1> [<param2>]] Parameter names to reset (wildcard at end allowed)

   reset_all     Reset all params to default
     [<exclude1> [<exclude2>]] Do not reset matching params (wildcard at end allowed)

   index         Show param for a given index
     <index>     Index: an integer >= 0

   index_used    Show used param for a given index
     <index>     Index: an integer >= 0

   find          Show index of a param
     <param>     param name
nsh>

### ls -l /fs/microsd
ls -l /fs/microsd
/fs/microsd:
 -rw-rw-rw-    5471 param_import_fail.txt
 -rw-rw-rw-  128528 dataman
 drw-rw-rw-       0 log/
 drw-rw-rw-       0 ufw/
 -rw-rw-rw-       0 parameters_backup.bson
 drw-rw-rw-       0 uavcan.db/
 drw-rw-rw-       0 mavlink/
 -rw-rw-rw-     349 params
 -rw-rw-rw-       0 p224_diag_20261103.txt
nsh>

### bsondump docsize /fs/microsd/p224_diag_20261103.bson
bsondump docsize /fs/microsd/p224_diag_20261103.bson
ERROR [bsondump] open '/fs/microsd/p224_diag_20261103.bson' failed (2)
nsh>

```

### 插卡后 SD 诊断结论（2026-11-03）

- 用户确认上一轮无卡；插回后 vfat 正常挂载，dataman 已启动并完成 5 次读取，当前启动未复现 lseek 错误。
- 参数备份仍为 0 B，自动备份写入 errno 22 持续存在，空间充足。
- 为区分原备份文件问题，在确认不存在后创建 p224_diag_20261103.txt，echo 未报错，但立即 cat 报 read failed: Invalid argument。说明异常不局限于原 parameters_backup.bson；不能认定 SD 可正常读写。
- 尝试 param export 仅返回帮助，当前 CLI 不支持该命令，因此未创建 BSON 诊断文件，未导出参数。没有执行 param save、改参数、格式化或删除原文件。卡上保留上述新增文本诊断文件。
- 当前证据倾向文件系统/簇链或底层 SD 读写异常，但不足以区分卡损坏与固件 SDMMC 路径问题。建议先断电取卡，经读卡器备份并只读检查文件系统，或换已知正常卡对照；未授权修复前不运行 chkdsk /f 或格式化。
- 不建议通过飞控反复重启排查此卡：上游 rcS 在块设备存在但挂载失败时会自动尝试 mkfatfs，源码已确认。无卡时本板无检测脚也会声明存在块设备。
- COM53 已释放。尚未进行飞行/电机输出验证。

## 跳过 SD 的系统检查（2026-11-03）

仅状态查询，未解锁或改参数。电脑端 pymavlink 高层消息缓存遇到异常，改用同库底层 MAVLink 解析读取，不是飞控故障。

```text

### ver uri

NuttShell (NSH) NuttX-12.12.0
nsh> ver uri
Build uri: p224_vio-v1_default-r10-a1-af0c6eb3
nsh>

### commander status
commander status
INFO  [commander] Disarmed
INFO  [commander] navigation mode: Hold
INFO  [commander] user intended navigation mode: Hold
INFO  [commander] in failsafe: no
commander: cycle: 16165 events, 1152297us elapsed, 71.28us avg, min 37us max 1241us 60.825us rms
commander: preflight check: 1712 events, 297296us elapsed, 173.65us avg, min 105us max 722us 48.555us rms
nsh>

### sensors status
sensors status
selected gyro: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -0.0030, lp:  -0.0015 mean dev:   0.0000 RMS:   0.0084 conf:   1.0000
	val:   0.0053, lp:   0.0058 mean dev:  -0.0000 RMS:   0.0068 conf:   1.0000
	val:   0.0030, lp:   0.0009 mean dev:  -0.0000 RMS:   0.0228 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:   0.0030, lp:   0.0027 mean dev:   0.0001 RMS:   0.0075 conf:   1.0000
	val:  -0.0003, lp:   0.0015 mean dev:   0.0000 RMS:   0.0066 conf:   1.0000
	val:   0.0025, lp:   0.0027 mean dev:  -0.0000 RMS:   0.0217 conf:   1.0000

selected accel: 2490386 (1)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -1.6655, lp:  -1.6903 mean dev:  -0.0031 RMS:   0.0692 conf:   1.0000
	val:   0.0947, lp:   0.0713 mean dev:  -0.0001 RMS:   0.0692 conf:   1.0000
	val:   9.7051, lp:   9.6613 mean dev:   0.0199 RMS:   0.2252 conf:   1.0000
sensor #1, prio: 50, state: OK
	val:  -1.8019, lp:  -1.8037 mean dev:  -0.0023 RMS:   0.0593 conf:   1.0000
	val:   0.0818, lp:   0.0788 mean dev:   0.0001 RMS:   0.0665 conf:   1.0000
	val:   9.6582, lp:   9.6620 mean dev:   0.0130 RMS:   0.1776 conf:   1.0000

[vehicle_magnetometer] selected MAG: 396833 (0)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val:  -0.0219, lp:  -0.0220 mean dev:  -0.0006 RMS:   0.0075 conf:   1.0000
	val:  -0.0001, lp:  -0.0017 mean dev:   0.0009 RMS:   0.0072 conf:   1.0000
	val:  -0.6562, lp:  -0.6524 mean dev:  -0.0002 RMS:   0.0092 conf:   1.0000
MAG 396833 EN: 1, offset: [-0.194 -0.192 -0.166], scale: [1.000 1.000 1.000], Internal

[vehicle_air_data] selected BARO: 6845985 (0)
validator: best: 0, prev best: 0, failsafe: NO (0 events)
sensor #0, prio: 50, state: OK
	val: 102861.8516, lp: 102862.8906 mean dev: -15.3041 RMS:  56.0264 conf:   1.0000
	val:  63.6320, lp:  63.5925 mean dev:  -1.3151 RMS:   5.4612 conf:   1.0000
	val: -127.2066, lp: -127.2490 mean dev:   1.2481 RMS:   4.5982 conf:   1.0000
BARO 6845985 EN: 1, offset: 0.000, Internal

Airspeed status:
	no data

[vehicle_acceleration] selected sensor: 2490386, rate: 799.9 Hz, estimated bias: [0.0833 -0.0856 0.0359]
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal

[vehicle_angular_velocity] selected sensor: 2490386, rate: 8044.9 Hz FIFO, estimated bias: [0.00294 0.00133 0.00225]
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
vehicle_angular_velocity: gyro filter: 145046 events, 2274118us elapsed, 15.68us avg, min 11us max 199us 1.178us rms
vehicle_angular_velocity: gyro filter reset: 3 events
vehicle_angular_velocity: gyro selection changed: 3 events

[vehicle_gps_position] selected GPS: 0


[vehicle_imu] 0 - Accel: 6946826, interval: 1250.0 us (SD 97.8 us), Gyro: 6684682, interval: 1500.0 us (SD 125.4 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 6946826 EN: 1, offset: [-0.003 0.014 0.054], scale: [1.000 1.000 1.000], Internal
GYRO 6684682 EN: 1, offset: [0.003 -0.002 -0.002], Internal

[vehicle_imu] 1 - Accel: 2490386, interval: 1250.0 us (SD 30.8 us), Gyro: 2490386, interval: 1250.0 us (SD 30.8 us)
vehicle_imu: accel data gap: 1 events
vehicle_imu: gyro data gap: 1 events
ACC 2490386 EN: 1, offset: [0.000 0.014 -0.346], scale: [1.000 1.000 1.000], Internal
GYRO 2490386 EN: 1, offset: [0.035 0.004 -0.015], Internal
nsh>

### ekf2 status
ekf2 status
INFO  [ekf2] available instances: 2
INFO  [ekf2] 0: ACC: 6946826, GYRO: 6684682, MAG: 396833, healthy, test ratio: 1.0000000 (0.00000)
INFO  [ekf2] 1: ACC: 2490386, GYRO: 2490386, MAG: 396833, healthy, test ratio: 1.0000000 (0.00000) *

ekf2:0 EKF dt: 0.0102s, attitude: 1, local position: 0, global position: 0
ekf2: EKF update: 17787 events, 6339960us elapsed, 356.44us avg, min 104us max 1005us 130.732us rms
ekf2: IMU message missed: 1 events

ekf2:1 EKF dt: 0.0099s, attitude: 1, local position: 0, global position: 0
ekf2: EKF update: 18233 events, 7015426us elapsed, 384.77us avg, min 104us max 1742us 190.529us rms
ekf2: IMU message missed: 0 events
nsh>

### listener vehicle_status -n 1
listener vehicle_status -n 1

TOPIC: vehicle_status
 vehicle_status
    timestamp: 185692900 (0.231246 seconds ago)
    armed_time: 0
    takeoff_time: 0
    nav_state_timestamp: 1222998
    valid_nav_states_mask: 2147411327
    can_set_nav_states_mask: 8308095
    failure_detector_status: 1
    arming_state: 1
    latest_arming_reason: 0
    latest_disarming_reason: 0
    nav_state_user_intention: 4
    nav_state: 4
    executor_in_charge: 0
    hil_state: 0
    vehicle_type: 1
    failsafe: False
    failsafe_and_user_took_over: False
    failsafe_defer_state: 0
    gcs_connection_lost: True
    gcs_connection_lost_counter: 0
    high_latency_data_link_lost: False
    is_vtol: False
    is_vtol_tailsitter: False
    in_transition_mode: False
    in_transition_to_fw: False
    system_type: 2
    system_id: 1
    component_id: 1
    safety_button_available: True
    safety_off: True
    power_input_valid: True
    usb_connected: True
    open_drone_id_system_present: False
    open_drone_id_system_healthy: False
    parachute_system_present: False
    parachute_system_healthy: False
    rc_calibration_in_progress: False
    calibration_enabled: False
    pre_flight_checks_pass: False

nsh>

### listener estimator_status_flags -n 1
listener estimator_status_flags -n 1

TOPIC: estimator_status_flags 2 instances

Instance 0:
 estimator_status_flags
    timestamp: 187266117 (0.360094 seconds ago)
    timestamp_sample: 187075175 (190942 us before timestamp)
    control_status_changes: 31
    fault_status_changes: 0
    innovation_fault_status_changes: 2
    cs_tilt_align: True
    cs_yaw_align: True
    cs_gnss_pos: False
    cs_opt_flow: False
    cs_mag_hdg: True
    cs_mag_3d: False
    cs_mag_dec: True
    cs_in_air: False
    cs_wind: False
    cs_baro_hgt: True
    cs_rng_hgt: False
    cs_gps_hgt: False
    cs_ev_pos: False
    cs_ev_yaw: False
    cs_ev_hgt: False
    cs_fuse_beta: False
    cs_mag_field_disturbed: False
    cs_fixed_wing: False
    cs_mag_fault: False
    cs_fuse_aspd: False
    cs_gnd_effect: False
    cs_rng_stuck: False
    cs_gnss_yaw: False
    cs_mag_aligned_in_flight: False
    cs_ev_vel: False
    cs_synthetic_mag_z: False
    cs_vehicle_at_rest: True
    cs_gnss_yaw_fault: False
    cs_rng_fault: False
    cs_inertial_dead_reckoning: True
    cs_wind_dead_reckoning: False
    cs_rng_kin_consistent: False
    cs_fake_pos: False
    cs_fake_hgt: False
    cs_gravity_vector: True
    cs_mag: True
    cs_ev_yaw_fault: False
    cs_mag_heading_consistent: True
    cs_aux_gpos: False
    cs_rng_terrain: False
    cs_opt_flow_terrain: False
    cs_valid_fake_pos: True
    cs_constant_pos: False
    cs_baro_fault: False
    cs_gnss_vel: False
    cs_gnss_fault: False
    cs_yaw_manual: False
    cs_gnss_hgt_fault: False
    fs_bad_mag_x: False
    fs_bad_mag_y: False
    fs_bad_mag_z: False
    fs_bad_hdg: False
    fs_bad_mag_decl: False
    fs_bad_airspeed: False
    fs_bad_sideslip: False
    fs_bad_optflow_x: False
    fs_bad_optflow_y: False
    fs_bad_acc_vertical: False
    fs_bad_acc_clipping: False
    reject_hor_vel: False
    reject_ver_vel: False
    reject_hor_pos: False
    reject_ver_pos: False
    reject_yaw: False
    reject_airspeed: False
    reject_sideslip: False
    reject_hagl: False
    reject_optflow_x: False
    reject_optflow_y: False


Instance 1:
 estimator_status_flags
    timestamp: 187381867 (0.340733 seconds ago)
    timestamp_sample: 187190321 (191546 us before timestamp)
    control_status_changes: 29
    fault_status_changes: 0
    innovation_fault_status_changes: 2
    cs_tilt_align: True
    cs_yaw_align: True
    cs_gnss_pos: False
    cs_opt_flow: False
    cs_mag_hdg: True
    cs_mag_3d: False
    cs_mag_dec: True
    cs_in_air: False
    cs_wind: False
    cs_baro_hgt: True
    cs_rng_hgt: False
    cs_gps_hgt: False
    cs_ev_pos: False
    cs_ev_yaw: False
    cs_ev_hgt: False
    cs_fuse_beta: False
    cs_mag_field_disturbed: False
    cs_fixed_wing: False
    cs_mag_fault: False
    cs_fuse_aspd: False
    cs_gnd_effect: False
    cs_rng_stuck: False
    cs_gnss_yaw: False
    cs_mag_aligned_in_flight: False
    cs_ev_vel: False
    cs_synthetic_mag_z: False
    cs_vehicle_at_rest: True
    cs_gnss_yaw_fault: False
    cs_rng_fault: False
    cs_inertial_dead_reckoning: True
    cs_wind_dead_reckoning: False
    cs_rng_kin_consistent: False
    cs_fake_pos: False
    cs_fake_hgt: False
    cs_gravity_vector: True
    cs_mag: True
    cs_ev_yaw_fault: False
    cs_mag_heading_consistent: True
    cs_aux_gpos: False
    cs_rng_terrain: False
    cs_opt_flow_terrain: False
    cs_valid_fake_pos: True
    cs_constant_pos: False
    cs_baro_fault: False
    cs_gnss_vel: False
    cs_gnss_fault: False
    cs_yaw_manual: False
    cs_gnss_hgt_fault: False
    fs_bad_mag_x: False
    fs_bad_mag_y: False
    fs_bad_mag_z: False
    fs_bad_hdg: False
    fs_bad_mag_decl: False
    fs_bad_airspeed: False
    fs_bad_sideslip: False
    fs_bad_optflow_x: False
    fs_bad_optflow_y: False
    fs_bad_acc_vertical: False
    fs_bad_acc_clipping: False
    reject_hor_vel: False
    reject_ver_vel: False
    reject_hor_pos: False
    reject_ver_pos: False
    reject_yaw: False
    reject_airspeed: False
    reject_sideslip: False
    reject_hagl: False
    reject_optflow_x: False
    reject_optflow_y: False

nsh>

### listener vehicle_attitude -n 1
listener vehicle_attitude -n 1

TOPIC: vehicle_attitude
 vehicle_attitude
    timestamp: 189330603 (0.006476 seconds ago)
    timestamp_sample: 189330197 (406 us before timestamp)
    q: [-0.09453, -0.16468, 0.98150, -0.02437] (Roll: -179.0 deg, Pitch: -11.2 deg, Yaw: -161.0 deg)
    delta_q_reset: [-0.92288, -0.00000, 0.00000, -0.38508]
    quat_reset_counter: 1

nsh>

### listener sensor_baro -n 1
listener sensor_baro -n 1

TOPIC: sensor_baro
 sensor_baro
    timestamp: 191033932 (0.010845 seconds ago)
    timestamp_sample: 191033075 (857 us before timestamp)
    device_id: 6845985 (Type: 0x68, I2C:4 (0x76))
    pressure: 102861.40625
    temperature: 63.66594
    error_count: 0
nsh>

### listener adc_report -n 1
listener adc_report -n 1

TOPIC: adc_report
 adc_report
    timestamp: 192748783 (0.001085 seconds ago)
    device_id: 4294967295 (Type: 0xFF, UNKNOWN:31 (0xFF))
    raw_data: [0, 0, 10095, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    resolution: 65536
    v_ref: 3.30000
    channel_id: [4, 18, 20, -1, -1, -1, -1, -1, -1, -1, -1, -1]

nsh>

### listener cpuload -n 1
listener cpuload -n 1

TOPIC: cpuload
 cpuload
    timestamp: 194054302 (0.395929 seconds ago)
    load: 0.38502
    ram_usage: 0.27851
nsh>

### free
free
                   total       used       free    largest  nused  nfree
        Umem:     961840     264224     697616     468720   1105     59
        Prog:    2097184    1526624     570560     352096  47707  17830
nsh>

```

### 输出与配置状态（只读）

```text

### dshot status

NuttShell (NSH) NuttX-12.12.0
nsh> dshot status
INFO  [dshot] Outputs initialized: yes
INFO  [dshot] Outputs used: 0xf
INFO  [dshot] Outputs on: yes
dshot: cycle: 741 events, 4480us elapsed, 6.05us avg, min 4us max 666us 24.504us rms
dshot: bdshot rpm: 741 events
dshot: dshot telem: 0 events
INFO  [mixer_module] Param prefix: PWM_MAIN
control latency: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
Channel Configuration:
Channel 0: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 1: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 2: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 3: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 4: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 5: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 6: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 7: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 8: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
INFO  [arch_dshot] dshot driver stats:
INFO  [arch_dshot] Bidirectional DShot enabled
INFO  [arch_dshot] Timer 0, Channel 0: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 1: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 2: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 3: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
nsh>

### pwm_out status
pwm_out status
pwm_out: cycle: 747 events, 3113us elapsed, 4.17us avg, min 3us max 173us 6.205us rms
pwm_out: interval: 747 events, 299197.04us avg, min 226us max 300078us 10968.152us rms
INFO  [mixer_module] Param prefix: PWM_MAIN
control latency: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
Channel Configuration:
Channel 0: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 1: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 2: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 3: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 4: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 5: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 6: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 7: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Channel 8: func:   0, value: 1000, failsafe: 1000, disarmed: 1000, min: 1000, max: 2000, center: 65535
Timer 1: rate: 400 channels: 4 5
Timer 2: rate: 400 channels: 6 7
Timer 3: rate: 400 channels: 8
nsh>

### control_allocator status
control_allocator status
INFO  [control_allocator] Running
INFO  [control_allocator] Method: Auto
INFO  [control_allocator] Effectiveness Source: Multirotor
INFO  [control_allocator]   Effectiveness.T =
  | 0      | 1      | 2      | 3      | 4      | 5
 0|-6.50000  6.50000  0.32500  0        0       -6.50000
 1| 6.50000 -6.50000  0.32500  0        0       -6.50000
 2| 6.50000  6.50000 -0.32500  0        0       -6.50000
 3|-6.50000 -6.50000 -0.32500  0        0       -6.50000
 4| 0        0        0        0        0        0
 5| 0        0        0        0        0        0
 6| 0        0        0        0        0        0
 7| 0        0        0        0        0        0
 8| 0        0        0        0        0        0
 9| 0        0        0        0        0        0
10| 0        0        0        0        0        0
11| 0        0        0        0        0        0
12| 0        0        0        0        0        0
13| 0        0        0        0        0        0
14| 0        0        0        0        0        0
15| 0        0        0        0        0        0
INFO  [control_allocator]   minimum =
  | 0      | 1      | 2      | 3      | 4      | 5      | 6      | 7      | 8      | 9      |10      |11      |12      |13      |14      |15
 0| 0        0        0        0        0        0        0        0        0        0        0        0        0        0        0        0
INFO  [control_allocator]   maximum =
  | 0      | 1      | 2      | 3      | 4      | 5      | 6      | 7      | 8      | 9      |10      |11      |12      |13      |14      |15
 0| 1.00000  1.00000  1.00000  1.00000  0        0        0        0        0        0        0        0        0        0        0        0
INFO  [control_allocator]   Configured actuators: 4
control_allocator: cycle: 180094 events, 3756452us elapsed, 20.86us avg, min 18us max 183us 2.263us rms
nsh>

### mavlink status
mavlink status

instance #0:
	mavlink chan: #0
	type:		GENERIC LINK OR RADIO
	flow control: OFF
	rates:
	  tx: 801.0 B/s
	  txerr: 0.0 B/s
	  tx rate mult: 0.325
	  tx rate max: 1200 B/s
	  rx: 0.0 B/s
	  rx loss: 0.0%
	FTP enabled: YES, TX enabled: YES
	mode: Normal
	Forwarding: On
	MAVLink version: 2
	transport protocol: serial (/dev/ttyS0 @57600)

instance #1:
	mavlink chan: #1
	type:		USB CDC
	flow control: OFF
	rates:
	  tx: 21154.8 B/s
	  txerr: 0.0 B/s
	  tx rate mult: 1.000
	  tx rate max: 100000 B/s
	  rx: 0.0 B/s
	  rx loss: 21.6%
	Received Messages:
	  sysid:254, compid:  0, Total: 18 (lost: 493)
	  sysid:254, compid:190, Total: 16 (lost: 243)
	FTP enabled: YES, TX enabled: YES
	mode: Onboard
	Forwarding: On
	MAVLink version: 2
	transport protocol: serial (/dev/ttyACM0 @2000000)
nsh>

### param show PWM_MAIN*
param show PWM_MAIN*
Symbols: x = used, + = saved, * = unsaved
x   PWM_MAIN_CENT1 [651,963] : -1
x   PWM_MAIN_CENT2 [652,964] : -1
x   PWM_MAIN_CENT3 [653,965] : -1
x   PWM_MAIN_CENT4 [654,966] : -1
x   PWM_MAIN_CENT5 [655,967] : -1
x   PWM_MAIN_CENT6 [656,968] : -1
x   PWM_MAIN_CENT7 [657,969] : -1
x   PWM_MAIN_CENT8 [658,970] : -1
x   PWM_MAIN_CENT9 [659,971] : -1
x   PWM_MAIN_DIS1 [660,972] : 1000
x   PWM_MAIN_DIS2 [661,973] : 1000
x   PWM_MAIN_DIS3 [662,974] : 1000
x   PWM_MAIN_DIS4 [663,975] : 1000
x   PWM_MAIN_DIS5 [664,976] : 1000
x   PWM_MAIN_DIS6 [665,977] : 1000
x   PWM_MAIN_DIS7 [666,978] : 1000
x   PWM_MAIN_DIS8 [667,979] : 1000
x   PWM_MAIN_DIS9 [668,980] : 1000
x   PWM_MAIN_FAIL1 [669,981] : -1
x   PWM_MAIN_FAIL2 [670,982] : -1
x   PWM_MAIN_FAIL3 [671,983] : -1
x   PWM_MAIN_FAIL4 [672,984] : -1
x   PWM_MAIN_FAIL5 [673,985] : -1
x   PWM_MAIN_FAIL6 [674,986] : -1
x   PWM_MAIN_FAIL7 [675,987] : -1
x   PWM_MAIN_FAIL8 [676,988] : -1
x   PWM_MAIN_FAIL9 [677,989] : -1
x   PWM_MAIN_FUNC1 [678,990] : 0
x   PWM_MAIN_FUNC2 [679,991] : 0
x   PWM_MAIN_FUNC3 [680,992] : 0
x   PWM_MAIN_FUNC4 [681,993] : 0
x   PWM_MAIN_FUNC5 [682,994] : 0
x   PWM_MAIN_FUNC6 [683,995] : 0
x   PWM_MAIN_FUNC7 [684,996] : 0
x   PWM_MAIN_FUNC8 [685,997] : 0
x   PWM_MAIN_FUNC9 [686,998] : 0
x   PWM_MAIN_MAX1 [687,999] : 2000
x   PWM_MAIN_MAX2 [688,1000] : 2000
x   PWM_MAIN_MAX3 [689,1001] : 2000
x   PWM_MAIN_MAX4 [690,1002] : 2000
x   PWM_MAIN_MAX5 [691,1003] : 2000
x   PWM_MAIN_MAX6 [692,1004] : 2000
x   PWM_MAIN_MAX7 [693,1005] : 2000
x   PWM_MAIN_MAX8 [694,1006] : 2000
x   PWM_MAIN_MAX9 [695,1007] : 2000
x   PWM_MAIN_MIN1 [696,1008] : 1000
x   PWM_MAIN_MIN2 [697,1009] : 1000
x   PWM_MAIN_MIN3 [698,1010] : 1000
x   PWM_MAIN_MIN4 [699,1011] : 1000
x   PWM_MAIN_MIN5 [700,1012] : 1000
x   PWM_MAIN_MIN6 [701,1013] : 1000
x   PWM_MAIN_MIN7 [702,1014] : 1000
x   PWM_MAIN_MIN8 [703,1015] : 1000
x   PWM_MAIN_MIN9 [704,1016] : 1000
x   PWM_MAIN_REV [705,1017] : 0
x + PWM_MAIN_TIM0 [706,1018] : -3
x   PWM_MAIN_TIM1 [707,1019] : 400
x   PWM_MAIN_TIM2 [708,1020] : 400
x   PWM_MAIN_TIM3 [709,1021] : 400

 920/1537 parameters used.
nsh>

### param show DSHOT*
param show DSHOT*
Symbols: x = used, + = saved, * = unsaved
x   DSHOT_3D_DEAD_H [284,519] : 1000
x   DSHOT_3D_DEAD_L [285,520] : 1000
x   DSHOT_3D_ENABLE [286,521] : 0
x   DSHOT_BIDIR_EN [287,522] : 1
x   DSHOT_MIN [288,523] : 0.0550
x   DSHOT_TEL_CFG [289,524] : 0

 920/1537 parameters used.
nsh>

### param show SENS_BOARD*
param show SENS_BOARD*
Symbols: x = used, + = saved, * = unsaved
x   SENS_BOARD_ROT [845,1179] : 0
x   SENS_BOARD_X_OFF [846,1180] : 0.0000
x   SENS_BOARD_Y_OFF [847,1181] : 0.0000
x   SENS_BOARD_Z_OFF [848,1182] : 0.0000

 920/1537 parameters used.
nsh>

### param show FD_FAIL*
param show FD_FAIL*
Symbols: x = used, + = saved, * = unsaved
x   FD_FAIL_P [440,676] : 60
x   FD_FAIL_P_TTRI [441,677] : 0.3000
x   FD_FAIL_R [442,678] : 60
x   FD_FAIL_R_TTRI [443,679] : 0.3000

 920/1537 parameters used.
nsh>

### listener esc_status -n 1
listener esc_status -n 1
never published
nsh>

### listener input_rc -n 1
listener input_rc -n 1
never published
nsh>

### param show COM_ARM*
param show COM_ARM*
Symbols: x = used, + = saved, * = unsaved
x   COM_ARMABLE [210,437] : 1
x   COM_ARM_AUTH_ID [211,438] : 10
x   COM_ARM_AUTH_MET [212,439] : 0
x   COM_ARM_AUTH_REQ [213,440] : 0
x   COM_ARM_AUTH_TO [214,441] : 1.0000
x   COM_ARM_BAT_MIN [215,442] : -1.0000
x   COM_ARM_CHK_ESCS [216,443] : 0
x   COM_ARM_HFLT_CHK [217,444] : 1
x   COM_ARM_IMU_ACC [218,445] : 0.7000
x   COM_ARM_IMU_GYR [219,446] : 0.2500
x   COM_ARM_MAG_ANG [220,447] : 60
x   COM_ARM_MAG_STR [221,448] : 2
x   COM_ARM_MIS_REQ [222,449] : 0
x   COM_ARM_ODID [223,450] : 0
x   COM_ARM_SDCARD [224,451] : 1
x   COM_ARM_SWISBTN [225,452] : 0
x   COM_ARM_WO_GPS [226,453] : 1

 920/1537 parameters used.
nsh>

```

### 跳过 SD 的系统检查结论

- 用户确认当前背面朝上、略倾斜；Roll -179°、Pitch -11.2°，两组加计 Z 约 +9.7 m/s²，与该摆放相符。滚转 failure flag 不据此判定 IMU 故障；尚需正面平放及逐轴动作验证。
- 两组 IMU、磁力计、气压计 validator 均 OK；两个 EKF 实例均 healthy，tilt/yaw 对齐完成，未置磁场/气压故障标志。姿态有效；本地/全局位置无效，没有 GNSS/光流/外部定位输入，不能验证定位模式。
- CPU 负载 38.5%，RAM 27.9%；free 显示用户内存剩余 697616 B，最大连续块 468720 B，未见资源耗尽。
- ADC 内部温度通道 raw 10095 有效；外部电池通道均 0，用户此前确认仅 USB 供电，不能据此验收主电池采样。气压计自报温度约 63.7°C，仍需对照实际板温，不能直接判定为环境温度或硬件过热。
- DShot 初始化成功，mask 0x0f；双向启用，RPM 成功读取数 0、esc_status 未发布，尚未验证 ESC 回传。PWM 其余输出索引4/5、6/7、8分组运行于400 Hz。
- 当前已保存 PWM_MAIN_TIM0=-3，源码确认对应 DShot600；初版默认 -4/DShot300，被已保存参数覆盖。未擅自改回。
- PWM_MAIN_FUNC1～9 全部为0（禁用）；虽控制分配器配置四旋翼，输出尚未绑定 Motor1～4，因此不是可直接控制电机的配置。未设置映射、未执行输出测试。
- input_rc 从未发布；GPS 无数据。四串口中仅 ttyS0 的 MAVLink 已启动，另外三路未接外设回环验证，不能宣布四串口硬件全部正常。
- USB MAVLink txerr=0；rx loss统计包含多次短会话重置序列号，不把该累计值直接当 USB 物理丢包结论。
- 检查期间仍未解锁、未修改参数、未触发输出。所有串口已关闭。SD问题按用户要求跳过，未清除相关安全检查。

## 按用户要求改为 BDShot300（2026-11-03）

确认未解锁后设置 PWM_MAIN_TIM0=-4，不重启、不触发输出。

```text

### commander status

NuttShell (NSH) NuttX-12.12.0
nsh> commander status
INFO  [commander] Disarmed
INFO  [commander] navigation mode: Hold
INFO  [commander] user intended navigation mode: Hold
INFO  [commander] in failsafe: no
commander: cycle: 37732 events, 2757452us elapsed, 73.08us avg, min 37us max 1262us 62.684us rms
commander: preflight check: 3995 events, 697264us elapsed, 174.53us avg, min 105us max 1218us 59.717us rms
nsh>

### param set PWM_MAIN_TIM0 -4
param set PWM_MAIN_TIM0 -4
+ PWM_MAIN_TIM0: curr: -3 -> new: -4
nsh>

### param show PWM_MAIN_TIM0
param show PWM_MAIN_TIM0
Symbols: x = used, + = saved, * = unsaved
x   PWM_MAIN_TIM0 [706,1018] : -4

 920/1537 parameters used.
nsh>

### param show DSHOT_BIDIR_EN
param show DSHOT_BIDIR_EN
Symbols: x = used, + = saved, * = unsaved
x   DSHOT_BIDIR_EN [287,522] : 1

 920/1537 parameters used.
nsh>

### param status
param status
INFO  [parameters] summary: 920/1537 (used/total)
INFO  [parameters] storage array: 30/1537 elements (256 bytes total)
INFO  [parameters] storage array (custom defaults): 24/1537 elements (256 bytes total)
INFO  [parameters] auto save: on
INFO  [parameters] last auto save: 6.528 seconds ago
param: export: 2 events, 3018us elapsed, 1509.00us avg, min 217us max 2801us 1827.164us rms
param: find: 67224 events
param: get: 2646223 events
param: set: 31 events, 251us elapsed, 8.10us avg, min 3us max 11us 1.599us rms
nsh>

### dshot status
dshot status
INFO  [dshot] Outputs initialized: yes
INFO  [dshot] Outputs used: 0xf
INFO  [dshot] Outputs on: yes
dshot: cycle: 1432 events, 8017us elapsed, 5.60us avg, min 4us max 666us 17.651us rms
dshot: bdshot rpm: 1432 events
dshot: dshot telem: 0 events
INFO  [mixer_module] Param prefix: PWM_MAIN
control latency: 0 events, 0us elapsed, 0.00us avg, min 0us max 0us 0.000us rms
Channel Configuration:
Channel 0: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 1: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 2: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 3: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 4: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 5: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 6: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 7: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
Channel 8: func:   0, value: 0, failsafe: 0, disarmed: 0, min: 109, max: 1999, center: 65535
INFO  [arch_dshot] dshot driver stats:
INFO  [arch_dshot] Bidirectional DShot enabled
INFO  [arch_dshot] Timer 0, Channel 0: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 1: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 2: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
INFO  [arch_dshot] Timer 0, Channel 3: read 0, failed nibble 0, failed CRC 0, invalid/zero 0
nsh>

```

## BF资料整理（2026-11-03）

按用户要求新增 P224-VIO-v1-Betaflight移植资料.md，汇总硬件与实测边界，强调无DRDY、传感器坐标变换、BF独立链接布局和DMA适配。尚未选择BF版本或执行BF编译；未改动设备。

## USB 名称核查（2026-11-03）

Windows COM53 的 DEVPKEY_Device_BusReportedDeviceDesc 实测为 vio-v1，说明固件产品名已上报。FriendlyName 为 USB 串行设备 (COM53)，Device_Manufacturer 为 Microsoft，属于当前 Windows 通用串口驱动的设备属性。固件 CDCACM_VENDORSTR=p224、PRODUCTSTR=vio-v1；NuttX cdcacm_desc.c 用这两个字段生成字符串描述符。此次没有直接读取 USB iManufacturer 原始字符串，因此不把 Windows 的 Microsoft 字段当作固件厂商描述符实测值。未改驱动、注册表或固件。

## QGC 5.1.4 升级识别核查（2026-11-03）

用户确认应用和配套 bootloader 均已烧录。官方 v5.1.4 src/Comms/USBBoardInfo.json 中 1209:0001 无匹配，vio-v1 BL 也无描述备用匹配，自动识别缺口已确认。该版本 PX4FirmwareUpgradeThread.cc 支持手选未知端口：正常应用端口首次看到后等待移除并重新出现，再握手 bootloader；选择端口不会自动把正在运行的应用切为 bootloader。因此“手选后取消”仍需对照插拔步骤/错误日志，不能仅由白名单断言。未修改 USB ID/名称，未烧录或重启。来源 https://github.com/mavlink/qgroundcontrol/tree/v5.1.4/src/Comms 及 src/Vehicle/VehicleSetup/PX4FirmwareUpgradeThread.cc。

## QGC 5.1.4 端口选择消失根因

用户澄清点击 COM53 即丢失选择，尚未进入刷写阶段。已检查官方 v5.1.4 FirmwareUpgrade.qml：onAvailablePortsChanged 调用 _refreshSelection；手选端口若不是 Pixhawk/SiK 则 currentIsRecognized=false，进入 _preselectIndex；没有已知设备时返回 -1，清空 ComboBox 选择。当前 USB/名称不匹配识别规则，因此该逻辑与现象一致。此前建议选中后插拔不解决此阶段问题。正确的 QGC 端修复应在所选 systemLocation 仍存在时保留选择，不要求 recognized；识别表加入 1209:0001 也可用于本开发环境，但该 VID/PID 是通用测试 ID，不能作为通用发行版永久独占映射。来源 https://github.com/mavlink/qgroundcontrol/blob/v5.1.4/src/Vehicle/VehicleSetup/FirmwareUpgrade.qml。未改本机 QGC 或 USB 标识。

## 按用户要求试验 USB 名称（2026-11-03）

用户指定保留板卡 ID，并将厂商/产品名改为 microair h743 aio。按字面拆分厂商 microair、产品 h743 aio（不是 MicoAir 拼写），应用与 bootloader 两份 CDCACM 描述符均修改，USBMFGSTRING 同步。board_id/BOARD_TYPE=60000、VID:PID=1209:0001、引脚及 Flash 布局不变。bootloader 产品字符串此次亦按用户指定不加 BL 后缀；该名称不保证匹配 QGC 5.1.4 规则，仅作为用户要求的试验，尚未烧录验证。通过 GitHub Actions 编译两份固件，不在本机编译。

### 本次名称试验编译与交付

源代码提交：608f85146098a455303a982fcbd1f2b10d89a0ab。

- 应用：GitHub Actions #11 成功，https://github.com/cbcb224/PX4-Autopilot/actions/runs/41583608029 。本地目录 P224-VIO-v1-固件/application-microair-r11，Build ID p224_vio-v1_default-r11-a1-608f8514。
- Bootloader：GitHub Actions #12 成功，https://github.com/cbcb224/PX4-Autopilot/actions/runs/41583611039 。本地目录 P224-VIO-v1-固件/bootloader-microair-r12，Build ID p224_vio-v1_bootloader-r12-a1-608f8514。
- 两个下载 ZIP 的 SHA256 均与 GitHub artifact digest 一致；应用 ZIP 为 1f108de44804693610e07907f2d9ea588e3772c46ffdd82325620d4327a698ff，bootloader ZIP 为 f0131524c4959f5e5e595457a455f66f9f0d8ae1923abc70eef2a500c25b0c68。
- 两份 .px4 解压 image 均与配套 .bin 逐字节相同，board_id 均为 60000；二进制中均检出 microair、h743 aio。应用中检出本次 Build ID。
- 应用 .bin 大小 1482920 字节，SHA256 5b8d2868a900b956364d11a6882551b01a98f4ecb44c5c736df6a777be185ff1，ST-Link 起始地址 0x08020000。
- Bootloader .bin 大小 42392 字节，SHA256 3892a986956882e6686cd64e9f5b605433100aeb15ed48801b49be915ec4356b，ST-Link 起始地址 0x08000000。

本机仅编辑、静态检查、下载和校验，没有本地编译，没有自动烧录、重启或修改设备参数。要让应用与升级模式都显示此次名称，需要更新两份；QGC 5.1.4 的识别效果仍待用户烧录、拔插后实测，编译通过不代表已解决其端口选择问题。

## QGC USB VID/PID 兼容性试验（2026-11-03）

用户反馈名称修改仍不能识别，澄清本次要求修改 USB VID/PID。重新读取官方 QGC v5.1.4 src/Comms/USBBoardInfo.json，确认 9900:17（十六进制 26AC:0011）对应 Pixhawk 类别，名称 PX4 FMU V2。应用与 bootloader 的 CDCACM_VENDORID/PRODUCTID 同步改为此组合，作为识别兼容性试验；该 USB 身份不是本自制板的正式分配。保留 microair / h743 aio 名称以及私有 board_id/BOARD_TYPE=60000，保留引脚、Flash 布局和其余功能。识别后仍只能选择本板配套自定义固件，不应据 USB 名称选择其他硬件的默认固件。此修改针对端口识别阶段，不保证解决后续 bootloader 握手问题。两份固件均由 GitHub Actions 编译，待用户烧录后实测。
来源：https://github.com/mavlink/qgroundcontrol/blob/v5.1.4/src/Comms/USBBoardInfo.json 。

### USB VID/PID 试验编译结果

源码提交 8dfdf86ea7a587c25cd57cf36c50ae7ff747b6c7。
应用 Actions #13 成功：https://github.com/cbcb224/PX4-Autopilot/actions/runs/41584640970 。
Bootloader Actions #14 成功：https://github.com/cbcb224/PX4-Autopilot/actions/runs/41584644264 。

下载 ZIP 的 SHA256 均与 GitHub artifact digest 匹配：应用 c1da91b8729cfe1944f7a69502f5e8f7447c58fc3b2d17f1498723e99b83abda，bootloader 1aaf5ecfed9578af9e3cf6ffc4758a7f37475570405a7d0ebce22b2feba5b118。
两份二进制设备描述符均实测 VID=26AC、PID=0011；厂商/产品字符串检出 microair / h743 aio；.px4 board_id 均为60000，解压 image 与 .bin 逐字节匹配，Build ID 与本次任务吻合。
- application-usbid-r13/p224_vio-v1_default.bin：1482928 字节，SHA256 25da905698db12f5153436750d56f8479b860a0ee7f8d0cb8593971d528469b8。
- bootloader-usbid-r14/p224_vio-v1_bootloader.bin：42392 字节，SHA256 157d3cdcdabca419e959e110d3b83f30390cf8bcfb9f9d6bfac3d8b78aae1bea。

应用烧录地址 0x08020000，bootloader 烧录地址 0x08000000；两份均需更新。未本地编译，未自动烧录或重启，未修改设备参数。更换 USB 身份后 Windows 可能重新分配 COM 号；实际 QGC 端口识别和升级握手待用户烧录验证。

## 按 MicoAir H743 AIO USB ID 恢复原名称（2026-11-03）

用户反馈 26AC:0011 已可识别，并要求改用 microair / h743 aio 的 USB VID/PID、名称改回原来的。根据厂商文档对应的 PX4 目标 micoair_h743-aio，直接核对本仓库 v1.17.0 boards/micoair/h743-aio/nuttx-config：nsh 使用 VID:PID=1B8C:0036，bootloader 使用3162:004B；两种模式本来就是不同组合，分别照此应用，而不是假设一个统一 ID。这两组 ID 在 QGC 5.1.4 表中分别按 PX4 FMU V6U、PX4 DurandalV1 分类，显示名称未必是 MicoAir，但均为 Pixhawk 类。USB 身份仅作本板兼容性实验，不改变自制硬件身份或引脚配置。恢复厂商名 p224，应用产品名 vio-v1，bootloader 产品名 vio-v1 BL，USBMFGSTRING=p224。board_id/BOARD_TYPE 继续为60000。仅改 USB 描述符，固件仍须使用本板配套自定义文件。两份 GitHub Actions 编译，不本地编译，不自动烧录。
厂商目标来源：https://micoair.com/en/docs/flight-controller/micoair743-aio-series/micoair743-aio-35a-manual 。

### MicoAir ID / P224 名称编译结果

源码提交 794e671ef2c1188a73ff4c73fe0ca32e5a4ec46f。
应用 Actions #15 成功：https://github.com/cbcb224/PX4-Autopilot/actions/runs/41585684504 。
Bootloader Actions #16 成功：https://github.com/cbcb224/PX4-Autopilot/actions/runs/41585687388 。
下载 ZIP 的 SHA256 均与 GitHub artifact digest 匹配：应用 1524bc72f4dae78dc7427ff701df316593d78204792dcbf92c214143a77886ab，bootloader d8ba5b423e1f83c27dbe998f44f6489f61a194ee5c94acb352dd7024b91cbba6。
二进制 USB 设备描述符实测应用=1B8C:0036、bootloader=3162:004B，原厂商名 p224、对应产品名 vio-v1 / vio-v1 BL 均检出。两份 .px4 board_id=60000，解压 image 与 .bin 逐字节一致，Build ID 正确。
- application-micoid-r15/p224_vio-v1_default.bin：1482920 字节，SHA256 236b9cac5710b7c4ad04b21df3b1cc9749d719bf77aad15afbf28eab20252cd3。
- bootloader-micoid-r16/p224_vio-v1_bootloader.bin：42392 字节，SHA256 95784d28ce035dadb0eca50d2a9c7535ef1115e695878602b85cd4110f4db6de。

应用烧录地址 0x08020000，bootloader 地址 0x08000000；两份均需更新。未本地编译，未自动烧录或重启。此组合仍待用户实测。

## QGC 成功升级后的实机检查（2026-11-03）

用户提供 QGC 完整升级日志：发现 PX4 FMU V2 (COM55)，bootloader 版本5、board ID60000、Flash1835008字节；选用 application-micoid-r15/p224_vio-v1_default.px4，擦除、写入、校验完成并重启。表明本板经QGC自定义固件升级链路已成功。日志名称不能据此证明最新r16 bootloader已烧录；本次未重启进入bootloader读取其USB ID。

电脑当前实测应用端口 COM59，USB VID:PID=1B8C:0036，序列号0；Windows BusReportedDeviceDesc=vio-v1，产品名恢复已验证。首次读取串口拒绝访问，QGC进程仍运行；用户关闭QGC后串口访问成功。

只读 MAVLink shell 检查：
- ver all：HW P224_VIO_V1，PX4 1.17.0，git794e671ef2c1188a73ff4c73fe0ca32e5a4ec46f，Build URI p224_vio-v1_default-r15-a1-794e671e，NuttX12.12.0。已确认最新应用实际运行。
- commander：Disarmed，Hold，in failsafe=no。该结果不代表飞行前检查全部通过。
- sensors：两组gyro、accel，以及MAG396833、BARO6845985均state OK、conf1.0，无validator failsafe；gyro约1999.7Hz、accel约399.9Hz。每组IMU data gap累计1次，本次快照不能判断其是否仅为启动事件。气压计温度约65.9°C，与前次读数相近，未据此判定硬件过热。GPS和空速无数据。
- dshot：前四路Outputs used=0xf，Bidirectional DShot enabled；PWM_MAIN_TIM0=-4、DSHOT_BIDIR_EN=1，BDShot300配置保留。九路func均0，未配置电机功能；四路read0，无有效转速回传，不能据此宣称电调端到端验证完成。

本次未改参数、未解锁、未测试电机、未重启、未检查SD卡。所有串口查询完成后已释放COM59，可以重新打开QGC。

## 上电 USB 身份捕获（2026-11-03，Asia/Shanghai）

用户配合拔插USB，使用serial.tools.list_ports约80ms轮询，仅枚举而不打开串口。捕获：14:09:42 USB端口消失；14:09:48 COM55，VID:PID=26AC:0011，description=Legacy FMU (COM55)，serial=0，LOCATION=1-1.2；14:09:51端口消失；14:09:53 COM59，VID:PID=1B8C:0036，serial=0，LOCATION=1-1.2。同一物理USB位置从bootloader跳转至应用。

结论：当前bootloader实际仍为此前26AC:0011配置，最新r16预期3162:004B尚未在设备生效，不能仅归因于Windows显示名缓存。应用则为新1B8C:0036配置；此前QGC刷写应用成功不会自动更新0x08000000处bootloader。若需两种模式都采用MicoAir AIO参考ID，应使用ST-Link将bootloader-micoid-r16/p224_vio-v1_bootloader.bin写入0x08000000，不必因此重刷当前已确认r15应用。此次未烧录、未改参数、未主动重启。

## 更新 bootloader 后再次捕获（2026-11-03，Asia/Shanghai）

用户更新后再次配合拔插，USB只读枚举捕获：14:12:46端口消失；14:12:50 COM49，VID:PID=3162:004B，serial0，LOCATION=1-1.2；14:12:54端口消失；14:12:56 COM59，VID:PID=1B8C:0036，serial0，LOCATION=1-1.2。确认bootloader新USB身份已生效，并正常跳转到应用；两种模式的USB ID现均符合micoair/h743-aio参考配置。此检查未读取bootloader二进制哈希，不据枚举结果单独断言构建版本。未改设备参数、未打开串口、未主动重启或烧录。监测完成已停止。

## ELRS / UART7 接收机端口参数补齐（2026-11-03）

用户找不到接收机串口参数，确认接收机为ELRS、接UART7。核对当前板级default.px4board未启用DRIVERS_RC_CRSF_RC，也未启用通用rc_input，因此CRSF驱动的module.yaml不会生成RC_CRSF_PRT_CFG参数。新增CONFIG_DRIVERS_RC_CRSF_RC=y；rc.board_defaults加入param set-default RC_CRSF_PRT_CFG 103，对应TELEM3=/dev/ttyS2=物理UART7。仅设置默认值，不强行覆盖已保存参数；新固件生效后仍应检查实际参数及该口是否被其他服务占用。RC_CRSF_TEL_EN仍保留上游默认值0，本次不擅自启用接收机回传。UART7 RX接ELRS TX，UART7 TX接ELRS RX（需要双向通信时），共地；供电须符合接收机规格。

仅重编应用，bootloader及USB标识均无需改动。GitHub Actions编译，不本地编译，未操作电机。源码依据src/drivers/rc/crsf_rc/{Kconfig,module.yaml}与Tools/serial/generate_config.py的TEL3 index103映射。

用户补充RC_PORT_CONFIG也不存在。补入CONFIG_DRIVERS_RC_INPUT=y，生成RC_PORT_CONFIG及RC_INPUT_PROTO；默认RC_PORT_CONFIG=0，本板无RC_SERIAL_PORT回退定义，因此通用rc_input不占用串口。当前ELRS继续使用专用crsf_rc，默认RC_CRSF_PRT_CFG=103。不要将两个参数同时指向TELEM3。两类驱动同时编入仅提供配置选择，不应同时读取同一UART。

### 接收机驱动补齐编译交付

应用Actions #17成功：https://github.com/cbcb224/PX4-Autopilot/actions/runs/41612326052 ，源码a9e6bf98b7c206e0925fa3e39162b99e29b68915，Build ID p224_vio-v1_default-r17-a1-a9e6bf98。下载目录P224-VIO-v1-固件/application-elrs-r17，ZIP SHA256=f7381d7737811d853d9b53f47aef3e75d0f8095afd22377e6a039b7927590533，与GitHub digest一致。应用bin1503480字节，SHA256=761455f7d2126334625cf8871921d4309fb91714c0dfe228c1ce75adc6e3f889。

产物检查：bin中检出RC_PORT_CONFIG、RC_INPUT_PROTO、RC_CRSF_PRT_CFG、RC_CRSF_TEL_EN和本次Build ID；ELF符号表检出rc_input_main、crsf_rc_main，确认两驱动已编入；board_id60000、USB1B8C:0036保持，.px4解压image与bin逐字节一致。本机未编译、未烧录、未改设备参数。QGC选择本目录p224_vio-v1_default.px4即可更新应用，bootloader不用改。更新后检查RC_CRSF_PRT_CFG=103(TELEM3)，RC_PORT_CONFIG=0(Disabled)，重启应用后生效；ELRS端到端接收仍待实机验证。UART7 RX=PB3接接收机TX，UART7 TX=PA15接接收机RX，共地。

## 电池显示 -100% 排查（2026-11-03）

用户报告万用表12.37V、显示-100%，确认供电是可调电源而非电池。COM59只读检查确认运行r17；battery_status voltage_v=12.27155、connected=True、remaining=-1、cell_count=0、warning=3，ADC18原始24040、参考3.3V/65536，采样有效。BAT1_N_CELLS=0、BAT1_CAPACITY=-1、BAT1_V_DIV=10.0909、BAT1_V_CHARGED=4.05、BAT1_V_EMPTY=3.6、BAT1_I_OVERWRITE=0；电流0不能用作电流采样校准证据。源码battery.cpp在n_cells==0时返回SOC=-1，MAVLink发送乘100产生-100，属于无有效剩余电量估计，不是负电量或无电压。

万用表与飞控读数相差约0.10V（约0.8%），不是-100%的原因。可调电源本身无真实剩余电量，暂不擅设电池串数、容量或分压；若模拟3S可另行设置N_CELLS=3，得到模拟电压百分比；实际用电池时按真实串数/化学体系/容量配置。精确校准分压需要同时在飞控VBAT/GND端测量，再按现有分压乘实测电压/飞控电压校正。本次未改参数、未重启、未解锁，查询后释放串口。

## 仅见红灯的初步核查（2026-11-03）

用户报告只见红灯，未见绿蓝。文档/固件引脚一致：PC1红、PA0绿、PC0蓝，共阳低有效；led.c按低电平点亮，非CONFIG_ARCH_LEDS配置映射LED_BLUE=0至PC0、LED_RED=1至PC1、LED_GREEN=3至PA0。当前BOARD_OVERLOAD_LED=LED_RED、BOARD_ARMED_STATE_LED=LED_BLUE，未定义BOARD_ARMED_LED；没有把绿灯配置为常态状态指示。CONFIG_COMMON_LIGHT虽包含I2C RGB驱动，但板载三路GPIO没有对应的led_control订阅驱动，不能假设led_control命令能直接点亮这些GPIO。

设备只读查询：led_control color1(红)、mode5(快闪)、priority0；commander Disarmed，Altitude，failsafe=no。该消息与commander飞行前检查未通过时的红色快闪规则一致，但尚未读取vehicle_status确认pre_flight_checks_pass。软件消息不等同物理GPIO实测，不能据消息或红灯现象判定绿蓝LED损坏。现有蓝灯按commander板级状态逻辑应闪烁，还需区分物理红灯是否独立电源灯，以及逐色测试。现有应用没有gpio或tests命令，test是NSH条件判断，并非LED硬件测试。暂未修改源码、未执行点灯命令、未改参数或重启；等待用户确认红灯常亮/闪烁及独立电源灯情况。

### GPIO RGB 状态驱动补齐

用户确认红灯闪烁。现有蓝灯legacy输出与用户观察颜色不符，不能以此确认物理颜色映射或硬件损坏。新增可选src/drivers/lights/rgbled_gpio驱动：按GPIO_nLED_RED/GREEN/BLUE低有效直接控制一个RGB灯，通过上游LedController处理led_control颜色、闪烁及优先级，不用PWM、不占定时器或DMA。板级启用驱动，rc.board_extras启动；删除BOARD_HAS_CONTROL_STATUS_LEDS/BOARD_OVERLOAD_LED/BOARD_ARMED_STATE_LED，以免commander legacy逻辑与RGB驱动抢同组GPIO。保留led.c初始化及启动阶段故障指示。普通GPIO只支持亮灭，不提供渐变亮度。通过led_control on -c red/green/blue测试并以led_control reset释放高优先级覆盖（具体reset命令以源码确认）。板卡/USB/电机/接收机/电池参数不变。仅应用云端编译；尚未烧录，尚未验证物理颜色。测试只改LED，不能为测试灯色而解锁飞控。
