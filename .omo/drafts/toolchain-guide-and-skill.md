---
slug: toolchain-guide-and-skill
status: awaiting-approval
intent: clear
review_required: false
pending-action: write .omo/plans/toolchain-guide-and-skill.md
approach: Produce TWO deliverables for STM32 CMake+GCC+OpenOCD onboarding, both Windows-only, Chinese. (1) A beginner tutorial doc at project root (新手配置指南) covering download→PATH→CMakeLists→toolchain→openocd.cfg→tasks.json→keybindings, with every pitfall we actually hit written as an avoid-pitfall section plus "why" explanations, and a CubeMX-coexistence section. (2) A SKILL.md (AI-agent-executable) that, on a blank Windows machine, detects the 4 tools (arm-none-eabi-gcc / cmake / ninja / openocd), installs missing ones (winget-first, never re-installing what already exists), re-detects after user installs, auto-fixes PATH (safe PowerShell user-PATH, not setx), and verifies the full chain by a real configure+build.
---

# Draft: toolchain-guide-and-skill

## Components (topology ledger)
- C1 | Beginner tutorial doc (Chinese, project root) | active | 新工程文件
- C2 | SKILL.md that打通链路 on blank Windows machine | active | 新 skill 文件

## Decisions (with rationale, all user-confirmed)
- Doc language/location: Chinese, project ROOT (e.g. `新手配置指南.md`). User picked root over docs/.
- skill install behavior: ONE skill with TWO modes the AI/user picks at start — (a) 全自动/FULL-AUTO: winget directly installs missing tools; (b) 半自动/SEMI-AUTO: only detect + give official download links for user to install manually, then re-detect + auto-fix PATH + verify. BOTH modes: detect-first, never touch already-installed tools, end with configure+build verification. (avoids version clash on machines that already have half the toolchain.)
- DOC EDIT (user revision): REMOVE "别点 EIDE 编译按钮" guidance (de-emphasize EIDE entirely). REMOVE the "串口打印不动" section — this flow uses CMake LANGUAGE CXX (NOT renaming .c→.cpp), so serial print (INFO) was never touched and the topic never came up; it does not belong in the doc.
- Platform: Windows ONLY. Concrete PowerShell steps, real paths.
- Tool sources: pin to the versions the user actually verified working:
  - arm-none-eabi-gcc: Arm GNU Toolchain 15.2.Rel1 (user has it at D:\GCC\arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi\)
  - CMake: 4.4.0-rc3 (user has at C:\Tools\cmake-...), but doc should recommend latest stable CMake ≥3.22
  - ninja: bundled with STM32CubeCLT at C:\Users\<user>\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin (the "ninja自带" answer)
  - openocd: xPack OpenOCD 0.12.0 (user has it)
- Doc INCLUDES a CubeMX-coexistence section (user still regenerates from .ioc).
- skill closes the loop: 检测 → 缺什么报什么 → (装) → 重新检测 → PATH不对自动改 → 验证. PATH fix uses [Environment]::SetEnvironmentVariable(...,'User'), NOT setx (setx truncates at 1024 chars).

## Findings — the ACTUAL journey / pitfalls to document (cited)
All of these are REAL problems hit during this project. The doc must cover each as an "避坑" (pitfall) box.

1. **.c files containing C++** — Core/Src/main.c:78+ instantiates C++ objects (USER_CAN, PID_class, RC, TuChuan); RM_Lib.h/my_math.h have `class`. Keil compiled .c as C++ regardless; GCC compiles .c as C → `error: unknown type name 'class'`. FIX: `set_source_files_properties(... PROPERTIES LANGUAGE CXX)` on main.c/stm32f4xx_it.c/communication.c/my_math.c. (CMakeLists.txt:47)
2. **C-lenient conversions rejected by C++** — main.c:696/703/2136 pass int32_t* to int* → warning in C, hard error in C++. FIX: scope `-fpermissive` to only those force-C++ files. (CMakeLists.txt COMPILE_OPTIONS)
3. **Missing sources not in build** — RM_Lib.cpp, rgb_debug.cpp, my_math.c, CP_System.c, communication.c, example_data.c, nmea_dec.c, hipnuc_dec.c. FIX: add to target_sources. (CMakeLists.txt:55-65)
4. **Include path** — RM2023_Lib_V1.2 must be on include path or `RM_Lib.h: No such file`. (CMakeLists.txt:70)
5. **Toolchain C++ flags bug** — CMAKE_CXX_FLAGS didn't reliably get per-build-type -O/-g. FIX in cmake/gcc-arm-none-eabi.cmake:32-43 (apply Debug/Release -O flags to both C and CXX).
6. **No hex/bin** — needed objcopy POST_BUILD. FIX: add_custom_command with objcopy -O ihex / -O binary + size. (CMakeLists.txt:86-90)
7. **ninja not on PATH** — build preset uses Ninja generator; ninja found only in STM32CubeCLT bundle. FIX: add that bundle dir to user PATH. Symptom: `ninja: 无法将...识别为cmdlet` or CMake "generator Ninja not found".
8. **EIDE plugin hijacks build** — user pressed EIDE build button (arm-none-eabi-gcc -xc -std=c11) instead of CMake task → compiled .c as C → class errors + `ERROR build failed !`. FIX: use CMake tasks (Ctrl+Shift+B / Alt+,), disable EIDE, remove its keybindings.
9. **VSCode Profiles trap** — user runs profile `ff7ccb6`; keybindings written to default profile's User/keybindings.json weren't read. The real file was profiles/ff7ccb6/keybindings.json. Symptom: keys "没反应", Keyboard Shortcuts search shows "no binding".
10. **Alt+, taken by GitLens** (`gitlens.key.alt+,`); F7/Ctrl+Alt+D taken by EIDE. Must解绑 (prefix command with `-`) or disable those extensions.
11. **Default Keybindings is read-only** — user opened "Open DEFAULT Keyboard Shortcuts (JSON)" (read-only) instead of "Open Keyboard Shortcuts (JSON)" (writable).
12. **Folder vs Workspace open mode** — opening `.code-workspace` vs "Open Folder" reads DIFFERENT settings (`.code-workspace` settings block overrides `.vscode/settings.json`) AND may switch profile (so keybindings disappear). Recommend: open the FOLDER, keep config in .vscode/.
13. **Red squiggles on classes** — cpptools IntelliSense parses .c as C. Cause: `.code-workspace`/settings `"files.associations": {"*.c":"c","*.h":"c"}` and `C_Cpp.default.configurationProvider: cl.eide`. FIX: set *.c/*.h → cpp, remove cl.eide provider, add `C_Cpp.default.compileCommands` → build/Debug/compile_commands.json, then C/C++: Reset IntelliSense Database.
14. **"没有找到要运行的任务"** — VSCode opened the WRONG folder level (there are nested dirs: ...20260120 outer, ...20260111 correct, Gimbal_24hero inner). .vscode/tasks.json only loads when the opened root is exactly ...20260111.
15. **How to read a compile error** — only look at lines with `error:`, ignore `warning:`. Format file:line:col. The classic: main.c missing `;` → `expected ';' before ...`.
16. **Serial print stays as-is** — INFO (RM_Lib.h:30) = sprintf+HAL_UART_Transmit; NOT printf redirect. Doc should note we do NOT add _write/fputc.

## The working config artifacts (to embed in doc as reference)
- CMakePresets.json: Ninja generator, toolchainFile cmake/gcc-arm-none-eabi.cmake, Debug/Release/... presets.
- cmake/gcc-arm-none-eabi.cmake: TARGET_FLAGS `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`, g++ as compiler+linker, nano.specs, -lstdc++ -lsupc++, objcopy/size defined.
- openocd.cfg: `source [find interface/cmsis-dap.cfg]` / `transport select swd` / `source [find target/stm32f4x.cfg]` / `adapter speed 4000`.
- tasks.json: build (cmake --build --preset Debug, isDefault), flash (openocd program elf verify reset exit), build and flash (dependsOrder sequence).
- keybindings (in the ACTIVE profile): alt+oem_comma→runTask build, alt+n→flash, alt+m→build and flash.
- Flash memory sanity: F405 = 1MB flash / 192KB RAM; example build was FLASH 7.96%, RAM 11.93%.

## Scope IN
- Doc: full 0→1 Windows tutorial, Chinese, project root, all 16 findings as pitfalls-with-why, CubeMX-coexistence section, embedded reference configs.
- Skill: SKILL.md, detect 4 tools → winget-install missing (never touch existing) → re-detect → auto-fix user PATH (safe PowerShell) → verify with configure+build → report打通.

## Scope OUT (Must NOT have)
- No Linux/macOS content.
- Skill must NOT reinstall or overwrite tools that already exist / already on PATH.
- Skill must NOT use setx for PATH (truncation risk); use SetEnvironmentVariable User scope; back up PATH first.
- Do NOT modify the user's actual project source, CMakeLists, tasks, keybindings as part of THIS task (this task only PRODUCES the doc + skill; the earlier plan already did the project config).
- Doc must NOT instruct adding printf redirection (serial print stays as INFO/sprintf).
- Do NOT machine-translate to English; Chinese only.

## Open questions
(none — all resolved)

## Approval gate
status: awaiting-approval
pending-action: write .omo/plans/toolchain-guide-and-skill.md then user runs /start-work
