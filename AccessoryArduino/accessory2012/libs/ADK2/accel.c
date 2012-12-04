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
#include "accel.h"
#include "I2C.h"



char accelInit(void){	//0 on failure

    if(i2cScan(1, 0x19) == I2C_ERR_NAK) return 0; //not found

    i2cSingleWrite(1, 0x19, 0x24, 0x80);
    i2cSingleWrite(1, 0x19, 0x24, 0x00);

    i2cSingleWrite(1, 0x19, 0x20, 0x97); // 1.344khz sample rate, no low power, all axes enabled
    i2cSingleWrite(1, 0x19, 0x23, 0x80);
    i2cSingleWrite(1, 0x19, 0x24, 0x00);

    return 1;
}


void accelRead(int16_t* x, int16_t* y, int16_t* z){

    while(!(i2cSingleRead(1, 0x19, 0x27) & 8));

    if(x) *x = (int16_t)((uint16_t)i2cSingleRead(1, 0x19, 0x28) | ((uint16_t)i2cSingleRead(1, 0x19, 0x29) << 8));
    if(y) *y = (int16_t)((uint16_t)i2cSingleRead(1, 0x19, 0x2A) | ((uint16_t)i2cSingleRead(1, 0x19, 0x2B) << 8));
    if(z) *z = (int16_t)((uint16_t)i2cSingleRead(1, 0x19, 0x2C) | ((uint16_t)i2cSingleRead(1, 0x19, 0x2D) << 8));
}
