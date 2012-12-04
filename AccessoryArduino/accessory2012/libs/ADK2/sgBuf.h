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
#ifndef _SG_BUF_H_
#define _SG_BUF_H_


/*
	sg_buf is a linked-list based buffer which makes it easy to encapsulate
	data in places like bluetooth, where each layer needs to add some data
	to the packet on its way out. This is batter than normal buffers since
	no data has to be moved and header sizes need nto be known ahead of time.
*/

typedef struct sg_buf_node{

    struct sg_buf_node* next;
    uint32_t info;
    const uint8_t* data;

}sg_buf_node;

typedef struct{

    sg_buf_node* first;
    sg_buf_node* last;
    uint32_t totalLen;

}sg_buf;

typedef void* sg_iter;

#define SG_BUF_STATIC_INITIALIZER		{NULL, NULL, 0}

#define SG_FLAG_NEEDS_FREEING	1
#define SG_FLAG_MAKE_A_COPY	2

void sg_init(sg_buf* buf);
sg_buf* sg_alloc(void);
void sg_free(sg_buf* buf);
uint32_t sg_length(const sg_buf* buf);
char sg_add_front(sg_buf* buf, const uint8_t* data, uint32_t len, uint8_t flags);	//0 on fail
char sg_add_back(sg_buf* buf, const uint8_t* data, uint32_t len, uint8_t flags);	//0 on fail

void sg_concat_front(sg_buf* buf, sg_buf* second);	//prepend second to buf
void sg_concat_back(sg_buf* buf, sg_buf* second);		//append second to buf

sg_iter sg_iter_start(const sg_buf* buf);
char sg_iter_next(sg_iter* iterP, const uint8_t** buf, uint32_t* sz);

void sg_copyto(const sg_buf* buf, uint8_t* dst);

#endif
#endif
