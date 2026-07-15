#ifndef RM2023_LIB_CRC_H
#define RM2023_LIB_CRC_H

/*
 * 统一 CRC 模块 (crc-gongban)
 *
 * 工程内共有 4 种互不兼容的 CRC 算法，此处各保留唯一一份实现。
 * 每种算法的表、初值、查表方向、收尾逻辑均与原实现逐字节一致，
 * 仅做"搬家"，不改变任何计算结果。
 *
 * 关键红线：算法 A 与算法 B 的 256 项查表数值相同，但 A 收尾取反(~fcs)、
 * B 不取反，因此各留一份常量表，绝不合并共用。
 */

#include <stdint.h>

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 算法 A: CRC-ITU16（图传整包）
 * 初值 0xffff，查表 crctab16，收尾 ~fcs 取反。
 * 对应原实现: RM_Lib.cpp GetCrc16 / IsCrc16Good。 */
uint16_t crc_itu16(const unsigned char *pData, uint16_t nLength);
/* nLength 为包含 CRC 的总长度；magic value 0xf0b8 命中即为校验通过。 */
bool crc_itu16_verify(const unsigned char *pData, uint16_t nLength);

/* 算法 B: DJI-CRC16（裁判系统 / SuperPower）
 * 初值 0xffff，查表 wCRC_Table，不取反直接返回。
 * 对应原实现: CP_System.c Get_CRC16_Check_Sum / communication.c get_crc16。 */
uint16_t crc_dji16(const uint8_t *pchMessage, uint32_t dwLength);

/* 算法 C: DJI-CRC8（DJI 帧头）
 * 初值 0xff，查表 CRC8_TAB。
 * 对应原实现: CP_System.c Get_CRC8_Check_Sum。 */
uint8_t crc_dji8(const uint8_t *pchMessage, uint32_t dwLength);

/* 算法 D: Mini_PC 旧 CRC8
 * 初值 0x00，查表 crc_table。
 * 对应原实现: communication.c cal_crc_table。 */
uint8_t crc_minipc8(const uint8_t *ptr, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* RM2023_LIB_CRC_H */
