# M5Unit-Roller 工程架构与示例清单

路径：`/Users/nyufufu/Documents/Codes/M5Unit-Roller`  
官方库：`M5UnitRoller`（Arduino / PlatformIO）  
硬件：

- **Roller485**（SKU U182 / U182-Lite）— FOC 无刷电机，**RS485 或 I2C**
- **RollerCAN**（SKU U188 / U188-Lite）— 同系电机，**CAN 或 I2C**

文档：

- [Unit-Roller485](https://docs.m5stack.com/en/unit/Unit-Roller485)
- [Unit-RollerCAN](https://docs.m5stack.com/en/unit/Unit-RollerCAN)
- 仓库：[m5stack/M5Unit-Roller](https://github.com/m5stack/M5Unit-Roller)

---

## 1. 工程性质（和 CoreS3-UserDemo 的区别）

| | CoreS3-UserDemo | M5Unit-Roller |
|--|-----------------|---------------|
| 类型 | 完整固件 / 多页面 Demo | **驱动库 + 官方示例** |
| 入口 | `main.cpp` + PageManager | 各 `examples/**/*.ino` |
| 应用层 | LVGL 页面 MVC | 示例脚本：`setup`/`loop` 调库 API |
| 用途 | 主控 UI 与外设演示 | **控制 Roller 电机 / 经总线读写从设备** |

本仓库 **没有** 类似 UserDemo 的「HomeMenu / AppWiFi」多应用壳；  
「应用」= **examples 目录下的示例程序**。库本体在 `src/`。

---

## 2. 工程目录树

```
M5Unit-Roller/
├── platformio.ini          # PIO 配置（默认编译 examples/i2c/motor）
├── library.json            # PlatformIO 库元数据
├── library.properties      # Arduino 库元数据
├── README.md
├── LICENSE
├── compile_commands.json   # IDE 索引（本地生成）
│
├── src/                    # ★ 驱动库源码
│   ├── unit_roller_common.hpp / .cpp   # 公共枚举、I2C 寄存器表、互斥/工具
│   ├── unit_rolleri2c.hpp / .cpp       # I2C 控制（Grove 直连）
│   ├── unit_roller485.hpp / .cpp       # RS485 控制（+ 485 桥接 I2C 从机）
│   └── unit_rollercan.hpp / .cpp       # CAN 控制（ESP32 TWAI）
│
└── examples/               # ★ 示例「应用」
    ├── i2c/
    │   ├── motor/                    # I2C 电机四模式演示
    │   └── encoder_calibration/      # 磁编码器校准
    ├── rs485/
    │   ├── motor/                    # RS485 电机控制 + 读状态
    │   ├── slaveMotor/               # 经 485 配置从机 I2C 电机
    │   └── sensor/                   # 经 485 桥接读外部 I2C 传感器
    │       ├── readScd4x/
    │       ├── readSht3x/
    │       ├── readTof/
    │       └── readUltrasonic/
    └── can/
        ├── motor/
        ├── slaveMotor/
        └── sensor/                   # 结构同 rs485/sensor
            ├── readScd4x/
            ├── readSht3x/
            ├── readTof/
            └── readUltrasonic/
```

源码体量约（行数量级）：

| 文件 | 约行数 |
|------|--------|
| `unit_roller485.*` | ~2600 |
| `unit_rollercan.*` | ~4200 |
| `unit_rolleri2c.*` | ~1600 |
| `unit_roller_common.*` | ~320 |

---

## 3. 架构分层

```
┌─────────────────────────────────────────────┐
│  examples/*.ino  （应用层：业务演示）          │
│  M5.begin → begin 总线 → setMode/setSpeed…   │
└──────────────────┬──────────────────────────┘
                   │ 调用
┌──────────────────▼──────────────────────────┐
│  UnitRollerI2C / UnitRoller485 / UnitRollerCAN │
│  （协议封装：寄存器写 / 帧组包 / 应答校验）     │
└──────────────────┬──────────────────────────┘
                   │ 依赖
┌──────────────────▼──────────────────────────┐
│  unit_roller_common                         │
│  模式枚举、错误码、I2C 寄存器地址、工具函数   │
└──────────────────┬──────────────────────────┘
                   │ 底层
┌──────────────────▼──────────────────────────┐
│  Arduino: Wire / HardwareSerial / TWAI(CAN)  │
│  + M5Unified（示例里初始化板子）              │
└─────────────────────────────────────────────┘
```

### 3.1 公共层 `unit_roller_common`

| 内容 | 说明 |
|------|------|
| `roller_mode_t` | 1 速度 / 2 位置 / 3 电流 / 4 编码器 |
| `roller_bps_t` | 485：115200/19200/9600；CAN：1M/500k/125k |
| `roller_rgb_t` | RGB 默认 / 用户色 |
| `roller_errcode_t` | 写成功、CRC 失败、串口超时等 |
| `I2C_*_REG` | 整表 I2C 寄存器映射（输出、模式、速度、位置、电流、PID、温度、Vin…） |
| `I2C_ADDR` | 默认 **0x64** |
| 互斥 / `handleValue` | 多线程安全与 4 字节拼 int32 |

### 3.2 I2C 驱动 `UnitRollerI2C`

- 直连 Grove I2C（CoreS3 Port A：**SDA=G2，SCL=G1**）。
- 典型用法：

```cpp
UnitRollerI2C RollerI2C;
RollerI2C.begin(&Wire, 0x64, 2, 1, 400000);  // CoreS3 Port A
RollerI2C.setMode(ROLLER_MODE_SPEED);
RollerI2C.setSpeed(...);
RollerI2C.setSpeedMaxCurrent(...);
RollerI2C.setOutput(1);
// 读回：getSpeedReadback / getTemp / getVin / getErrorCode ...
```

**TTI 割草机 + CoreS3 Lite 优先参考这条路径。**

### 3.3 RS485 驱动 `UnitRoller485`（本项目重点）

- 经 `HardwareSerial` 发自定义帧（含 **CRC8**、应答校验）。
- 每个电机用 **id**（0~255）寻址。
- API 分两大类：

| 类别 | 作用 | 示例 API |
|------|------|----------|
| **本机 485 命令** | 直接控这台 Roller | `setMode` / `setSpeedMode` / `setOutput` / `getActualSpeed`… |
| **485→I2C 桥** | 经电机顶上 Grove 访问从设备 | `readI2c` / `writeI2c` / `writeSpeedMode` / `readTemp`… |

典型 RS485 初始化（示例引脚 16/17，可按板子改）：

```cpp
UnitRoller485 Roller485;
HardwareSerial mySerial(1);
Roller485.begin(&mySerial, 115200, SERIAL_8N1, 16, 17, -1, false, 10000UL, 112U);
Roller485.setMode(motorId, ROLLER_MODE_SPEED);
Roller485.setSpeedMode(motorId, 2400, 1200);  // 速度 + 限流
Roller485.setOutput(motorId, true);
```

### 3.4 CAN 驱动 `UnitRollerCAN`

- ESP32 **TWAI** CAN。
- API 风格类似 485（模式、速度、位置、电流、从机 I2C 等）。
- 示例默认 `beginCAN(1000000, 16, 17)`，电机 ID 常用 `0xA8`。

---

## 4. 电机四种工作模式

| 模式 | 枚举 | 含义 | 应用场景（TTI） |
|------|------|------|-----------------|
| Speed | `ROLLER_MODE_SPEED` | 转速闭环 + 限流 | **割草刀转速控制（一阶段核心）** |
| Position | `ROLLER_MODE_POSITION` | 位置闭环 + 限流 | 定点/行程 |
| Current | `ROLLER_MODE_CURRENT` | 电流/力矩 | 负载/堵转相关实验 |
| Encoder | `ROLLER_MODE_ENCODER` | 编码器/拨码计数 | 标定、位置记忆 |

公共保护相关能力（库 API 均有）：

- 堵转保护 / 解除保护（`setJamProtection` / `setRemoveProtection` 等）
- 温度、输入电压、实际电流/转速/位置读回
- 参数存 Flash、改波特率、改电机 ID、RGB / 按键切模式

---

## 5. 全部示例「应用」清单

### 总表

| # | 路径 | 总线 | 作用一句话 |
|---|------|------|------------|
| 1 | `examples/i2c/motor` | I2C | 四模式轮换演示 + 状态打印 |
| 2 | `examples/i2c/encoder_calibration` | I2C | 磁编码器校准流程 |
| 3 | `examples/rs485/motor` | RS485 | 速度模式使能 + 循环读温/压/速/流等 |
| 4 | `examples/rs485/slaveMotor` | RS485→I2C | 主电机 485 控本机，并写从机电机寄存器 |
| 5 | `examples/rs485/sensor/readScd4x` | RS485→I2C | 经桥读 SCD4x CO2 传感器 |
| 6 | `examples/rs485/sensor/readSht3x` | RS485→I2C | 经桥读 SHT3x 温湿度 |
| 7 | `examples/rs485/sensor/readTof` | RS485→I2C | 经桥读 ToF 测距 |
| 8 | `examples/rs485/sensor/readUltrasonic` | RS485→I2C | 经桥读超声波 |
| 9 | `examples/can/motor` | CAN | CAN 速度模式基础控制 |
| 10 | `examples/can/slaveMotor` | CAN→I2C | 类似 485 的从机配置 |
| 11 | `examples/can/sensor/*` | CAN→I2C | 与 rs485/sensor 同结构四传感器 |

---

### 1. `i2c/motor` — I2C 电机演示

| 层 | 干什么 |
|----|--------|
| 应用 | `setup` 初始化；`loop` 依次切电流/位置/速度/编码器 |
| 库 | `UnitRollerI2C` |
| 硬件 | Grove I2C（示例默认 SDA=2 SCL=1，**正是 CoreS3 Port A**） |

**逻辑一句话：** 每种模式 `setOutput(0)` → 设模式与目标 → `setOutput(1)` → 打印设定值与 readback。

对 TTI：可直接当 **CoreS3 + Roller I2C 起手模板**。

---

### 2. `i2c/encoder_calibration` — 编码器校准

| 层 | 干什么 |
|----|--------|
| 应用 | 检测设备 → 延时提示 → 触发工厂校准寄存器流程 |
| 库 | `UnitRollerI2C` + `START_ANGLE_CAL_REG` 等 |

**逻辑一句话：** 出厂/检修用磁编码器标定，非日常运行逻辑。

---

### 3. `rs485/motor` — RS485 电机（主示例）

| 层 | 干什么 |
|----|--------|
| 应用 | `begin` 串口 → 速度模式 + 使能 → loop 读温度/Vin/编码器/速度/位置/电流/模式/错误/RGB/PID |
| 库 | `UnitRoller485` |
| 注意 | 示例 UART 脚 **16/17**，接到不同主控时要改 |

**逻辑一句话：** 485 控电机 + 状态遥测，对应 TTI 物料表「Roller485 BLDC」路径。

---

### 4. `rs485/slaveMotor` — 485 主 + I2C 从电机

| 层 | 干什么 |
|----|--------|
| 应用 | 本机 `setSpeedMode`；同时 `writeMotorConfig` / `writeSpeedMode*` 配从机 `0x64` |
| 库 | `UnitRoller485` 的 **writeI2c 封装系列** |

**逻辑一句话：** 一台 Roller 当 485 网关，去写另一台 I2C Roller 或同总线从机。

---

### 5–8. `rs485/sensor/*` — 经 485 读传感器

| 示例 | 从机 I2C 用途 |
|------|----------------|
| `readScd4x` | CO2（地址 0x62 等） |
| `readSht3x` | 温湿度 |
| `readTof` | 激光测距 |
| `readUltrasonic` | 超声波 |

**逻辑一句话：** 不直接接主控 I2C，而走 **Roller 的 I2C 透传**（`writeI2c` / `readI2c`）。

TTI 里 ENV III 等若挂在 Roller 顶口可参考；若挂 CoreS3 Port 则更像普通 I2C 传感器库。

---

### 9. `can/motor` — CAN 电机

| 层 | 干什么 |
|----|--------|
| 应用 | TWAI 1Mbps，速度模式，读写速度/电流/输出 |
| 库 | `UnitRollerCAN` |

**逻辑一句话：** 与 485 电机示例对等，总线换成 CAN。

---

### 10–11. `can/slaveMotor` / `can/sensor/*`

与 RS485 侧 **对称**：主电机 CAN 控制 + 从机 I2C 配置 / 传感器透传。

---

## 6. PlatformIO 用法

当前 `platformio.ini`：

```ini
[platformio]
src_dir = examples/i2c/motor   ; 默认编哪个示例
lib_dir = .                    ; 根目录当库，自动编译 src/

[env:m5stack-core-esp32]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = m5stack/M5Unified@^0.1.14
```

**切换示例：** 改 `src_dir`，例如：

```ini
src_dir = examples/rs485/motor
```

编译前确认板型、串口引脚与电源（Roller 推荐 6–16V PWR485 或 5V Grove，见官方文档）。

---

## 7. 与 TTI 智能割草机的对应关系

| TTI 功能 | 本库怎么支撑 |
|----------|--------------|
| BLDC 闭环转速 / PID | `setMode(SPEED)` + `setSpeed`/`setSpeedMode` + PID API |
| 电流监测 / 负载 | `getActualCurrent` / `getCurrentReadback` + 限流设定 |
| 堵转保护 | jam/stall protection API + `getError` |
| 温度保护 | `getTemp` / `getActualTemp`（可再加外部 NTC） |
| 状态显示 | 读 RPM/电流/Vin/Temp 后在 CoreS3 UI 显示 |
| MQTT 上报 | 本库不包含；在 CoreS3 应用层读值后上传 |
| 主控 | CoreS3 / CoreS3-Lite + 本库 I2C 或 RS485 |

**推荐应用起步顺序（应用层）：**

1. 跑通 `examples/i2c/motor`（Port A G2/G1）  
2. 固定 **Speed 模式 + 限流 + 使能/停止** 最小闭环  
3. 循环读 **speed / current / temp / error**  
4. 再接到 CoreS3-UserDemo 式 UI 或独立固件里  

---

## 8. 关键 API 速查（应用层够用）

### I2C（`UnitRollerI2C`）

| 操作 | API |
|------|-----|
| 初始化 | `begin(Wire, 0x64, sda, scl, speed)` |
| 模式 | `setMode(ROLLER_MODE_*)` |
| 使能 | `setOutput(0/1)` |
| 速度 | `setSpeed` / `setSpeedMaxCurrent` / `setSpeedPID` |
| 位置 | `setPos` / `setPosMaxCurrent` / `setPosPID` |
| 电流 | `setCurrent` |
| 读回 | `getSpeedReadback` / `getPosReadback` / `getCurrentReadback` / `getTemp` / `getVin` / `getErrorCode` |

### RS485（`UnitRoller485`）

| 操作 | API |
|------|-----|
| 初始化 | `begin(serial, baud, config, rx, tx, dirPin, …)` |
| 模式/使能 | `setMode(id, mode)` / `setOutput(id, en)` |
| 速度模式 | `setSpeedMode(id, speed, current)` |
| 位置/电流模式 | `setPositionMode` / `setCurrentMode` |
| 保护 | `setJamProtection` / `setRemoveProtection` |
| 读状态 | `getActualSpeed/Position/Current/Temp/Vin` / `getError` / `getStatus` |
| 桥 I2C | `readI2c` / `writeI2c` / `writeSpeedMode` 等 |

返回值：`1`（`ROLLER_WRITE_SUCCESS`）表示成功；负数/0 为错误码（见 `roller_errcode_t`）。

---

## 9. 应用开发时怎么用这份清单

1. 先定总线：**I2C（Port A）** 还是 **RS485（PWR485）**  
2. 在 `examples` 里找同总线的 **motor** 示例抄起  
3. 只保留 TTI 需要的：**速度模式 + 限流 + 读反馈 + 保护**  
4. UI / MQTT 放在 **CoreS3 主工程**，本库只当「电机 Model 层」  
5. 不要从 `unit_roller485.cpp` 改协议；应用层只调公开 API  

---

## 10. 相关文件速查

| 用途 | 路径 |
|------|------|
| 公共定义 | `src/unit_roller_common.hpp` |
| I2C 驱动 | `src/unit_rolleri2c.hpp` / `.cpp` |
| RS485 驱动 | `src/unit_roller485.hpp` / `.cpp` |
| CAN 驱动 | `src/unit_rollercan.hpp` / `.cpp` |
| I2C 电机示例 | `examples/i2c/motor/motor.ino` |
| RS485 电机示例 | `examples/rs485/motor/motor.ino` |
| PIO 配置 | `platformio.ini` |
| 产品说明 | `README.md` |
