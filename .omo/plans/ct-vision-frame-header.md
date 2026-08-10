# ct-vision-frame-header - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 云台单片机现在会在原视频流前面识别一段 16 字节的辅助吊射数据头（帧头 C、T，两字节长度，再加 pitch/yaw/distance 三个数值），把这三个吊射数值实时读进来用于吊射瞄准，视频画面照旧原样转发给图传，不受影响。

**Why this approach:** 只改接收解析这一个地方（VD_2rx 函数），既复用了现有的视频接收和转发管线，又重新打通了"视觉数据驱动吊射"（之前这三个值是写死的固定数）。视频包本身一个字节都不动，风险最小。

**What it will NOT do:** 不改任何串口速率、缓冲区或图传帧格式；不动吊射的控制算法和 CAN 广播；不重新启用旧的那套视觉解析代码。

**前提（重要）：** 单片机现在只认带 `C,T` 头的 320 字节新帧。也就是说**发送端（视觉上位机）必须同步改成在视频前面加这 16 字节 CT 头**。在发送端改好之前，旧的 304 字节纯视频帧不会被转发（图传会停）。这是设计的必然结果，你要的正是"两端一起改"。

**Effort:** Quick
**Risk:** Low - 单点改动、缓冲区已够大、视频路径逻辑不变；唯一行为变化是吊射数值从写死变为实时（正是你要的）。前提是发送端同步加头（见上）。
**Decisions to sanity-check:** 帧总长按 320 字节校验（C,T + 长度2 + 三个float + 原视频304）；长度字段**不读取、不校验**（偏移固定已知）；三个数值按小端 float32 读取；发送端需把整帧 320 字节连续发出（中间无 IDLE 间隔）。

Your next move: 这份计划已按你确认的代码定稿，可直接开始执行（改代码由执行会话完成）。如需可再跑一次高精度评审。

---

> TL;DR (machine): Quick / Low risk. Single-site edit in VD_2rx() to parse 16B CT header (C,T,len2,3×f32) into vision_pitch/yaw/distance, forward unchanged 300B video @0x310. + host-side parser unit test. No baud/DMA/buffer/frame-format/CAN/lob-shot-logic changes.

## Scope
### Must have
- 单片机在 USART3 上接收合并帧 `C T | len(2) | pitch(4) | yaw(4) | distance(4) | V D | <原视频帧剩余>`，共 320 字节。
- 从偏移 4/8/12 各解析一个小端 float32，实时写入 `vision_pitch` / `vision_yaw` / `vision_distance`。
- 视频负载（原 VD 帧的 300 字节）保持不变，继续按现有逻辑打包成 `0x310` 图传帧从 USART6 转发。
- 新增 `#define CT_HEADER_LEN 16`。
- 帧校验：`C,T`@0,1 且 `V,D`@16,17 且 `VD_rx_byte == CT_HEADER_LEN + VD_DATA_NUM + 4`（==320）。
- 可选：一个主机侧（PC 编译）单元测试，喂一个构造好的 320 字节 CT 缓冲，断言解析出的三个 float 与视频起始偏移正确。

### Must NOT have (guardrails, anti-slop, scope boundaries)
- 不改 USART3/USART6 波特率、DMA 配置、缓冲区尺寸（`VD_2rx_buf[2][650]` 已够放 320）。
- 不改 `0x310` 输出帧格式、`TC.Data_Concatenation`、`HAL_UART_Transmit_IT` 调用。
- 不动 CAN `0x014` 广播（my_main.cpp:721-724）、`PITCH_Logic()`、吊射控制逻辑。
- 不动消费端 `my_main.cpp:971`（`Pitch_goal = -vision_pitch`）与 721-724 —— 目标仅靠 ISR 写全局变量即达成，消费端已在读这些变量。
- 不重新启用/修改 `stm32f4xx_it.c` / `communication.c` 里旧的 `getReceiveData` / `_response` 路径。
- 不读取、不校验长度字段 `[2..3]`（偏移固定已知，长度字段仅为协议占位）。不加 `volatile`、不重构无关代码。
- 不新增依赖、不改任何其它文件。
- 不得在未经明确决策的情况下悄悄让旧 304 字节纯视频帧无法转发 —— 本计划已明确采用"发送端同步加 CT 头"的方案（sender-lockstep），不做双格式兼容分支。
- 不得在 ARM 工具链缺失时伪造编译成功；若固件构建在执行环境中不可用，必须如实报告并回退到主机侧单元测试作为唯一可执行验证。

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: tests-after —主逻辑改动 + 一个独立主机侧单元测试（纯 C/C++，无 HAL 依赖，桌面编译器 gcc/g++ 或 MSVC 均可）。目标硬件不可用，故不做板上测试；板上实测作为记录在案的用户后续项。
- 编译验证（先探测工具链）：先检测 ARM 交叉工具链 / 构建系统是否存在（CMakePresets.json / CMakeLists.txt / MDK-ARM）。若存在则用工程自带构建确认固件干净编译，退出码 0，无新增 warning/error，并保存构建日志。若 ARM 工具链在执行环境中不可用：如实在 Evidence 文件中记录"固件构建在本环境不可用"，并以主机侧单元测试作为唯一可执行验证——**不得伪造编译成功**。
- 主机侧测试的局限（如实记录）：`test_ct_frame.c` 复刻解析偏移，验证的是"字节布局/字节序数学"的正确性，而非直接执行 ISR 中的 `VD_2rx()` 生产代码；两份偏移常量存在漂移风险，执行者应尽量把偏移常量（`CT_HEADER_LEN`、视频起点=20、4/8/12 字段偏移）以注释或共享定义方式对齐两侧。
- Evidence: .omo/evidence/task-<N>-ct-vision-frame-header.<ext>

## Execution strategy
### Parallel execution waves
> Wave 1 单任务（核心改动）。Wave 2 单任务（主机侧单元测试），与 Wave 1 有输入依赖（测试要复刻 Wave 1 的解析布局）。规模小，故意不过度拆分。

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| 1. VD_2rx() CT 解析 + 转发 | - | 2 | - |
| 2. 主机侧解析单元测试 | 1 (布局定义) | - | - |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. 在 VD_2rx() 中把 'V','D' 帧解析改为 'C','T' 合并帧：解析 3 个视觉 float + 转发不变的视频
  What to do:
    - 在 `Core/Src/my_main.cpp` 第 2153 行 `#define VD_DATA_NUM 300` 附近新增：`#define CT_HEADER_LEN 16`。
    - 把 `VD_2rx()`（当前第 2174-2197 行）内的帧判断块整体替换为如下逻辑：
      ```c
      // 组合帧 = 16B CT 头 (C,T + len2 + pitch4 + yaw4 + distance4) + 原始 VD 视频帧 (VD_DATA_NUM + 4)
      if (VD_2rx_buf[!VD_FIFO][0] == 'C' && VD_2rx_buf[!VD_FIFO][1] == 'T' &&
          VD_2rx_buf[!VD_FIFO][16] == 'V' && VD_2rx_buf[!VD_FIFO][17] == 'D' &&
          VD_rx_byte == CT_HEADER_LEN + VD_DATA_NUM + 4)
      {
          // 解析辅助吊射数据 (小端 float32)
          memcpy(&vision_pitch,    (const void *)(VD_2rx_buf[!VD_FIFO] + 4),  4);
          memcpy(&vision_yaw,      (const void *)(VD_2rx_buf[!VD_FIFO] + 8),  4);
          memcpy(&vision_distance, (const void *)(VD_2rx_buf[!VD_FIFO] + 12), 4);

          if (huart6.gState != HAL_UART_STATE_READY) { tx_drop_cnt++; }

          // 视频负载:原 VD 帧从偏移 CT_HEADER_LEN 开始,其 300B 负载在再 +4 处
          memcpy(NSQD_De_video_buffer, VD_2rx_buf[!VD_FIFO] + CT_HEADER_LEN + 4, VD_DATA_NUM);
          TC.Data_Concatenation(NSQD_De_video_buffer, TX_VD_buf, VD_DATA_NUM, 0x310);
          DMATX = HAL_UART_Transmit_IT(&huart6, TX_VD_buf, FRAME_HEADER_LENGTH + CMD_ID_LENGTH + VD_DATA_NUM + FRAME_TAIL_LENGTH);

          VD_2rx_buf[!VD_FIFO][0] = 0;
          VD_2rx_buf[!VD_FIFO][1] = 0;
          VD_rxcnt++;
          VD_rx_state = 1;
      }
      ```
    - 确认文件顶部已 `#include <string.h>`（memcpy）；若无则确认现有 memcpy 使用处（本函数原本第 2182 行就用了 memcpy，说明已可用），不要重复添加。
  Must NOT do:
    - 不改 `VD_2rx_buf` 尺寸、DMA 重装(第2171行)、IDLE 处理(2162-2169)、`Data_Concatenation`/`HAL_UART_Transmit_IT` 参数。
    - 不动 `vision_*` 的定义/声明（保持 my_main.cpp:265/304/306 与 main.h:39-41 不变，仅在中断里赋值）。
    - 不加 `volatile`，不重构 tx_drop_cnt 诊断，不碰 stm32f4xx_it.c 里注释掉的旧路径。
    - 不对长度字段 `[2..3]` 做强校验拒帧。
  Parallelization: Wave 1 | Blocked by: - | Blocks: 2
  References (executor has NO interview context - be exhaustive):
    - Core/Src/my_main.cpp:2158-2200 (VD_2rx 全函数，改动点)
    - Core/Src/my_main.cpp:2150-2157 (缓冲区与宏定义，CT_HEADER_LEN 加在 2153 附近)
    - Core/Src/my_main.cpp:2174 (原 'V','D' 判断，被替换)
    - Core/Src/my_main.cpp:2182-2184 (原 memcpy + 转发，保留但偏移调整)
    - Core/Src/my_main.cpp:265,304,306 (vision_yaw/pitch/distance 定义，被赋值的目标)
    - Core/Inc/main.h:39-41 (extern 声明，不改)
    - Core/Src/stm32f4xx_it.c:420 (USART3 IRQ 调用 VD_2rx，不改)
    - Core/Src/my_main.cpp:2035 (USART3 DMA arm, VD_RX_NUM=650 足够，不改)
    - Core/Inc/stm32f4xx_it.h:35 (VD_RX_NUM 宏，不改)
    - 帧布局: [0]'C' [1]'T' [2..3]len=12(LE) [4..7]pitch(f32 LE) [8..11]yaw [12..15]distance [16]'V' [17]'D' [18..19]VD子头 [20..319]300B视频
  Acceptance criteria (agent-executable):
    - 工具链探测：先检查 ARM 交叉编译器/构建系统是否可用。可用则运行仓库自带构建（CMakePresets.json / CMakeLists.txt，或 MDK-ARM），退出码 0，无新增 error/warning，保存日志；不可用则在 Evidence 中如实记录"固件构建不可用"，不得伪造成功。
    - 正向 grep（权威）：确认 my_main.cpp 中存在 `#define CT_HEADER_LEN 16`、`== 'C'`、`== 'T'`、`[16] == 'V'`、`[17] == 'D'`、三个 `memcpy(&vision_` 赋值、`VD_rx_byte == CT_HEADER_LEN + VD_DATA_NUM + 4`、以及 `+ CT_HEADER_LEN + 4` 的视频负载偏移。
    - 负向 grep（次要）：确认原判断条件 `VD_rx_byte == VD_DATA_NUM + 4`（不带 `CT_HEADER_LEN +` 前缀）已不再作为独立判断存在。
  QA scenarios (name the exact tool + invocation): 
    - happy: 工具链可用时编译成功（bash 跑构建命令，捕获退出码）；不可用时记录探测结果 + grep 全部命中。Evidence .omo/evidence/task-1-ct-vision-frame-header-build.txt
    - failure: 若编译失败，读取错误定位并修复；记录首次失败与修复 diff。Evidence .omo/evidence/task-1-ct-vision-frame-header-build.txt
  Commit: N (等用户确认后再由用户决定是否提交)

- [x] 2. 新增主机侧解析单元测试（复刻 CT 布局，桌面编译运行）
  What to do:
    - 在 `scripts/` 下新增独立测试文件 `scripts/test_ct_frame.c`（纯 C，无 HAL/STM32 依赖）。
    - 测试内容：构造一个 320 字节缓冲区，按 CT 布局填入已知值（如 pitch=33.7f、yaw=-2.9f、distance=1.25f，[0]='C' [1]='T' [2..3]=12(LE) [16]='V' [17]='D'，[20..319] 填可识别的视频字节序列如递增值）。
    - 复刻 Wave 1 的解析逻辑（同样的偏移 4/8/12 memcpy 取 float，视频负载起点 `CT_HEADER_LEN + 4 == 20`），断言：三个 float bit-exact 相等、帧头字节正确、`memcmp` 视频段与源 300 字节一致、总长 320。
    - 用 `gcc`（若不可用则 `g++` 或 `cl`）编译运行；任一断言失败以非零退出码报错。
  Must NOT do:
    - 不把测试文件加入固件构建（不改 CMakeLists.txt / MDK 工程，避免污染嵌入式构建）；它是独立桌面小程序。
    - 不 include 任何 STM32 HAL 头。不修改 Wave 1 的产品代码去迎合测试。
  Parallelization: Wave 2 | Blocked by: 1 | Blocks: -
  References (executor has NO interview context - be exhaustive):
    - Core/Src/my_main.cpp:2174-2197 (Wave 1 完成后的解析逻辑，测试须与之一致)
    - 帧布局（同 Todo 1）: 偏移 4=pitch,8=yaw,12=distance；视频起点 20；总长 320
    - float 大小 4 字节（.map 已确认），STM32 与桌面 x86 均小端 → bit 布局一致
  Acceptance criteria (agent-executable):
    - `gcc scripts/test_ct_frame.c -o <tmp>/test_ct_frame && <tmp>/test_ct_frame` 退出码 0，打印全部断言 PASS。
  QA scenarios (name the exact tool + invocation):
    - happy: 编译 + 运行退出码 0，Evidence .omo/evidence/task-2-ct-vision-frame-header-test.txt
    - failure: 故意临时把断言的期望 pitch 改错一次，确认测试能以非零退出码报错（证明测试有效），然后改回；记录该负向验证，Evidence 同上
  Commit: N (等用户确认)

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit — APPROVE (explore)
- [x] F2. Code quality review — APPROVE (oracle)
- [x] F3. Real manual QA — APPROVE (general: build exit 0, test exit 0, neg-control non-zero)
- [x] F4. Scope fidelity — APPROVE (explore)

## Commit strategy
- 默认不自动提交。全部完成并通过验证后，由用户决定是否提交。
- 若用户要求提交：单个提交，仅暂存 `Core/Src/my_main.cpp`（+ 可选 `scripts/test_ct_frame.c`），信息示例 `feat(video): prepend CT vision-aim header, parse pitch/yaw/distance in VD_2rx`。
- 提交前 `git status` + `git diff` 复核，不夹带无关改动。不 force、不 amend、不改 git config。

## Success criteria
- `VD_2rx()` 按 320 字节 CT 帧解析：`C,T`@0,1、`V,D`@16,17、总长校验通过时，`vision_pitch/yaw/distance` 被小端 float32 实时更新，300B 视频负载不变地经 `0x310` 从 USART6 转发。
- 新增 `#define CT_HEADER_LEN 16`。
- 若 ARM 工具链可用：固件用仓库自带构建干净编译（退出码 0，无新增 warning/error）；不可用则如实记录并以主机侧测试兜底，绝不伪造编译成功。
- 主机侧单元测试编译运行退出码 0，三个 float bit-exact、视频段 memcmp 一致、总长 320 全部断言通过；负向用例能正确失败。
- 未触碰 Must NOT have 列出的任何范围（波特率/DMA/缓冲尺寸/0x310 格式/CAN 0x014/吊射逻辑/旧 getReceiveData 路径）。
- Final verification wave F1-F4 全部 APPROVE。
