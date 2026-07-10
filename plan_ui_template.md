# Plan: Mower UI 清净应用层模板（feature/ui）

## Context

- 工程：`/Users/nyufufu/Documents/Codes/Mower`
- 当前分支：**`feature/ui`**（已确认）
- 参考：
  - `CoreS3-UserDemo` — PageManager + 页面 MVC（View/Model/控制）
  - `M5Unit-Roller` — 电机库已在 `lib/M5UnitRoller`（本阶段 UI 模板**不绑死**电机页）
- 目标：先做**清净可跑的应用层骨架**，只留少量示例 App，便于后续加 Mower 业务页。
- 你确认计划 OK 后再实现；实现完成且你验收后再 **commit**（本次 plan 批准 ≠ 自动 commit）。

## 范围（做 / 不做）

| 做 | 不做 |
|----|------|
| 精简 Page 框架（参考 UserDemo，去掉 10+ 个演示 App） | 整仓复制 UserDemo 图片/字体/相机/IMU 等 |
| 页面：`StartUp` → `HomeMenu` → `AppWiFi` + `AppScreen` | Camera / Mic / IMU / SD / RTC / I2C / Power 演示 |
| `main` 只做板级 + LVGL + `App_Init` | 在模板阶段把 Roller 逻辑塞进 UI |
| 保留现有 P1 电机代码为独立模块（可串口/后续 AppMotor） | 删除 P1 逻辑 |

## 推荐结构（清净）

```
Mower/
  platformio.ini          # + lvgl，保留 M5Unified / 本地 Roller 库
  include/config.h
  include/lv_conf.h       # 从 UserDemo 精简拷贝
  src/
    main.cpp              # M5.begin → lv_init → App_Init → lv_timer_handler
    App.cpp / App.h       # 注册 4 个页：StartUp, HomeMenu, AppWiFi, AppScreen
    motor/                # 现有 P1 逻辑迁入（MotorService），UI 暂不调用
      MotorService.cpp/h
    pages/
      AppFactory.cpp/h
      Page.h
      _Template/          # 可选：新页模板
      StartUp/
      HomeMenu/           # 2 个入口：WiFi | Screen（以后加 Mower）
      AppWiFi/            # 参考 UserDemo AppWiFi，简化列表 UI
      AppScreen/          # 简单“屏幕/状态”页：分辨率、触摸点、返回
  lib/
    M5UnitRoller/         # 已有，不动协议
    PageManager/          # 从 UserDemo 拷贝精简依赖
    m5gfx_lvgl/           # 显示桥接（UserDemo 同款思路）
```

**页面流：**

```
开机 StartUp（短动画或 1 秒标题）
  → HomeMenu
       → AppWiFi   （扫描 SSID 列表 + 返回）
       → AppScreen （状态/触摸演示 + 返回）
```

## 实现策略

1. **不整仓 fork UserDemo**  
   只抽：`PageManager`、`m5gfx_lvgl`、必要的 `lv_ext`（若有硬依赖再加）、`AppWiFi` 的逻辑骨架。  
   UI 用 **LVGL 内置控件 + 系统字体**，少图或无图，避免 `res/img` 膨胀。

2. **App 数量控制**  
   - `StartUp`：标题 “Mower UI” + 点击/定时进 Home  
   - `HomeMenu`：2 个按钮（WiFi / Screen）  
   - `AppWiFi`：参考 `CoreS3-UserDemo/src/pages/AppWiFi`（Scan + 列表）  
   - `AppScreen`：显示 “320×240 / touch xy / back”，证明触屏与页面切换 OK  

3. **现有 `main.cpp` 电机代码**  
   - 迁到 `src/motor/MotorService.*`  
   - 模板阶段 **默认不自动转电机**（避免 UI 调试误转）  
   - 串口命令可暂时保留在 `MotorService` 或 `#if MOWER_MOTOR_CLI`  

4. **依赖**（`platformio.ini`）  
   - 已有：`M5Unified`、`M5GFX`、本地 Roller  
   - 新增：`lvgl`（版本对齐 UserDemo 思路，优先 8.3.x 以匹配 PageManager API）  
   - build_flags：`-DLV_CONF_INCLUDE_SIMPLE`、`-Iinclude`、屏 320×240  

5. **验证**  
   - `pio run` 通过  
   - 烧录：能进 Home → WiFi 能扫到列表或显示空/失败提示 → Screen 页可返回  
   - 不要求美观  

## 关键参考路径

| 用途 | 路径 |
|------|------|
| 页面工厂/注册 | `CoreS3-UserDemo/src/App.cpp`, `pages/AppFactory.cpp` |
| WiFi 页 | `CoreS3-UserDemo/src/pages/AppWiFi/*` |
| 模板页结构 | `CoreS3-UserDemo/src/pages/_Template/*` |
| PageManager | `CoreS3-UserDemo/lib/PageManager/*` |
| LVGL 桥 | `CoreS3-UserDemo/lib/m5gfx_lvgl/*` |
| 当前电机 P1 | `Mower/src/main.cpp`, `include/config.h` |
| Roller API | `Mower/lib/M5UnitRoller/*` |

## 执行顺序（批准后）

1. 确认仍在 `feature/ui`，工作区干净或可接受改动  
2. 拷贝/精简 `PageManager` + `m5gfx_lvgl` + `lv_conf.h`  
3. 搭 `App` + 4 页骨架  
4. 实现精简 `AppWiFi`、`AppScreen`、`HomeMenu`、`StartUp`  
5. 迁移电机代码到 `motor/`，`main` 变薄  
6. `pio run` 修到编译过  
7. **停住等你验收** → 你点头后再 `git add` / `commit`（消息示例：`ui: clean page template with WiFi + Screen`）

## 明确不做（本迭代）

- 割草机业务 UI（档位/电流大屏）— 下一迭代在此模板上加 `AppMower`  
- MQTT / 复杂主题 / 多语言  
- 把 UserDemo 全部资源与 11 个 App 搬过来  
- 修改 Roller 固件  

## 风险

- PageManager 与 LVGL 版本需匹配；若 8.x API 不合，固定 UserDemo 同代 lvgl。  
- 资源全砍后启动更轻，但动画/图标会很朴素（符合“清净模板”）。  

## 一句话

**在 `feature/ui` 上，用 UserDemo 的页面架构做极简壳：StartUp → Home →（WiFi | Screen），电机 P1 代码挪到旁路模块；你验收 UI 骨架后再 commit。**
