# ct-vision-frame-header - Learnings

## [Baseline] Toolchain & build environment (VERIFIED by orchestrator)
- ARM firmware toolchain PRESENT: arm-none-eabi-gcc (D:\GCC\arm-gnu-toolchain-15.2.rel1), cmake 4.4, ninja 1.13.
- Host compiler for unit test: MSYS2 mingw64 gcc/g++ at D:\SOFTWARE\MSYS2\mingw64\bin\gcc.exe (NOT on PATH; call by full path).
- Firmware build: `cmake --preset Debug` then `cmake --build --preset Debug`. binaryDir=build/Debug. Output Gimbal_Demo.elf.
- BASELINE BUILD PASSES: exit 0. Pre-existing warnings ONLY (CP_System.h:363-366 unused static CRC decls; RWX LOAD segment; CMakeLists.txt:34 author-warning). NOT introduced by our change - do not attribute to the edit.
- Frame layout: [0]C [1]T [2-3]len [4-7]pitch [8-11]yaw [12-15]distance [16]V [17]D [18-19]VD-subheader [20-319]300B video. Total 320. Little-endian float32.
- VD_2rx() is ISR-context; memcpy already used at line 2182 -> string.h available. Consumers already read the vision globals; goal met by ISR write alone.

## [Task 1 DONE] CT combined-frame parse in VD_2rx() (my_main.cpp ONLY)
- Added `#define CT_HEADER_LEN 16` right after `#define VD_DATA_NUM 300` (my_main.cpp:2154).
- Replaced VD_2rx() frame-check block (was 2174-2197, now 2175-2199): guard now checks [0]=='C'&&[1]=='T'&&[16]=='V'&&[17]=='D' && VD_rx_byte==CT_HEADER_LEN+VD_DATA_NUM+4 (==320).
- Parses 3 float32 (LE) via memcpy from offsets 4/8/12 -> vision_pitch/yaw/distance (all `float` at my_main.cpp:265/304/306).
- Video payload memcpy source moved to `+ CT_HEADER_LEN + 4` (offset 20); Data_Concatenation + HAL_UART_Transmit_IT(huart6, 0x310) UNCHANGED.
- Dead commented video_send_ready block dropped (inside replaced region). tx_drop_cnt logic preserved.
- BUILD: cmake --preset Debug + cmake --build --preset Debug => EXIT 0. Only pre-existing warnings (RM_Lib.h reorder/field-init, my_main.cpp:2161 temp unused [pre-existing], CP_System.h CRC decls, RWX LOAD segment). No NEW warnings from the change.
- Evidence: .omo/evidence/task-1-ct-vision-frame-header-build.txt
- NOTE for Task 2: consumer offsets confirmed for host unit test -> C,T at 0/1; len2 at 2 (NOT validated); pitch@4 yaw@8 distance@12 (float32 LE); V,D at 16/17; VD subheader 18-19; 300B video at 20..319; total 320.

## Task 2: Host unit test for CT frame layout (2026-07-22)
- Created scripts/test_ct_frame.c (pure C, no HAL, NOT in firmware build) validating the 320B CT combined frame layout against Core/Src/my_main.cpp:2154,2176-2191.
- GOTCHA: MSYS2 gcc (D:\SOFTWARE\MSYS2\mingw64\bin\gcc.exe) fails SILENTLY (exit 1, empty stderr) when its bin dir is NOT on PATH - it can't find cc1/DLLs. FIX: prepend 'D:\SOFTWARE\MSYS2\mingw64\bin' to \C:\Tools\cmake-4.4.0-rc3-windows-x86_64\cmake-4.4.0-rc3-windows-x86_64\bin;D:\Git\mingw64\bin;D:\Git\usr\bin;C:\Users\zy147\bin;C:\MentorGraphics\9.5PADS\SDD_HOME\common\win32\bin;C:\MentorGraphics\9.5PADS\SDD_HOME\common\win32\lib;C:\MentorGraphics\9.5PADS\MGC_HOME.ixn\bin;C:\MentorGraphics\9.5PADS\MGC_HOME.ixn\lib;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0;C:\WINDOWS\System32\OpenSSH;D:\Bandizip;D:\Clionchain\mingw64\bin;D:\Clionchain\gcc-arm-none-eabi-10.3-2021.10\bin;D:\Clionchain\OpenOCD-20231002-0.12.0\bin;C:\Program Files\dotnet;C:\Program Files\Bandizip;C:\Program Files\nodejs;D:\Git\cmd;D:\SOFTWARE\MSYS2\usr\bin;C:\Users\zy147\AppData\Local\Microsoft\WindowsApps;D:\mingw64\bin;D:\Clionchain\CLion 2023.2.2\bin;C:\Users\zy147\.dotnet\tools;D:\SOFTWARE\9.5PADS\SDD_HOME\common\win32\lib;C:\MentorGraphics\9.5PADS\SDD_HOME\common\win32\lib;D:\RM\serialplot\bin;D:\Microsoft VS Code\bin;C:\Users\zy147\AppData\Roaming\npm;C:\Tools\ninja-win;C:\Tools\xpack-openocd-0.12.0-7-win32-x64\xpack-openocd-0.12.0-7\bin;C:\Tools\cmake-4.4.0-rc3-windows-x86_64\cmake-4.4.0-rc3-windows-x86_64\bin;D:\GCC\arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi\bin;C:\msys64\mingw64\bin;C:\Users\zy147\AppData\Local\stm32cube\bundles\ninja\1.13.1+st.1\bin before compiling. This wasted several runs; document for future host-compile tasks.
- Verified real: happy run exit 0 (ALL ASSERTIONS PASSED); negative control (pitch assertion -> 99.9f) exit 1; post-revert exit 0. Evidence: .omo/evidence/task-2-ct-vision-frame-header-test.txt
- Confirms firmware layout: floats LE at offsets 4/8/12, video at CT_HEADER_LEN+4=20, total CT_HEADER_LEN+VD_DATA_NUM+4=320. Both host x86 and Cortex-M4 little-endian => layout validated.

## Task 3: ct_data_len readout (2026-07-22)
- Added global olatile uint16_t ct_data_len = 0; at my_main.cpp:2159 (next to DMATX).
- Inside VD_2rx CT parse block, after vision_distance memcpy, added little-endian length read: ct_data_len = (uint16_t)(VD_2rx_buf[!VD_FIFO][2] | (VD_2rx_buf[!VD_FIFO][3] << 8)); Debug/telemetry only.
- if guard (lines 2176-2178) UNCHANGED: length field does NOT participate in frame rejection.
- Build: cmake --preset Debug + cmake --build --preset Debug => EXIT 0. No NEW warnings (only pre-existing RM_Lib/CP_System/temp/RWX/author warnings).
- Evidence: .omo/evidence/task-3-ct-len-readout-build.txt
