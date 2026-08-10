# nvic-priority-realign - Work Plan

## TL;DR (For humans)

**What you'll get:** �?Newcode 工程（`D:\Newcode\Gimbal_24hero_CH010_20260111`）的中断优先级从 demo �?几乎�?0 级保数据不丢�?改成�?*控制重要�?*分层，与模板工程理念对齐（但数值按你的实战排序自定，不是照抄模板）�?
**Why this approach:** 分组�?GROUP_4�? 位全抢占�? 最高）。demo 把控制环、IMU、遥控、CAN、图传全�?0 级，同级不能互相抢占，导致图传重活（VD_2rx）会堵住 2kHz 控制�?�?云台抖动。分层后关键控制不被次要中断打断�?
**What it will NOT do:** 不改任何中断处理逻辑、不�?DMA 流配置、不�?VD_2rx 结构、不�?TIM3/TIM11（未使用）、不�?GROUP_4 分组。只�?NVIC 优先级数值�?
**Effort:** 极小�? 文件、约 13 处数值改动）�?*Risk:** 中（中断时序敏感，但纯数值调整，可编译验�?+ 实机观察）�?
**Decisions（已与用户确认）:**
- 控制链全�?0 级最高、同级不互抢：陀螺仪(UART4) + 电机(CAN1、CAN2) + 控制�?TIM9) + 所�?DMA�?- 澄清：电机走 CAN 不走串口。串�?(UART4) 只是陀螺仪 IMU；电�?= CAN1(摩擦轮等) + CAN2(云台LK)�?- 串口1 = 最不重要。TIM3/TIM11 未使用�?- 图传/遥控/IMU定时器等 = 2 级；次要定时�?4~5 级；看门�?串口1 = 6 级�?
## Scope

**事实澄清（重要）�?* 电机�?CAN，不走串口。CAN1 = 摩擦轮等 6 个电�?0x201~0x206)、Mini_Pitch、拨盘；CAN2 = 云台 LK6010 Pitch/Yaw 电机(0x141/0x142)。串�?(UART4) = 陀螺仪 IMU(hipnuc 协议)，只管姿态反馈。控制链 = 陀螺仪(UART4) + 电机(CAN1/CAN2) + 控制�?TIM9) + DMA，全�?0 级最高�?
**IN（只�?NVIC 优先级数值）�?*
- `Core/Src/can.c`：CAN1�?�?，升入控制链最高档）；CAN2 仅核对（�?0�?- `Core/Src/tim.c`：TIM1_UP、TIM4、TIM5、TIM6、TIM7、TIM8(两个向量)
- `Core/Src/usart.c`：UART5、USART1、USART3、USART6（UART4 已是 0，保持）

**OUT / Must-NOT-Have�?*
- 不改 `dma.c`（DMA 流已�?0，保持）�?- 不改任何 IRQHandler 函数体、DMA 配置、缓冲区、VD_2rx�?- 不动 TIM3_IRQn、TIM11（用户确认未使用）�?- 不改 UART4(陀螺仪) 优先级（已是 0，属控制链最高）�?- 不改 TIM9(控制�?、CAN2 优先级（已是 0，属控制链最高）�?- 不改 NVIC_PRIORITYGROUP_4 分组�?- 不新�?删除任何中断使能�?
## Verification strategy

编译验证为主（无测试框架、离开实机无法功能 QA）：
- �?`cmake --build --preset Debug` 编译，必须干净通过（可有原�?warning，不得有�?error）�?- 逐一 grep 确认每个 IRQn 的最终优先级数�?= 目标值，且重复设置点全部一致�?- 实机验证（用户执行）：观察收图传/视频时云台是否还抖、IMU/遥控/CAN 是否正常�?
## Execution strategy

单文件顺序编辑，全部改完后一次编译。无并行必要�? 个小文件）�?关键陷阱：`TIM1_BRK_TIM9_IRQn`（tim.c:474�?18）和 `TIM8_BRK_TIM12_IRQn`（tim.c:601�?44）各出现两次�?*必须两处都改成同�?*，否则后执行�?MspInit 覆盖前面的�?
## 目标优先级总表�?=最高，GROUP_4�?
| 中断向量 | 目标 | 当前 | 文件:�?|
|---|---|---|---|
| UART4_IRQn (串口4 陀螺仪 IMU) | 0 | 0 | usart.c:227 (不变) |
| TIM1_BRK_TIM9_IRQn (TIM9 2kHz控制�? | 0 | 0 | tim.c:474,618 (不变) |
| CAN1_RX0_IRQn (摩擦轮等电机) | 0 | 3 | can.c:125 |
| CAN2_RX1_IRQn (云台LK电机) | 0 | 0 | can.c:156 (不变) |
| 所�?DMA �?| 0 | 0 | dma.c (不变) |
| UART5_IRQn (遥控) | 2 | 0 | usart.c:281 |
| TIM8_BRK_TIM12_IRQn (TIM12 IMU 1kHz) | 2 | 4 | tim.c:601,644 |
| USART3_IRQn (图传接收) | 2 | 0 | usart.c:391 |
| USART6_IRQn (图传/VT13) | 2 | 0 | usart.c:437 |
| TIM1_UP_TIM10_IRQn (TIM1 50Hz) | 2 | 7 | tim.c:476 |
| TIM7_IRQn (160Hz) | 4 | 7 | tim.c:586 |
| TIM5_IRQn (800Hz) | 5 | 0 | tim.c:556 |
| TIM8_UP_TIM13_IRQn (TIM8 20Hz) | 5 | 6 | tim.c:603 |
| TIM4_IRQn (RGB灯带) | 5 | 0 | tim.c:541 |
| TIM6_DAC_IRQn (看门�? | 6 | 10 | tim.c:571 |
| USART1_IRQn | 6 | 0 | usart.c:327 |

## Todos

- [x] 1. `Core/Src/can.c:125`: �?`HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 3, 0)` �?`0` �?CAN1(摩擦轮等电机)升为最�?0（属控制链）�?  - Refs: can.c:125 (CAN1 MspInit)�?  - Acceptance: grep `CAN1_RX0_IRQn` 优先级参�?== 0；无其他 CAN1 设置点�?  - QA happy: 编译通过。QA fail: 若数值非0则失败�?  - Commit: `nvic: CAN1_RX0 priority 3->0`

- [x] 2. `Core/Src/can.c:156`: 确认 `HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 0, 0)` 已是 `0`（云台LK电机，属控制链，保持最高）�?无需改动，仅核对�?  - Refs: can.c:156 (CAN2 MspInit)�?  - Acceptance: grep `CAN2_RX1_IRQn` 优先�?== 0（本就为0，确认未被误改）�?  - QA happy: 编译通过。QA fail: 数值非0�?  - Commit: (no-op，若已为0则无改动)

- [x] 3. `Core/Src/usart.c:281`: �?`UART5_IRQn` 优先�?`0` �?`2` �?遥控接收降为 2�?  - Refs: usart.c:281 (UART5 MspInit)�?  - Acceptance: grep `UART5_IRQn` 优先�?== 2；UART4_IRQn(227) 保持 0 未动�?  - QA happy: 编译通过。QA fail: 数值非2 或误改了 UART4�?  - Commit: `nvic: UART5 priority 0->2`

- [x] 4. `Core/Src/usart.c:327`: �?`USART1_IRQn` 优先�?`0` �?`6` �?串口1 降为最低档 6�?  - Refs: usart.c:327 (USART1 MspInit)�?  - Acceptance: grep `USART1_IRQn` 优先�?== 6�?  - QA happy: 编译通过。QA fail: 数值非6�?  - Commit: `nvic: USART1 priority 0->6`

- [x] 5. `Core/Src/usart.c:391`: �?`USART3_IRQn` 优先�?`0` �?`2` �?图传接收降为 2�?  - Refs: usart.c:391 (USART3 MspInit)�?  - Acceptance: grep `USART3_IRQn` 优先�?== 2�?  - QA happy: 编译通过。QA fail: 数值非2�?  - Commit: `nvic: USART3 priority 0->2`

- [x] 6. `Core/Src/usart.c:437`: �?`USART6_IRQn` 优先�?`0` �?`2` �?图传/VT13降为 2�?  - Refs: usart.c:437 (USART6 MspInit)�?  - Acceptance: grep `USART6_IRQn` 优先�?== 2�?  - QA happy: 编译通过。QA fail: 数值非2�?  - Commit: `nvic: USART6 priority 0->2`

- [x] 7. `Core/Src/tim.c:476`: �?`TIM1_UP_TIM10_IRQn` 优先�?`7` �?`2` �?TIM1(50Hz舵机)升为 2�?  - Refs: tim.c:476 (TIM1 MspInit)。注�?TIM1_UP �?TIM1 �?update 向量，与 TIM9(BRK)向量不同�?  - Acceptance: grep `TIM1_UP_TIM10_IRQn` 优先�?== 2；`TIM1_BRK_TIM9_IRQn`(474,618) 仍为 0�?  - QA happy: 编译通过。QA fail: 数值非2 或误动了 BRK_TIM9�?  - Commit: `nvic: TIM1_UP priority 7->2`

- [x] 8. `Core/Src/tim.c:541`: �?`TIM4_IRQn` 优先�?`0` �?`5` �?RGB灯带降为 5�?  - Refs: tim.c:541 (TIM4 MspInit)�?  - Acceptance: grep `TIM4_IRQn` 优先�?== 5�?  - QA happy: 编译通过。QA fail: 数值非5�?  - Commit: `nvic: TIM4 priority 0->5`

- [x] 9. `Core/Src/tim.c:556`: �?`TIM5_IRQn` 优先�?`0` �?`5` �?TIM5(800Hz)降为 5�?  - Refs: tim.c:556 (TIM5 MspInit)�?  - Acceptance: grep `TIM5_IRQn` 优先�?== 5�?  - QA happy: 编译通过。QA fail: 数值非5�?  - Commit: `nvic: TIM5 priority 0->5`

- [x] 10. `Core/Src/tim.c:571`: �?`TIM6_DAC_IRQn` 优先�?`10` �?`6` �?看门狗设�?6�?  - Refs: tim.c:571 (TIM6 MspInit)�?  - Acceptance: grep `TIM6_DAC_IRQn` 优先�?== 6�?  - QA happy: 编译通过。QA fail: 数值非6�?  - Commit: `nvic: TIM6 priority 10->6`

- [x] 11. `Core/Src/tim.c:586`: �?`TIM7_IRQn` 优先�?`7` �?`4` �?TIM7(160Hz)升为 4�?  - Refs: tim.c:586 (TIM7 MspInit)�?  - Acceptance: grep `TIM7_IRQn` 优先�?== 4�?  - QA happy: 编译通过。QA fail: 数值非4�?  - Commit: `nvic: TIM7 priority 7->4`

- [x] 12. `Core/Src/tim.c:601` �?`tim.c:644`: �?`TIM8_BRK_TIM12_IRQn` 两处 `4` 全部�?�?`2` �?TIM12(IMU 1kHz)升为 2�?  - Refs: tim.c:601 (TIM8 MspInit)、tim.c:644 (可能�?TIM12 MspInit 或另一�?。两处必须同值�?  - Acceptance: grep `TIM8_BRK_TIM12_IRQn` 全部出现点优先级 == 2（两处一致）�?  - QA happy: 编译通过。QA fail: 两处不一致或�?�?  - Commit: `nvic: TIM8_BRK_TIM12 priority 4->2 (both sites)`

- [x] 13. `Core/Src/tim.c:603`: �?`TIM8_UP_TIM13_IRQn` 优先�?`6` �?`5` �?TIM8(20Hz)降为 5�?  - Refs: tim.c:603 (TIM8 MspInit)�?  - Acceptance: grep `TIM8_UP_TIM13_IRQn` 优先�?== 5�?  - QA happy: 编译通过。QA fail: 数值非5�?  - Commit: `nvic: TIM8_UP priority 6->5`

## Final verification wave

- [x] F1. 全量核对：grep 三个文件所�?`HAL_NVIC_SetPriority`，逐条对照"目标优先级总表"，确认每个数值正确、重复向量（TIM1_BRK_TIM9、TIM8_BRK_TIM12）各处一致、UART4 �?TIM9 保持 0、DMA 未改�?- [x] F2. 编译：`cmake --build --preset Debug`，必须无�?error（原�?warning 可接受），生�?Gimbal_Demo.elf�?- [x] F3. （用户实机）验证：收图传/视频时云台不再抖；IMU、遥控、CAN 正常；看门狗、串�? 无异常�?
## Commit strategy

可合并为单次提交 `nvic: realign interrupt priorities by control-criticality (demo->field)`，或按上述每�?Commit 行分提交。用户偏好为准。仅当用户明确要求才提交�?
## Success criteria

- 三个文件 13 处优先级数值全部等于目标表�?- TIM1_BRK_TIM9_IRQn、TIM8_BRK_TIM12_IRQn 的重复设置点各自一致�?- UART4_IRQn、TIM1_BRK_TIM9_IRQn 仍为 0；DMA 全部仍为 0�?- 工程干净编译通过�?- 未改动任何中断逻辑 / DMA 配置 / TIM3 / TIM11 / 分组�?