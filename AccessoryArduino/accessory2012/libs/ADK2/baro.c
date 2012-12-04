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
#include "baro.h"
#include "I2C.h"

static struct {
    int16_t AC1;
    int16_t AC2;
    int16_t AC3;
    uint16_t AC4;
    uint16_t AC5;
    uint16_t AC6;
    int16_t B1;
    int16_t B2;
    int16_t MB;
    int16_t MC;
    int16_t MD;
} baro;



#define BARO_REG_CTRL     0xF4
#define BARO_REG_OUT_MSB  0xF6
#define BARO_REG_OUT_LSB  0xF7
#define BARO_REG_OUT_XLSB 0xF8
#define BARO_SCO          0x20



char baroInit(void){	//0 on failure

    if(i2cSingleRead(1, 0x77, 0xD0) != 0x55) return 0; //not found

    baro.AC1 = (i2cSingleRead(1, 0x77, 0xAA) << 8) | i2cSingleRead(1, 0x77, 0xAB);
    baro.AC2 = (i2cSingleRead(1, 0x77, 0xAC) << 8) | i2cSingleRead(1, 0x77, 0xAD);
    baro.AC3 = (i2cSingleRead(1, 0x77, 0xAE) << 8) | i2cSingleRead(1, 0x77, 0xAF);
    baro.AC4 = (i2cSingleRead(1, 0x77, 0xB0) << 8) | i2cSingleRead(1, 0x77, 0xA1);
    baro.AC5 = (i2cSingleRead(1, 0x77, 0xB2) << 8) | i2cSingleRead(1, 0x77, 0xA3);
    baro.AC6 = (i2cSingleRead(1, 0x77, 0xB4) << 8) | i2cSingleRead(1, 0x77, 0xA5);
    baro.B1 = (i2cSingleRead(1, 0x77, 0xB6) << 8) | i2cSingleRead(1, 0x77, 0xA7);
    baro.B2 = (i2cSingleRead(1, 0x77, 0xB8) << 8) | i2cSingleRead(1, 0x77, 0xA9);
    baro.MB = (i2cSingleRead(1, 0x77, 0xBA) << 8) | i2cSingleRead(1, 0x77, 0xBB);
    baro.MC = (i2cSingleRead(1, 0x77, 0xBC) << 8) | i2cSingleRead(1, 0x77, 0xBD);
    baro.MD = (i2cSingleRead(1, 0x77, 0xBE) << 8) | i2cSingleRead(1, 0x77, 0xBF);

    return 1;
}

void baroRead(uint8_t oss, long* kPa, long* decicelsius){

    long ut, up, x1, x2, x3, b3, b4, b5, b6, p, t;
    unsigned long b7;

    // Read uncompensated temp
    oss = oss & 0x03; // Mask off to protect bits
    i2cSingleWrite(1, 0x77, BARO_REG_CTRL, 0x2E);
    while(i2cSingleRead(1, 0x77, BARO_REG_CTRL) & BARO_SCO) {}
    ut = (i2cSingleRead(1, 0x77, BARO_REG_OUT_MSB) << 8) | i2cSingleRead(1, 0x77, BARO_REG_OUT_LSB);

    // Read uncompensated pressure
    i2cSingleWrite(1, 0x77, BARO_REG_CTRL, 0x34 | (oss << 6));
    while(i2cSingleRead(1, 0x77, BARO_REG_CTRL) & BARO_SCO) {}
    up = ((i2cSingleRead(1, 0x77, BARO_REG_OUT_MSB) << 16) | (i2cSingleRead(1, 0x77, BARO_REG_OUT_LSB) << 8)
         | i2cSingleRead(1, 0x77, BARO_REG_OUT_XLSB)) >> (8 - oss);

    // Calculate true temperature (in 0.1 degrees C)
    x1 = ((ut - baro.AC6) * baro.AC5) >> 15;
    x2 = (baro.MC << 11) / (x1 + baro.MD);
    b5 = x1 + x2;
    t = (b5 + 8) >> 4;
    if(decicelsius) *decicelsius = t;

    // Calculate true pressure
    b6 = b5 - 4000;
    x1 = (baro.B2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (baro.AC2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((baro.AC1 * 4 + x3) << oss) + 2) / 4;


    x1 = (baro.AC3 * b6) >> 13;
    x2 = (baro.B1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (baro.AC4 * (unsigned long)(x3 + 32768)) >> 15;
    b7 = ((unsigned long)up - b3) * (50000 >> oss);
    if (b7 < 0x80000000) {
        p = (b7 * 2) / b4;
    } else {
        p = (b7 / b4) * 2;
    }
    x1 = ((p >> 3) * (p >> 3)) >> 10; // Split because they seem to be using floating point in the datasheet example
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * p) >> 16;
    p = p + ((x1 + x2 + 3791) >> 4);

    if(kPa) *kPa = p;
}

