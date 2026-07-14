# Plan: EzData2 HTTP 上传 CSV（STOP 后）

## Context

- 项目：`Mower_ui`（CoreS3 + Unit Roller demo）
- 已有：SD 手动 REC/STOP 写 `/mower_XX.csv`；EzData2 **MQTT** 上报 `running/soc/used_mAh`（短连 + 短 flush，secrets 本地）
- 目标：STOP 存盘成功后，用官方 **HTTP §3.2** 把该 CSV **再传一份到云**
- 协议依据：[EzData2 Device 接口 §3.2](https://docs.m5stack.com/zh_CN/guide/ezdata/ezdata_v2_protocol)  
  - `POST https://ezdata2.m5stack.com/api/v2/device/uploadDeviceFile`  
  - `multipart/form-data`：`deviceToken` + `file`  
  - HTTP `code: 200` 成功；可选 MQTT `cmd:105` 带文件 URL（**demo 不依赖**）
- **不做：** RTOS、断点续传、进度条 UI、改电机逻辑、等 105 长 pump、git commit（你自己验收 commit）

---

## 1. MoSCoW

| 优先级 | 内容 |
|--------|------|
| **Must** | STOP 且 `lines_ > 0` 后 POST 该 CSV；复用 WiFi + token secrets；串口清晰 log；失败不删本地文件 |
| **Should** | HTTPS demo 用 `setInsecure()` 先跑通；上传超时可配置；空 token/无卡 skip |
| **Could** | 串口顺带提一句「105 URL 需长连才稳，本版不等」 |
| **Won't** | 等 `cmd:105`、自动重试队列、RTOS、改 60s MQTT 字段逻辑、上传进度条 |

---

## 2. 行为（核心）

```
用户 AppSD: REC  →  写 CSV（现有）
用户 AppSD: STOP →  closeFile（现有）
              →  【仍挂载 SD】g_ez2.uploadLogFile(path)   // 新增
              →  SdUnmount（现有）
```

- 上传对象：**刚 close 的那份** `path_`（如 `/mower_01.csv`），不是 MQTT 字段。
- 成功标准：**HTTP 响应 code == 200**（或 HTTPClient 返回 200 且 body 可解析时以文档为准）。
- 失败：只打 `[EZ2] file fail …`，SD 文件保留。

### 关键顺序（避免踩坑）

当前 `SdLogger::stop()` 是：

```text
closeFile → SdUnmount → 打 status
```

Unmount 后默认 **打不开文件**。因此必须：

```text
closeFile →（lines_>0 且 path 非空）upload → SdUnmount → status
```

上传在 **仍 held_mount_** 时读 SD。

---

## 3. 实现拆分（保持简单）

### 3.1 Config

`include/config_ezdata2.h` 增加（非 secret）：

```cpp
#define EZ2_FILE_UPLOAD_URL  "https://ezdata2.m5stack.com/api/v2/device/uploadDeviceFile"
#define EZ2_HTTP_TIMEOUT_MS  30000
```

Token / WiFi 继续只在 `config_ezdata2_secrets.h`。

### 3.2 `EzData2Client`

新增公开方法（命名可微调）：

```cpp
bool uploadLogFile(const char* path);
```

内部大致：

1. token / path 空 → skip + log  
2. `ensureWifi_()`（复用现有）  
3. `SdIsMounted()` 或依赖调用方已挂载；`SD.open(path, FILE_READ)`  
4. `WiFiClientSecure` + `HTTPClient`  
   - demo：`setInsecure()`（证书链在板端省事）  
   - `setTimeout(EZ2_HTTP_TIMEOUT_MS)`  
5. `multipart/form-data`：字段 `deviceToken`、`file`（文件名用 basename，如 `mower_01.csv`）  
6. 读文件写入 body（优先：**按块读 SD 写 client**，避免整文件进大 `String`；demo 若文件很小也可整读，但实现时按块更稳）  
7. 看 HTTP code → log `file ok/fail` + bytes / code  
8. **不等** MQTT 105；**不**为此重新 connect MQTT  

可把 `ensureWifi_()` 保持 private，`uploadLogFile` 与 `uploadNow` 共用。

### 3.3 `SdLogger::stop()`

```cpp
void SdLogger::stop() {
    closeFile();
    // path_ / lines_ 在 close 后仍有效
    if (path_[0] && lines_ > 0) {
        g_ez2.uploadLogFile(path_);
    }
    if (held_mount_) {
        SdUnmount();
        held_mount_ = false;
    }
    // status 文案可略提 cloud，或仍只报 saved path
}
```

- `#include "cloud/EzData2Client.h"` 仅在 `.cpp`，避免头文件环依赖。  
- `lines_ == 0`（空录）不上传。

### 3.4 与 MQTT 字段上报的关系

| 通道 | 何时 | 内容 |
|------|------|------|
| MQTT 100/101 | start + 60s | running / soc / used_mAh |
| HTTP §3.2 | STOP only | 整份 CSV 文件 |

互不合并。STOP 时若正好也到 60s，可能连续两次网络（先文件后字段或相反）——demo 可接受；**不**为消重叠上状态机。

---

## 4. 串口 log（够 debug、不吵）

建议固定前缀 `[EZ2]` / `[LOG]`：

```
[LOG] closed /mower_01.csv lines=120
[EZ2] file upload /mower_01.csv bytes=…
[EZ2] wifi already ip=…
[EZ2] file HTTP 200 ok
// 或
[EZ2] file fail open|wifi|http code=…|timeout
[LOG] stop
```

不打印 token；不把整份 CSV 打串口。

---

## 5. 风险与对策（demo 级）

| 风险 | 对策 |
|------|------|
| 同步 HTTP 堵 UI | 接受；短录制 CSV 通常几秒内；timeout 30s 封顶 |
| HTTPS 证书 | `setInsecure()` demo；注释标明 |
| 内存 | 分块读 SD，不整文件 `String` |
| 供电 + WiFi | 与既有 FA 相同；12V DinBase 更稳 |
| 文档无文件大小上限 | demo 短 REC；失败看 HTTP code |
| Unmount 过早 | stop 内 **先上传再 unmount** |

---

## 6. 文件清单

| 文件 | 动作 |
|------|------|
| `include/config_ezdata2.h` | URL + HTTP timeout |
| `src/cloud/EzData2Client.h` | 声明 `uploadLogFile` |
| `src/cloud/EzData2Client.cpp` | HTTP multipart 实现 |
| `src/sd/SdLogger.cpp` | stop：close → upload → unmount |
| `Plan_SD_local.md`（本文件） | 计划书落在项目根目录 |

不改：`main` 的 `g_ez2.poll()` 字段逻辑、电机、UI 页面（除非 status 字符串顺手带 `cloud`）。

---

## 7. 验证（板子）

1. 有 secrets、WiFi 可用。  
2. REC 若干秒 → STOP。  
3. 串口：`closed` → `file upload` → `HTTP 200`（或明确 fail code）。  
4. SD 上文件仍在。  
5. my.m5stack / EzData2 侧能看到上传文件（或文档示例中的 OSS URL 一类结果）。  
6. 再跑电机 60s：MQTT 字段上报仍正常。  
7. 空 STOP / 无 token：skip，不崩。

---

## 8. 实现顺序

1. Config 常量  
2. `uploadLogFile` + 本地可编译的 multipart  
3. 挂钩 `SdLogger::stop` 顺序  
4. 板测调 timeout / insecure  
5. 你验收；commit 由你

---

## 一句话

**STOP 关盘后、卸盘前，用现有 token 对 EzData2 `uploadDeviceFile` 做一次 HTTPS multipart POST；以 HTTP 200 为准，不等 105，失败只打 log，本地 CSV 始终保留。**
