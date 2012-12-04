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
#include "mag.h"
#include "I2C.h"




char magInit(void){  //1->success, 0->fail

    if(i2cSingleRead(1, 0x1E, 0x0A) != 0x48) return 0; //not found
    if(i2cSingleRead(1, 0x1E, 0x0B) != 0x34) return 0; //not found
    if(i2cSingleRead(1, 0x1E, 0x0C) != 0x33) return 0; //not found

    i2cSingleWrite(1, 0x1E, 0x00, 0x14); // No temp sensor
    i2cSingleWrite(1, 0x1E, 0x01, 0x20); // Gain setting of 1.3 gauss
    i2cSingleWrite(1, 0x1E, 0x02, 0x00); // Continuous conversion mode 

    return 1;
}

void magRead(int16_t* x, int16_t* y, int16_t* z) {

    while(!(i2cSingleRead(1, 0x1E, 0x09) & 1)); //wait

    if(x) *x = (int16_t)(((uint16_t)i2cSingleRead(1, 0x1E, 0x03) << 8) | (uint16_t)i2cSingleRead(1, 0x1E, 0x04));
    if(y) *y = (int16_t)(((uint16_t)i2cSingleRead(1, 0x1E, 0x05) << 8) | (uint16_t)i2cSingleRead(1, 0x1E, 0x06));
    if(z) *z = (int16_t)(((uint16_t)i2cSingleRead(1, 0x1E, 0x07) << 8) | (uint16_t)i2cSingleRead(1, 0x1E, 0x08));
}
