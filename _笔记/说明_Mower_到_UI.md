# 从 `Mower/`（Arduino 业务）到 `Mower_ui/`（应用层）的适配说明

> 目的：记住**分工**和**映射**，方便继续在 **Arduino / CLI 侧（`Mower/`）** 加业务；  
> UI 侧先稳定，**不急着把新逻辑堆进应用层**。

---

## 1. 两个工程各干什么

| | **`Mower/`** | **`Mower_ui/`** |
|--|--------------|-----------------|
| 角色 | **电机业务原型 / CLI 验证** | **多页面 UI 固件** |
| 框架 | 单文件 `main.cpp` 式 Arduino | UserDemo：PageManager + LVGL + 多 App |
| 交互 | 串口 + 简易触摸分区 | 各 App 页面 + 全局串口 |
| 电机 | 逻辑全在 `main.cpp` | 抽成 **`MotorService`**，UI 只调用 |
| 目标 | 把档位 / 启停 / 保护跑稳、好改 | 好看、可导航、可扩展其它页 |

一句话：

- **`Mower/`** = 先把 **业务对不对** 做对（算法、阈值、保护）。  
- **`Mower_ui/`** = 同一套业务挂到 **页面壳** 上；壳来自 UserDemo，不是重写电机协议。

---

## 2. 业务是怎么“搬”过去的

```
Mower/src/main.cpp          Mower_ui/
─────────────────          ──────────────────────────────
config.h              →    include/mower_config.h   （档位/引脚/profile）
UnitRollerI2C 用法     →    lib/M5UnitRoller/       （驱动拷贝，协议不改）
toggle / setGear /    →    src/motor/MotorService.* （业务内核）
clearFault / 软堵转 /
串口 t 1 2 3 r …
简易屏 drawUi         →    pages/AppMower/*         （只负责显示和按钮）
（无）                 →    其余 App（Power/IMU…）  （板级自检，与电机弱相关）
```

### 2.1 配置

- `Mower/include/config.h` → `Mower_ui/include/mower_config.h`  
- 仍是：Port A I2C、`0x64`、Eco/Normal/Turbo、`MOWER_PWR_PROFILE`、负载阈值等。

### 2.2 驱动

- 不改 M5 协议实现；把 **I2C Roller 库**放进 `Mower_ui/lib/M5UnitRoller/`。  
- 应用层只调 `setMode` / `setSpeed` / `setOutput` / readback 等公开 API。

### 2.3 业务内核 → `MotorService`（关键）

把原 `main.cpp` 里的**无状态机**收成全局服务，而不是写进某个 LVGL 页：

| 原 `Mower` 概念 | `MotorService` |
|-----------------|----------------|
| `motorToggle` | `toggle()` |
| `setGear` / `nextGear` | `setGear()` / `nextGear()` |
| 用户停机 | Toggle 停（不闩锁） |
| （UI 增强）急刹 | `eStop()` → 停 + **fault 闩锁** |
| `clearFault` / `r` | `clearFault()` |
| loop 里 jam/过压/软堵转 | `poll()` |
| `handleSerial` | `handleSerial()`（键位对齐 + 增加 `e`） |

**为何独立成服务、不塞在 `AppMower` 里？**

1. 换页后电机若还在转，仍要 **poll 保护**。  
2. 串口在 **任意页面** 都能控机。  
3. 以后改业务：优先改 **`Mower/` 验证 → 再同步 `MotorService`**，UI 少动。

### 2.4 UI 壳 → `AppMower`

| 层 | 做什么 |
|----|--------|
| **View** | 按钮、状态文字（不碰 I2C） |
| **Model** | 几乎只读 `MotorService::telemetry()` |
| **AppMower.cpp** | 点击 → `MotorService::*`；定时刷新标签 |

Home 多一个 **MOWER** 入口；工厂 / `Install` 注册页面。  
UserDemo 其它页（Power/IMU/…）保留做板测，**不是**电机业务来源。

### 2.5 主循环挂钩

```
Mower loop:     串口 → 触摸 → 读电机 → 保护 → 打状态/画屏
Mower_ui loop:  MotorService::handleSerial + poll  →  lv_timer_handler
```

电机生命周期：`App_Init()` 里 `MotorService::begin()`（在 `M5.begin` / LVGL 之后）。

---

## 3. 对应关系速查

| 能力 | `Mower/` | `Mower_ui/` |
|------|----------|-------------|
| 三档 | 串口 `1/2/3`、触摸切档 | 同串口 + 三个按钮 |
| 启停 | `t` / 左区触摸 | `t` + RUN/STOP |
| 清故障 | `r` / 右区触摸 | `r` + Reset |
| 急刹闩锁 | （CLI 以故障闩锁为主） | 显式 **E-STOP / `e`** |
| 遥测 | 串口周期打印 + 简易屏 | 串口 + AppMower 标签 |
| 协议/寄存器 | 不改 | 不改 |

---

## 4. 推荐工作流（你接下来优化 Arduino 时）

```
① 在 Mower/ 改业务、串口验证
② 行为稳定后，把同一逻辑合进 MotorService（保持 API 尽量稳定）
③ AppMower 只跟着加按钮/显示（若需要）
④ 文档：说明_Mower_ui.md / 本文件；协议细节仍看 说明_Roller485.md
```

**原则：**

- **业务真相** 先以 `Mower/` + `MotorService` 为准。  
- **UI 不发明第二套状态机**（不要在 View 里再写一套启停）。  
- 新功能（更多保护、调度、日志…）优先在 **Arduino 侧闭环**，再 thin-wrap 到服务层。

---

## 5. 文件锚点

| 侧 | 路径 |
|----|------|
| CLI 业务 | `Mower/src/main.cpp` · `Mower/include/config.h` |
| UI 业务核 | `Mower_ui/src/motor/MotorService.*` |
| UI 配置 | `Mower_ui/include/mower_config.h` |
| UI 页面 | `Mower_ui/src/pages/AppMower/` |
| 挂钩 | `Mower_ui/src/App.cpp` · `main.cpp` |
| 本仓总说明 | `说明_Mower_ui.md` |

---

*一句话记忆：`Mower` 是大脑草稿本；`MotorService` 是同一大脑的库形式；`AppMower` 只是面板。*
