# ZMK 5×5 计算器小键盘 — 开发计划

## 1. 项目概述

基于 SuperMini nRF52840（nice_nano_v2 兼容）打造一款 **5×5 矩阵小键盘**，集成 **SSD1306 128×64 OLED 显示屏**，核心特色功能为 **板载计算器**。

### 设计目标

| 项 | 说明 |
|---|---|
| 矩阵规格 | 5 行 × 5 列，共 25 键 |
| 芯片 | SuperMini nRF52840（nice_nano_v2 兼容） |
| 显示屏 | SSD1306 128×64 I2C OLED |
| 计算器 | 支持 +、-、*、/、% 五种运算，显示完整公式与结果 |
| 连接方式 | BLE 蓝牙 |
| 框架 | ZMK v0.3（Zephyr RTOS） |

---

## 2. 硬件引脚规划

### 2.1 原始引脚（5×6 矩阵）

原 overlay 使用 P0.17、P0.20 作为矩阵行线。但这两个引脚是 nice_nano_v2 默认 I2C 总线 (i2c0) 的 SDA/SCL。为接入 OLED 屏幕，必须释放这两个引脚。

### 2.2 新引脚分配（5×5 矩阵 + I2C 显示屏）

**矩阵引脚（10 个 GPIO）：**

| 功能 | 引脚 | Pro Micro 标号 |
|------|------|---------------|
| Row 0 | P0.06 | D1 |
| Row 1 | P0.08 | D0 |
| Row 2 | P0.22 | D4 |
| Row 3 | P0.24 | D5 |
| Row 4 | P1.00 | D6 |
| Col 0 | P0.11 | D7 |
| Col 1 | P1.04 | D8 |
| Col 2 | P1.06 | D9 |
| Col 3 | P0.31 | D19 |
| Col 4 | P0.09 | D18 |

**I2C 显示屏引脚：**

| 功能 | 引脚 | 说明 |
|------|------|------|
| SDA | P0.17 | nice_nano_v2 i2c0 默认 |
| SCL | P0.20 | nice_nano_v2 i2c0 默认 |

**其他引脚：**

| 功能 | 引脚 | 说明 |
|------|------|------|
| WS2812 | P0.02 | RGB 底灯（保留，默认禁用） |

---

## 3. 键位布局

### 3.1 物理布局 (5×5)

```
┌───────┬───────┬───────┬───────┬───────┐
│ CALC  │   /   │   *   │ BKSP  │   %   │  Row 0
├───────┼───────┼───────┼───────┼───────┤
│   7   │   8   │   9   │   -   │ NONE  │  Row 1
├───────┼───────┼───────┼───────┼───────┤
│   4   │   5   │   6   │   +   │ NONE  │  Row 2
├───────┼───────┼───────┼───────┼───────┤
│   1   │   2   │   3   │   =   │ NONE  │  Row 3
├───────┼───────┼───────┼───────┼───────┤
│   0   │   .   │ NONE  │ ENTER │ NONE  │  Row 4
└───────┴───────┴───────┴───────┴───────┘
  Col 0   Col 1   Col 2   Col 3   Col 4
```

### 3.2 Layer 0 — 普通小键盘模式

正常工作时，按键发送标准 Keypad 键码到主机：

| 位置 | 按键 | ZMK 绑定 |
|------|------|----------|
| (0,0) | 计算器切换 | `&calc CALC_TOGGLE` |
| (0,1) | / | `&kp KP_SLASH` |
| (0,2) | * | `&kp KP_MULTIPLY` |
| (0,3) | 退格 | `&kp BSPC` |
| (0,4) | % | `&kp PRCNT` |
| (1,0)-(1,2) | 7 8 9 | `&kp KP_N7` ... |
| (1,3) | - | `&kp KP_MINUS` |
| (2,0)-(2,2) | 4 5 6 | `&kp KP_N4` ... |
| (2,3) | + | `&kp KP_PLUS` |
| (3,0)-(3,2) | 1 2 3 | `&kp KP_N1` ... |
| (3,3) | = | `&kp EQUAL` |
| (4,0) | 0 | `&kp KP_N0` |
| (4,1) | . | `&kp KP_DOT` |
| (4,3) | Enter | `&kp KP_ENTER` |
| 空位 | 预留 | `&none` |

### 3.3 Layer 1 — 计算器模式

按下 CALC 键后切换至此层，按键由计算器引擎处理，不发送到主机：

| 位置 | 功能 | ZMK 绑定 |
|------|------|----------|
| (0,0) | 退出计算器 | `&calc CALC_TOGGLE` |
| (0,1)-(0,4) | ÷ × ← % | `&calc CALC_OP_DIV` ... |
| (1,0)-(1,2) | 7 8 9 | `&calc CALC_NUM_7` ... |
| (1,3) | - | `&calc CALC_OP_SUB` |
| (2,0)-(2,2) | 4 5 6 | `&calc CALC_NUM_4` ... |
| (2,3) | + | `&calc CALC_OP_ADD` |
| (3,0)-(3,2) | 1 2 3 | `&calc CALC_NUM_1` ... |
| (3,3) | = (求值) | `&calc CALC_EVAL` |
| (4,0) | 0 | `&calc CALC_NUM_0` |
| (4,1) | . | `&calc CALC_DOT` |
| (4,3) | 清空 | `&calc CALC_CLEAR` |

---

## 4. 计算器功能架构

### 4.1 状态机

```
    ┌──────────┐
    │   IDLE   │ ← 初始状态 / CLEAR / RESET
    └────┬─────┘
         │ 输入数字
    ┌────▼─────┐
    │  INPUT   │ ← 持续输入数字/小数点/运算符
    └────┬─────┘
         │ 按 = 求值
    ┌────▼──────┐
    │  RESULT   │ ← 显示结果
    └────┬──────┘
         │ 输入数字 → 回到 INPUT (新表达式)
         │ 输入运算符 → 回到 INPUT (链式计算，结果作为前缀)
    ┌────▼──────┐
    │   ERROR   │ ← 除零/溢出等异常
    └───────────┘
         │ 任意输入 → 回到 IDLE
```

> 注：实际实现中将 OPERATOR 状态合并进 INPUT，运算符作为表达式字符串的一部分处理，
> 连续运算符输入时自动替换最后一个运算符。

### 4.2 计算引擎

**核心设计原则：每个运算操作绑定独立函数**

```c
float calc_op_add(float a, float b);  // 加法
float calc_op_sub(float a, float b);  // 减法
float calc_op_mul(float a, float b);  // 乘法
float calc_op_div(float a, float b);  // 除法（除零检查）
float calc_op_mod(float a, float b);  // 取模
// 后续可扩展更多运算符...
```

**表达式解析器**（支持运算符优先级）：

```
expr    → term (('+' | '-') term)*
term    → factor (('*' | '/' | '%') factor)*
factor  → number
number  → [0-9]+ ('.' [0-9]+)?
```

- `*`、`/`、`%` 优先级高于 `+`、`-`
- 表达式以字符串形式存储，按 `=` 时解析并求值
- 最大表达式长度：64 字符

### 4.3 显示模块

使用 LVGL（ZMK 内置）渲染计算器界面：

```
┌─────────────────────────┐
│ [ CALC ]                │  标题（lv_label, unscii_8 字体）
│ -------------------- │  分隔线（lv_label, 文本破折号）
│ 123+456*789             │  表达式区（lv_label, 滚动模式）
│                         │
│            = 359907     │  结果区（lv_label, 右对齐）
└─────────────────────────┘
```

- 标题 + 分隔线：均使用 `lv_label`（避免依赖 LVGL line widget）
- 表达式区：使用 8px unscii_8 紧凑字体，`LV_LABEL_LONG_SCROLL_CIRCULAR` 滚动
- 结果区：右对齐，使用 LVGL 默认字体
- 显示更新通过 `k_work` 提交，线程安全
- 非计算器模式：显示 `[ CalcPad ]` 标题 + 实时按键名称
- 单屏双模式架构：同一个 LVGL screen，通过 `calc_display_set_mode()` 切换标题和内容
- 按键事件监听器（`key_display.c`）使用 ZMK 事件系统捕获 `position_state_changed`
- 开机延迟 2 秒后自动显示普通模式界面

---

## 5. 文件结构

```
zmk-test3/
├── CMakeLists.txt                              # 模块构建入口
├── Kconfig                                     # 模块 Kconfig 定义
├── boards/shields/breadboard_test/
│   ├── breadboard_test.overlay                 # [更新] 5×5矩阵 + OLED
│   ├── breadboard_test.keymap                  # [更新] 双层键位
│   ├── Kconfig.shield                          # [现有]
│   └── Kconfig.defconfig                       # [新增] Shield 默认配置
├── config/
│   ├── nice_nano_v2.conf                       # [更新] 显示+计算器配置
│   └── west.yml                                # [现有]
├── dts/bindings/behaviors/
│   └── zmk,behavior-calc.yaml                  # 自定义行为 DTS 绑定
├── include/dt-bindings/zmk/
│   └── calc_keys.h                             # 计算器按键参数定义
├── src/
│   ├── calculator/
│   │   ├── calc_engine.h                       # 计算引擎头文件
│   │   ├── calc_engine.c                       # 计算引擎实现
│   │   ├── calc_display.h                      # 显示模块头文件
│   │   ├── calc_display.c                      # 显示模块实现（单屏双模式）
│   │   ├── key_display.h                       # [新增] 按键事件监听器头文件
│   │   └── key_display.c                       # [新增] ZMK 事件监听 + 实时按键显示
│   └── behaviors/
│       └── behavior_calc.c                     # 自定义 ZMK Behavior
├── docs/
│   ├── plan.md                                 # 本文档
│   └── test_guide.md                           # [新增] 测试指南
├── zephyr/
│   └── module.yml                              # [更新] 模块注册
└── build.yaml                                  # [现有]
```

---

## 6. 实现步骤

| 阶段 | 任务 | 依赖 |
|------|------|------|
| 1 | 更新 `module.yml`，添加 `CMakeLists.txt`、`Kconfig` | — |
| 2 | 更新 `overlay`：5×5 矩阵 + I2C OLED 设备树 | — |
| 3 | 创建 `calc_keys.h`：定义所有按键参数宏 | — |
| 4 | 创建 `calc_engine`：计算引擎（状态机+解析器+运算函数） | 阶段3 |
| 5 | 创建 `calc_display`：LVGL 显示模块 | 阶段4 |
| 6 | 创建 `behavior_calc`：自定义 ZMK Behavior | 阶段3-5 |
| 7 | 创建 DTS binding YAML | 阶段6 |
| 8 | 更新 `keymap`：双层键位映射 | 阶段3,7 |
| 9 | 更新 `nice_nano_v2.conf`：启用显示+计算器 | 阶段1 |
| 10 | 创建 `Kconfig.defconfig`：Shield 默认值 | 阶段1 |

---

## 7. 风险评估

| 风险 | 等级 | 应对方案 |
|------|------|---------|
| I2C 引脚与矩阵冲突 | 高 | 重新分配行线引脚，释放 P0.17/P0.20 |
| ZMK v0.3 自定义 Behavior 兼容性 | 中 | 遵循 ZMK 内部 Behavior API 模式 |
| 浮点精度问题 | 中 | nRF52840 FPU 仅支持单精度，改用 `float` 替代 `double` |
| LVGL 线程安全 | 中 | Behavior 运行在事件队列，LVGL 在显示线程。使用脏标志 + `k_work` 延迟更新 |
| 连续运算符输入 | 中 | 引擎层面检测并替换最后一个运算符，防止解析错误 |
| Behavior 初始化时序 | 中 | LVGL 可能在 behavior_init 时未就绪，延迟到首次激活计算器时初始化显示 |
| OLED 长表达式溢出 | 低 | LVGL 滚动模式 + 表达式长度上限 64 字符 |
| 除零/溢出错误 | 低 | 异常检测后进入 ERROR 状态，显示错误提示 |
| 功耗增加 | 低 | 计算器退出时关闭 OLED 刷新 |
| P0.09 引脚可用性 | 低 | 需用户确认硬件连接，可替换为其他空闲引脚 |

---

## 8. 审查改进记录

| 改进项 | 原方案 | 改进后 |
|--------|--------|--------|
| 浮点类型 | `double` | `float`（利用硬件 FPU） |
| 显示更新 | 直接操作 LVGL | 脏标志 + k_work 延迟到安全上下文 |
| 运算符处理 | 未处理连续运算符 | 检测并替换最后一个运算符 |
| 初始化时序 | behavior_init 中初始化显示 | 延迟到首次激活时初始化 |
| 字体 | 假设字体可用 | 在 Kconfig 中显式启用 lv_font_unscii_8 |
| 结果格式化 | `%.10g` | `%.7g`（float 精度约 7 位有效数字） |
| 分隔线实现 | `lv_line_create` | `lv_label` + 文本破折号（避免 line widget 依赖） |
| module.yml | 缺少 kconfig 字段 | 添加 `kconfig: Kconfig`（Zephyr 3.5 必需） |
| CMake include | 未包含 ZMK app 路径 | 添加 `target_include_directories(... ${APPLICATION_SOURCE_DIR}/include)` |
| CBPRINTF | 仅启用 FP_SUPPORT | 添加前置依赖 `CONFIG_CBPRINTF_COMPLETE=y` |

---

## 9. 后续扩展接口

计算器引擎预留扩展接口，后续可添加：
- 括号支持 `(` `)`
- 平方根 `√`
- 幂运算 `^`
- 正负切换 `±`
- 计算历史记录
- 更多运算符只需：① 在 `calc_keys.h` 添加宏 ② 实现对应 `calc_op_xxx()` 函数 ③ 在 keymap 绑定
