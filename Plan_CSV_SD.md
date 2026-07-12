# Plan: 本地数据记录（SD → CSV）— 手动 REC/STOP

## Context

- 智能工具电机模拟（非导航机器人）
- 面试：手动开始/结束一段 CSV，拔卡看曲线

## 已确认决策（最新）

| 项 | 选择 |
|--|--|
| 记录方式 | **手动 REC / STOP**（不跟 Mower 运行自动启停） |
| UI | AppSD 一个开关按钮 |
| 采样 | REC 后每 **200ms** 写一行（含停机状态，便于看启停曲线） |
| 文件 | `/mower_%03u.csv` 每次 REC 新建 |
| 列 | `ms,rpm,current_mA,power_W,gear,running,fault` |

## 行为

```
STOP 状态 → 点按钮 REC → 开文件、写表头、开始采样
REC 状态  → 每 200ms 一行（不依赖 running）
          → 再点 STOP → flush + close → 保存
```

- `main` 里仍 `g_sd_logger.poll()`（仅在 REC 时真正写）
- 离开 AppSD 页也可继续录（直到 STOP）；挂载引用计数保留

## 验收

1. 未 REC 时 RUN → 无新 CSV  
2. REC → 行数增加 → STOP → 文件可打开  
3. 再 REC → 新编号文件  
