# Draft: mac-migration

- intent: clear
- review_required: false
- status: plan-written
- pending_action: worker writes Mac迁移配置指南.md verbatim from plan's 完整文稿 section (Todo 1)
- approach: Produce ONE standalone Chinese md guide (`Mac迁移配置指南.md`) that a Mac-side agent
  follows end-to-end to reproduce the Windows STM32 dev environment on a brand-new Apple Silicon
  MacBook Pro (M5), and migrate the VSCode setup.

## Locked decisions (from user Q&A)
1. Mac = Apple Silicon M5 → Homebrew prefix /opt/homebrew, native arm64 binaries.
2. Code transfer = GitHub. Mac clones. Repos:
   - Gimbal: https://github.com/zy-2006-08/Gimbal_26hero_CH010_20260725
   - Chassis: https://github.com/zy-2006-08/Chassisl_26_SHANGTAIJIE
   Guide must handle the git-ignored `launch.json` (recreate on Mac) — confirmed .gitignore ignores /.vscode/launch.json and /build.
   NOTE: gimbal-chassis.code-workspace is NOT in either repo (lives one level up on Windows), so clone
   won't bring it — guide ch.9 recreates it on Mac.
3. Scope = BOTH projects (Gimbal_26hero_CH010_20260725 + Chassisl_26_SHANGTAIJIE) + multi-root
   gimbal-chassis.code-workspace.
4. Flasher = auto-switch J-Link ↔ CMSIS-DAP, rewritten as macOS shell script flash.sh
   (mirrors Windows flash.ps1 behavior).
5. Keybindings = Cmd+, → build, Cmd+M → build and flash, Cmd+N → flash.
6. Ozone = needed (macOS Apple Silicon). Fix hardcoded old Windows paths in ozone/yuntai.jdebug.
7. VSCode migration = repo configs auto-travel + install clangd/cortex-debug + write Cmd keybindings
   + recreate launch.json.

## Verified facts (librarian, cited)
- Toolchain: `brew install --cask gcc-arm-embedded` (Arm GNU toolchain, native arm64),
  `brew install cmake ninja`. OpenOCD formula is `open-ocd` (alias `openocd`).
- IMPORTANT: cortex-debug warns Homebrew default OpenOCD is INCOMPATIBLE. Use xPack OpenOCD
  (github.com/xpack-dev-tools/openocd-xpack/releases) for the debug/flash path, OR `brew install open-ocd --HEAD`.
- J-Link: SEGGER J-Link Software Pack has native Apple Silicon installer. CLI binary is `JLinkExe`
  (NOT JLink.exe). Install path /Applications/SEGGER/JLink_Vxxx. Provides JLinkExe + JLinkGDBServer.
- J-Link USB detection on macOS: `system_profiler SPUSBDataType | grep -qi 0x1366` (VID 0x1366 = SEGGER).
  ioreg alt: idVendor decimal 4966.
- CMSIS-DAP works driver-free on macOS via HID (open-ocd depends on hidapi + libusb). No kext.
- Ozone: native Apple Silicon, download from segger.com/downloads/jlink/#Ozone.
- VSCode keybindings.json: ~/Library/Application Support/Code/User/keybindings.json.
  `code --install-extension` works; IDs llvm-vs-code-extensions.vscode-clangd, marus25.cortex-debug.

## Migration-specific gotchas the guide MUST cover
- macOS keybindings use "cmd" not "alt"; build task label must match exactly.
- flash task command in tasks.json is Windows powershell → must become `bash flash.sh` (or `${fileWorkspaceFolder}/scripts/flash.sh`). Both projects' tasks.json.
- launch.json (git-ignored) recreated per project, cortex-debug pointing at xPack openocd path or JLinkGDBServer.
- ozone/yuntai.jdebug hardcodes D:/Newcode/Gimbal_24hero_CH010_20260111 paths → rewrite to Mac clone path.
- .code-workspace folders[].path already correct (Gimbal_26hero_CH010_20260725 + Chassisl_26_SHANGTAIJIE) — verify against actual Mac clone dir names.
- build/ not cloned (git-ignored) so NO chapter-14 stale-cache issue on fresh clone — must build once first.
- flash.sh must be chmod +x on Mac.
- Chassis is a SEPARATE git repo; both must be cloned as siblings, and the guide file lives in Gimbal repo.
