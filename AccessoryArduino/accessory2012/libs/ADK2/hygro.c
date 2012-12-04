/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define ADK_INTERNAL
#include "fwk.h"
#include "I2C.h"
#include "hygro.h"
#include "dbg.h"

#define HYGRO_HOLD 1

#define HYGRO_I2C_ADDR       0x40
#define HYGRO_TRIG_TEMP_HOLD 0xE3
#define HYGRO_TRIG_HUM_HOLD  0xE5
#define HYGRO_TRIG_TEMP      0xF3
#define HYGRO_TRIG_HUM       0xF5
#define HYGRO_WRITE_USER     0xE6
#define HYGRO_READ_USER      0xE7
#define HYGRO_RESET          0xFE

#define TEMP_SLOPE           (175.72)
#define TEMP_OFFSET          (-46.85)
#define HUM_SLOPE            (125)
#define HUM_OFFSET           (-6)
char hygroInit( void )
{
    if (I2C_ALL_OK != i2cSingleWrite(1, HYGRO_I2C_ADDR, HYGRO_WRITE_USER, 0x02)) // Write defaults to user register
        return 0;
    return 1;
}

char hygroRead( int32_t *temp, int32_t *humidity )
{
    uint8_t cmd, vals[2], ret;
    uint16_t val;
    int32_t stat, oldstat, tmp;
#if HYGRO_HOLD
    cmd = HYGRO_TRIG_TEMP_HOLD;
    if (I2C_ALL_OK != i2cOp(1, HYGRO_I2C_ADDR, &cmd, 1, vals, 2))
        return 0;
#else
    cmd = HYGRO_TRIG_TEMP;
    if (I2C_ALL_OK != i2cOp(1, HYGRO_I2C_ADDR, &cmd, 1, NULL, 0))
        return 0;
    do {
        stat = i2cOp(1, HYGRO_I2C_ADDR, NULL, 0, vals, 2);
    } while (stat != I2C_ALL_OK);
#endif

    val = (vals[1] & 0xFC) | (((uint16_t)vals[0]) << 8);

    *temp = (int)(TEMP_OFFSET * 8) + ((int)(TEMP_SLOPE * 8) * (int)val / 65536); // 8 fixed point

#if HYGRO_HOLD
    cmd = HYGRO_TRIG_HUM_HOLD;
    if (I2C_ALL_OK != i2cOp(1, HYGRO_I2C_ADDR, &cmd, 1, vals, 2))
        return 0;
#else
    cmd = HYGRO_TRIG_HUM;
    if (I2C_ALL_OK != i2cOp(1, HYGRO_I2C_ADDR, &cmd, 1, NULL, 0))
        return 0;
    do {
        stat = i2cOp(1, HYGRO_I2C_ADDR, NULL, 0, vals, 2);
    } while (stat != I2C_ALL_OK);
#endif

    val = (vals[1] & 0xFC) | (((uint16_t)vals[0]) << 8);
    *humidity = (int32_t)(HUM_OFFSET * 8) + ((int32_t)(HUM_SLOPE * 8) * (int32_t)val / 65536);
    return 1;
}
