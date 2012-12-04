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
#include "btSDP.h"
#include <string.h>
#include "sgBuf.h"
#include "btL2CAP.h"


#define SDP_PDU_Error_Response				1
#define SDP_PDU_Service_Search_Request			2
#define SDP_PDU_Service_Search_Response			3
#define SDP_PDU_Service_Attribute_Request		4
#define SDP_PDU_Service_Attribute_Response		5
#define SDP_PDU_Service_Search_Attribute_Request	6
#define SDP_PDU_Service_Search_Attribute_Response	7

#define SDP_ERR_Invalid_SDP_Version			0x0001
#define SDP_ERR_Invalid_Service_Record_Handle		0x0002
#define SDP_ERR_Invalid_Request_Syntax			0x0003
#define SDP_ERR_Invalid_PDU_Size			0x0004
#define SDP_ERR_Invalid_Continuation_State		0x0005
#define SDP_ERR_Insufficient_Resources			0x0006


typedef struct{

    uint64_t hi, lo;

}uuid;

#define MAX_UUIDS_IN_SEARCH		12	//as per spec
#define MAX_ATTRS_IN_SEARCH_STRING	8	//as per my opinion
#define MAX_SEARCH_RESULTS		16	//no more than this will ever be returned

typedef struct SdpService{

    struct SdpService* next;

    uint32_t handle;
    const uint8_t* descriptor;
    uint16_t descrLen;

}SdpService;

typedef struct{

    uint16_t aclConn;
    uint16_t remChan;

    uint32_t contDescr; //which continuation descriptor we expect
    uint8_t* result;
    uint32_t resultSz;
    uint16_t numMatches; //for servicesearch

}SdpInstance;


static SdpService* knownServices = NULL;
static uint32_t sdpContVal = 0x12345678;
static uint32_t sdpNextHandle = 0;

static const uint8_t sdpDescrSdp[] =
{
    //define the SDP service itself
        //service class ID list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x01, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 3,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x10, 0x00, // ServiceDiscoveryServerServiceClassID
        //ProtocolDescriptorList
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x04, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 8,
            SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 6,
                SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x01, 0x00, // L2CAP
                SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), L2CAP_PSM_SDP >> 8, L2CAP_PSM_SDP & 0xFF, // L2CAP PSM

        //browse group list
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x05, SDP_ITEM_DESC(SDP_TYPE_ARRAY, SDP_SZ_u8), 3,
            SDP_ITEM_DESC(SDP_TYPE_UUID, SDP_SZ_2), 0x10, 0x02, // Public Browse Group

        //magic data #1
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0xDD, 0xDD, SDP_ITEM_DESC(SDP_TYPE_TEXT, SDP_SZ_u8), 19, 0x53, 0x57, 0x3A, 0x20, 0x44, 0x6D, 0x69, 0x74, 0x72, 0x79, 0x20, 0x47, 0x72, 0x69, 0x6e, 0x62, 0x65, 0x72, 0x67,
        //magic data #2
        SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0xDD, 0xDE, SDP_ITEM_DESC(SDP_TYPE_TEXT, SDP_SZ_u8), 19, 0x48, 0x57, 0x3A, 0x20, 0x45, 0x72, 0x69, 0x63, 0x20, 0x53, 0x63, 0x68, 0x6c, 0x61, 0x65, 0x70, 0x66, 0x65, 0x72
};

static const uuid bt_base_uuid = {0x0000000000001000ULL, 0x800000805F9B34FBULL};

static void sdpIntToUUID(uuid* dst, uint32_t val){

    *dst = bt_base_uuid;
    dst->hi += ((uint64_t)val) << 32;
}

static char sdpUuidEqual(const uuid* a, const uuid* b){

    return a->lo == b->lo && a->hi == b->hi;
}

static uint32_t btSdpGetElemSz(const uint8_t** descr){

    const uint8_t* ptr = *descr;
    uint8_t item = *ptr++;
    uint32_t sz = 0;

    if((item >> 3) != SDP_TYPE_NIL){

        switch(item & 7){

            case SDP_SZ_1:
            case SDP_SZ_2:
            case SDP_SZ_4:
            case SDP_SZ_8:
            case SDP_SZ_16:

                sz = 1 << (item & 7);
                break;

            case SDP_SZ_u8:

                sz = *ptr++;
                break;

            case SDP_SZ_u16:

                sz = ptr[0];
                sz = (sz << 8) | ptr[1];
                ptr += 2;
                break;

            case SDP_SZ_u32:

                sz = ptr[0];
                sz = (sz << 8) | ptr[1];
                sz = (sz << 8) | ptr[2];
                sz = (sz << 8) | ptr[3];
                ptr += 4;
                break;
        }
    }
    *descr = ptr;
    return sz;
}

static uint8_t btSdpGetUUID(const uint8_t** descr, uuid* dst){ //return num bytes consumed

    uint32_t sz, i;
    const uint8_t* orig = *descr;

    if(((**descr) >> 3) != SDP_TYPE_UUID) return 0; //not valid UUID type
    sz = btSdpGetElemSz(descr);

    switch(sz){

        case 2:

            sdpIntToUUID(dst, (((uint32_t)((*descr)[0])) << 8) | ((*descr)[1]));
            break;

        case 4:

            sdpIntToUUID(dst, (((uint32_t)((*descr)[0])) << 24) | (((uint32_t)((*descr)[1])) << 16) | (((uint32_t)((*descr)[2])) << 8) | ((*descr)[3]));
            break;

        case 16:

            dst->lo = 0;
            dst->hi = 0;

            for(i = 0; i < 8; i++){

                dst->lo = (dst->lo << 8) | (*descr)[i];
                dst->hi = (dst->lo << 8) | (*descr)[i + 8];
            }
            break;

        default:

            return 0;
    }
    *descr += sz;

    return (*descr) - orig;
}

static void btStdRecursiveWalk(void* itemList, uint8_t* listSzP, sg_buf** walkResultP, const uint8_t** ptr, uint32_t len){ 

    uint32_t sz;
    uint8_t typ, numWantedIDs;
    const uint8_t* end = (*ptr) + len;
    uuid id;
    sg_buf* result = NULL;
    char isID = 1, skipNext = 0;
    uuid* wantedIDs;
    uint8_t* numWantedIDsP;
    uint32_t* wantedRanges;
    uint8_t wantedRangesListSz;


    if(walkResultP){	//copy-traversal

        result = sg_alloc();
        if(!result) return;

        wantedIDs = NULL;
        numWantedIDsP = NULL;
        wantedRanges = itemList;
        wantedRangesListSz = *listSzP;
    }
    else{		//search for UUIDs

        wantedIDs = itemList;
        numWantedIDsP = listSzP;
        numWantedIDs = *numWantedIDsP;
        wantedRanges = NULL;
        wantedRangesListSz = 0;
    }

    while((*ptr) < end){

        typ = (**ptr) >> 3;

        if(wantedIDs && typ == SDP_TYPE_UUID){

            sz = btSdpGetUUID(ptr, &id);
            if(end < (*ptr)){

                dbgPrintf("SDP: UUID size > allowed size (%d, %d)\n", sz, end - (*ptr));
                goto out;
            }

            for(sz = 0; sz < numWantedIDs; sz++){

                if(sdpUuidEqual(wantedIDs + sz, &id)){

                    wantedIDs[sz] = wantedIDs[numWantedIDs - 1];
                    numWantedIDs--;
                    sz--;
                }
            }
        }
        else{

            const uint8_t* itemStart = *ptr;

            sz = btSdpGetElemSz(ptr);

            if(sz > (unsigned)(end - (*ptr))){

                dbgPrintf("SDP: element size > allowed size (%d, %d)\n", sz, end - (*ptr));
                goto out;
            }

            if(typ == SDP_TYPE_ARRAY || typ == SDP_TYPE_OR_LIST){

                btStdRecursiveWalk(wantedIDs, &numWantedIDs, NULL, ptr, sz);
            }
            else{

                (*ptr) += sz;
            }
            if(walkResultP){

                if(isID){

                    uint16_t attrID;
                    uint8_t i;

                    if(sz != 2) dbgPrintf("SDP: attrib ID not 16 bits!\n");

                    attrID = (*ptr)[-2];
                    attrID = (attrID << 8) | (*ptr)[-1];

                    skipNext = 2;
                    for(i = 0; i < wantedRangesListSz && skipNext; i++){

                        if(attrID >= (wantedRanges[i] >> 16) && attrID <= (wantedRanges[i] & 0xFFFF)) skipNext = 0; //in range
                    }
                }
                isID ^= 1;

                if(skipNext){

                    skipNext--;
                }
                else{

                    if(!sg_add_back(result, itemStart, (*ptr) - itemStart, SG_FLAG_MAKE_A_COPY)){

                        sg_free(result);
                        return;
                    }
                }
            }
        }
    }

out:
    if(walkResultP) *walkResultP = result;
    if(numWantedIDsP) *numWantedIDsP = numWantedIDs;
}

static char btSdpPutIntoGroup(sg_buf* buf){

    uint8_t i, sizeFieldSz, sizeFieldName;
    uint32_t sz = sg_length(buf);
    uint8_t res[5];

    //figure out needed header size field
    if(sz < 0x100){

        sizeFieldSz = 1;
        sizeFieldName = SDP_SZ_u8;
        sz <<= 24;
    }
    else if(sz < 0x10000){

        sizeFieldSz = 2;
        sizeFieldName = SDP_SZ_u16;
        sz <<= 16;
    }
    else{

        sizeFieldSz = 4;
        sizeFieldName = SDP_SZ_u32;
    }

    //add the header
    res[0] = SDP_ITEM_DESC(SDP_TYPE_ARRAY, sizeFieldName);
    for(i = 0; i < sizeFieldSz; i++, sz <<= 8) res[1 + i] = sz >> 24;
    return sg_add_front(buf, res, 1 + sizeFieldSz, SG_FLAG_MAKE_A_COPY);
}

static sg_buf* btSdpError(const uint8_t* trans, uint16_t errNum){

    sg_buf* buf;
    uint8_t data[] = {SDP_PDU_Error_Response, trans[0], trans[1], errNum >> 8, errNum, 0, 0};

    buf = sg_alloc();
    if(buf){

        if(!sg_add_front(buf, data, sizeof(data), SG_FLAG_MAKE_A_COPY)){

            sg_free(buf);
            buf = NULL;
        }
    }
    return NULL;
}

static sg_buf* btSdpProcessRequest(SdpInstance* inst, const uint8_t* req, uint16_t reqSz){
    uint8_t trans[2] ,cmd, contStateSz, numIDs = 0, numAttrs = 0;
    uint32_t maxReplSz = 0, wantedHandle = 0, sz;
    uint32_t attrs[MAX_ATTRS_IN_SEARCH_STRING];
    SdpService* results[MAX_SEARCH_RESULTS];
    uuid ids[MAX_UUIDS_IN_SEARCH];
    const uint8_t* end;
    sg_buf* result;
    unsigned i, j;

    cmd = *req++;
    trans[0] = *req++;
    trans[1] = *req++;

    reqSz -= 5;
    if(reqSz != (((uint16_t)req[0]) << 8) + req[1]) return btSdpError(trans, SDP_ERR_Invalid_PDU_Size);
    req += 2;

    //dbgPrintf("SDP request cmd %d (session %02X%02X) with %d bytes of data\n", cmd, trans[0], trans[1], reqSz);

    if(cmd == SDP_PDU_Service_Search_Request || cmd == SDP_PDU_Service_Search_Attribute_Request){

        if((*req) >> 3 != SDP_TYPE_ARRAY) return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);
        sz = btSdpGetElemSz(&req);
        end = req + sz;

        while(req < end){

            if(numIDs == MAX_UUIDS_IN_SEARCH) return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);	//too many requests
            if(!btSdpGetUUID(&req, &ids[numIDs++])) return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);	//malformed UUID
        }
    }
    else if(cmd == SDP_PDU_Service_Attribute_Request){

        for(i = 0; i < 4; i++) wantedHandle = (wantedHandle << 8) | *req++;
    }
    else{

        dbgPrintf("SDP: invalid request: %d\n", cmd);
        return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);
    }

    for(i = 0; i < 2; i++) maxReplSz = (maxReplSz << 8) | *req++;

    if(cmd == SDP_PDU_Service_Attribute_Request || cmd == SDP_PDU_Service_Search_Attribute_Request){

        if((*req) >> 3 != SDP_TYPE_ARRAY) return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);
        sz = btSdpGetElemSz(&req);
        end = req + sz;

        while(req < end){

            if(numAttrs == MAX_UUIDS_IN_SEARCH) return btSdpError(trans, SDP_ERR_Insufficient_Resources);	//too many -> unsupported request -> fail
            sz = btSdpGetElemSz(&req);
            if(sz == 2){

                sz = 0;
                for(i =0; i < 2; i++) sz = (sz << 8) | *req++;
                sz |= sz << 16;
            }
            else if(sz == 4){

                sz = 0;
                for(i =0; i < 4; i++) sz = (sz << 8) | *req++;
            }
            else return btSdpError(trans, SDP_ERR_Invalid_Request_Syntax);	//fail -> invalid number format
            attrs[numAttrs++] = sz;
        }
    }

    contStateSz = *req++;

    if(contStateSz){		// verify continuation is valid or fail
        uint32_t contState = 0;

        if(contStateSz != sizeof(uint32_t)) return btSdpError(trans, SDP_ERR_Invalid_Continuation_State);
        for(i = 0; i < 4; i++) contState = (contState << 8) | *req++;

        if(contState != inst->contDescr || !inst->result){

            dbgPrintf("SDP: invalid continuation state. Wanted %08X, got %08X\n", inst->contDescr, contState);
            if(inst->result){

                free(inst->result);
                inst->result = NULL;
            }
            return btSdpError(trans, SDP_ERR_Invalid_Continuation_State);
        }
    }
    else{			//perform the actual search

        SdpService* curSvc = knownServices;
        uint8_t numFound = 0;

        //cleanup first
        if(inst->result){

            free(inst->result);
            inst->result = NULL;
        }

        //perform the search
        if(cmd == SDP_PDU_Service_Search_Request || cmd == SDP_PDU_Service_Search_Attribute_Request){

            for(curSvc = knownServices; curSvc && numFound < MAX_SEARCH_RESULTS; curSvc = curSvc->next){

                const uint8_t* ptr = curSvc->descriptor;
                uuid uuids_copy[MAX_UUIDS_IN_SEARCH];
                uint8_t num;

                for(num = 0; num < numIDs; num++) uuids_copy[num] = ids[num];

                btStdRecursiveWalk(uuids_copy, &num, NULL, &ptr, curSvc->descrLen);
                if(!num) results[numFound++] = curSvc;
            }
        }
        else if(cmd == SDP_PDU_Service_Attribute_Request){

            for(curSvc = knownServices; curSvc && !numFound; curSvc = curSvc->next){

                if(curSvc->handle == wantedHandle) results[numFound++] = curSvc;
            }
            if(!numFound) return btSdpError(trans, SDP_ERR_Invalid_Service_Record_Handle);
        }

        //gather & prepare results
        if(cmd == SDP_PDU_Service_Attribute_Request || cmd == SDP_PDU_Service_Search_Attribute_Request){

            //we'll assemble the whole result in this buffer
            sg_buf* resultSoFar = sg_alloc();
            if(!resultSoFar) return btSdpError(trans, SDP_ERR_Insufficient_Resources);

            //process each match
            for(i = 0; i < numFound; i++){

                const uint8_t* ptr = results[i]->descriptor;
                sg_buf* res;

                //collect wanted attributes
                btStdRecursiveWalk(attrs, &numAttrs, &res, &ptr, results[i]->descrLen);
                if(!res) continue;

                //if requested, add the handle attribute
                for(j = 0; j < numAttrs; j++) if(SDP_ATTR_HANDLE >= (attrs[j] >> 16) && SDP_ATTR_HANDLE <= (attrs[j] & 0xFFFF)) break;
                if(j != numAttrs){
                    uint8_t buf[8] = {SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_2), 0x00, 0x00, SDP_ITEM_DESC(SDP_TYPE_UINT, SDP_SZ_4)};

                    buf[4] = results[i]->handle >> 24;
                    buf[5] = results[i]->handle >> 16;
                    buf[6] = results[i]->handle >> 8;
                    buf[7] = results[i]->handle;

                    if(!sg_add_back(res, buf, sizeof(buf), SG_FLAG_MAKE_A_COPY)){

                        sg_free(res);
                        free(res);
                        continue;
                    }
                }

                //wrap and append to the full results list
                if(btSdpPutIntoGroup(res)) sg_concat_back(resultSoFar, res);
                sg_free(res);
                free(res);
            }

            //wrap the whole thing if required
            if((cmd == SDP_PDU_Service_Search_Attribute_Request) && !btSdpPutIntoGroup(resultSoFar)){
                dbgPrintf("SDP: Failed to put results into a group\n");
                sg_free(resultSoFar);
                free(resultSoFar);
                return btSdpError(trans, SDP_ERR_Insufficient_Resources);
            }

            //flatten to a buffer
            uint8_t* buf = malloc(sg_length(resultSoFar));
            if(!buf){

                dbgPrintf("SDP: Failed to allocate flattened result array (%ub)\n", sg_length(resultSoFar));
                sg_free(resultSoFar);
                free(resultSoFar);
                return btSdpError(trans, SDP_ERR_Insufficient_Resources);
            }
            inst->resultSz = sg_length(resultSoFar);
            inst->result = buf;
            inst->contDescr = sdpContVal;
            sg_copyto(resultSoFar, buf);
            sg_free(resultSoFar);
            free(resultSoFar);
        }
        else if(cmd == SDP_PDU_Service_Search_Request){

            //allocate the array
            inst->resultSz = sizeof(uint32_t[numFound]);
            uint8_t* buf = malloc(inst->resultSz);

            if(!buf){

                dbgPrintf("SDP: Failed to allocate flattened result array (%ub)\n", sizeof(uint32_t[numFound]));
                return btSdpError(trans, SDP_ERR_Insufficient_Resources);
            }

            //process each match
            for(i = 0; i < numFound; i++){

                buf[i * 4 + 0] = results[i]->handle >> 24;
                buf[i * 4 + 1] = results[i]->handle >> 16;
                buf[i * 4 + 2] = results[i]->handle >> 8;
                buf[i * 4 + 3] = results[i]->handle;
            }

            //put everything in the right place
            inst->result = buf;
            inst->contDescr = sdpContVal;
            inst->numMatches = numFound;
        }
    }
    if(++sdpContVal == 0) sdpContVal = 0x01234567; //update continuation state to the next value

    //produce the packet to send
    uint8_t bufPrepend[9], bufPostpend[5] = {0, }, preSz = 5, postSz = 1;
    uint32_t sendSz = 0;
    result = sg_alloc();
    if(!result) return btSdpError(trans, SDP_ERR_Insufficient_Resources);

    if(cmd == SDP_PDU_Service_Attribute_Request || cmd == SDP_PDU_Service_Search_Attribute_Request){

        if(maxReplSz > 256) maxReplSz = 256;	//no harm in fragmenting - keep the packets small

        sendSz = inst->resultSz;
        if(sendSz > maxReplSz) sendSz = maxReplSz;

        bufPrepend[preSz++] = sendSz >> 8;
        bufPrepend[preSz++] = sendSz & 0xFF;
    }
    else if(cmd == SDP_PDU_Service_Search_Request){

        if(maxReplSz > 64) maxReplSz = 64;	//no harm in fragmenting - keep the packets small

        sendSz = inst->resultSz;
        if(sendSz > maxReplSz * sizeof(uint32_t)) sendSz = maxReplSz * sizeof(uint32_t);

        bufPrepend[preSz++] = inst->numMatches >> 8;
        bufPrepend[preSz++] = inst->numMatches & 0xFF;
        bufPrepend[preSz++] = (sendSz / sizeof(uint32_t)) >> 8;
        bufPrepend[preSz++] = (sendSz / sizeof(uint32_t)) & 0xFF;
    }

    if(!sg_add_back(result, inst->result, sendSz, SG_FLAG_MAKE_A_COPY)){

        dbgPrintf("SDP: Failed to attach reply. Droping");
        free(inst->result);
        inst->result = NULL;
        sg_free(result);
        free(result);
        return btSdpError(trans, SDP_ERR_Insufficient_Resources);
    }
    else{

        inst->resultSz -= sendSz;
        if(inst->resultSz){

            memcpy(inst->result, inst->result + sendSz, inst->resultSz);
            inst->result = realloc(inst->result, inst->resultSz);
        }
        else{
            free(inst->result);
            inst->result = NULL;
        }
    }

    if(inst->result){ //have more

        bufPostpend[0] = 4;
        for(i = 0; i < 4; i++) bufPostpend[i + 1] = inst->contDescr >> ((3 - i) << 3);
        postSz = 5;
    }

    bufPrepend[0] = cmd + 1; //response to this request
    bufPrepend[1] = trans[0];
    bufPrepend[2] = trans[1];
    bufPrepend[3] = (sendSz + preSz + postSz - 5) >> 8;
    bufPrepend[4] = (sendSz + preSz + postSz - 5) & 0xFF;
            
    if(sg_add_front(result, bufPrepend, preSz, SG_FLAG_MAKE_A_COPY) && sg_add_back(result, bufPostpend, postSz, SG_FLAG_MAKE_A_COPY)){

        return result;
    }
    sg_free(result);
    free(result);
    return btSdpError(trans, SDP_ERR_Insufficient_Resources);
}

static void* sdpServiceAlloc(uint16_t conn, uint16_t chan, uint16_t remChan){

    SdpInstance* inst = malloc(sizeof(SdpInstance));
    if(inst){

        inst->result = NULL;
        inst->aclConn = conn;
        inst->remChan = remChan;
    }
    return inst;
}

static void sdpServiceFree(void* service){

    SdpInstance* inst = (SdpInstance*)service;

    if(inst->result) free(inst->result);
    free(inst);
}

static void sdpServiceDataRx(void* service, const uint8_t* data, uint16_t size){

    SdpInstance* inst = (SdpInstance*)service;
    uint16_t conn = inst->aclConn;
    uint16_t remChan = inst->remChan;

    sg_buf* reply = btSdpProcessRequest(inst, data, size);
    if(reply){

/*//  -- ugly debugging code --
        unsigned i;
        uint8_t buf[256];
        sg_copyto(reply, buf);

        dbgPrintf("SDP req got (0x%x): ", size);
        for(i = 0; i < size; i++) dbgPrintf(" %02X", data[i]);
        dbgPrintf("\n");

        dbgPrintf("SDP reply sent (0x%x): ", sg_length(reply));
        for(i = 0; i < sg_length(reply); i++) dbgPrintf(" %02X", buf[i]);
        dbgPrintf("\n");
*/
        l2capServiceTx(conn, remChan, reply);
    }
}

void btSdpRegisterL2capService(){

    const L2capService sdp = {L2CAP_FLAG_SUPPORT_CONNECTIONS, sdpServiceAlloc, sdpServiceFree, sdpServiceDataRx};
    if(!l2capServiceRegister(L2CAP_PSM_SDP, &sdp)) dbgPrintf("SDP L2CAP registration failed\n");

    btSdpServiceDescriptorAdd(sdpDescrSdp, sizeof(sdpDescrSdp));
}

void btSdpServiceDescriptorAdd(const uint8_t* descriptor, uint16_t descrLen){

    SdpService *t, *s = malloc(sizeof(SdpService));
    if(s){

        s->handle = sdpNextHandle;

        if(sdpNextHandle) sdpNextHandle++;
        else sdpNextHandle = SDP_FIRST_USER_HANDLE; //first add is special - it adds the SDP service itself

        s->descriptor = descriptor;
        s->descrLen = descrLen;
        s->next = NULL;

        t = knownServices;	//add at end
        while(t && t->next) t = t->next;
        if(t) t->next = s;
        else knownServices = s;
    }
}

void btSdpServiceDescriptorDel(const uint8_t* descriptor){

    SdpService *s = knownServices, *p = NULL;

    while(s && s->descriptor != descriptor){

        p = s;
        s = s->next;
    }
    if(p) p->next = s->next;
    else knownServices = s->next;

    free(s);
}


