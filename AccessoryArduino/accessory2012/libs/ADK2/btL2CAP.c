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
#include "sgBuf.h"
#include "btL2CAP.h"
#include "BT.h"
#include "dbg.h"
#include <string.h>

#define UGLY_SCARY_DEBUGGING_CODE	0

#define L2CAP_CMD_REJECT		0x01
#define L2CAP_CMD_CONN_REQ		0x02
#define L2CAP_CMD_CONN_RESP		0x03
#define L2CAP_CMD_CONFIG_REQ		0x04
#define L2CAP_CMD_CONFIG_RESP		0x05
#define L2CAP_CMD_DISC_REQ		0x06
#define L2CAP_CMD_DISC_RESP		0x07
#define L2CAP_CMD_ECHO_REQ		0x08
#define L2CAP_CMD_ECHO_RESP		0x09
#define L2CAP_CMD_INFO_REQ		0x0A
#define L2CAP_CMD_INFO_RESP		0x0B

#define L2CAP_CONN_SUCCESS		0x0000
#define L2CAP_CONN_FAIL_NO_SUCH_PSM	0x0002
#define L2CAP_CONN_FAIL_RESOURCES	0x0004

#define L2CAP_REJECT_REASON_WTF		0x0000	//command not understood
#define L2CAP_REJECT_REASON_INVALID_CID	0x0002

#define L2CAP_FIRST_USABLE_CHANNEL	0x0040

#define L2CAP_OPTION_MTU		1
#define L2CAP_DEFAULT_MTU		672	//as per spec

typedef struct Service{

    struct Service* next;
    uint16_t PSM;
    L2capService descr;

}Service;

typedef struct Connection{

    struct Connection* next;
    Service* service;
    void* serviceInstance;
    uint16_t conn, chan, remChan;

}Connection;

Service* services = NULL;
Connection* connections = NULL;

static uint8_t gL2capID = 1;

struct{

    uint16_t conn;
    uint16_t chan;
    uint8_t* buf;
    uint16_t lenGot;
    uint16_t lenNeed;
}gIncomingPieces[L2CAP_MAX_PIECED_MESSAGES] = {{0,},};



static inline uint16_t getLE16(const uint8_t* ptr){

    return (((uint16_t)ptr[1]) << 8) + ptr[0];
}

static inline void putLE16(uint8_t* ptr, uint16_t val){

    ptr[0] = val & 0xFF;
    ptr[1] = val >> 8;
}

void l2capServiceTx(uint16_t conn, uint16_t remChan, sg_buf* data){

    uint8_t hdr[4];
    uint16_t len = sg_length(data);

    putLE16(hdr + 0, len);
    putLE16(hdr + 2, remChan);

    if(sg_add_front(data, hdr, 4, SG_FLAG_MAKE_A_COPY)){

        #if UGLY_SCARY_DEBUGGING_CODE
            uint32_t i;
            uint8_t buf[256];
            sg_copyto(data, buf);
            dbgPrintf("L2CAP TX: ");
            for(i = 0; i < sg_length(data); i++) dbgPrintf(" %02X", buf[i]);
            dbgPrintf("\n");
        #endif

        btAclDataTx(conn, 1, BT_BCAST_NONE, data);
    }
    else{

        sg_free(data);
        free(data);
    }
}

static void l2capSendControlRawBuf(uint16_t conn, const uint8_t* data, uint16_t len){

    sg_buf* buf;

    buf = sg_alloc();
    if(buf){

        if(sg_add_back(buf, data, len, SG_FLAG_MAKE_A_COPY)){

            l2capServiceTx(conn, 1, buf);
        }
        else{

            sg_free(buf);
            free(buf);
        }
    }
}

static void l2capConnectionCloseEx(Connection* c, Connection* prev, char sendDiscPacket){

    uint8_t discCmd[6];

    if(c->service->descr.serviceInstanceFree) c->service->descr.serviceInstanceFree(c->serviceInstance);

    if(sendDiscPacket){

        discCmd[0] = L2CAP_CMD_DISC_REQ;
        discCmd[1] = gL2capID++;
        putLE16(discCmd + 2, c->remChan);
        putLE16(discCmd + 4, c->chan);

        l2capSendControlRawBuf(c->conn, discCmd, 6);
    }

    if(prev) prev->next = c->next;
    else connections = c->next;
    free(c);
}

static Connection* l2capFindConnection(uint16_t conn, uint16_t chan, Connection** prev){

    Connection *c = connections, *p = NULL;

    while(c && (c->conn != conn || c->chan != chan)){

        p = c;
        c = c->next;
    }

    if(prev) *prev = p;
    return c;
}

static uint16_t l2capFindFreeLocalChannel(uint16_t conn){

    Connection *c = connections;
    uint32_t chan = L2CAP_FIRST_USABLE_CHANNEL;

    //first try something fast
    while(c){
        if(c->chan >= chan && c->conn == conn) chan = c->chan + 1;
        c = c->next;
    }
    if(chan <= 0xFFFF) return chan;

    //else do something slower
    for(chan = L2CAP_FIRST_USABLE_CHANNEL; chan <= 0xFFFF; chan++){

        c = connections;
        while(c && (c->chan != chan || c->conn != conn)) c = c->next;
        if(!c) break;
    }

    if(chan <= 0xFFFF) return chan;

    //now we failed
    return 0;
}

void l2capServiceCloseConn(uint16_t conn, uint16_t chan){

    Connection *c, *p;

    if((c = l2capFindConnection(conn, chan, &p))) l2capConnectionCloseEx(c, p, 1);
}

static void l2capHandleControlChannel(uint16_t conn, const uint8_t* data, uint16_t size){

    char rejectCommand = 0;
    uint16_t rejectReason = L2CAP_REJECT_REASON_WTF;
    Service* s = services;
    Connection* c;
    uint8_t cmd, id;
    uint8_t buf[16];
    uint16_t chan, remChan, len;

    if(!gL2capID) gL2capID++;

    if(size < 2){

        rejectCommand = 1;
        dbgPrintf("L2CAP: control packet too small\n");
    }
    else{

        cmd = *data++;
        id = *data++;
        len = getLE16(data);
        data += 2;
        size -= 4;

        if(len != size){

            dbgPrintf("L2CAP (control packet internal sizes mismatch (%u != %u)\n", len, size);
            rejectCommand = 1;
        }
        else switch(cmd){

            case L2CAP_CMD_CONN_REQ:{

                uint16_t PSM;

                //get some request data
                if(size != 4){

                    dbgPrintf("L2CAP: ConnectionRequest packet size is wrong(%u)\n", size);
                    rejectCommand = 1;
                    break;
                }

                PSM = getLE16(data + 0);
                remChan = getLE16(data + 2);

                //init the reply
                buf[0] = L2CAP_CMD_CONN_RESP;
                buf[1] = id;
                putLE16(buf + 2, 8);		//length
                putLE16(buf + 4, 0);		//DCID
                putLE16(buf + 6, remChan);	//SCID
                putLE16(buf + 10, 0);		//no further information

                //find the service
                while(s && s->PSM != PSM) s = s->next;
                if(!s || !(s->descr.flags & L2CAP_FLAG_SUPPORT_CONNECTIONS)){

                    dbgPrintf("L2CAP: rejecting conection to unknown PSM 0x%04X\n", PSM);
                    putLE16(buf + 8, L2CAP_CONN_FAIL_NO_SUCH_PSM);
                }
                else{

                    void* instance = NULL;

                    chan = 0;
                    c = malloc(sizeof(Connection));
                    if(c) chan = l2capFindFreeLocalChannel(conn);
                    if(chan) instance = s->descr.serviceInstanceAllocate(conn, chan, remChan);

                    if(instance){

                        putLE16(buf + 4, chan);
                        putLE16(buf + 8, L2CAP_CONN_SUCCESS);

                        c->service = s;
                        c->serviceInstance = instance;
                        c->conn = conn;
                        c->chan = chan;
                        c->remChan = remChan;
                        c->next = connections;
                        connections = c;
                    }
                    else{

                        putLE16(buf + 8, L2CAP_CONN_FAIL_RESOURCES);

                        if(c) free(c);
                    }
                }
                size = 12;
                break;
            }

            case L2CAP_CMD_CONFIG_REQ:{

                uint16_t flags;

                //get some request data
                if(size < 4){
                    dbgPrintf("L2CAP: ConfigurationRequest packet size is wrong(%u)\n", size);
                    rejectCommand = 1;
                    break;
                }

                chan = getLE16(data + 0);
                flags = getLE16(data + 2);
                if(flags & 1){ //flags continue - we do not support that

                    size = 0;
                    break;
                }
                size -= 4;
                data += 4;

                //locate the connection at hand
                c = l2capFindConnection(conn, chan, NULL);
                if(!c){
                    dbgPrintf("L2CAP: ConfigurationRequest for an unknown channel %u.%u\n", conn, chan);
                    rejectCommand = 1;
                    break;
                }
                chan = c->remChan;

                //reply with just our rx-MTU
                buf[0] = L2CAP_CMD_CONFIG_RESP;	
                buf[1] = id;
                putLE16(buf + 2, 10);			//length
                putLE16(buf + 4, c->remChan);		//SCID
                putLE16(buf + 6, 0);			//flags
                putLE16(buf + 8, 0);			//success
                buf[10] = L2CAP_OPTION_MTU;		//mtu_property.type
                buf[11] = 2;				//mtu_property.len
                putLE16(buf + 12, BT_RX_BUF_SZ - 8);	//mtu value
                l2capSendControlRawBuf(conn, buf, 14);	//send it
                
                //we need to send such a packet there too
                buf[0] = L2CAP_CMD_CONFIG_REQ;		//configuration request
                buf[1] = gL2capID++;
                putLE16(buf + 2, 4);			//length
                putLE16(buf + 4, c->remChan);		//SCID
                putLE16(buf + 6, 0);			//flags
                size = 8;
                break;
            }

            case L2CAP_CMD_CONFIG_RESP:{

                //we do nothing here - perhaps we should?
                size = 0;
                break;
            }

            case L2CAP_CMD_DISC_REQ:{

                Connection* p;

                //get some request data
                if(size != 4){

                    dbgPrintf("L2CAP: DisconnectionRequest packet size is wrong(%u)\n", size);
                    rejectCommand = 1;
                    break;
                }
                chan = getLE16(data + 0);
                remChan = getLE16(data + 2);
                c = l2capFindConnection(conn, chan, &p);

                //handle some likely error cases
                if(!c){
                    dbgPrintf("L2CAP: DisconnectionRequest for an unknown channel %u.%u\n", conn, chan);
                    rejectReason = L2CAP_REJECT_REASON_INVALID_CID;
                    rejectCommand = 1;	//reject as per spec
                    break;
                }
                if(c->remChan != remChan){
                    dbgPrintf("L2CAP: DisconnectionRequest for an unmatched channel on %u.%u (%u != %u)\n", conn, chan, c->remChan, remChan);
                    size = 0;		//drop as per spec
                    break;
                }

                //perform needed cleanup
                l2capConnectionCloseEx(c, p, 0);

                //send the reply
                buf[0] = L2CAP_CMD_DISC_RESP;	//disconnection response
                buf[1] = id;			//copied as required
                putLE16(buf + 2, 4);		//length
                putLE16(buf + 4, chan);		//DCID
                putLE16(buf + 6, remChan);	//SCID
                size = 8;
                break;
            }

            case L2CAP_CMD_DISC_RESP:{

                //nothing to do - we did cleanup when we requested the connection closure...
                size = 0;
                break;
            }

            case L2CAP_CMD_ECHO_REQ:{

                buf[0] = L2CAP_CMD_ECHO_RESP;	//ping response
                buf[1] = id;			//copied as required
                putLE16(buf + 2, 0);		//0-length replies are ok
                size = 4;
                break;
            }

            case L2CAP_CMD_INFO_REQ:{

                uint16_t info;

                //get some request data
                if(size != 2){

                    dbgPrintf("L2CAP: InformationRequest packet size is wrong(%u)\n", size);
                    rejectCommand = 1;
                    break;
                }
                info = getLE16(data + 0);

                buf[0] = L2CAP_CMD_INFO_RESP;	//information response
                buf[1] = id;			//copied as required
                putLE16(buf + 4, info);		//info type
                if(info == 1){	//connectionless mtu

                    putLE16(buf + 6, 0);	//success
                    putLE16(buf + 8, BT_RX_BUF_SZ - 8);
                    putLE16(buf + 2, 6);	//length
                    size = 10;
                }
                else if(info == 2){ //extended features

                    putLE16(buf + 6, 0);	//success
                    putLE16(buf + 8, 0); 
                    putLE16(buf + 10, 0); 
                    putLE16(buf + 2, 8);	//length
                    size = 12;
                }
                else{		//whatever this request is, we do not support it

                    putLE16(buf + 6, 1);	//info type not supported
                    putLE16(buf + 2, 4);	//length
                    size = 8;
                }
                break;
            }

            default:{

                dbgPrintf("L2CAP: unexpected command 0x%02X recieved\n", cmd);
                size = 0;
            }
        }
    }

    if(rejectCommand){

        buf[0] = L2CAP_CMD_REJECT;	//disconnection response
        buf[1] = id;			//copied as required
        putLE16(buf + 2, 4);		//length
        putLE16(buf + 4, rejectReason);	//rejection reason
        size = 6;
    }

    if(size) l2capSendControlRawBuf(conn, buf, size);
}


char l2capServiceRegister(uint16_t PSM, const L2capService* svcData){

    Service* s;

    //first, see if this PSM is taken
    s = services;
    while(s){

        if(s->PSM == PSM) return 0;
        s = s->next;
    }

    //next, try to allocate the memory
    s = malloc(sizeof(Service));
    if(!s) return 0;

    //then, init it and link it in
    s->PSM = PSM;
    s->descr = *svcData;
    s->next = services;
    services = s;

    return 1;
}

char l2capServiceUnregister(uint16_t PSM){

    Service *s = services, *ps = NULL;
    Connection *c, *pc = NULL;

    //first, find it in the list
    while(s && s->PSM != PSM){

        ps = s;
        s = s->next;
    }
    if(!s) return 0;

    //then, see if any connections are using it, and if so, kill them
    do{
        c = connections;
        while(c){

            if(c->service == s){

                l2capConnectionCloseEx(c, pc, 1);
                break;
            }

            pc = c;
            c = c->next;
        }
    }while(c);

    //now delete the service record
    if(ps) ps->next = s->next;
    else services = s->next;
    free(s);
    return 1;
}

void l2capAclLinkDataRx(uint16_t conn, char first, const uint8_t* data, uint16_t size){

    uint16_t chan, len;
    unsigned i;
    char freeIt = 0;

    #if UGLY_SCARY_DEBUGGING_CODE
        dbgPrintf("L2CAP data:");
        for(chan = 0; chan < size; chan++) dbgPrintf(" %02X", data[chan]);
        dbgPrintf("\n\n");
    #endif

    if(first){

        len = getLE16(data + 0);
        chan = getLE16(data + 2);
        data += 4;
        size -= 4;

        if(size >= len){
            if(size > len) dbgPrintf("L2CAP: ACL provided likely invalid L2CAP packet (ACL.len=%u L2CAP.len=%u)\n", size, len);
        }
        else{
            for(i = 0; i < L2CAP_MAX_PIECED_MESSAGES; i++){

                if(gIncomingPieces[i].conn == conn){

                    dbgPrintf("L2CAP: conn %d: 'first' frame while another incomplete already buffered. Dropping the old one.\n", conn);
                    free(gIncomingPieces[i].buf);
                    gIncomingPieces[i].conn = 0;
                    break;
                }
            }
            if(i == L2CAP_MAX_PIECED_MESSAGES) for(i = 0; i < L2CAP_MAX_PIECED_MESSAGES && gIncomingPieces[i].conn; i++);
            if(i == L2CAP_MAX_PIECED_MESSAGES){

                dbgPrintf("L2CAP: not enough buffer slots to buffer incomplete frame. Dropping\n");
            }
            else{

                uint8_t* ptr = malloc(size);
                if(!ptr){

                    dbgPrintf("L2CAP: cannot allocate partial frame buffer. Dropping\n");
                    return;
                }
                memcpy(ptr, data, size);
                gIncomingPieces[i].buf = ptr;
                gIncomingPieces[i].lenGot = size;
                gIncomingPieces[i].lenNeed = len - size;
                gIncomingPieces[i].chan = chan;
                gIncomingPieces[i].conn = conn;
            }
            return;
        }
    }
    else{

        uint8_t* ptr;

        for(i = 0; i < L2CAP_MAX_PIECED_MESSAGES && gIncomingPieces[i].conn != conn; i++);
        if(i == L2CAP_MAX_PIECED_MESSAGES){
            dbgPrintf("L2CAP: unexpected 'non-first' frame for conn %u. Dropping.\n", conn);
            return;
        }

        if(size > gIncomingPieces[i].lenNeed){

            dbgPrintf("L2CAP: 'non-first' frame too large. Need %u bytes, got %u. Dropping.\n", gIncomingPieces[i].lenNeed, size);
            return;
        }

        ptr = realloc(gIncomingPieces[i].buf, gIncomingPieces[i].lenGot + size);
        if(!ptr){

            dbgPrintf("L2CAP: failed to resize buffer for partial frame receive. Droping\n");
            free(gIncomingPieces[i].buf);
            gIncomingPieces[i].conn = 0;
            return;
        }

        memcpy(ptr + gIncomingPieces[i].lenGot, data, size);
        gIncomingPieces[i].buf = ptr;
        gIncomingPieces[i].lenGot += size;
        gIncomingPieces[i].lenNeed -= size;

        if(gIncomingPieces[i].lenNeed) return; //data still not complete

        gIncomingPieces[i].conn = 0;
        chan = gIncomingPieces[i].chan;
        len = gIncomingPieces[i].lenGot;
        data = ptr;
        freeIt = 1;
    }

    if(chan == 0) dbgPrintf("L2CAP: data on connection %u.0\n", conn);
    else if(chan == 1) l2capHandleControlChannel(conn, data, len);
    else if(chan == 2){	//connectionless

        uint16_t PSM = getLE16(data + 0);
        data += 2;
        len -= 2;

        Service* s = services;

        while(s && s->PSM != PSM) s = s->next;
        if(!s || !(s->descr.flags & L2CAP_FLAG_SUPPORT_CONNECTIONLESS)) dbgPrintf("L2CAP: connectionless data on %u.2 for unknown PSM 0x%04X\n", conn, PSM);
        else s->descr.serviceRx(NULL, data, len);
    }
    else{			//conection-oriented

        Connection* c = l2capFindConnection(conn, chan, NULL);
        if(!c) dbgPrintf("L2CAP: data for nonexistent connection %u.%u\n", conn, chan);
        else c->service->descr.serviceRx(c->serviceInstance, data, len);
    }

    if(freeIt) free(data);
}

void l2capAclLinkUp(uint16_t conn){

    //nothing here [yet?]
}

void l2capAclLinkDown(uint16_t conn){

    Connection *p, *c;
    unsigned i;

    do{

        p = NULL;
        c = connections;

        while(c && c->conn != conn){

            p = c;
            c = c->next;
        }

        if(c) l2capConnectionCloseEx(c, p, 0);

    }while(c);

    for(i = 0; i < L2CAP_MAX_PIECED_MESSAGES; i++) if(gIncomingPieces[i].conn == conn){

        free(gIncomingPieces[i].buf);
        gIncomingPieces[i].conn = 0;
    }
}

