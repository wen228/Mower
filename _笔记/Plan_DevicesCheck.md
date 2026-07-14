# Plan: Device Self-Check（复用 I2C App）

## Goal

把现有 **AppI2C** 从「I2C 总线扫地址」改成 **设备自检清单**，用于 demo 上电/进菜单时一眼看外设是否在。

- **入口**：仍走 Home 里原来的 I2C App（可不改菜单图标名；有空再改文案）
- **不改**：AppMower、串口电机日志、故障 tip 逻辑
- **风格**：简洁；OK 绿 / NG 黄（不用红吓人）；Cam 例外固定红 Disabled；可刷新

---

## 1. MoSCoW

| 优先级 | 内容 |
|--------|------|
| **Must** | 去掉地址网格扫描展示；改为固定几项外设检测；每项 `NAME: OK/NG`；OK 绿、NG 黄；进入页自动检一次；「Scan again」改为重新自检 |
| **Should** | 检测失败不崩；串口可选一行总览（可不加，避免吵） |
| **Could** | 菜单/标题改成 “Self-Check” / “Check” |
| **Won't** | 新 Page、改 AppMower、强制连 WiFi、阻塞式长自检、中文字库、RTOS |

---

## 2. 检查项与判定（demo 级）

| 显示名 | 判定（简单） | 备注 |
|--------|--------------|------|
| **BLDC** | `g_mower.status().ready` 为真 → OK | 依赖 `Mower::begin()` 已在启动路径调过；否则可对 `0x64` 轻量 probe |
| **WiFi** | `WiFi.status() == WL_CONNECTED` → OK | 未连 = NG（不在此页 `begin` 联网） |
| **SD** | `SdCardPresent()` → OK | 不强制写文件 |
| **IMU** | 读 BMI270（或工程里已有 IMU init/读接口）一次成功 → OK | 与 AppIMU 同源硬件即可 |
| **Cam** | 固定显示 `Cam: Disabled`（红字） | 不用摄像头业务；CoreS3 有硬件也不测 |

结果仅 **OK / NG**，不做分数、不写 CSV。

---

## 3. UI 行为

```
进入 AppI2C
  → 清空原地址容器
  → 跑 checklist
  → 显示多行 label：
        BLDC: OK     (绿)
        WiFi: NG     (黄)
        SD:   NG     (黄)
        IMU:  OK     (绿)
        RTC:  OK     (绿)

点 “Scan again”（或改文案 “Recheck”）
  → 再跑一遍 checklist
```

- **去掉**：Port 切换扫 In/Ex 总线、地址 `02X` 网格  
- **保留**：Home / Next、整体布局壳（背景可先不动，避免大改 View 资源）  
- **Port 顶栏按钮**：可隐藏或改为无操作 / 触发 Recheck（实现时选改动最小的一种）

---

## 4. 代码落点（尽量少文件）

| 文件 | 动作 |
|------|------|
| `src/pages/AppI2C/AppI2C.cpp` | `Update()` 改为 checklist；事件里 Recheck |
| `src/pages/AppI2C/AppI2CView.*` | 仅在必要时：少改控件；地址 container 复用为结果列表 |
| `AppI2CModel.*` | 可选：放 `runSelfCheck()` 返回结构体；也可全写在 Presenter，保持最小 |

**依赖调用（只读/轻量）**：`g_mower`、`SdCardPresent` / `SdMount`、`WiFi`、`M5.Rtc`、现有 IMU 读法（抄 AppIMU 最短路径）。

---

## 5. 实现顺序

1. 停用扫描地址 UI 路径  
2. 实现 4～5 项 bool 检测  
3. 用 label 列表刷新颜色  
4. 绑定 Recheck  
5. 板测：拔 SD / 断 WiFi / 无电机 看 NG  

---

## 6. 验收

1. 进 I2C App：看到 checklist，不是地址表  
2. 电机在：`BLDC: OK` 绿  
3. 未连 WiFi：`WiFi: NG` 黄  
4. 无卡：`SD: NG` 黄  
5. Recheck 刷新结果  
6. AppMower / 串口行为与现在一致  

---

## 7. 风险与边界

| 点 | 处理 |
|----|------|
| `g_mower` 未 begin | begin 已在系统启动则 OK；否则自检里一次 `begin` 或 probe |
| WiFi NG 吓人 | Demo 说明：仅表示当前未关联，不是硬件坏 |
| 背景图仍是 I2C 产测图 | 可接受；Could 再换标题 |
| 时间 | 小改动，适合剩余 1～2 天窗口 |

---

## 一句话

**I2C App = 设备自检页：BLDC / WiFi / SD / IMU / Cam(Disabled 红)→ OK 绿 / NG 黄（不用红吓人）；Cam 例外固定红 Disabled；可刷新；不动 Mower 主控与串口。**


---

## Review notes（用户确认）

- 根目录计划书：`Plan_DevicesCheck.md`
- **不要 RTC**；改为硬编码 **`Cam: Disabled` 红色**
- 原版 I2C 扫地址 UI **可删**，以自检清单为主
- NG 用 **黄色**
- 可开始写代码
