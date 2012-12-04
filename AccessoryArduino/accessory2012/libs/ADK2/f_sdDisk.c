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
#include "dbg.h"
#include "SD.h"
#include "f_ff.h"
#include "f_diskio.h"


static SD sd;

DWORD get_fattime (void){
    return 0; //hehehehehehehe
}

WCHAR ff_convert (WCHAR c, UINT dir){

    return c;
}

WCHAR ff_wtoupper (WCHAR c){

    if(c >= 'a' && c<='z') c -= 'a' - 'A';
    return c;
}

DSTATUS disk_initialize (BYTE drv){

    if(drv) return STA_NODISK;

    if(!sdInit(&sd)){

        dbgPrintf("ADK: SD init failed\n");
        return STA_NODISK;
    }

    dbgPrintf("disk size: %lu sectors\n", sdGetNumSec(&sd));

    return 0;
}

DSTATUS disk_status (BYTE drv){

    if(drv) return STA_NODISK;

    return 0;
}

DRESULT disk_read (BYTE drv, BYTE *buff, DWORD sector, BYTE numSec){

    if(drv) return STA_NODISK;


    while(numSec--){
        if(!sdSecRead(&sd, sector++, buff)) return RES_ERROR;
        buff+=512;
    }
    return RES_OK;
//this is more clever, but does not yet work

    if(!sdReadStart(&sd, sector)) return RES_ERROR;
    while(numSec){
        buff = sdStreamSec(buff);
        if(--numSec) sdNextSec(&sd);
    }
    sdSecReadStop(&sd);
    return RES_OK;
}

DRESULT disk_write (BYTE drv, const BYTE *buff, DWORD sector, BYTE numSec){

    if(drv) return STA_NODISK;

    while(numSec--){
        if(!sdSecWrite(&sd, sector++, buff)) return RES_ERROR;
        buff+=512;
    }
    return RES_OK;
}

DRESULT disk_ioctl (BYTE drv, BYTE ctrl, void *buff){

    DRESULT res;

    if(drv) return STA_NODISK;

    switch (ctrl){
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            res = RES_OK;
            break;

        case GET_SECTOR_COUNT :   /* Get number of sectors on the disk (DWORD) */
            *(DWORD*)buff = sdGetNumSec(&sd);
            res = RES_OK;
            break;

        case GET_SECTOR_SIZE :   /* Get sectors on the disk (WORD) */
            *(DWORD*)buff = 512;
            res = RES_OK;
            break;

        case CTRL_SYNC :   /* Make sure that data has been written */
            res = RES_OK;
            break;

        default:
            res = RES_PARERR;
    }
    return res;
}
