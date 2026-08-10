# fric-speed-3700-5250 - draft

intent: clear
review_required: false
status: awaiting-approval
pending_action: write .omo/plans/fric-speed-3700-5250.md

## Request
键鼠 (keyboard/mouse) when NOT in deploy mode (R not pressed) -> friction wheel 3700 RPM.
遥控 (physical remote) 左上右下 (s1=UP, s2=DOWN = SHANG_XIA_MODE) -> 5250 RPM, for tuning 吊射 (lobbing).

## Grounding (verified)
- Single source file: Core/Src/my_main.cpp.
- Speed setpoints: my_main.cpp:211
  `int16_t MCL_MAX_Speed_Near = 5250, MCL_MAX_Speed_Far = 5250, MCL_MAX_Speed_Now = 3700;`
  (3700 init is transient; overwritten every loop.)
- Speed selector in MCL_Logic(): my_main.cpp:423-430
  - `if (MYmode == DIAO_SHE_MODE || deploy_flag)` -> Far
  - `else if (MYmode != PROTECT_MODE)` -> Near
- SHANG_XIA_MODE (左上右下) decoded at my_main.cpp:377 from s1=UP,s2=DOWN. Currently -> Near.
- Keyboard/mouse over VT13 link can only produce s1==s2 (both UP or both DOWN, RM_Lib.h:861-867),
  so 左上右下 (SHANG_XIA_MODE) is ONLY reachable from the physical DT16 remote. Clean distinction.
- deploy_flag toggled by R key at my_main.cpp:1068.
- Mode macros: my_main.cpp:55-62 (SHANG_XIA_MODE=2, DIAO_SHE_MODE=5).

## Design (recommended)
1. my_main.cpp:211 -> set MCL_MAX_Speed_Near = 3700 (normal combat / keyboard-mouse). Far stays 5250.
2. my_main.cpp:423 -> add `|| MYmode == SHANG_XIA_MODE` so 左上右下 selects Far (5250).
3. Fix stale comments at 425 (//5050) and 429 (//3685) to match real values.

## Adopted default (confirm)
- Keyboard/mouse + deploy (R pressed) keeps Far = 5250 (deploy already selects Far). User only
  specified non-deploy = 3700, so deploy = higher 5250 is the natural reading.
- DIAO_SHE_MODE (中下) keeps 5250 (existing 吊射 mode, unchanged).
- All other combat modes (战斗/双中/小陀螺 etc.) inherit Near = 3700.

## Verification
- Build via project toolchain (CMake preset / MDK-ARM). Static confirm of selector logic.
- No hardware in loop; agent-executed QA = build + code assertions.
