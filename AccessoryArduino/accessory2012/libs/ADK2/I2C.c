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
#include "I2C.h"
#include "coop.h"
#include "dbg.h"

// the hardware interface in this chip is as pleasant to use as cuddling a pirahna, hence the bit-bang solution.
// to use the hardware, set I2C_BITBANG to 0.
// to use the peripheral DMA with the I2C hardware, set I2C_PDC to 1.
#define I2C_BITBANG 0
#define I2C_PDC 1
#define I2C_INT_PDC 1

#define I2C_STAT_DONE 0
#define I2C_STAT_LASTREAD 2
#define I2C_STAT_BUSY 1
#define I2C_STAT_NACK 3

typedef struct _i2cInst
{
    Twi *pTwi;
    volatile uint8_t status;
    uint8_t *lastByte;
} i2c_i;

static i2c_i i2cInst[I2C_NUM_INSTANCES] = { { TWI0, 0, 0 }, { TWI1, 0, 0 } };

static char sdaRd(uint8_t instance);
static void sdaLo(uint8_t instance);
static void sdaHi(uint8_t instance);
static void sclLo(uint8_t instance);
static void sclHi(uint8_t instance);
static void i2cBitTx(uint8_t instance, char bit);
static void i2cStop(uint8_t instance);
static void i2cPdcDisable(Twi *pTwi);

	//sda first please
static const uint8_t pins[I2C_NUM_INSTANCES][2] = { { PORTA(17), PORTA(18) }, { PORTB(12), PORTB(13) } };
static uint32_t delayTicks[I2C_NUM_INSTANCES];


static void i2cDelay(uint8_t instance){

    fwkDelay(delayTicks[instance]);
}

void i2cClock(uint8_t instance, uint32_t clock){
#if I2C_BITBANG
    delayTicks[instance] = (TICKS_PER_MS * 1000) / (8 * clock * 2);
#else
    Twi *pTwi;
    uint32_t dwCkDiv = 0;
    uint32_t dwClDiv;
    uint32_t dwOk = 0;
    if (instance == 0)
        pTwi = TWI0;
    else
        pTwi = TWI1;

    while (!dwOk) {
        dwClDiv = ((BOARD_MCK / (2 * clock)) - 4) / (1 << dwCkDiv);
        if (dwClDiv <= 255)
            dwOk = 1;
        else
            dwCkDiv++;
    }
    pTwi->TWI_CWGR = 0;
    pTwi->TWI_CWGR = (dwCkDiv << 16) | (dwClDiv << 8) | dwClDiv;
#endif
}

// i2c interrupt handler used only for PDC mode
// to prevent runaway operations caused by not responding
// fast enough and setting the stop bit.
void i2c_handler( i2c_i *pInst )
{
    uint32_t status;
    Twi *pTwi = pInst->pTwi;

    status = pTwi->TWI_SR & pTwi->TWI_IMR; // Masked status

    if (TWI_SR_TXBUFE & status) { // Transmit has completed.
        i2cPdcDisable(pTwi);
        pTwi->TWI_CR = TWI_CR_STOP;
        pTwi->TWI_IDR = TWI_IDR_TXBUFE;
        pTwi->TWI_IER = TWI_IER_TXCOMP;
    } else if (TWI_SR_RXBUFF & status) { // Receive has completed
        if (pInst->status == I2C_STAT_LASTREAD) {
            i2cPdcDisable(pTwi);
            pTwi->TWI_IDR = TWI_IDR_RXBUFF;
            pTwi->TWI_IER = TWI_IER_TXCOMP;
        } else {
            pInst->status = I2C_STAT_LASTREAD;
            i2cPdcDisable(pTwi);
            pTwi->TWI_CR = TWI_CR_STOP;
            pTwi->TWI_RPR = pInst->lastByte;
            pTwi->TWI_RCR = 1;
            pTwi->TWI_PTCR = TWI_PTCR_RXTEN;
        }
    } else if (TWI_SR_TXCOMP & status) {
        pInst->status = I2C_STAT_DONE;
        pTwi->TWI_IDR = TWI_IDR_TXCOMP;
    } else if (TWI_SR_NACK & status) {
        pInst->status = I2C_STAT_NACK;
        i2cPdcDisable(pTwi);
    }
}

void TWI0_Handler(void)
{
    i2c_handler(&i2cInst[0]);
}

void TWI1_Handler(void)
{
    i2c_handler(&i2cInst[1]);
}

uint8_t i2cInit(uint8_t instance, uint32_t clock){
    uint8_t i;

    if(instance >= I2C_NUM_INSTANCES) return I2C_ERR_INVAL;

    gpioSetFun(pins[instance][0], GPIO_FUNC_GPIO);
    gpioSetFun(pins[instance][1], GPIO_FUNC_GPIO);
    gpioSetPullup(pins[instance][0], 1);
    gpioSetPullup(pins[instance][1], 1);
    gpioSetDir(pins[instance][0], 0);
    gpioSetDir(pins[instance][1], 0);
    gpioSetVal(pins[instance][0], 0);
    gpioSetVal(pins[instance][1], 0);
    sdaHi(instance);
    sclHi(instance);
    delayTicks[instance] = (TICKS_PER_MS * 1000) / (8 * clock * 2);
    i2cDelay(instance);
    i2cDelay(instance);
    for (i = 0; i < 100; i++)       // Try to reset the bus
        i2cBitTx(instance, 1);
    i2cStop(instance);

#if !I2C_BITBANG
    PMC_EnablePeripheral(instance ? ID_TWI1 : ID_TWI0);
    gpioSetFun(pins[instance][0], GPIO_FUNC_A);
    gpioSetFun(pins[instance][1], GPIO_FUNC_A);
    TWI_ConfigureMaster(instance ? TWI1 : TWI0, clock, BOARD_MCK);

#if I2C_INT_PDC
    NVIC_EnableIRQ(instance ? TWI1_IRQn : TWI0_IRQn);
    if (instance)
        TWI1->TWI_IDR = 0x00F77;
    else
        TWI0->TWI_IDR = 0x00F77;
#endif

#endif


    return I2C_ALL_OK;
}

static void i2cStart(uint8_t instance){

    sdaHi(instance);
    sclHi(instance);
    i2cDelay(instance);
    sdaLo(instance);
    i2cDelay(instance);
    sclLo(instance);
    i2cDelay(instance);
}

static void i2cStop(uint8_t instance){

    i2cDelay(instance);
    sdaLo(instance);
    i2cDelay(instance);
    sclHi(instance);
    i2cDelay(instance);
    sdaHi(instance);
}

static char i2cBitRx(uint8_t instance){

    char ret;

    sdaHi(instance);
    i2cDelay(instance);
    sclHi(instance);
    i2cDelay(instance);
    ret = sdaRd(instance);
    sclLo(instance);

    return ret;
}

static void i2cBitTx(uint8_t instance, char bit){

    if(bit) sdaHi(instance);
    else sdaLo(instance);
    i2cDelay(instance);
    sclHi(instance);
    i2cDelay(instance);
    sclLo(instance);
}

static char i2cSend(uint8_t instance, uint8_t val){	//return 1 -> ack, 0->nak

    uint8_t i;

    for(i = 0; i < 8; i++){
        i2cBitTx(instance, !!(val & 0x80));
        val <<= 1;
    }
    return !i2cBitRx(instance);
}

static uint8_t i2cRecv(uint8_t instance, char ack){

    uint8_t i, v = 0;

    for(i = 0; i < 8; i++){

        v <<= 1;
        if(i2cBitRx(instance)) v |= 1;
    }
    i2cBitTx(instance, !ack);
    return v;
}


static void i2cWaitComplete(Twi *pTwi)
{
    while(!(pTwi->TWI_SR & TWI_SR_TXCOMP)) coopYield();
}

static uint32_t i2cWaitRx(Twi *pTwi)
{
    uint32_t stat;
    do {
        coopYield();
        stat = pTwi->TWI_SR;
        if (stat & TWI_SR_NACK) {
            return stat;
        }
    } while (!(stat & TWI_SR_RXRDY));
}

static uint32_t i2cWaitTx(Twi *pTwi)
{
    uint32_t stat;
    do {
        coopYield();
        stat = pTwi->TWI_SR;
        if (stat & TWI_SR_NACK) {
            return stat;
        }
    } while (!(stat & TWI_SR_TXRDY));
    return 0;
}

static void i2cPdcDisable(Twi *pTwi)
{
    pTwi->TWI_PTCR = TWI_PTCR_RXTDIS;
    pTwi->TWI_PTCR = TWI_PTCR_TXTDIS;
    pTwi->TWI_RPR = 0;
    pTwi->TWI_RCR = 0;
    pTwi->TWI_TPR = 0;
    pTwi->TWI_TCR = 0;
}

static uint32_t i2cWaitPdcTx(Twi *pTwi)
{
    uint32_t stat;
    do {
        coopYield();
        stat = pTwi->TWI_SR;
        if (stat & TWI_SR_NACK) {
            return stat;
        }
    } while (!(stat & TWI_SR_TXBUFE));
    return stat;
}

static uint32_t i2cWaitPdcRx(Twi *pTwi)
{
    uint32_t stat;
    do {
        coopYield();
        stat = pTwi->TWI_SR;
        if ((stat & TWI_SR_NACK) || ((stat & TWI_SR_TXCOMP) && !(stat & TWI_SR_RXBUFF)))  {
            return stat;
        }
    } while(!(stat & TWI_SR_RXBUFF));
    return stat;
}

uint8_t i2cOp(uint8_t instance, uint8_t addr, const uint8_t* sendData, uint8_t sendSz, uint8_t* recvData, uint8_t recvSz){

    uint8_t ret = I2C_ERR_NAK;
    uint32_t stat;

#if I2C_PDC
    uint8_t dat[256];
    uint8_t i;
    uint32_t tmp;
    if (sendData != NULL && (uint32_t)sendData < 0x20000000) {  /* Undocumented feature: PDC can't read from flash */
        if (sendSz > 256)
            return I2C_ERR_INVAL;
        for (i = 0; i < sendSz; i++)
            dat[i] = *sendData++;
        sendData = dat;
    }
#endif

#if !I2C_BITBANG
    Twi *pTwi;
#endif

    if(instance >= I2C_NUM_INSTANCES) return I2C_ERR_INVAL;

#if !I2C_BITBANG
    pTwi = (instance == 0) ? TWI0 : TWI1;
#endif

#if I2C_BITBANG
    addr <<= 1;

    if(sendSz || !recvSz){	//if neither send nor recv requested, do a single send -> this is for a scan

        i2cStart(instance);

        if(!i2cSend(instance, addr)) goto out;
        while(sendSz--) if(!i2cSend(instance, *sendData++)) goto out;
    }
    if(recvSz){

        i2cStart(instance); //if we already sent something, restart

        if(!i2cSend(instance, addr | 1)) goto out;
        while(recvSz){
            recvSz--;
            *recvData++ = i2cRecv(instance, !!recvSz);
        }
    }

    ret = I2C_ALL_OK;
#else

    if (sendSz == 0 && recvSz == 0) {   // Quick (for a scan)
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = (addr << 16);
        pTwi->TWI_CR = TWI_CR_QUICK;
        if (TWI_SR_NACK & i2cWaitTx(pTwi)) {
            i2cWaitComplete(pTwi);
            ret = I2C_ERR_NAK;
            goto out;
        }
        i2cWaitComplete(pTwi);
    }

#if I2C_PDC
    if (sendSz != 0) {
        pTwi->TWI_TPR = (uint32_t)sendData;
        pTwi->TWI_TCR = sendSz;
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = (addr << 16);
        pTwi->TWI_IADR = 0;
        pTwi->TWI_IADR = 0;
        pTwi->TWI_PTCR = TWI_PTCR_TXTEN;
#if I2C_INT_PDC
        i2cInst[instance].status = I2C_STAT_BUSY;
        pTwi->TWI_IER = TWI_IER_TXBUFE | TWI_IER_NACK;
        while((i2cInst[instance].status != I2C_STAT_DONE) &&
              (i2cInst[instance].status != I2C_STAT_NACK))
            coopYield();
        if (i2cInst[instance].status == I2C_STAT_NACK) {
            ret = I2C_ERR_NAK;
            goto out;
        }
#else

        if (TWI_SR_NACK & i2cWaitPdcTx(pTwi)) {
            i2cPdcDisable(pTwi);
            i2cWaitComplete(pTwi);
            ret = I2C_ERR_NAK;
            goto out;
        }
        i2cPdcDisable(pTwi);
        pTwi->TWI_CR = TWI_CR_STOP;
        i2cWaitComplete(pTwi);
#endif
    }

    if (recvSz == 1) {              // We are forced to do the one byte reads manually.
        pTwi->TWI_RPR = (uint32_t)recvData;
        pTwi->TWI_RCR = 1;
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = TWI_MMR_MREAD | (addr << 16);
        pTwi->TWI_CR = TWI_CR_START | TWI_CR_STOP;
        pTwi->TWI_PTCR = TWI_PTCR_RXTEN;
#if I2C_INT_PDC
        i2cInst[instance].status = I2C_STAT_LASTREAD;
        pTwi->TWI_IER = TWI_IER_RXBUFF | TWI_IER_NACK;
        while((i2cInst[instance].status != I2C_STAT_DONE) &&
              (i2cInst[instance].status != I2C_STAT_NACK))
            coopYield();
        if (i2cInst[instance].status == I2C_STAT_NACK) {
            ret = I2C_ERR_NAK;
            goto out;
        }
#else

        if (TWI_SR_NACK & i2cWaitPdcRx(pTwi)) {
            i2cPdcDisable(pTwi);
            i2cWaitComplete(pTwi);
            ret = I2C_ERR_NAK;
            goto out;
        }
        i2cPdcDisable(pTwi);
        i2cWaitComplete(pTwi);
#endif
    }

    if (recvSz > 1) {
        pTwi->TWI_RPR = (uint32_t)recvData;
        pTwi->TWI_RCR = recvSz - 1; // Last byte read is handled manually
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = TWI_MMR_MREAD | (addr << 16);
        pTwi->TWI_CR = TWI_CR_START;
        pTwi->TWI_PTCR = TWI_PTCR_RXTEN;

#if I2C_INT_PDC
        i2cInst[instance].lastByte = recvData + recvSz - 1;
        i2cInst[instance].status = I2C_STAT_BUSY;
        pTwi->TWI_IER = TWI_IER_RXBUFF | TWI_IER_NACK;
        while((i2cInst[instance].status != I2C_STAT_DONE) &&
              (i2cInst[instance].status != I2C_STAT_NACK))
            coopYield();
        if (i2cInst[instance].status == I2C_STAT_NACK) {
            ret = I2C_ERR_NAK;
            goto  out;
        }
#else
        if (TWI_SR_NACK & i2cWaitPdcRx(pTwi)) {
            i2cPdcDisable(pTwi);
            i2cWaitComplete(pTwi);
            ret = I2C_ERR_NAK;
            goto out;
        }
        i2cPdcDisable(pTwi);
        pTwi->TWI_CR = TWI_CR_STOP;
        pTwi->TWI_RPR = (uint32_t)(recvData + recvSz - 1);
        pTwi->TWI_RCR = 1;
        pTwi->TWI_PTCR = TWI_PTCR_RXTEN;
        if (TWI_SR_NACK & i2cWaitPdcRx(pTwi)) {
            i2cPdcDisable(pTwi);
            i2cWaitComplete(pTwi);
            ret = I2C_ERR_NAK;
            goto out;
        }
        i2cPdcDisable(pTwi);
        i2cWaitComplete(pTwi);
#endif
    }
    ret = I2C_ALL_OK;
#else
    if (sendSz != 0) {
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = (addr << 16);
        pTwi->TWI_IADR = 0;
        pTwi->TWI_IADR = 0;
        while (sendSz--) {
            pTwi->TWI_THR = *sendData++;
            stat = i2cWaitTx(pTwi);
            if (stat & TWI_SR_NACK) {
                ret = I2C_ERR_NAK;
                goto out;
            }
        }
        pTwi->TWI_CR = TWI_CR_STOP;
        i2cWaitComplete(pTwi);
    }

    if (recvSz == 1) {
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = TWI_MMR_MREAD | (addr << 16);
        pTwi->TWI_IADR = 0;
        pTwi->TWI_IADR = 0;
        pTwi->TWI_CR = TWI_CR_START | TWI_CR_STOP;
        if (TWI_SR_NACK & i2cWaitRx(pTwi)) {
            ret = I2C_ERR_NAK;
            goto out;
        }
        *recvData = pTwi->TWI_RHR;
        i2cWaitComplete(pTwi);
    } else if (recvSz >= 1) {
        pTwi->TWI_MMR = 0;
        pTwi->TWI_MMR = TWI_MMR_MREAD | (addr << 16);
        pTwi->TWI_IADR = 0;
        pTwi->TWI_IADR = 0;
        pTwi->TWI_CR = TWI_CR_START;
        while (recvSz--) {
            if (TWI_SR_NACK & i2cWaitRx(pTwi)) {
                ret = I2C_ERR_NAK;
                goto out;
            }
            dbgPrintf("\0");
            *recvData++ = pTwi->TWI_RHR;
            if (recvSz == 1) pTwi->TWI_CR = TWI_CR_STOP;
        }
        i2cWaitComplete(pTwi);
    }

    ret = I2C_ALL_OK;
#endif
#endif

out:

#if I2C_BITBANG
    i2cStop(instance);
#else
    if (ret != I2C_ALL_OK) {
        pTwi->TWI_CR = TWI_CR_STOP;
        i2cWaitComplete(pTwi);
    }
#endif

    return ret;
}

uint8_t i2cSingleWrite(uint8_t instance, uint8_t addr, uint8_t reg, uint8_t val){

    unsigned char data[2] = {reg, val};

    return i2cOp(instance, addr, data, 2, NULL, 0);
}

uint8_t i2cSingleRead(uint8_t instance, uint8_t addr, uint8_t reg){

    uint8_t ret, val;

    ret = i2cOp(instance, addr, &reg, 1, &val, 1);

    if(ret != I2C_ALL_OK) val = 0xFF;

    return val;
}

uint8_t i2cQuick(uint8_t instance, uint8_t addr, uint8_t byte){

    return i2cOp(instance, addr, &byte, 1, NULL, 0);
}

uint8_t i2cScan(uint8_t instance, uint8_t addr){

     return i2cOp(instance, addr, NULL, 0, NULL, 0);
}

static char sdaRd(uint8_t instance){

    return gpioGetVal(pins[instance][0]);
}

static void sdaLo(uint8_t instance){

    gpioSetDir(pins[instance][0], 0);
}

static void sdaHi(uint8_t instance){

    gpioSetDir(pins[instance][0], 1);
}

static void sclLo(uint8_t instance){

    gpioSetDir(pins[instance][1], 0);
}

static void sclHi(uint8_t instance){

    gpioSetDir(pins[instance][1], 1);
    while(!gpioGetVal(pins[instance][1]));
}



