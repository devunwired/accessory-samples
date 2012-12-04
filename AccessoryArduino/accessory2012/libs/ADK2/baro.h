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
#ifndef _BARO_H_
#define _BARO_H_


//oversample coefficients (higher for more accuracy, lower for more speed)
#define BARO_OSS_1X       0x00
#define BARO_OSS_2X       0x01
#define BARO_OSS_4X       0x02
#define BARO_OSS_8X       0x03

char baroInit(void);	//0 on failure
void baroRead(uint8_t oss, long* kPa, long* decicelsius);


#endif
#endif

