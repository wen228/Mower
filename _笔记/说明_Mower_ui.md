# Mower_ui 工程说明

路径：本仓库根目录（`Documents/Codes/Mower_ui`）  
分支示例：`App/Mower`  
主控：M5 **CoreS3 / CoreS3 Lite** + LVGL 多页面 UI  

> **本仓库从哪来：** 基于 M5Stack **CoreS3-UserDemo** 裁剪、改写，  
> 用于 TTI 智能割草机 **应用层 UI + 电机控制**，不是官方 UserDemo 原样。  
> 官方 UserDemo 的完整应用清单见同目录 **`说明_CoreS3.md`**（对照用，不等于本仓现状）。

---

## 1. 工程定位

| 项 | 说明 |
|----|------|
| 框架 | PlatformIO + Arduino + M5Unified/M5GFX + LVGL 8 + PageManager |
| 页面模型 | 每页 **View / Model / AppXxx（控制）**，与 UserDemo 相同结构 |
| 业务重点 | **AppMower** + 全局 **`MotorService`**（Port A I2C Roller） |
| 独立电机 CLI | 见旁路工程 `Documents/Codes/Mower`（无 LVGL，纯串口/简易屏） |
| 电机协议参考 | `说明_Roller485.md`（M5Unit-Roller 库与示例） |

已从 UserDemo **删除**（本仓无源码、无注册）：

- AppWiFi、AppCamera、AppMic、AppSD  

仍保留部分演示页便于板级自检：Power / IMU / Touch / I2C / RTC。

---

## 2. 架构说明（页面 MVC + 全局电机服务）

### 2.1 页面层（与 UserDemo 同套路）

| 本工程 | 类比 | 职责 |
|--------|------|------|
| **View**（`*View.cpp`） | UI | LVGL 布局、按钮、标签 |
| **Model**（`*Model.cpp`） | 数据 | 传感器等；AppMower 的 Model 很薄 |
| **AppXxx.cpp** | 控制 | 事件、定时器、`Replace` 跳转 |

生命周期：

```
进页: View.Create → 绑事件 / Model 初始化
在页: 定时器或点击 → 读数据 → 改 View
出页: 删定时器、View.Delete、必要时释放硬件
```

### 2.2 电机层（本仓新增，跨页面）

| 模块 | 路径 | 职责 |
|------|------|------|
| **MotorService** | `src/motor/MotorService.*` | 启停、三档、急刹、清故障、堵转/错误 poll、串口命令 |
| **mower_config** | `include/mower_config.h` | Port A 引脚、档位转速、5V/12V profile |
| **M5UnitRoller** | `lib/M5UnitRoller/` | `UnitRollerI2C` 驱动（从 Mower 工程拷入） |

- `App_Init()` 里调用 `MotorService::begin()`  
- `loop()` 里始终：`handleSerial()` + `poll()`（**换到别的 App 也仍监控故障**）  
- UI 只调 `MotorService`，不直接 `setOutput`  

硬件默认（`mower_config.h`）：

| 项 | 值 |
|----|-----|
| 总线 | CoreS3 **Port A** I2C |
| SDA / SCL | G2 / G1 |
| 地址 | `0x64` |
| 模式 | 速度模式 Speed |
| 档位 | Eco 30% / Normal 70% / Turbo 100%（相对 `SPEED_FULL`） |
| 默认供电 profile | `MOWER_PWR_PROFILE=0`（5V Grove 验证，`SPEED_FULL=60000`） |

---

## 3. 当前已注册页面总表

注册：`src/pages/AppFactory.cpp` + `src/App.cpp` 的 `Install`。

| # | 页面 | 目录 | 作用一句话 |
|---|------|------|------------|
| 1 | **StartUp** | `StartUp/` | 开机动画 + 音效，点击进主菜单 |
| 2 | **HomeMenu** | `HomeMenu/` | 主菜单入口 |
| 3 | **AppMower** | `AppMower/` | **割草机电机：三档 / Toggle / E-STOP / Reset + 遥测** |
| 4 | **AppPower** | `AppPower/` | 电池/USB 电压、电源模式 |
| 5 | **AppIMU** | `AppIMU/` | 姿态球 + 罗盘（CoreS3） |
| 6 | **AppTouch** | `AppTouch/` | 触摸画线测试 |
| 7 | **AppI2C** | `AppI2C/` | 内部 / PortA/B/C I2C 扫描 |
| 8 | **AppRTC** | `AppRTC/` | 关机、亮度、深睡、RTC 定时唤醒 |

另有 **`_Template/`**：新页模板，**未**注册。

---

## 4. 主菜单入口（以源码为准）

`HomeMenuView`：第一排 4 个演示图标 + 网格第 5 格 **MOWER** 文字钮 + 右上角 Sys。

| 控件 | 目标页 |
|------|--------|
| `imgbtn_list[0]` | AppPower |
| `imgbtn_list[1]` | AppIMU（`M5CORES3`） |
| `imgbtn_list[2]` | AppTouch |
| `imgbtn_list[3]` | AppI2C |
| `btn_mower` | **AppMower** |
| `imgbtn_list[5]`（右上 Sys） | AppRTC |

---

## 5. 各应用说明（本仓现状）

### 5.1 StartUp — 启动页

开机音 `poweron_2_5s`、动画与渐亮 → 点击 → `HomeMenu`。逻辑与 UserDemo 同源。

### 5.2 HomeMenu — 主菜单

纯导航。入口见上表。

### 5.3 AppMower — 割草机控制（业务页）

| 层 | 干什么 |
|----|--------|
| View | Home/Next、状态与遥测文字、Eco/Normal/Turbo、RUN·STOP、E-STOP、Reset |
| Model | 薄封装，读 `MotorService::telemetry()` |
| 控制 | 按钮 → `MotorService::*`；约 200ms 刷新 UI |

**界面：**

- Eco / Normal / Turbo：切档（运行中会改目标速度）  
- RUN/STOP：Toggle 正常启停  
- **E-STOP**：立刻停机 + **fault 闩锁**，须 Reset 后才能再启  
- **Reset**：清故障、重开堵转保护，保持停止  

**串口**（`USBSerial`，工程内 `begin(115200)`，任意页面可用）：

| 键 | 动作 |
|----|------|
| `t` / 空格 | Toggle 启停 |
| `1` / `2` / `3` | Eco / Normal / Turbo |
| `e` | E-STOP（闩锁） |
| `r` | 清故障 |
| `s` | 打一行状态 |
| `h` / `?` | 帮助 |

**安全：** HW jam / 过压 / 软堵转会停机并闩锁；与 CLI 版 `Mower` Phase1 同一套思路。

**Next：** `AppMower` → `AppPower`。

### 5.4 AppPower — 电源

AXP 采样、电池/USB/Bus 模式。与 UserDemo 同源，本仓保留。

### 5.5 AppIMU — 惯性/罗盘（CoreS3）

BMI270 + BMM150 + Madgwick；顶栏可触发磁力校准。

### 5.6 AppTouch — 触摸测试

下半屏画线，顶栏清屏。

### 5.7 AppI2C — I2C 扫描

Internal / Port A / B / C 扫地址。  
**注意：** 扫 Port A 时会占用与 **Roller 相同的 Port A**；测电机时尽量别同时乱扫 Port A。

Port 宏（`include/config.h`）：

| 端口 | GPIO（工程宏） |
|------|----------------|
| Port A | `PORTA_PIN_0=1`（SCL）、`PORTA_PIN_1=2`（SDA）— 与 Roller 一致 |
| Port B | 8 / 9 |
| Port C | 18 / 17 |
| 内部 I2C | SDA=12、SCL=11 |

### 5.8 AppRTC — 系统/休眠

关机 / 亮度 / 回菜单 / 深睡 / RTC 定时唤醒。

### 5.9 `_Template` — 新页模板

复制目录 → 改类名 → `AppFactory` 增加 `APP_CLASS_MATCH` → `App.cpp` `Install` → `HomeMenu` 加入口。

---

## 6. 页面关系（简图）

```
开机
  └─ StartUp ──点击──► HomeMenu
                         ├─ AppMower   ★ 电机
                         ├─ AppPower
                         ├─ AppIMU
                         ├─ AppTouch
                         ├─ AppI2C
                         └─ AppRTC

各 App：Home → HomeMenu；Next 见各页 onEvent
主循环：MotorService::handleSerial + poll（全局）
```

---

## 7. 编译与烧录

```bash
cd /Users/nyufufu/Documents/Codes/Mower_ui
~/.platformio/penv/bin/pio run -e M5CoreS3
~/.platformio/penv/bin/pio run -e M5CoreS3 -t upload
```

- 默认 env：`platformio.ini` → `M5CoreS3`  
- 串口监控建议 **115200**  
- 无 Roller 时 UI 仍可进，状态显示 `NO ROLLER`，串口会报未找到  

---

## 8. 相关文件速查

| 用途 | 路径 |
|------|------|
| 页面工厂 | `src/pages/AppFactory.cpp` |
| 应用初始化 | `src/App.cpp` |
| 主循环 | `src/main.cpp` |
| 电机服务 | `src/motor/MotorService.h` / `.cpp` |
| 电机配置 | `include/mower_config.h` |
| 板级/UserDemo 宏 | `include/config.h` |
| Roller I2C 库 | `lib/M5UnitRoller/` |
| 资源池 | `src/res/ResourcePool.*` |
| 页面管理器 | `lib/PageManager/` |
| 原版 UserDemo 清单（对照） | `说明_CoreS3.md` |
| Roller 库与示例 | `说明_Roller485.md` |

---

## 9. 和另外两份说明的关系

| 文档 | 写什么 | 不要当成 |
|------|--------|----------|
| **本文件 `说明_Mower_ui.md`** | **本仓库当前代码与用法** | 官方 UserDemo 全量功能 |
| **`说明_CoreS3.md`** | 上游 CoreS3-UserDemo 应用清单（历史/对照） | 本仓已注册页面列表 |
| **`说明_Roller485.md`** | M5Unit-Roller 库与官方 examples | 本仓 LVGL 页面结构 |
