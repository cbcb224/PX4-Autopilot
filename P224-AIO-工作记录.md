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
