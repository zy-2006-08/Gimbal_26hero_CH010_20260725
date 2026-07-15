# cmake-gcc-cpp-openocd - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 你的 F405 工程能用 CMake+GCC 一次编译通过(包含之前只有 Keil 能编�?C++ �?,自动生成 .elf/.hex/.bin,并且�?VSCode 里用 Alt+, 编译、Alt+N 烧录(CMSIS-DAP + OpenOCD)、Alt+M 编译并烧录�?
**Why this approach:** 关键发现是你�?main.c / stm32f4xx_it.c / communication.c 虽然�?.c 后缀,里面却是 C++ 代码(创建�?C++ 对象、include 了满�?class �?RM_Lib.h)。按你的选择,�?CMake �?set_source_files_properties 强制把这三个文件�?C++ �?不改文件名、不改内�?这样 STM32CubeMX 以后还能安全地重新生成代码�?
**What it will NOT do:** 不动你的串口打印(INFO / Mini_PC_INFO 一个字节都不改);不加 printf 重定�?不重命名或改�?main.c 等生成文件的内容�?
**Effort:** Medium
**Risk:** Low-Medium - 主要风险�?main.c/stm32f4xx_it.c 是通过子目�?INTERFACE 库进入构建的,LANGUAGE 属性可能不直接生效,计划里已写明验证方法�?-x c++ 的兜底方案�?**Decisions to sanity-check:** C++ 标准�?C++17;一键任务默认针�?Debug 预设;快捷键会由执行环节直接写进你�?VSCode 用户�?keybindings.json(不用手动�?,同时会清掉里面两个旧�?EIDE 快捷�?F7 编译、Ctrl+Alt+D 下载)。这几点你过一眼�?
Your next move: 认可就可以进入执行环�?由单独的 worker 按此计划改代�?;或让我调整。全部执行细节在下方�?
---

> TL;DR (machine): Medium effort, Low-Med risk. Deliverables: C++ builds under CMake+GCC for STM32F405RGT6, .bin/.hex generated, CMSIS-DAP+OpenOCD one-key build/flash/build+flash with Alt+, / Alt+N / Alt+M. Serial printing untouched.

## Scope
### Must have
- CMake+arm-none-eabi-gcc compiles the whole project INCLUDING the C++ RM library (RM_Lib.cpp, rgb_debug.cpp) with zero errors.
- The 3 mixed files (Core/Src/main.c, Core/Src/stm32f4xx_it.c, RM2023_Lib_V1.2/communication.c) compile as C++ WITHOUT renaming or editing their content.
- All currently-missing sources added to build: RM_Lib.cpp, rgb_debug.cpp, my_math.c, CP_System.c, example_data.c, nmea_dec.c, hipnuc_dec.c.
- RM2023_Lib_V1.2 added to the include path.
- Release/Debug optimization+debug flags apply to C++ files too (toolchain bug fixed).
- Build produces Gimbal_Demo.elf AND Gimbal_Demo.hex AND Gimbal_Demo.bin.
- openocd.cfg for CMSIS-DAP + SWD + stm32f4x present at project root.
- .vscode/tasks.json provides 3 tasks: build, flash, build+flash �?via cmake and openocd (not EIDE).
- .vscode/keybindings.json binds Alt+, = build, Alt+N = flash, Alt+M = build+flash. Global, no `when`.

### Must NOT have (guardrails, anti-slop, scope boundaries)
- Do NOT modify serial printing: INFO (RM_Lib.h:30) and Mini_PC_INFO (communication.h:28) stay byte-for-byte.
- Do NOT add printf redirection (_write / fputc / __io_putchar).
- Do NOT rename main.c / stm32f4xx_it.c / communication.c; do NOT edit their code content (only set CMake LANGUAGE property).
- Do NOT put user sources or LANGUAGE hacks into cmake/stm32cubemx/CMakeLists.txt (CubeMX regenerates it).
- Do NOT add a `when` clause to keybindings.
- Do NOT reduce scope to an MVP/subset.

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none (embedded firmware, no unit-test harness in repo) + agent-executed build/flash verification. The "test" is a clean cross-compile and a successful OpenOCD flash/verify.
- Evidence: .omo/evidence/task-<N>-cmake-gcc-cpp-openocd.<ext>

## Execution strategy
### Parallel execution waves
> Wave 1 (build system) is largely one coherent CMake edit �?kept as sequenced todos because they touch overlapping regions of the same file. Wave 2 (tooling: openocd.cfg, tasks.json, keybindings.json) is independent files, fully parallelizable. Wave 3 = final verification.

- Wave 1 (CMake/build): T1, T2, T3, T4, T5, T6 �?mostly same-file, sequence to avoid edit conflicts.
- Wave 2 (tooling): T7, T8, T9 �?T7→T8→T9 chain (openocd.cfg �?tasks reference it �?keybindings reference task names).
- Wave 3: final verification.

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| T1 enable CXX | - | T2,T3,T5 | T7,T8,T9 |
| T2 add sources | T1 | T5 | T7,T8,T9 |
| T3 force-C++ 3 files | T1 | T5 | T7,T8,T9 |
| T4 include dir | - | T5 | T7,T8,T9 |
| T5 objcopy hex/bin | T1 | - (flash uses elf) | T7,T8,T9 |
| T6 toolchain CXX flags | - | build correctness | T1-T4,T7,T8,T9 |
| T7 openocd.cfg | - | T8 | T1-T6 |
| T8 workspace tasks.json | T7 | T9 | T1-T6 |
| T9 user keybindings.json | T8 | - | T1-T6 |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [x] 1. CMakeLists.txt: enable C++ language + set C++17 standard
  What to do / Must NOT do: In the MAIN CMakeLists.txt (project root CMakeLists.txt), change line 31 `enable_language(C ASM)` to `enable_language(C CXX ASM)`. Near the C standard block (lines 11-13), add `set(CMAKE_CXX_STANDARD 17)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`, `set(CMAKE_CXX_EXTENSIONS ON)`. Must NOT edit cmake/stm32cubemx/CMakeLists.txt. Must NOT touch serial printing.
  Parallelization: Wave 1 | Blocked by: none | Blocks: T2,T3,T5
  References: CMakeLists.txt:11-13 (C standard block), CMakeLists.txt:31 (enable_language). cmake/gcc-arm-none-eabi.cmake:5-7,15-16 already declare CXX compiler forced + g++ �?so CXX just needs enabling.
  Acceptance criteria (agent-executable): `cmake --preset Debug` reconfigures with no "CXX not enabled" errors; CMakeCache shows CMAKE_CXX_COMPILER=arm-none-eabi-g++.
  QA scenarios: happy = configure succeeds; failure = temporarily rename a .cpp to confirm CXX would have been invoked (revert). Evidence .omo/evidence/task-1-cmake-gcc-cpp-openocd.txt
  Commit: N (batch with wave) | build(cmake): enable C++ language and C++17 standard

- [x] 2. CMakeLists.txt: add all missing C and C++ sources to the target
  What to do / Must NOT do: In the MAIN CMakeLists.txt `target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...)` block (lines 49-51), add these user sources with paths relative to project root: RM2023_Lib_V1.2/RM_Lib.cpp, RM2023_Lib_V1.2/rgb_debug.cpp, RM2023_Lib_V1.2/my_math.c, RM2023_Lib_V1.2/CP_System.c, RM2023_Lib_V1.2/communication.c, Core/Src/example_data.c, Core/Src/nmea_dec.c, Core/Src/hipnuc_dec.c. Do NOT add main.c/stm32f4xx_it.c here (already provided by the stm32cubemx interface lib). Must NOT add these to cmake/stm32cubemx/CMakeLists.txt.
  Parallelization: Wave 1 | Blocked by: T1 | Blocks: T5
  References: CMakeLists.txt:49-51 (empty user target_sources). Missing-source list from draft Findings. main.c:30-35 includes RM_Lib.h/communication.h/my_math.h/hipnuc_dec.h/CP_System.h/rgb_debug.h/example_data. communication.c is force-C++ in T3.
  Acceptance criteria (agent-executable): `cmake --build --preset Debug` compiles all 8 added files (no "undefined reference" to their symbols at link).
  QA scenarios: happy = all objects built + linked; failure = check build.ninja lists each new source. Evidence .omo/evidence/task-2-cmake-gcc-cpp-openocd.txt
  Commit: N | build(cmake): add RM library and missing core sources

- [x] 3. CMakeLists.txt: force main.c / stm32f4xx_it.c / communication.c to compile as C++
  What to do / Must NOT do: In the MAIN CMakeLists.txt (after add_executable and after add_subdirectory(cmake/stm32cubemx) so the sources exist), add `set_source_files_properties(${CMAKE_SOURCE_DIR}/Core/Src/main.c ${CMAKE_SOURCE_DIR}/Core/Src/stm32f4xx_it.c ${CMAKE_SOURCE_DIR}/RM2023_Lib_V1.2/communication.c PROPERTIES LANGUAGE CXX)`. Use absolute ${CMAKE_SOURCE_DIR} forms so the property binds to the same source entry the stm32cubemx interface lib added. Must NOT rename these files. Must NOT edit their contents. Must NOT touch INFO/Mini_PC_INFO.
  Parallelization: Wave 1 | Blocked by: T1 | Blocks: T5
  References: main.c:78-133 (global C++ object instantiation), stm32f4xx_it.c:36,67 (extern RC YK / TuChuan TC), communication.c:3 (#include RM_Lib.h). cmake/stm32cubemx/CMakeLists.txt:24,31 provide main.c & stm32f4xx_it.c via INTERFACE lib. CRITICAL: because main.c/stm32f4xx_it.c enter the target through a subdirectory INTERFACE lib, the LANGUAGE property set in the main CMakeLists may not bind cleanly �?executor MUST verify (see QA). Fallback if it does not bind: use `set_source_files_properties(... PROPERTIES COMPILE_OPTIONS "-x;c++")`, or add the 3 files directly to the main target with the LANGUAGE property.
  Acceptance criteria (agent-executable): build/Debug/compile_commands.json entry for main.c invokes arm-none-eabi-g++ (or gcc with -x c++). Same for stm32f4xx_it.c and communication.c.
  QA scenarios: happy = compile_commands.json shows g++/-x c++ for all 3; failure = a `class` error from gcc means the property did not bind �?apply fallback. Evidence .omo/evidence/task-3-cmake-gcc-cpp-openocd.txt (paste the 3 compile_commands entries)
  Commit: N | build(cmake): compile mixed .c files as C++ without renaming

- [x] 4. CMakeLists.txt: add RM2023_Lib_V1.2 to the include path
  What to do / Must NOT do: In the MAIN CMakeLists.txt `target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ...)` block (lines 54-56), add `RM2023_Lib_V1.2`. Must NOT remove existing CubeMX include dirs.
  Parallelization: Wave 1 | Blocked by: none | Blocks: T5
  References: CMakeLists.txt:54-56 (empty user include block). main.c:30 `#include "RM_Lib.h"` resolves only if RM2023_Lib_V1.2 is on the path. RM_Lib.h, communication.h, my_math.h, CP_System.h, rgb_debug.h all live in RM2023_Lib_V1.2.
  Acceptance criteria (agent-executable): no "RM_Lib.h: No such file or directory" during build.
  QA scenarios: happy = headers resolve; failure = grep build log for "No such file". Evidence .omo/evidence/task-4-cmake-gcc-cpp-openocd.txt
  Commit: N | build(cmake): add RM library include directory

- [x] 5. CMakeLists.txt: generate .hex and .bin via POST_BUILD objcopy
  What to do / Must NOT do: In the MAIN CMakeLists.txt, after add_executable and target config, add `add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD ...)` running `${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex` and `${CMAKE_OBJCOPY} -O binary -S $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin`, plus `${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>`. Output next to the .elf in the build dir. Must NOT change linker script or specs.
  Parallelization: Wave 1 | Blocked by: T1 | Blocks: none (flash uses elf)
  References: cmake/gcc-arm-none-eabi.cmake:17 (CMAKE_OBJCOPY defined), :18 (CMAKE_SIZE). No existing objcopy step. Executable suffix .elf (gcc-arm-none-eabi.cmake:20-22).
  Acceptance criteria (agent-executable): after `cmake --build --preset Debug`, build/Debug/ contains Gimbal_Demo.hex and Gimbal_Demo.bin (non-empty).
  QA scenarios: happy = both files exist and non-empty; failure = ls build/Debug and confirm. Evidence .omo/evidence/task-5-cmake-gcc-cpp-openocd.txt
  Commit: N | build(cmake): emit hex and bin via objcopy post-build

- [x] 6. Toolchain: apply Debug/Release opt+debug flags to C++ too
  What to do / Must NOT do: In cmake/gcc-arm-none-eabi.cmake, fix the ordering bug: line 39 composes CMAKE_CXX_FLAGS from CMAKE_C_FLAGS BEFORE the -O0/-g3 (Debug, line 32) and -Os/-g0 (Release, line 35) are appended to C_FLAGS. Restructure so per-build-type opt/debug flags also reach CMAKE_CXX_FLAGS (e.g. append the Debug/Release block to both C and CXX, or compose CXX_FLAGS after the build-type block and re-add -fno-rtti -fno-exceptions -fno-threadsafe-statics). Must NOT change -mcpu/-mfpu/-mfloat-abi, linker script, or specs.
  Parallelization: Wave 1 (independent file) | Blocked by: none | Blocks: correct optimized build
  References: cmake/gcc-arm-none-eabi.cmake:29-30 (C flags base), :31-36 (Debug/Release appended to C_FLAGS only), :39 (CXX_FLAGS composed too early), :27 TARGET_FLAGS must stay.
  Acceptance criteria (agent-executable): Debug build �?compile_commands.json entry for a .cpp shows -O0 -g3; Release �?-Os. C++ flags (-fno-rtti -fno-exceptions) still present.
  QA scenarios: happy = flags present per build type; failure = configure Release preset and inspect a .cpp command. Evidence .omo/evidence/task-6-cmake-gcc-cpp-openocd.txt
  Commit: N | build(toolchain): apply build-type optimization flags to C++

- [x] 7. New file: openocd.cfg for CMSIS-DAP + SWD + STM32F4x
  What to do / Must NOT do: Create openocd.cfg at project root: `source [find interface/cmsis-dap.cfg]`, `transport select swd`, `source [find target/stm32f4x.cfg]`, optional `adapter speed 4000`. Must NOT assume ST-Link.
  Parallelization: Wave 2 | Blocked by: none | Blocks: T8
  References: user confirmed adapter = CMSIS-DAP, tool = OpenOCD, chip = STM32F405RGT6 (stm32f4x). Flash target = build/Debug/Gimbal_Demo.elf.
  Acceptance criteria (agent-executable): `openocd -f openocd.cfg -c "init; reset halt; exit"` connects+halts (needs HW). Without HW: `openocd -f openocd.cfg -c "exit"` parses config with no syntax error.
  QA scenarios: happy = "target halted"; failure = parse-only run catches syntax error. Evidence .omo/evidence/task-7-cmake-gcc-cpp-openocd.txt
  Commit: N | build(openocd): add CMSIS-DAP SWD config for STM32F405

- [x] 8. .vscode: rewrite workspace tasks.json (3 CMake/OpenOCD tasks)
  What to do / Must NOT do: Rewrite .vscode/tasks.json (workspace) to REPLACE the EIDE-command tasks with:
    (a) "build" �?`cmake --build --preset Debug` (group build, isDefault true);
    (b) "flash" �?`openocd -f openocd.cfg -c "program build/Debug/Gimbal_Demo.elf verify reset exit"`;
    (c) "build and flash" �?task with `dependsOrder: sequence`, `dependsOn: ["build","flash"]`.
  Must NOT keep EIDE commands.
  Parallelization: Wave 2 | Blocked by: T7 | Blocks: T9
  References: .vscode/tasks.json:1-40 (current EIDE tasks to replace). CMakePresets.json:44-46 (Debug build preset). openocd.cfg (T7).
  Acceptance criteria (agent-executable): tasks.json valid JSON; "build and flash" runs build then flash in sequence.
  QA scenarios: happy = Tasks: Run Task lists build/flash/build and flash, each invokes right command; failure = JSON-lint the file. Evidence .omo/evidence/task-8-cmake-gcc-cpp-openocd.txt
  Commit: N | build(vscode): CMake+OpenOCD tasks replacing EIDE

- [x] 9. Edit USER keybindings.json: remove EIDE keys, add Alt+, / Alt+N / Alt+M
  What to do / Must NOT do: Directly edit the USER-level keybindings file at C:\Users\zy147\AppData\Roaming\Code\User\keybindings.json (this is the real file VSCode reads �?the executor edits it FOR the user, no manual paste). REMOVE the two existing EIDE bindings (`{"key":"f7","command":"project.build"}` and `{"key":"ctrl+alt+d","command":"project.download"}`). ADD three bindings (NO `when` clause): `{"key":"alt+oem_comma","command":"workbench.action.tasks.runTask","args":"build"}`, `{"key":"alt+n","command":"workbench.action.tasks.runTask","args":"flash"}`, `{"key":"alt+m","command":"workbench.action.tasks.runTask","args":"build and flash"}`. Keep the file as a valid JSON array. Must NOT add a `when` clause. Must NOT touch any other user setting.
  Parallelization: Wave 2 | Blocked by: T8 (task names must match) | Blocks: none
  References: C:\Users\zy147\AppData\Roaming\Code\User\keybindings.json (current: line 4-6 F7→project.build, line 7-10 ctrl+alt+d→project.download �?both EIDE, to be removed). Task names from T8: "build", "flash", "build and flash". Keys confirmed by user: Alt+, build / Alt+N flash / Alt+M build+flash, global, no when. Alt+, �?VSCode key id `alt+oem_comma`.
  Acceptance criteria (agent-executable): keybindings.json is a valid JSON array containing exactly the 3 new bindings and NOT containing f7/project.build or ctrl+alt+d/project.download; no `when` keys present.
  QA scenarios: happy = JSON parses, 3 bindings present, EIDE ones gone; failure = JSON-lint and grep for "project.build"/"project.download" (must be absent). Evidence .omo/evidence/task-9-cmake-gcc-cpp-openocd.txt
  Commit: N (user-global file, outside repo �?do NOT commit) | n/a

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [x] F1. Plan compliance audit �?every todo done as specified; the 3 mixed files compile as C++ (compile_commands.json proof); all 8 sources in build.
- [x] F2. Build quality review �?clean `cmake --build --preset Debug` with zero errors; check `--print-memory-usage` output for FLASH/RAM within STM32F405RGT6 limits (1MB flash / 192KB RAM); Release preset also builds.
- [x] F3. Real manual QA �?build/Debug/ has Gimbal_Demo.elf + .hex + .bin; openocd.cfg parses; (with HW) OpenOCD flashes+verifies; the 3 VSCode tasks run; user keybindings.json valid, has the 3 new bindings, and the 2 EIDE bindings are gone.
- [x] F4. Scope fidelity �?serial printing (INFO/Mini_PC_INFO) byte-for-byte unchanged; no printf redirect added; no file renamed; no edits to main.c/stm32f4xx_it.c/communication.c content; cmake/stm32cubemx/CMakeLists.txt untouched; no `when` on keybindings.

## Commit strategy
- One commit per wave (Wave 1 build-system changes, Wave 2 tooling), or a single squashed commit �?worker's choice. Suggested messages are on each todo. Do NOT commit unless the user asks.
- Do NOT stage generated build artifacts (build/, *.hex, *.bin, *.map).

## Success criteria
- `cmake --preset Debug && cmake --build --preset Debug` succeeds with zero errors, producing Gimbal_Demo.elf, .hex, .bin.
- The C++ RM library and the 3 mixed .c files compile as C++ under arm-none-eabi-g++.
- OpenOCD with openocd.cfg flashes the elf over CMSIS-DAP (verified on HW at exec time).
- Alt+, builds, Alt+N flashes, Alt+M builds+flashes (once user copies keybindings into their user keybindings.json).
- Serial printing untouched; no printf redirection; no renamed/edited generated files; CubeMX regeneration still safe.
