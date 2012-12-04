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
#include "printf.h"
#include <stdarg.h>

#define USE_UART 0


void (*putcharF)(char) = NULL;

static char writeF(void* unused, char c){


    if(c == '\n') if(putcharF) putcharF('\r');  //serial consoles need this
    if(putcharF) putcharF(c);
    return 1;
}

void dbgPrintf(const char* fmt, ...){

    va_list vl;

    va_start(vl, fmt);
    _cvsprintf(writeF, NULL, fmt, vl);
    va_end(vl);
}

void dbgSetPutchar(void (*fn)(char)){

    putcharF = fn;
}
