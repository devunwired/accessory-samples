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
#ifndef _BT_L2CAP_H_
#define _BT_L2CAP_H_

#include <stdint.h>

#define L2CAP_FLAG_SUPPORT_CONNECTIONS		1
#define L2CAP_FLAG_SUPPORT_CONNECTIONLESS	2

#define L2CAP_CHAN_CONNECTIONLESS		2	//pass to capServiceTx as needed


#define L2CAP_PSM_SDP			0x0001
#define L2CAP_PSM_RFCOMM		0x0003
#define L2CAP_PSM_AVDTP			0x0019


#define L2CAP_MAX_PIECED_MESSAGES	4		//most broken messages we support *at once*

typedef struct{

    uint8_t flags;

    void* (*serviceInstanceAllocate)(uint16_t conn, uint16_t chan, uint16_t remChan);
    void (*serviceInstanceFree)(void* service);

    void (*serviceRx)(void* service, const uint8_t* data, uint16_t size);

}L2capService;

#ifdef ADK_INTERNAL

#include "sgBuf.h"

//API for services
void l2capServiceTx(uint16_t conn, uint16_t remChan, sg_buf* data);
void l2capServiceCloseConn(uint16_t conn, uint16_t chan);

//API for service management
char l2capServiceRegister(uint16_t PSM, const L2capService* svcData);
char l2capServiceUnregister(uint16_t PSM);


//API for ACL
void l2capAclLinkUp(uint16_t conn);
void l2capAclLinkDataRx(uint16_t conn, char first, const uint8_t* data, uint16_t size);
void l2capAclLinkDown(uint16_t conn);


#endif
#endif
