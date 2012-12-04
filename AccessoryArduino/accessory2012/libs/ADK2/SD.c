/*
Copyright (c) 2011, Dmitry Grinberg (as published on http://dmitrygr.com)
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
#include "SD.h"





static const uint8_t pinCD = PORTA(5);
static const uint8_t pinsSD[] = {PORTA(7), PORTA(19), PORTA(20), PORTA(21)};
#define CS	0
#define SCLK	1
#define MOSI	2
#define MISO	3



#define FLAG_TIMEOUT        0x80
#define FLAG_PARAM_ERR        0x40
#define FLAG_ADDR_ERR        0x20
#define FLAG_ERZ_SEQ_ERR    0x10
#define FLAG_CMD_CRC_ERR    0x08
#define FLAG_ILLEGAL_CMD    0x04
#define FLAG_ERZ_RST        0x02
#define FLAG_IN_IDLE_MODE    0x01



char gSdSpiFast = 1;

static void sdClockSpeed(char fast){

    gSdSpiFast = fast;
}

static uint8_t sdSpiByte(uint32_t v){

    uint32_t r = 0;

    #define BIT(m, d)								\
		do{								\
                        gpioSetVal(pinsSD[MOSI], !!(v & (m)));			\
                        gpioSetVal(pinsSD[SCLK], 1);				\
                        d;							\
                        if(gpioGetVal(pinsSD[MISO]))				\
				r |= (m);					\
			gpioSetVal(pinsSD[SCLK], 0);				\
		}while(0)

    #define ASMBIT				\
		"lsls  %1, #1		\n\t"	\
		"ite   cs		\n\t"	\
		"strcs r4, [r2, #0x30]	\n\t"	\
		"strcc r4, [r2, #0x34]	\n\t"	\
		"str   r1, [r2, #0x30]	\n\t"	\
		"ldr   r3, [r2, #0x3C]	\n\t"	\
		"str   r1, [r2, #0x34]	\n\t"	\
		"lsrs  r3, #22		\n\t"	\
		"adcs  %0, %0		\n\t"
	
    if(gSdSpiFast){

        
        asm(
                "mov r4, #0x100000	\n\t"	//mosi
                "mov r1, #0x080000	\n\t"	//sclk
                "ldr r2, =0x400E0E00	\n\t"	//PIOA
                "lsl %1, #24		\n\t"	//cleverness
                ASMBIT
                ASMBIT
                ASMBIT
                ASMBIT
                ASMBIT
                ASMBIT
                ASMBIT
                ASMBIT
            :"=l"(r)
            :"l"(v)
            :"cc", "r4", "r1", "r2", "r3"
        );

    }
    else{

        volatile uint32_t x;
        uint8_t t;

        for(t = 0; t < 8; t++){

            BIT(1 << (7 - t), for(x = 0; x < 0x10; x += 2) x--);
            for(x = 0; x < 0x10; x += 2) x--;
        }
    }

    return r;
}

uint8_t* __attribute__((naked)) sdStreamSec(uint8_t* buf){

#define RDBIT					\
		"str   r1, [r2, #0x30]	\n\t"	\
		"ldr   r3, [r2, #0x3C]	\n\t"	\
		"str   r1, [r2, #0x34]	\n\t"	\
		"lsrs  r3, #22		\n\t"	\
		"adcs  r4, r4		\n\t"

    asm(
            "    push {r4,r5,lr}	\n\t"
            "    ldr r2, =0x400E0E00	\n\t"
            "    mov r1, #0x100000	\n\t"  //mosi
            "    str r1, [r2, #0x30]	\n\t"
            "    mov r1, #0x080000	\n\t"
            "    mov r5, #128		\n\t"
            "loop:			\n\t"
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
                RDBIT
            "    rev  r4, r4		\n\t"
            "    str  r4, [r0], #4	\n\t"
            "    subs r5, #1		\n\t"
            "    bne loop		\n\t"
            "    pop {r4,r5,pc}		\n\t"
    );
}

static void sdChipSelect(char on){

    gpioSetVal(pinsSD[CS], !on);
}

static uint8_t sdCrc7(uint8_t* chr,uint8_t cnt,uint8_t crc){

    uint8_t i, a;
    uint8_t Data;

    for(a = 0; a < cnt; a++){

        Data = chr[a];

        for(i = 0; i < 8; i++){

            crc <<= 1;

            if( (Data & 0x80) ^ (crc & 0x80) ) crc ^= 0x09;

            Data <<= 1;
        }
    }

    return crc & 0x7F;
}

static inline void sdPrvSendCmd(uint8_t cmd, uint32_t param, char crc){

    uint8_t send[6];
    uint8_t L = crc ? 6 : 5;

    send[0] = cmd | 0x40;
    send[1] = param >> 24;
    send[2] = param >> 16;
    send[3] = param >> 8;
    send[4] = param;
    send[5] = crc ? (sdCrc7(send, 5, 0) << 1) | 1 : 0;

    for(cmd = 0; cmd < L; cmd++) sdSpiByte(send[cmd]);
}

static uint8_t sdPrvSimpleCommand(uint8_t cmd, uint32_t param, char crc, char cmdDone){    //do a command, return R1 reply

    uint8_t ret;
    uint8_t i = 0;

    sdChipSelect(1);
    sdPrvSendCmd(cmd, param, crc);

    do{        //our max wait time is 128 byte clocks (1024 clock ticks)

        ret = sdSpiByte(0xFF);

    }while(i++ < 128 && (ret == 0xFF));
    if(cmdDone){
        sdChipSelect(0);
        sdSpiByte(0xFF);
    }

    return ret;
}

static uint8_t sdPrvReadData(uint8_t* data, uint32_t sz, char cmdDone){

    uint8_t ret;
    uint16_t tries = 20000;

    do{
        ret = sdSpiByte(0xFF);
        if((ret & 0xF0) == 0x00) return ret;    //fail
        if(ret == 0xFE) break;
        tries--;
    }while(tries);

    if(!tries) return 0xFF;

    *data = ret;

    ret = 0;

    while(sz--) *data++ = sdSpiByte(0xFF);

    if(cmdDone){
        sdChipSelect(0);
        sdSpiByte(0xFF);
    }

    return ret;
}

static uint8_t sdPrvACMD(uint8_t cmd, uint32_t param, char crc){

    uint8_t ret;

    ret = sdPrvSimpleCommand(55, 0, crc, 1);
    if(ret & FLAG_TIMEOUT) return ret;
    if(ret & FLAG_ILLEGAL_CMD) return ret;

    return sdPrvSimpleCommand(cmd, param, crc, 1);
}

static char sdPrvCardInit(char sd, char hc){

    uint32_t time = 0;
    uint32_t resp;
    uint32_t param;

    param = hc ? (1UL<< 30) : 0;

    while(time++ < 10000UL){    //retry 10..0 times

        resp = sd ? sdPrvACMD(41, param, 1) : sdPrvSimpleCommand(1, param, 1, 1);
        if(resp & FLAG_TIMEOUT) break;
        if(!(resp & FLAG_IN_IDLE_MODE)) return 1;
    }

    return 0;
}

static uint32_t sdPrvGetBits(uint8_t* data,uint32_t numBytesInArray,uint32_t startBit,uint32_t len){//for CID and CSD data..

    uint32_t bitWrite = 0;
    uint32_t numBitsInArray = numBytesInArray * 8;
    uint32_t ret = 0;

    do{

        uint32_t bit,byte;

        bit = numBitsInArray - startBit - 1;
        byte = bit / 8;
        bit = 7 - (bit % 8);

        ret |= ((data[byte] >> bit) & 1) << (bitWrite++);

        startBit++;
    }while(--len);

    return ret;
}

static uint32_t sdPrvGetCardNumBlocks(char mmc,uint8_t* csd){

    uint32_t ver = sdPrvGetBits(csd,16,126,2);
    uint32_t cardSz = 0;


    if(ver == 0 || (mmc && ver <= 2)){

        uint32_t cSize = sdPrvGetBits(csd,16,62,12);
        uint32_t cSizeMult = sdPrvGetBits(csd,16,47,3);
        uint32_t readBlLen = sdPrvGetBits(csd,16,80,4);
        uint32_t blockLen,blockNr;
        uint32_t divTimes = 9;        //from bytes to blocks division

        blockLen = 1UL << readBlLen;
        blockNr = (cSize + 1) * (1UL << (cSizeMult + 2));

        /*
             multiplying those two produces result in bytes, we need it in blocks
             so we shift right 9 times. doing it after multiplication might fuck up
             the 4GB card, so we do it before, but to avoid killing significant bits
             we only cut the zero-valued bits, if at the end we end up with non-zero
             "divTimes", divide after multiplication, and thus underuse the card a bit.
             This will never happen in reality since 512 is 2^9, and we are
             multiplying two numbers whose product is a multiple of 2^9, so they
             togethr should have at least 9 lower zero bits.
        */

        while(divTimes && !(blockLen & 1)){

            blockLen = blockLen >> 1;
            divTimes--;
        }
        while(divTimes && !(blockNr & 1)){

            blockNr = blockNr >> 1;
            divTimes--;
        }

        cardSz = (blockNr * blockLen) >> divTimes;
    }
    else if(ver == 1){

        cardSz = sdPrvGetBits(csd,16,48,22)/*num 512K blocks*/ << 10;
    }


    return cardSz;
}

char sdInit(SD* sd){

    uint8_t v, t;
    uint16_t retries = 1050;
    uint8_t respBuf[16];

    dbgPrintf("SD: initing\n");

    gpioSetFun(pinCD, GPIO_FUNC_GPIO);
    gpioSetDir(pinCD, 1);

    for(v = 0; v < sizeof(pinsSD); v++){

        gpioSetFun(pinsSD[v], GPIO_FUNC_GPIO);
        gpioSetDir(pinsSD[v], 0);
        gpioSetVal(pinsSD[v], 0);
    }
    gpioSetDir(pinsSD[MISO], 1);
    gpioSetVal(pinsSD[CS], 1);

    sd->inited = 0;
    sd->SD = 0;
    sd->HC = 0;

    sdClockSpeed(0);
    sdChipSelect(0);
    for(v = 0; v < 10; v++) sdSpiByte(0xFF);    //80 clocks with CS not asserted to give card time to init

    while(0x01 != sdPrvSimpleCommand(0, 0, 1, 1)) if(!--retries) return 0;	//always a good idea
    
    v = sdPrvSimpleCommand(8, 0x000001AAUL, 1, 0);    //try CMD8 to init SDHC cards
    if(v & FLAG_TIMEOUT) return 0;
    sdSpiByte(0xFF);	//ignore the reply
    sdSpiByte(0xFF);
    sdSpiByte(0xFF);
    sdSpiByte(0xFF);
    sdChipSelect(0);
    sdSpiByte(0xFF);

    v = sdPrvSimpleCommand(55, 0, 1, 1);            //see if this is SD or MMC
    if(v & FLAG_TIMEOUT) return 0;
    sd->SD = !(v & FLAG_ILLEGAL_CMD);

    dbgPrintf("SD: card is likely %s\n", sd->SD ? "SD" : "MMC");

    if(sd->SD){

        if(!sdPrvCardInit(1, 1) && !sdPrvCardInit(1, 0)){

            return 0;
        }

        v = sdPrvSimpleCommand(58,0, 1, 0);    //see if it is HC
        t = sdSpiByte(0xFF);	//get the reply
        sdSpiByte(0xFF);
        sdSpiByte(0xFF);
        sdSpiByte(0xFF);
        sdChipSelect(0);
        sdSpiByte(0xFF);
        if(v) t = 0;
        if(t & 0x40) sd->HC = 1;
    }
    else{

        if(!sdPrvCardInit(0, 1) && !sdPrvCardInit(0, 0)){

            return 0;
        }
    }

    v = sdPrvSimpleCommand(59, 0, 1, 1);        //crc off
    if(v & FLAG_TIMEOUT) return 0;

    v = sdPrvSimpleCommand(9, 0, 0, 0);           //read CSD
    if(v & FLAG_TIMEOUT) return 0;

    v = sdPrvReadData(respBuf, 16, 1);
    if(v) return 0;

    sd->numSec = sdPrvGetCardNumBlocks(!sd->SD, respBuf);
    sd->inited = 1;

    sdClockSpeed(1);

    return 1;
}

uint32_t sdGetNumSec(SD* sd){

    return sd->inited ? sd->numSec : 0;
}

char sdSecRead(SD* sd, uint32_t sec, void* buf){    //CMD17

    uint8_t v, retry = 0;
    char ret = 0;

    if(!sd->inited) return 0;

    do{

        v = sdPrvSimpleCommand(17, sd->HC ? sec : sec << 9, 0, 0);
        if(v & FLAG_TIMEOUT) return 0;

        v = sdPrvReadData(buf, 0, 0);
        sdStreamSec(buf);
        sdSpiByte(0xFF);  //skip crc
        sdSpiByte(0xFF);
        sdChipSelect(0);
        if(!v){
            ret = 1;
            break;
        }
        
    
    }while(++retry < 5);    //retry up to 5 times

    return ret;
}


char sdSecWrite(SD* sd, uint32_t sec, const void* buf){    //CMD24

    uint32_t v16;
    uint8_t v, retry = 0;
    uint8_t* buf_ = buf;
    char ret = 0;


    if(!sd->inited) return 0;

    do{

        v = sdPrvSimpleCommand(24, sd->HC ? sec : sec << 9, 0, 0);
        if(v & FLAG_TIMEOUT) return 0;

        sdSpiByte(0xFF);    //as per SD-spi spec, we give it 8 clocks to consider the ramifications of the command we just sent

        sdSpiByte(0xFE);    //start of data block
        for(v16 = 0; v16 < SD_BLOCK_SIZE; v16++) sdSpiByte(*buf_++);    //data


        while((v = sdSpiByte(0xFF)) == 0xFF);    //wait while card isnt answering
        while(sdSpiByte(0xFF) != 0xFF);    //wait while card is busy

        if((v & 0x1F) == 5){
            ret = 1;
            break;
        }

    }while(++retry < 5);        //retry up to 5 times

    return ret;
}





//stream mode
char sdReadStart(SD* sd, uint32_t sec){

    uint8_t v;

    v = sdPrvSimpleCommand(18, sd->HC ? sec : sec << 9, 0, 0);
    if(v & FLAG_TIMEOUT) return 0;

    do{
        v = sdSpiByte(0xFF);
    }while(v == 0xFF);
    if(v != 0xFE){
        sdChipSelect(0);
        return 0;
    }
    return 1;
}

void sdNextSec(SD* sd){

    uint8_t v;

    sdSpiByte(0xFF);	//skip crc
    sdSpiByte(0xFF);

    do{
        v = sdSpiByte(0xFF);
    }while(v == 0xFF);
}

void sdSecReadStop(SD* sd){

    sdChipSelect(0);
    sdSpiByte(0xFF);	//skip crc
    sdSpiByte(0xFF);

    //cancel read
    sdPrvSimpleCommand(12, 0, 0, 1);
}






























