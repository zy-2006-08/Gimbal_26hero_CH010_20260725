---
slug: disable-eide-keep-cmake
status: awaiting-approval
intent: clear
review_required: false
pending-action: write .omo/plans/disable-eide-keep-cmake.md
approach: Disable the EIDE build system in this STM32F405 project (reversibly, without deleting source or the CMake setup) so the user can only build via the CMake+GCC tasks (Alt+, / Alt+N / Alt+M). Root cause of the user's error was that they built via the EIDE plugin (arm-none-eabi-gcc -xc -std=c11) instead of the CMake tasks, so all C++ classes failed. Neutralize every EIDE hook: workspace configurationProvider, cl.eide extension recommendation, and the two .eide project folders (renamed to .eide.disabled, reversible). Do NOT touch source, CMakeLists, INFO printing.
---

# Draft: disable-eide-keep-cmake

## Components (topology ledger)
- C1 | Workspace files stop routing to EIDE (both .code-workspace) | active | Gimbal_24hero.code-workspace, Gimbal_24hero/Gimbal_24hero.code-workspace
- C2 | Two .eide project folders neutralized (renamed .eide -> .eide.disabled) | active | .eide/, Gimbal_24hero/.eide/
- C3 | clangd IntelliSense points at CMake compile_commands.json (replaces cl.eide provider) | active | .code-workspace settings

## Findings (cited - path:lines)
- Root cause (from user's build log): build command was `C:\Users\zy147\.eide\tools\gcc_arm\bin\arm-none-eabi-gcc.exe -c -xc -std=c11 ... ./Core/Src/main.c` → EIDE plugin compiled main.c as C (-xc -std=c11), so every `class` in RM_Lib.h/my_math.h and `constexpr` in communication.h errored. EIDE output ends `ERROR build failed !`.
- CMake path is proven working: full clean rebuild (40 targets) links Gimbal_Demo.elf + .hex + .bin, zero errors (verified in prior session).
- Gimbal_24hero.code-workspace:13 AND Gimbal_24hero/Gimbal_24hero.code-workspace:13 -> `"C_Cpp.default.configurationProvider": "cl.eide"` (routes IntelliSense to EIDE).
- Both .code-workspace:35 -> `"cl.eide"` in extensions.recommendations.
- Both .code-workspace:9-11 -> clangd.arguments has `--header-insertion=never` only (no compile-commands dir).
- .eide/ has eide.yml + files.options.yml (project root). Gimbal_24hero/.eide/ has the same (a nested duplicate EIDE project).
- .vscode/tasks.json -> ALREADY rewritten to CMake+OpenOCD tasks (done in prior plan). No EIDE commands remain.
- User keybindings.json -> F7/Ctrl+Alt+D EIDE bindings ALREADY removed (done in prior plan).
- .vscode/settings.json -> only C_Cpp.errorSquiggles + one files.association; no EIDE provider here.
- There are TWO workspace files (root + Gimbal_24hero/ subfolder) and TWO .eide folders — the nested Gimbal_24hero/ copy is a secondary EIDE project. Both must be handled or the user could reopen the wrong one.

## Decisions (with rationale)
- REVERSIBLE disable, not delete: rename `.eide` -> `.eide.disabled` (both locations) rather than deleting, so it can be restored. EIDE only recognizes a project when a `.eide/eide.yml` is present; renaming the folder makes EIDE stop treating the folder as an EIDE project (removes the build button) while preserving the config.
- Remove `cl.eide` from configurationProvider and recommendations in BOTH .code-workspace files; point clangd at build/Debug (CMake's CMAKE_EXPORT_COMPILE_COMMANDS output) so IntelliSense still works via CMake, not EIDE.
- Do NOT uninstall the cl.eide VSCode extension globally (that is a user-level tool that may be used by other projects) — only stop THIS project from using it.
- Keep everything else (source, CMakeLists, toolchain, openocd.cfg, INFO printing) untouched.

## Scope IN
- Both .code-workspace: remove `C_Cpp.default.configurationProvider: cl.eide`; remove `cl.eide` from recommendations; add clangd `--compile-commands-dir=build/Debug` (or set C_Cpp fallback) so IntelliSense uses CMake output.
- Rename `.eide` -> `.eide.disabled` at project root and in Gimbal_24hero/ (reversible).
- Verify EIDE no longer provides a build path and CMake build still works.

## Scope OUT (Must NOT have)
- Do NOT delete .eide folders (rename only, reversible).
- Do NOT uninstall the cl.eide extension from VSCode globally.
- Do NOT modify source files, CMakeLists.txt, cmake/gcc-arm-none-eabi.cmake, openocd.cfg, tasks.json (already correct).
- Do NOT touch serial printing (INFO / Mini_PC_INFO).
- Do NOT delete the .code-workspace files (the user may open them).

## Open questions
(none — user chose "disable EIDE, keep CMake only")

## Approval gate
status: awaiting-approval
pending-action: write .omo/plans/disable-eide-keep-cmake.md then user runs /start-work
