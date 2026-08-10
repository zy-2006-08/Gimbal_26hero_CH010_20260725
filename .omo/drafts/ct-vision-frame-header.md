---
slug: ct-vision-frame-header
status: awaiting-approval->approved
intent: clear
review_required: false
pending-action: write .omo/plans/ct-vision-frame-header.md (DONE)
approach: In VD_2rx() (my_main.cpp), replace the plain 'V','D' frame check with a combined 'C','T' frame check that (1) parses vision_pitch/yaw/distance from the 16-byte CT header (little-endian float32) and (2) forwards the unchanged 304-byte VD video sub-frame out USART6 as the 0x310 referee frame exactly as today.
---

# Draft: ct-vision-frame-header

## Components (topology ledger)
- ct-parse | USART3 RX parses combined CT frame: fills 3 vision floats + forwards 300B video | active | Core/Src/my_main.cpp:2158-2200

## Open assumptions (announced defaults) - USER CONFIRMED "对的开始"
- byte order | little-endian for len + 3 floats | STM32 is little-endian; matches existing response.pitch.f layout | reversible
- frame validation | gate on C,T @0,1 + V,D @16,17 + total bytes == CT_HEADER_LEN(16)+VD_DATA_NUM(300)+4 == 320 | offsets are fixed/known; length field parsed but NOT hard-validated | reversible
- length field [2..3] | = 12 (bytes of the 3 floats: pitch+yaw+distance) | user stated "统计的是 pitch yaw distance" | reversible
- video sub-frame unchanged | original VD frame = 'V','D' + 2 subheader bytes + 300 payload = 304 bytes, kept intact after the 16B CT header | user: "原来是怎么样的视频包现在还是怎么样" | reversible

## Findings (cited - path:lines)
- VD_2rx() receiver: Core/Src/my_main.cpp:2158-2200. Frame check at :2174 `buf[0]=='V' && buf[1]=='D' && VD_rx_byte==VD_DATA_NUM+4`. Payload copy at :2182 `memcpy(NSQD_De_video_buffer, buf+4, VD_DATA_NUM)`. Forward at :2183-2184 (Data_Concatenation 0x310 + HAL_UART_Transmit_IT huart6).
- Buffers: Core/Src/my_main.cpp:2150 `VD_2rx_buf[2][VD_RX_NUM]` (VD_RX_NUM=650, stm32f4xx_it.h:35) - 320 bytes fits. :2153 `#define VD_DATA_NUM 300`. :2154 NSQD_De_video_buffer[300]. :2155 TX_VD_buf.
- vision vars: Core/Src/my_main.cpp:265 `float vision_yaw=-2.9;`, :304 `float vision_pitch=33.7;`, :306 `float vision_distance=0;`. extern in Core/Inc/main.h:39-41. All float32, 4 bytes (map confirmed 0x20000044/48, 0x20003574).
- consumers: my_main.cpp:971 `Pitch_goal = -vision_pitch;` in DIAO_SHE_MODE lob-shot. CAN 0x014 broadcast at :721-724 (int16 = float*100).
- USART3 DMA arm: my_main.cpp:2035 `HAL_UART_Receive_DMA(&MINI_PC_USART_HANDLE, VD_2rx_buf[0], sizeof(...))`, MINI_PC_USART_HANDLE=huart3 (communication.h:27), 460800 baud (usart.c:138). IDLE-line + ping-pong VD_FIFO.
- USART3 IRQ calls VD_2rx: Core/Src/stm32f4xx_it.c:420. Old getReceiveData vision-parse path commented out at :391,403,407-411.

## Decisions (with rationale)
- Single edit site: VD_2rx() in my_main.cpp. No new files, no buffer resize (650 >= 320).
- Add `#define CT_HEADER_LEN 16` near VD_DATA_NUM.
- Parse floats via memcpy into vision_pitch/yaw/distance (avoids strict-aliasing UB vs pointer cast).
- Re-enables vision-driven lob-shot: vision_pitch no longer static 33.7; user confirmed this is intended.
- length field parsed but not hard-rejected (offsets fixed). Not adding volatile (matches prior commented pattern; same risk profile).

## Scope IN
- Modify VD_2rx() frame parse to CT combined frame; extract 3 floats; forward unchanged 300B video @0x310.
- Add CT_HEADER_LEN macro.
- Optional host-side unit test that feeds a crafted 320B CT buffer through the parse math and asserts extracted floats + video offset.

## Scope OUT (Must NOT have)
- Do NOT change USART3/USART6 baud, DMA setup, buffer sizes, or the 0x310 output frame format.
- Do NOT touch CAN 0x014 broadcast, PITCH_Logic, or lob-shot control logic.
- Do NOT re-enable/modify the old getReceiveData / _response path in stm32f4xx_it.c or communication.c.
- Do NOT add volatile / refactor unrelated code. Do NOT hard-reject on length mismatch.

## Open questions
- (resolved) direction = RX+parse; video unchanged; len counts 3 floats; verify by showing code + compile.

## Approval gate
status: approved (user: "对的开始" after reviewing the exact code diff)

## Metis review (folded in)
session: ses_076d3836affecjoCspg8ppWMs3
- Layout/endianness/buffer/memcpy/goal-guardrail: all VERIFIED CORRECT.
- G1 (sender-lockstep): folded → TL;DR 前提 + Must-NOT (no dual-format branch, documented lockstep).
- G2 (contiguous 320B, no mid-frame IDLE): folded → Decisions-to-sanity-check.
- G3 (ARM toolchain may be absent): folded → Verification (probe-first, no fake build) + Must-NOT.
- G4 (host test duplicates parse logic): folded → Verification limitation note.
- G5 (length field "parsed" vs ignored): folded → corrected to "不读取、不校验".
- G6 (non-volatile shared floats): acknowledged, no action (single-copy atomic on M4, matches prior style).
- G7 (grep spacing fragile): folded → positive grep authoritative, negative secondary.
