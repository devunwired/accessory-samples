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
#ifndef _BT_H_
#define _BT_H_

#include "sgBuf.h"

typedef struct{

    uint8_t mac[6];
    uint8_t PSRM, PSPM, PSM;
    uint16_t co;  //clock offset
    uint32_t dc;  //class

}BtDiscoveryResult;

#define SUPORT_SSP		1

#define BT_CONN_LINK_TYPE_SCO	0
#define BT_CONN_LINK_TYPE_ACL	1

#define BT_BCAST_NONE		0	//point to point
#define BT_BCAST_ACTIVE		1	//to all active slaves
#define BT_BCAST_PICONET	2	//to all slaves (even parked)

#define BT_RX_BUF_SZ		1024
#define BT_BAUDRATE		1750000


#define BT_LINK_KEY_SIZE	16
#define BT_MAC_SIZE		6


#define BT_SSP_DONE_VAL		0x0FF00000

typedef struct{

    void* userData;

    char (*BtDiscoveryF)(void* userData, BtDiscoveryResult* r);
    char (*BtConnReqF)(void* userData, const uint8_t* mac, uint32_t devClass, uint8_t linkType);	//return 1 to accept
    void (*BtConnStartF)(void* userData, uint16_t conn, const uint8_t* mac, uint8_t linkType, uint8_t encrMode);
    void (*BtConnEndF)(void* userData, uint16_t conn, uint8_t reason);
    uint8_t (*BtPinRequestF)(void* userData, const uint8_t* mac, uint8_t* buf);	//fill buff with PIN code, return num bytes used (16 max) return 0 to decline

    char (*BtLinkKeyRequestF)(void* userData, const uint8_t* mac, uint8_t* buf); //fill buff with the 16-byte link key if known. return 1 if yes, 0 if no
    void (*BtLinkKeyCreatedF)(void* userData, const uint8_t* mac, const uint8_t* key); //save the key, if you want to...

    void (*BtAclDataRxF)(void* userData, uint16_t conn, char first, uint8_t bcastType, const uint8_t* data, uint16_t sz);

    void (*BtSspShow)(void* userData, const uint8_t* mac, uint32_t sspVal);

}BtFuncs;

char btInit(const BtFuncs* btf);

char btLocalMac(uint8_t* buf);
char btSetLocalName(const char* name);
char btGetRemoteName(const uint8_t* mac, uint8_t PSRM, uint8_t PSM, uint16_t co, char* nameBuf);
void btScan(void);
char btDiscoverable(char on);
char btConnectable(char on);
char btSetDeviceClass(uint32_t cls);
void btDeinit(void);

void btAclDataTx(uint16_t conn, char first, uint8_t bcastType, sg_buf* buf);

#endif
#endif

