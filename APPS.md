# CoreS3-UserDemo 应用清单

路径：`src/pages/`  
注册入口：`src/pages/AppFactory.cpp`  
版本参考：`include/config.h` 中 `DEMO_VERSION`

---

## 架构说明（类似 MVC）

本工程每个功能页大致是 **页面级 MVC**（都在同一固件内，不是 Web 前后端分离）：

| 本工程 | 类比 | 职责 |
|--------|------|------|
| **View**（`*View.cpp`） | 前端 UI | LVGL 控件布局、换图/改文字 |
| **Model**（`*Model.cpp`） | 业务/数据 | 读传感器、WiFi、SD、电源等 |
| **AppXxx.cpp** | Controller | 绑事件、定时刷新、页面跳转 |

通用生命周期：

```
进页: View.Create → 绑事件 / Model 初始化
在页: 定时器或点击 → 读 Model → 改 View
出页: 删定时器、View.Delete、必要时 Model 释放硬件
```

---

## 全部应用总表

| # | 页面 | 目录 | 作用一句话 |
|---|------|------|------------|
| 1 | **StartUp** | `StartUp/` | 开机动画 + 音效，点一下进主菜单 |
| 2 | **HomeMenu** | `HomeMenu/` | 主菜单，点图标进各功能页 |
| 3 | **AppWiFi** | `AppWiFi/` | 扫描并列出附近 WiFi |
| 4 | **AppCamera** | `AppCamera/` | 摄像头预览 + 光感/接近（CoreS3） |
| 5 | **AppMic** | `AppMic/` | 麦克风波形显示 |
| 6 | **AppPower** | `AppPower/` | 电池/USB 电压、电源模式切换 |
| 7 | **AppIMU** | `AppIMU/` | 姿态球 + 罗盘（CoreS3） |
| 8 | **AppSD** | `AppSD/` | SD 卡插拔检测与文件列表 |
| 9 | **AppTouch** | `AppTouch/` | 触摸画线测试 |
| 10 | **AppI2C** | `AppI2C/` | 内部/PortA/B/C I2C 地址扫描 |
| 11 | **AppRTC** | `AppRTC/` | 关机、亮度、深睡、RTC 定时唤醒 |

另有 **`_Template/`**：新页面模板，**未**注册进 `AppFactory`。

> **机型差异：**  
> - `M5CORES3`：含 Camera、IMU 等完整入口  
> - `M5CORES3SE`：主菜单/跳转会裁掉部分功能（见 `HomeMenu.cpp` 条件编译）

---

## 各应用说明

### 1. StartUp — 启动页

| 层 | 干什么 |
|----|--------|
| View | 开机动画时间线、机型示意图 |
| Model | 基本空（逻辑在控制层） |
| 控制 | 播开机音、渐亮屏幕、点屏跳 `HomeMenu` |

**逻辑一句话：** 进页播 `poweron_2_5s` → 动画 + 亮度升高 → 点击 → 主菜单。

---

### 2. HomeMenu — 主菜单

| 层 | 干什么 |
|----|--------|
| View | 最多 9 个图标按钮 |
| Model | 基本空 |
| 控制 | 点击 → `Replace("Pages/xxx")` |

**逻辑一句话：** 纯导航中枢。CoreS3 有 Camera/IMU；SE 会裁掉部分入口。

图标大致对应（以 CoreS3 为准，以源码 `onEvent` 为准）：

| 索引 | 目标页 |
|------|--------|
| 0 | AppWiFi |
| 1 | AppCamera（CoreS3） |
| 2 | AppMic |
| 3 | AppPower |
| 4 | AppIMU（CoreS3） |
| 5 | AppSD |
| 6 | AppTouch |
| 7 | AppI2C |
| 8 | AppRTC |

---

### 3. AppWiFi — WiFi 扫描

| 层 | 干什么 |
|----|--------|
| View | SSID 列表、「扫描中」提示、Home/Next |
| Model | `WiFi.scan`、取 SSID/RSSI |
| 控制 | 进页扫描；定时等结果填列表；顶栏可重扫 |

**逻辑一句话：** 进页 Scan → 轮询完成 → 列表显示 → Home 回菜单 / Next 去下一页。

---

### 4. AppCamera — 摄像头（CoreS3）

| 层 | 干什么 |
|----|--------|
| View | 背景框、传感器数据标签、导航钮 |
| Model | `esp_camera` 取帧；LTR553 光感/接近 |
| 控制 | 定时取帧画到屏幕固定区域，更新传感器数 |

**逻辑一句话：** 进页初始化相机 → 高频 `Update` 刷预览 → 出页 deinit。Next 常去 Mic。

---

### 5. AppMic — 麦克风

| 层 | 干什么 |
|----|--------|
| View | 背景 + Home/Next |
| Model | 开/关麦、读 PCM 缓冲 |
| 控制 | 双缓冲画左右声道波形 sprite 推到屏上 |

**逻辑一句话：** 进页 `MicBegin` → 约 30ms 读数画波形 → 出页 `MicEnd`。

---

### 6. AppPower — 电源

| 层 | 干什么 |
|----|--------|
| View | 背景、电池图标、电压/温度标签、USB/Bus 按钮 |
| Model | AXP2101 采样、充电状态、USB/Bus 模式位 |
| 控制 | 1s 刷新电压等；点 USB/Bus 改电源模式并换图 |

**逻辑一句话：** 展示供电状态，并可切换 USB/Bus 进出模式。

---

### 7. AppIMU — 惯性/罗盘（CoreS3）

| 层 | 干什么 |
|----|--------|
| View | 罗盘、小球、校准提示 |
| Model | BMI270/BMM150、Madgwick 滤波、磁力校准存 NVS |
| 控制 | 读姿态 → 小球随 pitch/roll 动、罗盘转角；顶栏触发校准 |

**逻辑一句话：** 传感器 → 滤波 → 改 UI；可校准磁力计。

---

### 8. AppSD — SD 卡

| 层 | 干什么 |
|----|--------|
| View | 文件列表、提示字、导航钮 |
| Model | 卡在位检测、`SD.begin` / `end` |
| 控制 | 有卡列目录；无卡提示；中间钮重扫 |

**逻辑一句话：** 插拔感知 + 浅层目录列表。

---

### 9. AppTouch — 触摸测试

| 层 | 干什么 |
|----|--------|
| View | 背景区域、导航钮 |
| Model | 很薄（直接用 `M5.Touch`） |
| 控制 | 10ms 读触摸点，在屏下半部画线；顶栏清屏 |

**逻辑一句话：** 手指轨迹测试，不是业务 App。

---

### 10. AppI2C — I2C 扫描

| 层 | 干什么 |
|----|--------|
| View | 地址网格、提示、「再扫」等 |
| Model | 逻辑多在控制层用 `M5.In_I2C` / `Ex_I2C` |
| 控制 | 切 Internal / PortA / B / C，scan 出地址填网格 |

**逻辑一句话：** 查总线上有哪些设备地址，调试扩展模块很有用。

Port 引脚（见 `include/config.h`，与 CoreS3 / CoreS3-Lite 一致）：

| 端口 | 相关 GPIO（工程宏） |
|------|---------------------|
| Port A | `PORTA_PIN_0=1`（SCL）、`PORTA_PIN_1=2`（SDA） |
| Port B | 8 / 9 |
| Port C | 18 / 17 |
| 内部 I2C | SDA=12、SCL=11 |

---

### 11. AppRTC — 系统/休眠相关

| 层 | 干什么 |
|----|--------|
| View | 背景 + 6 个热区按钮 |
| Model | 关机、亮度、深睡、RTC 定时唤醒 |
| 控制 | 点哪个按钮调 Model 对应动作 |

**逻辑一句话：** 关机 / 亮度 / 回菜单 / 深睡 / 5s 唤醒 / 1min 唤醒。

---

### （附）`_Template` — 新页模板

未在工厂注册。新建功能页时：复制该目录 → 改类名 → 在 `AppFactory.cpp` 增加 `APP_CLASS_MATCH` → 在 `HomeMenu` 增加入口。

---

## 页面跳转关系

```
开机
  └─ StartUp ──点击──► HomeMenu
                         ├─ AppWiFi
                         ├─ AppCamera   (CoreS3)
                         ├─ AppMic
                         ├─ AppPower
                         ├─ AppIMU      (CoreS3)
                         ├─ AppSD
                         ├─ AppTouch
                         ├─ AppI2C
                         └─ AppRTC

各 App 内：
  Home  → Replace("Pages/HomeMenu")
  Next  → 串到下一功能页（见各 App 的 onEvent）
```

---

## 应用开发时怎么用这份清单

1. 先在 **总表** 里找最接近的 App  
2. 对照其 **View / Model / 控制** 三层改业务  
3. 新页从 **`_Template`** 起步，并注册到 **`AppFactory`** + **HomeMenu**  
4. UI 图片/字体/音效用 **`src/res/ResourcePool`** 按名字取（`GetImage` / `GetFont` / `GetWav`）

---

## 相关文件速查

| 用途 | 路径 |
|------|------|
| 页面工厂 | `src/pages/AppFactory.cpp` |
| 应用初始化 | `src/App.cpp` / `src/App.h` |
| 入口 | `src/main.cpp` |
| 板级配置 / 引脚 | `include/config.h` |
| 资源池 | `src/res/ResourcePool.h` / `.cpp` |
| 页面管理器 | `lib/PageManager/` |
