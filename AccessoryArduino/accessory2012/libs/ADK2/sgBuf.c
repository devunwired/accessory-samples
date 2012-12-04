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
#include <string.h>


#define INFO_MASK_NEEDS_FREE	0x80000000
#define INFO_MASK_SIZE		0x00FFFFFF


void sg_init(sg_buf* buf){

    buf->first = NULL;
    buf->last = NULL;
    buf->totalLen = 0;
}

sg_buf* sg_alloc(void){

    sg_buf* b = malloc(sizeof(sg_buf));
    if(b) sg_init(b);

    return b;
}

void sg_free(sg_buf* buf){

    sg_buf_node* n = buf->first;
    sg_buf_node* t;

    while(n){

        if(n->info & INFO_MASK_NEEDS_FREE) free(n->data);

        t = n->next;
        free(n);
        n = t;
    }
}

uint32_t sg_length(const sg_buf* buf){

    return buf->totalLen;
}

static sg_buf_node* sg_alloc_node(const uint8_t* data, uint32_t len, uint8_t flags){

    sg_buf_node* n;
    uint32_t sz;

    if(len &~ INFO_MASK_SIZE) return NULL; //too big

    sz = sizeof(sg_buf_node);
    if(flags & SG_FLAG_MAKE_A_COPY) sz += len;

    n = malloc(sz);

    if(n){

        if(flags & SG_FLAG_MAKE_A_COPY){

            uint8_t* ptr = (uint8_t*)(n + 1);
            memcpy(ptr, data, len);
            data = ptr;
            flags &=~ SG_FLAG_NEEDS_FREEING;	//definitely not
        }

        n->info = len | ((flags & SG_FLAG_NEEDS_FREEING) ? INFO_MASK_NEEDS_FREE : 0);
        n->data = data;
    }
    return n;
}

char sg_add_front(sg_buf* buf, const uint8_t* data, uint32_t len, uint8_t flags){

    sg_buf_node* n = sg_alloc_node(data, len, flags);

    if(!n) return 0;

    n->next = buf->first;
    buf->first = n;
    if(!buf->last) buf->last = n;

    buf->totalLen += len;

    return 1;
}

char sg_add_back(sg_buf* buf, const uint8_t* data, uint32_t len, uint8_t flags){

    sg_buf_node* n = sg_alloc_node(data, len, flags);

    if(!n) return 0;

    n->next = NULL;
    if(buf->last) buf->last->next = n;
    else buf->first = n;
    buf->last = n;

    buf->totalLen += len;

    return 1;
}

void sg_concat_back(sg_buf* buf, sg_buf* second){

    buf->totalLen += second->totalLen;

    if(buf->last) buf->last->next = second->first;
    else buf->first = second->first;
    buf->last = second->last;

    sg_init(second);
}

void sg_concat_front(sg_buf* buf, sg_buf* second){

    sg_concat_back(second, buf);
    *buf = *second;

    sg_init(buf);
}

sg_iter sg_iter_start(const sg_buf* buf){

    return (sg_iter)(buf->first);
}

char sg_iter_next(sg_iter* iterP, const uint8_t** buf, uint32_t* sz){

    sg_buf_node* n = (sg_buf_node*)*iterP;

    if(!n) return 0;	//end of line

    if(buf) *buf = n->data;
    if(sz) *sz = n->info & INFO_MASK_SIZE;

    *iterP = n->next;
    return 1;
}

void sg_copyto(const sg_buf* buf, uint8_t* dst){

    sg_buf_node* n = buf->first;
    uint32_t len;

    while(n){

        len = n->info & INFO_MASK_SIZE;

        memcpy(dst, n->data, len);
        dst += len;

        n = n->next;
    }
}












