---
slug: tune-2006-lift-pid
status: awaiting-approval
intent: clear
review_required: false
pending-action: write .omo/plans/tune-2006-lift-pid.md
approach: >
  Enable runtime PID parameter tuning over huart1 (text commands) + multi-channel serialplot
  telemetry, connect the currently-disabled motor output, add travel-limit clamp [0,40000] and
  speed-loop current clamp 10000, then have the worker session run a fixed inner-then-outer
  cascade tuning procedure that sends params, reads serialplot data, and iterates by explicit rules.
---

# Draft: tune-2006-lift-pid

## Components (topology ledger)
- C1 | Motor-output enable + safety clamps (connect 0x202 current, travel clamp, speed clamp) | active | main.c:2037-2102
- C2 | Runtime PID param command parser over huart1 | active | RM_Lib.h:28, communication.c
- C3 | Multi-channel serialplot telemetry | active | main.c:1807
- C4 | Fixed auto-tuning procedure (inner speed loop then outer position loop) | active | main.c:2035-2037

## Open assumptions (announced defaults)
- Travel limit direction | [0, +40000], up = positive | zero=stall point, diaoshe target +36700, DUZHUAN current -3000 (down) | reversible (macro)
- Command UART | huart1 (same as printf/serialplot) | user confirmed choice 3 | reversible
- Speed-loop output clamp | 10000 (matches C610 max current cmd) | user confirmed choice 2 | reversible

## Findings (cited - path:lines)
- Cascade: main.c:2035-2037 - position `_mang`(KP0.6,±30000) -> speed `_sp`(KP0.5,±16000) -> current. ~1kHz on CAN RX.
- PID_update is positional PID with per-term clamps: RM_Lib.cpp:1648-1665. I and D currently zero.
- PID objects constructed with hardcoded gains: main.c:90-93.
- Debug UART = huart1 (PRINTF_USART_HANDLE): RM_Lib.h:28; INFO macro line 30. Telemetry printf at main.c:1807.
- CRITICAL: motor output NOT connected - main.c:2102 sends `CAN_2.Send_RM(0x200, BP_output, 0, 0, 0)`; the line carrying Mini_Pitch_output (2101) is commented out. Motor receives 0 current regardless of PID.
- Travel: zero=0 (stall zero), diaoshe target DIAO_SHE_ENCODER_OFFSET=36700 (main.c:284), down current -3000 (main.c:285). Up is positive; range 0..+40000.
- Mini_Pitch_targe is int32_t (main.c:233); Mini_Pitch_output int16_t (main.c:1982).
- Load: 2006 driving a weighted lift mechanism -> gravity steady-state droop -> I term mandatory.

## Decisions (with rationale)
- Approach A (safe auto-iterate), NOT relay-feedback auto-tune - weighted mechanism can slam/break under forced critical oscillation.
- Runtime param tuning first (prereq): hardcoded gains force recompile+flash per iteration = untunable.
- Worker session runs the send-params/read-serialplot/iterate loop automatically; user watches curves as backstop. Planner session cannot access the serial port.
- Inner loop (speed) tuned first, then outer (position) - standard cascade order.
- I term required on both loops to kill gravity droop.

## Scope IN
- Connect motor output with safety.
- Runtime PID param command parser (huart1).
- Multi-channel serialplot telemetry.
- Travel clamp [0,40000], speed-loop clamp 10000.
- Fixed inner-then-outer tuning procedure with explicit adjust rules the worker follows.

## Scope OUT (Must NOT have)
- No relay-feedback / forced-critical-oscillation auto-tuning.
- No changes to other motors/loops (BoPan, friction wheels, yaw, gimbal pitch).
- No changes to CAN/DMA/interrupt low-level config.
- No claim of final "tuned" numbers - starting values + rules only; convergence happens on real hardware.

## Open questions
- (none - all owner-decisions resolved)

## Approval gate
status: awaiting-approval
Presented approach summary in chat; user replied "执行" (approve). Proceeding to write plan.
