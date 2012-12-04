/*
Copyright (c) 2011, Dmitry Grinberg (as published for DGOS on http://dgosblog.blogspot.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following condition is met: 

* Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define ADK_INTERNAL
#include "fwk.h"
#include "printf.h"
#include <stdarg.h>

typedef struct {

    void* dest;
    uint32_t maxChars;

    void* furtherCallback;
    void* furtherUserData;

} PrintData;

typedef char (*StrPrintfExCbk)(void* userData, char chr);    //return 0 to stop printing

static uint32_t StrPrvPrintfEx_number(StrPrintfExCbk putc_,void* userData,unsigned long long number,uint32_t base,char zeroExtend,char isSigned,char capitals,uint32_t padToLength, char* bail){

    char buf[64];
    uint32_t idx = sizeof(buf) - 1;
    uint32_t chr, i;
    char neg = 0;
    uint32_t numPrinted = 0;

    *bail = 0;

    if(padToLength > 63) padToLength = 63;

    buf[idx--] = 0;    //terminate

    if(isSigned){

        if(number & 0x8000000000000000ULL){

            neg = 1;
            number = -number;
        }
    }

    do{
        chr = number % base;
        number = number / base;

        buf[idx--] = (chr >= 10)?(chr + (capitals ? 'A' : 'a') - 10):(chr + '0');

        numPrinted++;

    }while(number);

    if(neg){

        buf[idx--] = '-';
        numPrinted++;
    }

    if(padToLength > numPrinted){

        padToLength -= numPrinted;
    }
    else{

        padToLength = 0;
    }

    while(padToLength--){

        buf[idx--] = zeroExtend?'0':' ';
        numPrinted++;
    }

    idx++;


    for(i = 0; i < numPrinted; i++){

        if(!putc_(userData,(buf + idx)[i])){

            *bail = 1;
            break;
        }
    }


    return numPrinted;
}

static uint32_t StrVPrintf_StrLen_withMax(const char* s,uint32_t max){

    uint32_t len = 0;

    while((*s++) && (len < max)) len++;

    return len;
}

static uint32_t StrVPrintf_StrLen(const char* s){

    uint32_t len = 0;

    while(*s++) len++;

    return len;
}

static inline char prvGetChar(const char** fmtP){

    char ret;
    const char* fmt;

    fmt = *fmtP;
    ret = *fmt++;
    *fmtP = fmt;

    return ret;
}

static unsigned long long SignExt32to64(uint32_t v_){

    unsigned long long v = v_;

    if(v & 0x80000000) v |= 0xFFFFFFFF00000000ULL;

    return v;
}

static uint32_t StrVPrintfEx(StrPrintfExCbk putc_f,void* userData, const char* fmtStr,va_list vl){

    char c;
    uint32_t i, numPrinted = 0;
    unsigned long long val64;

#define putc_(_ud,_c)    if(!putc_f(_ud,_c)) goto out;

    while((c = prvGetChar(&fmtStr)) != 0){

        if(c == '\n'){

            putc_(userData,c);
            numPrinted++;
        }
        else if(c == '%'){

            char zeroExtend = 0, useLong = 0, bail = 0, useVeryLong = 0;
            uint32_t padToLength = 0,len;
            const char* str;
            int capitals = 0;

more_fmt:

            c = prvGetChar(&fmtStr);

            switch(c){

                case '%':

                    putc_(userData,c);
                    numPrinted++;
                    break;

                case 'c':

                    putc_(userData,va_arg(vl,unsigned int));
                    numPrinted++;
                    break;

                case 's':

                    str = va_arg(vl,char*);
                    if(!str) str = "(null)";
                    if(padToLength){

                        len = StrVPrintf_StrLen_withMax(str,padToLength);
                    }
                    else{

                        padToLength = len = StrVPrintf_StrLen(str);
                    }
                    if(len > padToLength) len = padToLength;
                    else{

                        for(i=len;i<padToLength;i++) putc_(userData,L' ');
                    }
                    numPrinted += padToLength;
                    for(i = 0; i < len; i++){

                        putc_(userData,*str++);
                    }
                    numPrinted += len;
                    break;

                case '0':

                    if(!zeroExtend && !padToLength){

                        zeroExtend = 1;
                        goto more_fmt;
                    }

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':

                    padToLength = (padToLength * 10) + c - '0';
                    goto more_fmt;

                case 'u':

                    val64 = useVeryLong ? va_arg(vl,unsigned long long) : va_arg(vl,uint32_t);
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData,val64,10,zeroExtend,0,0,padToLength, &bail);
                    if(bail) goto out;
                    break;

                case 'd':

                    val64 = useVeryLong ? va_arg(vl,unsigned long long) : SignExt32to64(va_arg(vl,uint32_t));
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData,val64,10,zeroExtend,1,0,padToLength, &bail);
                    if(bail) goto out;
                    break;

                case 'X':

                    capitals = 1;

                case 'x':

                    val64 = useVeryLong ? va_arg(vl,unsigned long long) : va_arg(vl,uint32_t);
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData,val64,16,zeroExtend,0,capitals,padToLength, &bail);
                    if(bail) goto out;
                    break;

                case 'p':

                    putc_(userData,'0');
                    numPrinted++;
                    putc_(userData,'x');
                    numPrinted++;

                    val64 = va_arg(vl,unsigned long);
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData,val64,16,0,0,0,0, &bail);
                    if(bail) goto out;
                    break;

                case 'b':

                    val64 = useVeryLong ? va_arg(vl,unsigned long long) : va_arg(vl,uint32_t);
                    numPrinted += StrPrvPrintfEx_number(putc_f, userData,val64,2,zeroExtend,0,0,padToLength, &bail);
                    if(bail) goto out;
                    break;

                case 'l':
                    if(useLong) useVeryLong = 1;
                    useLong = 1;
                    goto more_fmt;

                default:

                    putc_(userData,c);
                    numPrinted++;
                    break;
            }
        }
        else{

            putc_(userData,c);
            numPrinted++;
        }
    }

    putc_(userData,0);    //terminate it
    numPrinted++;

out:

    return numPrinted;
}


static char StrPrintF_putc(void* ud, char c){

    PrintData* pd = ud;
    char** dst = pd->dest;
    char ret = 1;

    if(pd->maxChars-- == 1){

        c = 0;
        ret = 0;
    }

    if(pd->furtherCallback){

        ret = ((printf_write_c)pd->furtherCallback)(pd->furtherUserData, c);
    }
    else{
        *(*dst)++ = c;
    }


    return ret;
}

uint32_t _sprintf(char* dst, const char* fmtStr, ...){

    uint32_t ret;
    va_list vl;
    PrintData pd;

    pd.dest = &dst;
    pd.maxChars = 0xFFFFFFFF;
    pd.furtherCallback = NULL;

    va_start(vl,fmtStr);

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    va_end(vl);

    return ret;
}

uint32_t _snprintf(char* dst, uint32_t maxChars, const char* fmtStr, ...){

    uint32_t ret;
    va_list vl;
    PrintData pd;

    pd.dest = &dst;
    pd.maxChars = maxChars;
    pd.furtherCallback = NULL;

    va_start(vl,fmtStr);

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    va_end(vl);

    return ret;
}

uint32_t _csprintf(printf_write_c writeF, void* writeD, const char* fmtStr, ...){

    uint32_t ret;
    va_list vl;
    PrintData pd;

    pd.dest = NULL;
    pd.maxChars = 0xFFFFFFFF;
    pd.furtherCallback = writeF;
    pd.furtherUserData = writeD;

    va_start(vl,fmtStr);

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    va_end(vl);

    return ret;
}

uint32_t _csnprintf(printf_write_c writeF, void* writeD, uint32_t maxChars, const char* fmtStr, ...){

    uint32_t ret;
    va_list vl;
    PrintData pd;

    pd.dest = NULL;
    pd.maxChars = maxChars;
    pd.furtherCallback = writeF;
    pd.furtherUserData = writeD;

    va_start(vl,fmtStr);

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    va_end(vl);

    return ret;
}

uint32_t _vsprintf(char* dst, const char* fmtStr, va_list vl){

    uint32_t ret;
    PrintData pd;

    pd.dest = &dst;
    pd.maxChars = 0xFFFFFFFF;
    pd.furtherCallback = NULL;

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    return ret;
}

uint32_t _vsnprintf(char* dst, uint32_t maxChars, const char* fmtStr, va_list vl){

    uint32_t ret;
    PrintData pd;

    pd.dest = &dst;
    pd.maxChars = maxChars;
    pd.furtherCallback = NULL;

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    return ret;
}

uint32_t _cvsprintf(printf_write_c writeF, void* writeD, const char* fmtStr, va_list vl){

    uint32_t ret;
    PrintData pd;

    pd.dest = NULL;
    pd.maxChars = 0xFFFFFFFF;
    pd.furtherCallback = writeF;
    pd.furtherUserData = writeD;

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    return ret;
}

uint32_t _cvsnprintf(printf_write_c writeF, void* writeD, uint32_t maxChars, const char* fmtStr, va_list vl){

    uint32_t ret;
    PrintData pd;

    pd.dest = NULL;
    pd.maxChars = maxChars;
    pd.furtherCallback = writeF;
    pd.furtherUserData = writeD;

    ret = StrVPrintfEx(&StrPrintF_putc, &pd, fmtStr,vl);

    return ret;
}
