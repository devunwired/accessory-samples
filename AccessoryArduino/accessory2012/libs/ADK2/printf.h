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
#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdarg.h>

typedef char (*printf_write_c)(void* userData, char c);  //return 0 to stop printing, else 1


uint32_t _sprintf(char* dst, const char* fmtStr, ...);
uint32_t _snprintf(char* dst, uint32_t maxChars, const char* fmtStr, ...);
uint32_t _csprintf(printf_write_c writeF, void* writeD, const char* fmtStr, ...);
uint32_t _csnprintf(printf_write_c writeF, void* writeD, uint32_t maxChars, const char* fmtStr, ...);
uint32_t _vsprintf(char* dst, const char* fmtStr, va_list vl);
uint32_t _vsnprintf(char* dst, uint32_t maxChars, const char* fmtStr, va_list vl);
uint32_t _cvsprintf(printf_write_c writeF, void* writeD, const char* fmtStr, va_list vl);
uint32_t _cvsnprintf(printf_write_c writeF, void* writeD, uint32_t maxChars, const char* fmtStr, va_list vl);

#endif
#endif

