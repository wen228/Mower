# Plan: EzData2 云端遥测（低频短连接 MQTT）

> 状态：计划中（尚未实现）  
> 项目：`Mower_ui`（CoreS3 Lite + Unit Roller，智能工具电机 demo）  
> 目标：把关键运行状态同步到 M5 **EzData2** 云端，方便展台/网页查看；**不**做实时曲线、**不**依赖 Node-RED。

---

## 0. 优先级（MoSCoW）

| 优先级 | 内容 |
|--------|------|
| **Must** | 短连接 MQTT；约 5 min `poll`；config（WiFi/token/mac）；串口日志；`Status` 主字段批量 101；网页先建字段；`main` 挂 `g_ez2.poll()` |
| **Should** | `uploadNow()`；上传前 WiFi 检查；与 SD REC 独立 |
| **Could** | 上传结果 UI / 立即上传按钮；NVS 存 token；次要字段（如 `err`/`sys`/`energy`） |
| **Won't** | 长连接常驻；WebSocket；板端 down/103·104 查询；多级故障码表/历史；Node-RED；改电机逻辑；高频实时遥测 |

字段原则：`Mower::Status` 有展示价值的尽量传；`fault` 只镜像 bool `"0"`/`"1"`，不单独做故障码体系。

---

## 1. 背景与选型结论

### 1.1 产品语境

- 定位：手持/工具电机模拟（链锯式刀头），非导航机器人。
- 已有：软坡、档位、stall、库仑 SOC、SD CSV（手动 REC/STOP）、多页 LVGL UI。
- 云端优先级：**展示** > 工程完备；模拟 demo，字段宜少。

### 1.2 为何是 EzData2（而非 1.0 / 裸 MQTT）

| 方案 | 优点 | 缺点 | 本项目 |
|------|------|------|--------|
| **EzData 1.0** | 官方 `M5_EzData`，`setData` 极简；HTTP 短会话 | API 基本 **int**；debugger 较简 | 备选；若 2.0 联调超时可回退 |
| **EzData 2.0** | 官方看板 [my.m5stack.com](https://my.m5stack.com/ezdata2/workflow)；`value` 为 **string**（可写 float 文本）；协议文档完整 | 无通用「一行 setData」Arduino 库；需 MQTT + 自定义 JSON cmd | **主选** |
| 裸 MQTT + 自建看板 | 协议标准 | 还要前端/Node-RED | 明确不做 |

### 1.3 已确认的实现原则

| 项 | 决策 |
|----|------|
| 传输 | **MQTT 3.1.1**（不用 WebSocket） |
| 连接策略 | **短连接**：需要上传时 `connect → publish → disconnect`，**不** 24h 长连 |
| 上传周期 | 约 **5 分钟** 一次（可配置）；非实时 |
| 注册/绑定 | **人工完成**；固件写死 `deviceToken` + MAC（或 NVS）；板端可不含 register HTTP |
| 字段 | 最小集合（见 §4）；网页端先建好字段，板端主要用 **101 修改** |
| 下行/查询 | demo **可不实现** 板端解析 down；展示靠网页控制台 |
| 与 SD CSV | 并行：CSV 本地细采样；云端粗状态快照 |
| 假死风险 | 平时不断开 MQTT 会话；仅上传窗口内短时 `loop()` |

---

## 2. 参考资料与链接

### 2.1 官方（实现必读）

| 说明 | URL |
|------|-----|
| **EzData2 设备协议（MQTT/WS + JSON cmd）** | https://docs.m5stack.com/en/guide/ezdata/ezdata_v2_protocol |
| EzData2 快速开始 / 控制台概念 | https://docs.m5stack.com/en/guide/ezdata/ezdata_v2 |
| 控制台（登录后 Data） | https://my.m5stack.com/ezdata2/workflow |
| Board / deviceType 名称参考 | https://github.com/m5stack/m5stack-board-id/blob/main/board.csv |

### 2.2 对照（非本阶段实现，选型用过）

| 说明 | URL |
|------|-----|
| EzData **1.0** Arduino | https://docs.m5stack.com/en/arduino/ezdata/ezdata_arduino |
| EzData 1.0 库 `M5_EzData` | https://github.com/m5stack/M5_EzData |
| EzData 1.0 debugger | `https://ezdata.m5stack.com/debugger/?token={token}` |
| UiFlow2 EzData2 积木（参考概念） | https://docs.m5stack.com/en/uiflow2/blockly/ezdata/ezdata_manager |

### 2.3 协议摘要（来自 v2_protocol）

**注册（人工/一次性，板端可选）：**

```http
POST https://ezdata2.m5stack.com/api/v2/device/registerMac
Content-Type: application/json

{"deviceType":"basic","mac":"AABBCCDDEEFF"}
→ {"code":0,"data":"<deviceToken>","msg":"success"}
```

- `mac`：无冒号、大小写不敏感  
- 拿到 token 后：在 my.m5stack.com → Add Group → **Token** 粘贴绑定  

**MQTT：**

| 项 | 值 |
|----|-----|
| Host | `uiflow2.m5stack.com` |
| Port | `1883`（明文，demo 可接受） |
| Protocol | MQTT 3.1.1 |
| ClientId / Name | `ez{mac}ez`（mac 无冒号） |
| Username | `{deviceToken}` |
| Password | 无 |
| 上行 topic | `$ezdata/{deviceToken}/up` |
| 下行 topic | `$ezdata/{deviceToken}/down`（本阶段可不订阅） |

**业务 payload（JSON，MQTT 与 WS 相同）：**

| requestType | 含义 | demo 是否用 |
|-------------|------|-------------|
| 100 | 设备 **新增** 字段 | 可选；推荐网页先建字段 |
| 101 | 设备 **修改** 字段 | **主路径** |
| 102 | 删除 | 否 |
| 103 | 查询列表 | 否（网页看） |
| 104 | 查询单条 | 否 |
| 500 | 错误 | 仅日志 |

`body.value` 类型为 **string**（可 `"3200"` / `"12.3"` / `"1"`）。

---

## 3. 关键 API（计划对外接口）

模块名建议：`EzData2Client`（路径见 §6）。

### 3.1 配置（`include/config_ezdata2.h` 或 `config.h` 段）

```cpp
// 示例占位 — 实现时替换为真实值（勿提交私密 token 到公开仓库时可改用 NVS）
#define EZ2_WIFI_SSID       "your_ssid"
#define EZ2_WIFI_PASS       "your_pass"
#define EZ2_DEVICE_TOKEN    "xxxxxxxx"          // 注册绑定后
#define EZ2_MAC_NO_COLON    "AABBCCDDEEFF"      // 与注册一致
#define EZ2_UPLOAD_INTERVAL_MS  (5UL * 60UL * 1000UL)  // 5 min
#define EZ2_MQTT_HOST       "uiflow2.m5stack.com"
#define EZ2_MQTT_PORT       1883
#define EZ2_CONNECT_TIMEOUT_MS  8000
#define EZ2_FLUSH_MS            500    // publish 后短时 loop
```

### 3.2 类 / C API（推荐类）

```cpp
class EzData2Client {
public:
    // 仅保存配置；不强制 setup 里连 WiFi/MQTT
    void begin(const char* ssid, const char* pass,
               const char* deviceToken, const char* macNoColon);

    // 主循环：仅做「到点则触发一次短连接上传」；平时无长连接
    void poll();

    // 可选：UI 按钮「立即上传」
    bool uploadNow();

    // 单字段：requestType=101，value 为 C 字符串
    bool setField(const char* name, const char* value);

    bool wifiOk() const;
    uint32_t lastUploadMs() const;
    bool lastUploadOk() const;

private:
    bool ensureWifi_();
    bool mqttConnect_();
    void mqttDisconnect_();
    bool publishJson_(const char* name, const char* value, int requestType);
    void uploadSnapshot_();  // 从 Mower 取数 → 多字段 setField
};
```

全局实例（与 `g_sd_logger` 对称）：

```cpp
extern EzData2Client g_ez2;
// main loop:
//   g_ez2.poll();
```

### 3.3 依赖库（`platformio.ini` 计划增加）

```ini
lib_deps =
  ... existing ...
  knolleary/PubSubClient@^2.8
  bblanchon/ArduinoJson@^6.21.0
```

（若工程已间接带上 Json 可复用版本，避免双份冲突。）

---

## 4. 云端字段约定（最小可讲完整）

### 4.1 状态字段（周期快照）

网页 Group 下预先创建同名 Data；板端 **101 更新**。

| name | value 示例 | 来源（Mower） | 说明 |
|------|------------|---------------|------|
| `rpm` | `"3200"` | 实际转速 | 整数或一位小数均可（string） |
| `tgt` | `"3000"` | `target_raw` / 目标 | |
| `soc` | `"87.5"` | `batt_soc_pct` | float → 字符串 |
| `running` | `"0"` / `"1"` | `running` | |
| `fault` | `"0"` / `"1"` | `fault` | 当前是否故障 |
| `current_mA` | `"1250"` | 电流 | 可选 |
| `power_W` | `"12.3"` | 功率 | 可选 |

**演示话术：** 隔约 5 分钟（或点「立即上传」）把工具关键状态推到 M5 云，控制台可见。

### 4.2 故障历史（可选第二阶段）

EzData2 每个 `name` 更像「当前字段 + 历史点数」，与 1.0 的 list API 不完全相同。

| 阶段 | 做法 |
|------|------|
| **MVP** | 只更新 `fault` 当前值，讲「云端可见故障位」 |
| **增强** | 故障边沿时：`fault_last` = 码；或 `fault_log` value 拼短字符串 `"1@123s"`；网页看最近写入 |

不必第一版就做完整故障时间线；本地 CSV / 后续再补。

### 4.3 故障码约定（与本地一致即可）

| 值 | 含义 |
|----|------|
| 0 | 正常 |
| 1 | Stall / 堵转 |
| 2 | 过流等（若已有） |
| 3 | I2C/通信异常（若已有） |

---

## 5. 伪代码

### 5.1 短连接上传（核心）

```
function uploadSnapshot():
    if not ensureWifi():
        log "wifi fail"; return false

    clientId = "ez" + macNoColon + "ez"
    mqtt.setServer(EZ2_MQTT_HOST, 1883)
    mqtt.setUsername(deviceToken)   // 按库 API：connect 时传入
    if not mqtt.connect(clientId, deviceToken, ""):
        log "mqtt connect fail"; return false

    topicUp = "$ezdata/" + deviceToken + "/up"

    for each field in {rpm, tgt, soc, running, fault, ...}:
        json = {
          "deviceToken": deviceToken,
          "body": {
            "name": field.name,
            "value": field.valueAsString,   // 注意 string
            "requestType": 101
          }
        }
        mqtt.publish(topicUp, serialize(json))
        // 字段间可 delay(20~50) 降低 burst

    t0 = now
    while now - t0 < FLUSH_MS:
        mqtt.loop()     // 仅短窗口

    mqtt.disconnect()
    return true
```

### 5.2 主循环（无长连接）

```
function poll():
    if millis() - lastAttempt < UPLOAD_INTERVAL_MS:
        return
    lastAttempt = millis()
    lastOk = uploadSnapshot()
    // 失败不重试死磕；等下一个 5 分钟
```

### 5.3 与 main 集成

```
// main.cpp loop() 现有：
Mower_handleSerial()
Mower_poll()
g_sd_logger.poll()
g_ez2.poll()          // 新增：到点才真正联网上传
lv_timer_handler()
delay(10)
```

### 5.4 JSON 示例（单字段 update）

```json
{
  "deviceToken": "4fbb52fb5b6243e083377f45d216820f",
  "body": {
    "name": "rpm",
    "value": "3200",
    "requestType": 101
  }
}
```

### 5.5 一次性人工准备（不写进固件逻辑也可）

```
1. 记下 CoreS3 WiFi MAC（去冒号）
2. POST registerMac → deviceToken
3. my.m5stack.com 登录 → Add Group by Token → 粘贴
4. 在该 Group 下 Add Data：rpm, tgt, soc, running, fault, ...
5. 填入 config：EZ2_DEVICE_TOKEN / EZ2_MAC_NO_COLON / WiFi
6. 烧录 → 等 interval 或点 Upload → 控制台看 value 更新
```

---

## 6. 项目代码结构（计划）

```
Mower_ui/
├── Plan_EzData2.md              ← 本文件
├── platformio.ini               ← 增加 PubSubClient、ArduinoJson
├── include/
│   ├── config.h                 ← 可选：include config_ezdata2 或开关宏
│   └── config_ezdata2.h         ← 新建：WiFi/token/interval/host
└── src/
    ├── main.cpp                 ← loop 增加 g_ez2.poll()
    ├── motor/
    │   └── Mower.h/.cpp         ← 只读快照字段，不改业务逻辑
    ├── sd/                      ← 现有 CSV，与云端独立
    │   ├── SdLogger.*
    │   └── SdMount.*
    ├── cloud/                   ← 新建目录
    │   ├── EzData2Client.h
    │   └── EzData2Client.cpp    ← 短连接 MQTT + JSON 101
    └── pages/
        └── AppMower/ 或 AppSD/ 或 极简 AppCloud/
            └── （可选 UI）「云：上次成功/失败」「立即上传」
```

### 6.1 职责划分

| 模块 | 职责 | 不职责 |
|------|------|--------|
| `EzData2Client` | WiFi 确保、MQTT 短会话、JSON publish、定时 | 不解析业务、不改电机控制 |
| `Mower` | 提供 rpm/soc/fault 等只读状态 | 不知道云 |
| `main` | `poll()` 挂接 | 不写 JSON |
| UI（可选） | 显示 last status / 触发 `uploadNow()` | 不直接碰 MQTT |
| 人工控制台 | 注册、绑 token、建字段、看数 | — |

### 6.2 与现有风格对齐

- 参考 `sd/SdLogger`：`extern` 全局 + `poll()`，不阻塞 UI 主路径过久。  
- 上传窗口允许最多数秒级阻塞（5 分钟一次可接受）；若以后要更丝滑，再迁到独立 task（**非 MVP**）。  
- 不 drive-by 重构 PageManager / App 层。

### 6.3 建议文件体量

| 文件 | 预估 |
|------|------|
| `EzData2Client.cpp/.h` | ~100–180 行 |
| `config_ezdata2.h` | ~30 行 |
| `main.cpp` 改动 | +2～5 行 |
| UI（若做） | +50 行级 |

---

## 7. 实现阶段

### Phase A — MVP（优先）

1. 人工注册 + 绑定 + 网页建字段  
2. `EzData2Client` 短连接 + 101 更新 5 个字段  
3. `main` 挂 `poll()`，默认 5 min  
4. USBSerial 打印 connect/publish 成功失败  
5. 控制台验证 value 变化  

### Phase B — 体验

1. 可选：App 页「立即上传」+ last result  
2. 可选：故障边沿额外写 `fault_last`  
3. WiFi/token 存 NVS（避免改代码）  

### Phase C — 不做（除非明确要求）

- 长连接常驻 `mqtt.loop()`  
- WebSocket  
- Node-RED / 自建 broker  
- 板端 103/104 查询与本地故障历史完整对齐  
- TLS 8883  

---

## 8. 验收标准

1. 填好 WiFi + token + mac 后，上电等待 ≤ interval（或调用 `uploadNow`），**my.m5stack.com** 上 `rpm/soc/running/fault` 更新为当前值。  
2. 上传过程中 UI 可短暂卡顿，但上传结束后恢复正常；**平时无 MQTT 连接**（可用串口确认仅在上传窗口 connect）。  
3. WiFi 关闭时：上传失败，**不崩溃**，下个周期再试。  
4. 与 SD REC：**互不依赖**；可同时开或只开其一。  

---

## 9. 风险与注意

| 风险 | 缓解 |
|------|------|
| 字段不存在导致 101 失败 | 网页先 Add Data；或首次用 100 |
| `connect` 阻塞 | 超时；失败即返回 |
| token 泄露 | 私有仓库或 NVS；勿贴公开 README |
| 1883 明文 | demo 接受 |
| 与 LVGL 抢时间 | 5 min 一次；上传中可接受短阻塞 |
| CoreS3 MAC 与注册不一致 | 串口打印实际 MAC，与 config 对齐 |
| PubSubClient 缓冲区 | JSON 短消息；`setBufferSize` 若需加大 |

---

## 10. 和 EzData1 的关系（回退路径）

若 MQTT 联调成本过高、时间不够：

- 回退 **EzData1 HTTP** + `M5_EzData`（int / 定点）。  
- 字段名尽量相同（`rpm`/`soc`/…），业务层只换 `cloud` 后端实现。  
- 建议在 `EzData2Client` 旁预留同一「快照结构体」，避免 UI 绑死协议。

---

## 11. 一句话总结

> **EzData2 = 官方看板 + MQTT 上 JSON cmd；本项目用「约 5 分钟一次的短连接上传」更新少量 string 字段，不养长连接、不做 Node-RED，模块落在 `src/cloud/EzData2Client`，由 `main` 的 `poll()` 驱动。**

---