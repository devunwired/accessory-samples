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
#ifdef ADK_INTERNAL
#ifndef _I2C_H_
#define _I2C_H_


#define I2C_ALL_OK		0	//all OK
#define I2C_ERR_NAK		1	//got a NAK
#define I2C_ERR_INVAL		2	//invalid command given

#define I2C_NUM_INSTANCES	2

//basic interface
uint8_t i2cInit(uint8_t instance, uint32_t clock);
void i2cClock(uint8_t instance, uint32_t clock);
uint8_t i2cOp(uint8_t instance, uint8_t addr, const uint8_t* sendData, uint8_t sendSz, uint8_t* recvData, uint8_t recvSz);

//convenience
uint8_t i2cSingleWrite(uint8_t instance, uint8_t addr, uint8_t reg, uint8_t val);
uint8_t i2cSingleRead(uint8_t instance, uint8_t addr, uint8_t reg);
uint8_t i2cQuick(uint8_t instance, uint8_t addr, uint8_t byte);
uint8_t i2cScan(uint8_t instance, uint8_t addr);

#endif
#endif

