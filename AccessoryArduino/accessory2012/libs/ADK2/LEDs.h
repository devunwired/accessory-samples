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
#ifndef _LEDS_H_
#define _LEDS_H_

#include <stdint.h>




#define NUM_LEDS        64

#define NUM_DIGITS	6
#define NUM_ICONS	8

extern const uint8_t seg_table[]; 		//7-segment difits: 0-9a-z_-.()
extern const uint8_t digit_table[NUM_DIGITS];	//"offset" values to each of the 6 digits
extern const uint8_t icon_table[NUM_ICONS];	//"offset" values to each of the 8 icons

void ledsInit(void);

//front LED display
	
    //draw to the backbuffer
    void ledWrite(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b);
    void ledDrawIcon(uint8_t icon, uint8_t r, uint8_t g, uint8_t b);
    void ledDrawLetter(char letter, uint8_t val, uint8_t r, uint8_t g, uint8_t b);

    //flush the backbuffer to the display
    void ledUpdate(void);

//onboard debug LED

    void ledDbgState(char on);

#endif
#endif

