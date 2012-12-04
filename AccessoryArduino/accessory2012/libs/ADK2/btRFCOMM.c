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
#include "btRFCOMM.h"
#include "BT.h"
#include "dbg.h"
#include "btL2CAP.h"
#include "coop.h"

//NOTE: to be good citizens, we really should verify incoming CRC. We have better things to do...sorry...
//Plus, they do not provide safety for the data since data is not checksummed.
//We are, however, forced to produce good CRCs.

#define UGLY_SCARY_DEBUGGING_CODE	0

#define BT_RFCOMM_DLCI_MSK_EA	0x01
#define BT_RFCOMM_DLCI_MSK_CR	0x02
#define BT_RFCOMM_DLCI_MSK_D	0x04
#define BT_RFCOMM_DLCI_SHIFT	2

#define BT_SZ_MASK_OVER		0x01

#define BT_RFCOMM_CMD_DM	0x0F
#define BT_RFCOMM_CMD_SABM	0x3F
#define BT_RFCOMM_CMD_DISC	0x53
#define BT_RFCOMM_CMD_UA	0x73
#define BT_RFCOMM_CMD_UIH	0xEF
#define BT_RFCOMM_CMD_UIH_CRED	0xFF

//_C = command _R = resp
#define BT_RFCOMM_NSC_R		0x11
#define BT_RFCOMM_TEST_R	0x21
#define BT_RFCOMM_TEST_C	0x23
#define BT_RFCOMM_RLS_R		0x51
#define BT_RFCOMM_RLS_C		0x53
#define BT_RFCOMM_FCOFF_R	0x61
#define BT_RFCOMM_FCOFF_C	0x63
#define BT_RFCOMM_PN_R		0x81
#define BT_RFCOMM_PN_C		0x83
#define BT_RFCOMM_RPN_R		0x91
#define BT_RFCOMM_RPN_C		0x93
#define BT_RFCOMM_FCON_R	0xA1
#define BT_RFCOMM_FCON_C	0xA3
#define BT_RFCOMM_CLD_C		0xC3
#define BT_RFCOMM_MSC_R		0xE1
#define BT_RFCOMM_MSC_C		0xE3


#define BT_RFCOMM_REMOTE_CREDS	4
#define BT_RFCOMM_MAX_CRED_DEBT	(BT_RFCOMM_REMOTE_CREDS - 1)

typedef struct{

    uint8_t addr;
    uint8_t control;
    uint8_t length; //we do not support 2-byte length
    uint8_t data[]; //and one byte CRC (FCS)
}RfcommPacket;

typedef struct{

    BtRfcommPortOpenF oF;
    BtRfcommPortCloseF cF;
    BtRfcommPortRxF rF;

}RfcommPort;

static RfcommPort gPortHandlers[NUM_DLCIs] = {{0,}, };

static uint64_t reserved = 0xC000000000000003;	//these are always reserved

typedef struct{

    uint16_t aclConn;
    uint16_t remChan;
    struct{

        sg_buf* queued;
        uint16_t MTU;
        uint8_t credits;
        uint8_t creditsOwed;
        char on;

    }ports[NUM_DLCIs];

}RfcommInstanceState;

extern uint8_t btRfcommFcs(uint8_t *data, uint32_t len);

static void btRfcommSend(uint16_t conn, uint16_t remChan, const uint8_t* data, uint16_t sz){

    int i;

    #if UGLY_SCARY_DEBUGGING_CODE
        dbgPrintf("Sending RFCOMM packet:");
        for(i = 0; i < sz; i++) dbgPrintf(" %02X", data[i]);
        dbgPrintf("\n\n");
    #endif

    sg_buf* buf = sg_alloc();
    if(!buf) return;
    if(sg_add_front(buf, data, sz, SG_FLAG_MAKE_A_COPY)){

        l2capServiceTx(conn, remChan, buf);
        return;
    }
    sg_free(buf);
    free(buf);
}

static void btRfcommTxData(RfcommInstanceState* state, uint8_t dlci, sg_buf* buf){

    uint8_t data[5] = {(dlci << BT_RFCOMM_DLCI_SHIFT) | BT_RFCOMM_DLCI_MSK_EA, BT_RFCOMM_CMD_UIH};
    uint32_t dataLen = sg_length(buf);
    uint8_t hdrLen = 3;

    if(dataLen >= 128){ //need 2 length bytes

        data[2] = dataLen << 1;
        data[3] = dataLen >> 7;
        hdrLen++;
    } else data[2] = (dataLen << 1) | 1;

    if(state->ports[dlci].creditsOwed){ //use the oportunity to send the credits

        data[1] = BT_RFCOMM_CMD_UIH_CRED;
        data[hdrLen++] = state->ports[dlci].creditsOwed;
        state->ports[dlci].creditsOwed = 0;
    }

    if(sg_add_front(buf, data, hdrLen, SG_FLAG_MAKE_A_COPY)){

        data[0] = btRfcommFcs(data, 2);

        if(sg_add_back(buf, data, 1, SG_FLAG_MAKE_A_COPY)){

            l2capServiceTx(state->aclConn, state->remChan, buf);
            return;
        }
    }
    sg_free(buf);
    free(buf);
}

static void rfcommServiceDataRx(void* service, const uint8_t* req, uint16_t reqSz){

    RfcommInstanceState* state = service;
    const uint16_t conn = state->aclConn;
    const uint16_t remChan = state->remChan;
    const RfcommPacket* p = (RfcommPacket*)req; //note that we can do this only because packet has no non-8-bit-members
    const uint8_t* data = p->data;
    int i;
    uint8_t DLCI = p->addr >> BT_RFCOMM_DLCI_SHIFT;
    uint8_t ctrl = p->control;
    uint16_t len;
    uint8_t buf[32];


    if(p->length & 1){

        len = p->length >> 1;
    }
    else{

        len = *data++;
        len <<= 7;
        len |= p->length >> 1;
    }

    if(ctrl == BT_RFCOMM_CMD_SABM || ctrl == BT_RFCOMM_CMD_DISC){

        buf[ 0] = p->addr;
        buf[ 1] = BT_RFCOMM_CMD_UA;
        buf[ 2] = BT_SZ_MASK_OVER;
        buf[ 3] = btRfcommFcs(buf, 3);
        btRfcommSend(conn, remChan, buf, 4);

        if(state->ports[DLCI].queued){

            sg_free(state->ports[DLCI].queued);
            free(state->ports[DLCI].queued);
        }

        state->ports[DLCI].creditsOwed = 0;
        state->ports[DLCI].credits = 0;
        state->ports[DLCI].queued = NULL;

        if(ctrl == BT_RFCOMM_CMD_SABM){

            if(gPortHandlers[DLCI].oF) gPortHandlers[DLCI].oF(state, DLCI);
            else if(UGLY_SCARY_DEBUGGING_CODE) dbgPrintf("DLCI %2d opened\n", DLCI);
            state->ports[DLCI].on = 0;
        }
        else{

            if(gPortHandlers[DLCI].cF) gPortHandlers[DLCI].cF(state, DLCI);
            else if(UGLY_SCARY_DEBUGGING_CODE) dbgPrintf("DLCI %2d closed\n", DLCI);
            state->ports[DLCI].on = 1;
        }
        return;
    }

    if(ctrl == BT_RFCOMM_CMD_UIH && DLCI){ //we got a msg - send back credits if we already owe the limit, else buffer it

        if(++state->ports[DLCI].creditsOwed == BT_RFCOMM_MAX_CRED_DEBT){

            buf[ 0] = p->addr;
            buf[ 1] = BT_RFCOMM_CMD_UIH_CRED;
            buf[ 2] = BT_SZ_MASK_OVER;
            buf[ 3] = state->ports[DLCI].creditsOwed + 1;
            buf[ 4] = btRfcommFcs(buf, 2);
            btRfcommSend(conn, remChan, buf, 5);
            state->ports[DLCI].creditsOwed = 0;
        }
    }
    if(ctrl == BT_RFCOMM_CMD_UIH_CRED){

        state->ports[DLCI].credits += *data++;
        ctrl = BT_RFCOMM_CMD_UIH;

        if(state->ports[DLCI].queued && state->ports[DLCI].credits){

            state->ports[DLCI].credits--;
            btRfcommTxData(state, DLCI, state->ports[DLCI].queued);
            state->ports[DLCI].queued = NULL;
        }
    }

    if(DLCI == 0){ //control channel transaction
        if(ctrl == BT_RFCOMM_CMD_UIH){ //data recieved

            switch(data[0]){

                case BT_RFCOMM_PN_C:{		//negotiations are for wusses!

                    uint8_t targetDLCI = data[2] & 0x3F;

                    state->ports[targetDLCI].credits = data[9];
                    state->ports[targetDLCI].MTU = (((uint16_t)data[7]) << 8) | data[6];

                    buf[ 0] = BT_RFCOMM_DLCI_MSK_EA;
                    buf[ 1] = BT_RFCOMM_CMD_UIH;
                    buf[ 2] = BT_SZ_MASK_OVER + (10 << 1);
                    buf[ 3] = BT_RFCOMM_PN_R;
                    buf[ 4] = BT_SZ_MASK_OVER + (8 << 1);
                    buf[ 5] = targetDLCI;
                    buf[ 6] = (data[3] == 0xF0) ? 0xE0 : 0x00;	//as per spec
                    buf[ 7] = data[4];				//copy as per spec
                    buf[ 8] = 0;				//0 as per spec
                    buf[ 9] = RFCOMM_MTU & 0xFF;		//max_packet_size.lo
                    buf[ 10] = RFCOMM_MTU >> 8;			//max_packet_size.hi
                    buf[ 11] = 0;				//0 retransmissions
                    buf[ 12] = BT_RFCOMM_REMOTE_CREDS;		//initial credits
                    buf[ 13] = btRfcommFcs(buf, 2);
                    btRfcommSend(conn, remChan, buf, 14);
                    break;
                }

                case BT_RFCOMM_RPN_C:		//we need to echo back the negotiation
                case BT_RFCOMM_MSC_C:{		//we need to echo back the control signals

                    if(len > 127) dbgPrintf("Control packet way too big\n");
                    else{

                        buf[ 0] = BT_RFCOMM_DLCI_MSK_EA;
                        buf[ 1] = BT_RFCOMM_CMD_UIH;
                        buf[ 2] = p->length;
                        buf[ 3] = (data[0] == BT_RFCOMM_MSC_C) ? BT_RFCOMM_MSC_R : BT_RFCOMM_RPN_C;
                        buf[ 4] = data[1];
                        for(i = 0; i < data[1] >> 1; i++) buf[ 5 + i] = data[2 + i];
                        buf[ 5 + i] = btRfcommFcs(buf, 2);
                        btRfcommSend(conn, remChan, buf, 6 + i);

                        if(data[0] == BT_RFCOMM_MSC_C){
                            //now we send our own signals (coincidentally they are identical ;-)
                            buf[ 3] = BT_RFCOMM_MSC_C;
                            btRfcommSend(conn, remChan, buf, 6 + i);
                        }
                    }
                    break;
                }

                case BT_RFCOMM_RPN_R:
                case BT_RFCOMM_MSC_R:{

                    //nothing to do about this :)
                    break;
                }

                default:

                    dbgPrintf("Recieved unhandled RFCOMM packet:");
                    for(i = 0; i < reqSz; i++) dbgPrintf(" %02X", req[i]);
                    dbgPrintf("\n\n");
                    break;
            }
        }
    }
    else if(ctrl == BT_RFCOMM_CMD_UIH){

        if(len){

            if(gPortHandlers[DLCI].rF) gPortHandlers[DLCI].rF(state, DLCI, data, len);
            else if(UGLY_SCARY_DEBUGGING_CODE){

                dbgPrintf("RFCOMM DLCI %d data:", p->addr >> 2);
                for(i = 0; i < len; i++) dbgPrintf(" %02X", data[i]);
                dbgPrintf("\n\n\n");
            }
        }
    }
    else{

        dbgPrintf("Recieved unhandled RFCOMM cmd (dlci=%d):", DLCI);
        for(i = 0; i < reqSz; i++) dbgPrintf(" %02X", req[i]);
        dbgPrintf("\n\n");
    }
}

static void* rfcommServiceAlloc(uint16_t conn, uint16_t chan, uint16_t remChan){

    RfcommInstanceState* state = malloc(sizeof(RfcommInstanceState));
    int i;

    if(!state) return NULL;

    state->aclConn = conn;
    state->remChan = remChan;

    for(i = 0; i < NUM_DLCIs; i++){

        state->ports[i].creditsOwed = 0;
        state->ports[i].credits = 0;
        state->ports[i].on = 0;
        state->ports[i].queued = 0;
    }

    return state;
}

static void rfcommServiceFree(void* service){

    free(service);
}

void btRfcommRegisterL2capService(){

    const L2capService rfcom = {L2CAP_FLAG_SUPPORT_CONNECTIONS, rfcommServiceAlloc, rfcommServiceFree, rfcommServiceDataRx};
    if(!l2capServiceRegister(L2CAP_PSM_RFCOMM, &rfcom)) dbgPrintf("SDP L2CAP registration failed\n");
}

void btRfcommRegisterPort(uint8_t dlci, BtRfcommPortOpenF oF, BtRfcommPortCloseF cF, BtRfcommPortRxF rF){

    if(dlci >= NUM_DLCIs) return;	//no such DLCI;

    gPortHandlers[dlci].oF = oF;
    gPortHandlers[dlci].cF = cF;
    gPortHandlers[dlci].rF = rF;
}

void btRfcommPortTx(void* port, uint8_t dlci, const uint8_t* data, uint16_t size){

    RfcommInstanceState* state = port;

    if(dlci >= NUM_DLCIs || dlci == 0) return; //cannot send there, buddy...

    sg_buf* buf = sg_alloc();
    if(!buf) return;

    if(!sg_add_front(buf, data, size, SG_FLAG_MAKE_A_COPY)){

        sg_free(buf);
        free(buf);
    }
    else{

        if(state->ports[dlci].credits){	//send immediately

            state->ports[dlci].credits--;
            btRfcommTxData(state, dlci, buf);
        }
        else{				//enqueue it

            if(state->ports[dlci].queued){

                while(state->ports[dlci].queued && state->ports[dlci].on) coopYield();
                if(!state->ports[dlci].on) return; //port closed - packet dropped
            }
            state->ports[dlci].queued = buf;
        }
    }
}

#define RFCOMM_DLCI_PREFERENCE_NONE	0x80	//use this param to the below
#define RFCOMM_DLCI_NEED_EVEN		0x81
#define RFCOMM_DLCI_NEED_ODD		0x82

uint8_t btRfcommReserveDlci(uint8_t preference){

    uint8_t start = 0, end = 64, step = 2;

    if(preference == RFCOMM_DLCI_PREFERENCE_NONE) step = 1;
    else if(preference == RFCOMM_DLCI_NEED_EVEN);
    else if(preference == RFCOMM_DLCI_NEED_ODD) start++;
    else{

        start = preference;
        end = preference + 1;
        step = 1;
    }
    
    while(start < end && (reserved & (1ULL << ((uint64_t)start)))) start += step;

    if(start >= end) return 0; //we failed

    reserved |= (1ULL << ((uint64_t)start));

    return start;
}

void btRfcommReleaseDlci(uint8_t dlci){

    reserved &=~ (1ULL << ((uint64_t)dlci));
}

























static const uint8_t crctab[256] = {
	0x00,	0x91,	0xE3,	0x72,	0x07,	0x96,	0xE4,	0x75,
	0x0E,	0x9F,	0xED,	0x7C,	0x09,	0x98,	0xEA,	0x7B,
	0x1C,	0x8D,	0xFF,	0x6E,	0x1B,	0x8A,	0xF8,	0x69,
	0x12,	0x83,	0xF1,	0x60,	0x15,	0x84,	0xF6,	0x67,
	0x38,	0xA9,	0xDB,	0x4A,	0x3F,	0xAE,	0xDC,	0x4D,
	0x36,	0xA7,	0xD5,	0x44,	0x31,	0xA0,	0xD2,	0x43,
	0x24,	0xB5,	0xC7,	0x56,	0x23,	0xB2,	0xC0,	0x51,
	0x2A,	0xBB,	0xC9,	0x58,	0x2D,	0xBC,	0xCE,	0x5F,
	0x70,	0xE1,	0x93,	0x02,	0x77,	0xE6,	0x94,	0x05,
	0x7E,	0xEF,	0x9D,	0x0C,	0x79,	0xE8,	0x9A,	0x0B,
	0x6C,	0xFD,	0x8F,	0x1E,	0x6B,	0xFA,	0x88,	0x19,
	0x62,	0xF3,	0x81,	0x10,	0x65,	0xF4,	0x86,	0x17,
	0x48,	0xD9,	0xAB,	0x3A,	0x4F,	0xDE,	0xAC,	0x3D,
	0x46,	0xD7,	0xA5,	0x34,	0x41,	0xD0,	0xA2,	0x33,
	0x54,	0xC5,	0xB7,	0x26,	0x53,	0xC2,	0xB0,	0x21,
	0x5A,	0xCB,	0xB9,	0x28,	0x5D,	0xCC,	0xBE,	0x2F,
	0xE0,	0x71,	0x03,	0x92,	0xE7,	0x76,	0x04,	0x95,
	0xEE,	0x7F,	0x0D,	0x9C,	0xE9,	0x78,	0x0A,	0x9B,
	0xFC,	0x6D,	0x1F,	0x8E,	0xFB,	0x6A,	0x18,	0x89,
	0xF2,	0x63,	0x11,	0x80,	0xF5,	0x64,	0x16,	0x87,
	0xD8,	0x49,	0x3B,	0xAA,	0xDF,	0x4E,	0x3C,	0xAD,
	0xD6,	0x47,	0x35,	0xA4,	0xD1,	0x40,	0x32,	0xA3,
	0xC4,	0x55,	0x27,	0xB6,	0xC3,	0x52,	0x20,	0xB1,
	0xCA,	0x5B,	0x29,	0xB8,	0xCD,	0x5C,	0x2E,	0xBF,
	0x90,	0x01,	0x73,	0xE2,	0x97,	0x06,	0x74,	0xE5,
	0x9E,	0x0F,	0x7D,	0xEC,	0x99,	0x08,	0x7A,	0xEB,
	0x8C,	0x1D,	0x6F,	0xFE,	0x8B,	0x1A,	0x68,	0xF9,
	0x82,	0x13,	0x61,	0xF0,	0x85,	0x14,	0x66,	0xF7,
	0xA8,	0x39,	0x4B,	0xDA,	0xAF,	0x3E,	0x4C,	0xDD,
	0xA6,	0x37,	0x45,	0xD4,	0xA1,	0x30,	0x42,	0xD3,
	0xB4,	0x25,	0x57,	0xC6,	0xB3,	0x22,	0x50,	0xC1,
	0xBA,	0x2B,	0x59,	0xC8,	0xBD,	0x2C,	0x5E,	0xCF
};

uint8_t btRfcommFcs(uint8_t *data, uint32_t len){ //calculate fcs value for RFCOMM

    uint32_t i;
    uint8_t crcval =	0xFF;

    for (i = 0; i < len; i++) crcval = crctab[crcval ^ (*data++)];

    return 0xFF - crcval;
}
