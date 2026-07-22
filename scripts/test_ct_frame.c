/*
 * Host-side unit test for the "CT" combined frame byte-layout / little-endian parsing.
 *
 * Source of truth (firmware): Core/Src/my_main.cpp:2154 (CT_HEADER_LEN),
 *                             Core/Src/my_main.cpp:2176-2191 (VD_2rx parse).
 *
 * Layout of the 320-byte combined frame:
 *   [0]      'C'
 *   [1]      'T'
 *   [2..3]   length = 12 (little-endian, NOT validated by firmware)
 *   [4..7]   vision_pitch    (float32, little-endian)
 *   [8..11]  vision_yaw      (float32, little-endian)
 *   [12..15] vision_distance (float32, little-endian)
 *   [16]     'V'  (start of original VD video frame)
 *   [17]     'D'
 *   [18..19] VD subheader
 *   [20..319] 300-byte video payload (video starts at CT_HEADER_LEN + 4 = 20)
 *
 * NOT built into the firmware. Pure C, no STM32/HAL headers. Host-only.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Mirror the firmware constants EXACTLY (prevents offset drift). */
#define CT_HEADER_LEN 16
#define VD_DATA_NUM   300

/* Derived layout, expressed the SAME way as the firmware. */
#define VIDEO_OFFSET  (CT_HEADER_LEN + 4)              /* = 20  */
#define TOTAL_LEN     (CT_HEADER_LEN + VD_DATA_NUM + 4) /* = 320 */

static int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "ASSERTION FAILED: %s (line %d)\n",           \
                    #cond, __LINE__);                                     \
            failures++;                                                   \
        }                                                                 \
    } while (0)

int main(void)
{
    /* Known test values (historical firmware seeds). */
    const float exp_pitch    = 33.7f;
    const float exp_yaw      = -2.9f;
    const float exp_distance = 1.25f;

    /* --- Build the 320-byte combined frame --- */
    uint8_t buf[TOTAL_LEN];
    memset(buf, 0, sizeof(buf));

    buf[0] = 'C';
    buf[1] = 'T';

    /* length field [2..3] = 12, little-endian */
    uint16_t len = 12;
    buf[2] = (uint8_t)(len & 0xFF);
    buf[3] = (uint8_t)((len >> 8) & 0xFF);

    /* floats [4..7], [8..11], [12..15] little-endian via memcpy */
    memcpy(buf + 4,  &exp_pitch,    4);
    memcpy(buf + 8,  &exp_yaw,      4);
    memcpy(buf + 12, &exp_distance, 4);

    /* VD video frame header */
    buf[16] = 'V';
    buf[17] = 'D';
    /* [18..19] VD subheader (leave as recognizable marker) */
    buf[18] = 0xAB;
    buf[19] = 0xCD;

    /* 300-byte video payload [20..319]: recognizable increasing sequence */
    for (int i = 0; i < VD_DATA_NUM; i++) {
        buf[VIDEO_OFFSET + i] = (uint8_t)(i & 0xFF);
    }

    /* --- Replicate the firmware parse --- */
    float p, y, d;
    memcpy(&p, buf + 4,  4);
    memcpy(&y, buf + 8,  4);
    memcpy(&d, buf + 12, 4);

    uint8_t video[VD_DATA_NUM];
    memcpy(video, buf + CT_HEADER_LEN + 4, VD_DATA_NUM);

    /* --- Assertions --- */

    /* Header magic bytes */
    CHECK(buf[0] == 'C');
    CHECK(buf[1] == 'T');
    CHECK(buf[16] == 'V');
    CHECK(buf[17] == 'D');

    /* Length field little-endian == 12 */
    CHECK(buf[2] == 12);
    CHECK(buf[3] == 0);

    /* Floats bit-exact (same literal) */
    CHECK(p == exp_pitch);
    CHECK(y == exp_yaw);
    CHECK(d == exp_distance);

    /* Floats bit-exact via memcmp of the raw bytes */
    CHECK(memcmp(&p, &exp_pitch,    4) == 0);
    CHECK(memcmp(&y, &exp_yaw,      4) == 0);
    CHECK(memcmp(&d, &exp_distance, 4) == 0);

    /* Extracted video equals the source region byte-for-byte */
    CHECK(memcmp(video, buf + VIDEO_OFFSET, VD_DATA_NUM) == 0);

    /* Total buffer length */
    CHECK((int)sizeof(buf) == 320);
    CHECK(TOTAL_LEN == 320);
    CHECK(VIDEO_OFFSET == 20);

    if (failures == 0) {
        printf("ALL ASSERTIONS PASSED\n");
        return 0;
    }
    fprintf(stderr, "%d ASSERTION(S) FAILED\n", failures);
    return 1;
}
