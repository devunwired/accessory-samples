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
#include "als.h"
#include "I2C.h"



char alsInit(void){  //1->success, 0->fail

    if(i2cScan(1, 0x44) == I2C_ERR_NAK) return 0; //not found

    i2cSingleWrite(1, 0x44, 0x00, 0x00);	// Probably unnecessary
    i2cSingleWrite(1, 0x44, 0x01, (0x4) << 4);	// Turn on all the features, but no interrupts
    i2cSingleWrite(1, 0x44, 0x02, 0x40);	// Turn on IR comp, max gain settings

    i2cSingleWrite(1, 0x44, 0x03, (0x0B) << 4);	// Set it to 110mA LED current.

    return 1;
}

void alsRead(uint16_t* prox, uint16_t* clear, uint16_t* R, uint16_t* G, uint16_t* B, uint16_t* IR, uint16_t* temp){

    if(prox) *prox = ((uint16_t)i2cSingleRead(1, 0x44, 0x10) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0x11);
    if(clear) *clear = ((uint16_t)i2cSingleRead(1, 0x44, 0x4) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0x5);
    if(R) *R = ((uint16_t)i2cSingleRead(1, 0x44, 0x6) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0x7);
    if(G) *G = ((uint16_t)i2cSingleRead(1, 0x44, 0x8) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0x9);
    if(B) *B = ((uint16_t)i2cSingleRead(1, 0x44, 0xA) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0xB);
    if(IR) *IR = ((uint16_t)i2cSingleRead(1, 0x44, 0xC) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0xD);
    if(temp) *temp = ((uint16_t)i2cSingleRead(1, 0x44, 0x12) << 8) | (uint16_t)i2cSingleRead(1, 0x44, 0x13);
}
