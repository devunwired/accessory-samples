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
#ifndef _BT_RFCOMM_H_
#define _BT_RFCOMM_H_

#include <stdint.h>

#define NUM_DLCIs		62
#define RFCOMM_MTU		511
#define RFCOMM_DLCI_PREFERENCE_NONE	0x80	//use this param to the below
#define RFCOMM_DLCI_NEED_EVEN		0x81
#define RFCOMM_DLCI_NEED_ODD		0x82

typedef void (*BtRfcommPortOpenF)(void* port, uint8_t dlci);
typedef void (*BtRfcommPortCloseF)(void* port, uint8_t dlci);
typedef void (*BtRfcommPortRxF)(void* port, uint8_t dlci, const uint8_t* buf, uint16_t sz);


#ifdef ADK_INTERNAL

#include "BT.h"


void btRfcommRegisterPort(uint8_t dlci, BtRfcommPortOpenF oF, BtRfcommPortCloseF cF, BtRfcommPortRxF rF);
void btRfcommPortTx(void* port, uint8_t dlci, const uint8_t* data, uint16_t size); //makes a copy of your buffer


uint8_t btRfcommReserveDlci(uint8_t preference);	//return dlci if success, zero if fail
void btRfcommReleaseDlci(uint8_t dlci);


void btRfcommRegisterL2capService(void);


#endif
#endif

