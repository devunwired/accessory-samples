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
#ifndef _BT_SDP_H_
#define _BT_SDP_H_

#include <stdint.h>

//advised reading: https://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm

#define SDP_TYPE_NIL			0
#define SDP_TYPE_UINT			1
#define SDP_TYPE_SINT			2
#define SDP_TYPE_UUID			3
#define SDP_TYPE_TEXT			4
#define SDP_TYPE_BOOL			5
#define SDP_TYPE_ARRAY			6	//"data element sequence"
#define SDP_TYPE_OR_LIST		7	//"data element alternative" - pick one of these
#define SDP_TYPE_URL			8

#define SDP_SZ_NIL			0
#define SDP_SZ_1			0
#define SDP_SZ_2			1
#define SDP_SZ_4			2
#define SDP_SZ_8			3
#define SDP_SZ_16			4
#define SDP_SZ_u8			5
#define SDP_SZ_u16			6
#define SDP_SZ_u32			7

#define SDP_ITEM_DESC(type, sz)		(((type << 3) & 0xF8) | (sz & 7))


#define SDP_ATTR_HANDLE				0x0000
#define SDP_ATTR_SVC_CLS_ID_LIST		0x0001
#define SDP_ATTR_SVC_ID				0x0003
#define SDP_ATTR_PROTOCOL_DESCR_LIST		0x0004
#define SDP_ATTR_BROWSE_GRP_LIST		0x0005

#define SDP_FIRST_USER_HANDLE			0x00010000


#ifdef ADK_INTERNAL

#include "sgBuf.h"

void btSdpServiceDescriptorAdd(const uint8_t* descriptor, uint16_t descrLen); //a copy will NOT be made
void btSdpServiceDescriptorDel(const uint8_t* descriptor);


void btSdpRegisterL2capService(void);


#endif
#endif


