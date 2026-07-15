# disable-eide-keep-cmake - Work Plan

## TL;DR (For humans)
<!-- Fill this LAST, after the detailed plan below is written, so it summarizes the REAL plan. -->
<!-- Plain English for a non-engineer: NO file paths, NO todo numbers, NO wave/agent/tool names. -->

**What you'll get:** 彻底停用 EIDE 编译系统,以后这个工程只能走 CMake+GCC(Alt+, / Alt+N / Alt+M),不会再像刚才那样误点 EIDE 把 C++ 当 C 编而报一屏错。IntelliSense(代码补全/跳转)改由 CMake 提供,照常能用。

**Why this approach:** 刚才那一屏报错的真正原因是——你点的是 EIDE 插件的编译按钮,它用 `arm-none-eabi-gcc -xc -std=c11` 把 main.c 当纯 C 编,所以所有 `class` 都报错。CMake 那条路我这边编是通过的。所以要把 EIDE 的所有入口都掐掉:两个 workspace 文件里的 EIDE 配置、两个 .eide 工程文件夹。全部用可逆方式(文件夹改名而非删除)。

**What it will NOT do:** 不删 .eide 文件夹(只改名,能还原);不卸载 VSCode 的 EIDE 插件(别的工程可能还用);不动任何源码、CMakeLists、串口打印。

**Effort:** Quick
**Risk:** Low - 都是配置改动 + 文件夹改名,完全可逆;唯一验证点是改完后 CMake 还能正常编(计划里已含验证)。
**Decisions to sanity-check:** 用"改名 .eide→.eide.disabled"而不是删除(可逆);IntelliSense 改指向 build/Debug 的 CMake 输出;不全局卸载 EIDE 插件。这几点你过一眼。

Your next move: 认可后运行 `/start-work` 执行(由执行环节改文件,我作为规划角色不直接改)。全部执行细节在下方。

---

> TL;DR (machine): Quick effort, Low risk. Deliverable: EIDE build path disabled reversibly in this project so only the CMake+GCC tasks build it; IntelliSense re-pointed at CMake output; source/CMake/printing untouched.

## Scope
### Must have
- Both .code-workspace files stop routing to EIDE: remove `"C_Cpp.default.configurationProvider": "cl.eide"` and remove `"cl.eide"` from extensions.recommendations.
- IntelliSense keeps working via CMake instead of EIDE: point clangd at `build/Debug` (CMake already exports compile_commands.json).
- Both `.eide` project folders (root + Gimbal_24hero/) renamed to `.eide.disabled` so EIDE no longer treats the folder as an EIDE project (removes the EIDE build button) — REVERSIBLE.
- After changes: the CMake build (`cmake --build --preset Debug`) still succeeds and produces elf/hex/bin.

### Must NOT have (guardrails, anti-slop, scope boundaries)
- Do NOT delete the `.eide` folders — rename only (reversible restore by renaming back).
- Do NOT uninstall the `cl.eide` extension from VSCode globally (other projects may use it).
- Do NOT modify source files, CMakeLists.txt, cmake/gcc-arm-none-eabi.cmake, openocd.cfg, or .vscode/tasks.json (already correct from prior plan).
- Do NOT touch serial printing (INFO / Mini_PC_INFO).
- Do NOT delete the .code-workspace files.

## Verification strategy
> Zero human intervention - all verification is agent-executed.
- Test decision: none (config change) + agent-executed build verification. The real test is: EIDE hooks gone AND CMake build still passes.
- Evidence: .omo/evidence/task-<N>-disable-eide-keep-cmake.txt

## Execution strategy
### Parallel execution waves
> Wave 1: T1, T2 (workspace file edits — two separate files, parallel). Wave 2: T3 (rename both .eide folders). Wave 3: T4 verification (CMake build still works + EIDE hooks gone). T3 depends on nothing but is grouped after to keep the config edits together; it can actually run parallel to T1/T2.

- Wave 1: T1, T2, T3 (independent: two workspace files + folder renames)
- Wave 2: T4 verification

### Dependency matrix
| Todo | Depends on | Blocks | Can parallelize with |
| --- | --- | --- | --- |
| T1 root .code-workspace | - | T4 | T2, T3 |
| T2 nested .code-workspace | - | T4 | T1, T3 |
| T3 rename both .eide folders | - | T4 | T1, T2 |
| T4 verify build + hooks gone | T1,T2,T3 | - | - |

## Todos
> Implementation + Test = ONE todo. Never separate.
<!-- APPEND TASK BATCHES BELOW THIS LINE WITH edit/apply_patch - never rewrite the headers above. -->
- [ ] 1. Root Gimbal_24hero.code-workspace: remove EIDE hooks, point clangd at CMake
  What to do / Must NOT do: Edit `D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111\Gimbal_24hero.code-workspace`:
    (a) DELETE the line `"C_Cpp.default.configurationProvider": "cl.eide",` (line 13).
    (b) In `clangd.arguments` (lines 9-11), ADD `"--compile-commands-dir=build/Debug"` after `"--header-insertion=never"` so IntelliSense uses CMake's compile_commands.json.
    (c) In `extensions.recommendations` (line 35), REMOVE the `"cl.eide",` entry.
  Keep everything else (folders, other settings, other recommendations) intact. Valid JSON.
  Parallelization: Wave 1 | Blocked by: none | Blocks: T4
  References: Gimbal_24hero.code-workspace:9-11 (clangd args), :13 (cl.eide provider), :35 (cl.eide recommendation). CMakeLists.txt:31 already sets CMAKE_EXPORT_COMPILE_COMMANDS TRUE → build/Debug/compile_commands.json exists.
  Acceptance criteria (agent-executable): file is valid JSON; grep for "cl.eide" returns 0 matches; "--compile-commands-dir=build/Debug" present.
  QA scenarios: happy = JSON parses + no cl.eide; failure = JSON-lint. Evidence .omo/evidence/task-1-disable-eide-keep-cmake.txt
  Commit: N | chore(vscode): remove EIDE provider from root workspace

- [ ] 2. Nested Gimbal_24hero/Gimbal_24hero.code-workspace: same EIDE removal
  What to do / Must NOT do: Apply the EXACT same 3 edits as T1 to `D:\RM_RMUC26_Guosai\Gimbal_24hero_CH010_20260120\Gimbal_24hero_CH010_20260111\Gimbal_24hero\Gimbal_24hero.code-workspace` (it is an identical duplicate). Valid JSON.
  Parallelization: Wave 1 | Blocked by: none | Blocks: T4
  References: Gimbal_24hero/Gimbal_24hero.code-workspace:9-11, :13, :35 (identical content to root workspace file).
  Acceptance criteria (agent-executable): file valid JSON; no "cl.eide"; "--compile-commands-dir=build/Debug" present.
  QA scenarios: happy = JSON parses + no cl.eide; failure = JSON-lint. Evidence .omo/evidence/task-2-disable-eide-keep-cmake.txt
  Commit: N | chore(vscode): remove EIDE provider from nested workspace

- [ ] 3. Rename both .eide project folders to .eide.disabled (reversible)
  What to do / Must NOT do: Rename (NOT delete) these two folders so EIDE stops recognizing the project (removes its build button):
    - `D:\...\Gimbal_24hero_CH010_20260111\.eide`  ->  `.eide.disabled`
    - `D:\...\Gimbal_24hero_CH010_20260111\Gimbal_24hero\.eide`  ->  `.eide.disabled`
  Use `Rename-Item` (PowerShell) or `git mv` if tracked. If a `.eide.disabled` already exists, append a timestamp. This is REVERSIBLE — renaming back restores EIDE. Must NOT delete the contents (eide.yml, files.options.yml).
  Parallelization: Wave 1 | Blocked by: none | Blocks: T4
  References: .eide/ (eide.yml + files.options.yml), Gimbal_24hero/.eide/ (same). EIDE detects a project by the presence of .eide/eide.yml.
  Acceptance criteria (agent-executable): both `.eide` folders no longer exist at their original paths; both `.eide.disabled` folders exist with eide.yml inside.
  QA scenarios: happy = Test-Path on old = False, on new = True, eide.yml present in renamed; failure = list dir. Evidence .omo/evidence/task-3-disable-eide-keep-cmake.txt
  Commit: N | chore(eide): disable EIDE projects by renaming (reversible)

- [ ] 4. Verify: CMake build still passes AND all EIDE hooks are gone
  What to do / Must NOT do: Read-only verification (no edits). Run the CMake build and confirm EIDE is fully unhooked:
    1. Add ninja to PATH for the session: `$env:PATH = "C:\Users\zy147\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin;" + $env:PATH`
    2. `cmake --build --preset Debug` → must succeed, produce build/Debug/Gimbal_Demo.elf + .hex + .bin.
    3. Confirm no "cl.eide" in either .code-workspace.
    4. Confirm both original .eide paths are gone and .eide.disabled exist.
    5. Confirm .vscode/tasks.json still has the 3 CMake tasks (no EIDE commands) and user keybindings.json still has Alt+, / Alt+N / Alt+M.
  Must NOT edit anything.
  Parallelization: Wave 2 | Blocked by: T1,T2,T3 | Blocks: none
  References: prior plan cmake-gcc-cpp-openocd confirmed clean build. ninja at C:\Users\zy147\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin.
  Acceptance criteria (agent-executable): build exits 0 with elf/hex/bin present; grep "cl.eide" across both workspaces = 0; Test-Path old .eide = False.
  QA scenarios: happy = build OK + hooks gone; failure = capture error lines. Evidence .omo/evidence/task-4-disable-eide-keep-cmake.txt
  Commit: N | n/a (verification only)

## Final verification wave
> Runs in parallel after ALL todos. ALL must APPROVE. Surface results and wait for the user's explicit okay before declaring complete.
- [ ] F1. Plan compliance audit — both workspaces cleaned, both .eide renamed, clangd re-pointed.
- [ ] F2. Build quality — `cmake --build --preset Debug` passes, elf/hex/bin present.
- [ ] F3. Real manual QA — grep confirms zero "cl.eide" in workspaces; old .eide paths gone, .eide.disabled present with eide.yml; tasks.json + keybindings still correct.
- [ ] F4. Scope fidelity — no source/CMakeLists/toolchain/openocd/printing changes; .eide renamed not deleted; cl.eide extension NOT globally uninstalled; .code-workspace files not deleted.

## Commit strategy
- Optional single commit `chore(build): disable EIDE, standardize on CMake`. Do NOT commit unless the user asks. Do NOT stage build artifacts.

## Success criteria
- Building via EIDE is no longer possible in this project (no .eide project, no cl.eide provider); the only build path is the CMake tasks (Alt+, / Alt+N / Alt+M).
- CMake build still succeeds and produces elf/hex/bin.
- Everything reversible: renaming .eide.disabled back and re-adding cl.eide restores EIDE.
- Serial printing and all source untouched.
