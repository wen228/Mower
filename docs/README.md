# Smart Lawn Mower

Smart Lawn Mower 是一款基于 BLDC 刀盘电机的园艺工具原型。CoreS3 负责闭环转速控制、负载感知、软启动与故障闩锁；本地 SD 记录运行数据，云端低频同步机队状态，并通过 ESP-NOW 无线连接，实时推到外接大屏。形成控制、监测与互联的完整演示。

> 仓库为 PlatformIO 工程，主机代码在 ./src 目录下
> Atom 从机在 ./src_slave/EspNow_Telem_Slave.ino

---

## 硬件

| 角色 | 硬件 | 说明 |
|------|------|------|
| 主控 | **CoreS3 Lite** | LVGL UI、`g_mower`、SD、WiFi、ESP-NOW 发 |
| 电机 | **Unit Roller** Port A I2C `0x64` | 三档速度环刀盘 |
| 大屏 | **AtomS3R + Display Base** | ESP-NOW 收 + HDMI 看板 |
| 供电 | **DinBase 12V** 推荐 | USB+电机+WiFi 易 brownout，见`Note_USBPowerFA.md` |

---

## 架构（逻辑）

```

```

---

## 功能一览

| 模块 | 功能 | 要点 |
|------|------|------|
| **Mower** | 三档 / 软爬坡 / 限流 | Eco·Normal·Turbo；负载分档、软堵转；E-stop锁定 / jam锁定 / 低电锁定 |
| **AppMower** | 触控控机 | UI界面；实时信息显示；故障英文手册 |
| **Battery** | SoC估算 | 库仑计、功率积分、已用电量、低电停机 |
| **SD / CSV** | 本地记录 | 支持实时时间和日期，`t` = RTC + millis；文件预览，支持滑动；热插拔|
| **EzData2 MQTT** | 实时快照 | 每60s上传资产状态；{`running` / `soc` / `used_mAh`}；方便监控 |
| **HTTP 上传** | 工程记录 | `RTOS netWorker`；后台大文件上传，无阻塞；详细使用数据供学习用 |
| **Self-check** | 开机自检 | BLDC、WiFi、SD、IMU、Cam 连接性检测|
| **ESP-NOW** | 外接看板 | packed `EspNowTelem` ver2实现；无需wifi，无线连接，100m内稳定 |

### 串口前缀

| 前缀 | 含义 |
|------|------|
| `[Mow]` | 电机业务 |
| `[SD]` | 卡 / CSV |
| `[EZ]` | WiFi / MQTT / HTTP / RTOS 上传 |
| `[ESP]` | ESP-NOW |

### 刻意边界

- 无导航 / 路径规划  
- 无中文字库扩展（UI 英文）  
- 大屏 **只显示、不回控**  
- Encoder 方案已放弃  

---

## 关键路径

| 用途 | 路径 |
|------|------|
| 主工程 | `platformio.ini` → env `M5CoreS3` |
| 电机 | `src/motor/Mower.*` |
| 云 / 上传 | `src/cloud/EzData2Client.*`、`NetUpload.*` |
| ESP-NOW 发 | `src/cloud/EspNowTx.*`、`include/espnow_telem.h`、`config_espnow.h` |
| Atom 从机 | `Helpers_ino/EspNow_Telem_Slave.ino` 或 Arduino `EspNow_Telem_Slave` |
| Secrets | `include/config_ezdata2_secrets.h`（gitignore） |

---

## 编译 / 烧录（主控）

```bash
cd Mower_ui
pio run -e M5CoreS3
pio run -e M5CoreS3 -t upload
```

Atom：Arduino IDE 选 **AtomS3R**，打开 `EspNow_Telem_Slave`，库需 M5Unified + M5GFX。  
信道须与 CoreS3 串口 `[ESP] … ch=N` 一致（连 WiFi 时常为 AP 信道）。

---

## 演示脚本

见同目录 **[DEMO.md](./DEMO.md)**（约 2～3 分钟口播顺序）。
