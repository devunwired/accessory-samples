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
#include "LEDs.h"
#include "fwk.h"


static const uint8_t blanks[] = {PORTB(8), PORTC(25), PORTC(26), PORTC(27)};
#define MISO	PORTA(25)
#define MOSI	PORTA(26)
#define SCLK	PORTA(27)
#define XLAT	PORTA(28)
#define DBGLED	PORTC(9)

static const uint16_t led_gamma[256] =
{
    0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 11, 12, 14, 15, 17, 19,
    21, 23, 25, 27, 29, 32, 34, 37, 40, 43, 46, 49, 52, 55, 59, 62, 66, 70, 73,
    77, 82, 86, 90, 95, 99, 104, 109, 114, 119, 124, 129, 135, 140, 146, 152,
    158, 164, 170, 176, 182, 189, 196, 202, 209, 216, 224, 231, 238, 246, 254,
    261, 269, 277, 286, 294, 302, 311, 320, 328, 337, 347, 356, 365, 375, 384,
    394, 404, 414, 424, 435, 445, 456, 467, 477, 488, 500, 511, 522, 534, 545,
    557, 569, 581, 594, 606, 619, 631, 644, 657, 670, 683, 697, 710, 724, 738,
    752, 766, 780, 794, 809, 823, 838, 853, 868, 884, 899, 914, 930, 946, 962,
    978, 994, 1011, 1027, 1044, 1061, 1078, 1095, 1112, 1130, 1147, 1165, 1183,
    1201, 1219, 1237, 1256, 1274, 1293, 1312, 1331, 1350, 1370, 1389, 1409,
    1429, 1449, 1469, 1489, 1509, 1530, 1551, 1572, 1593, 1614, 1635, 1657,
    1678, 1700, 1722, 1744, 1766, 1789, 1811, 1834, 1857, 1880, 1903, 1926,
    1950, 1974, 1997, 2021, 2045, 2070, 2094, 2119, 2143, 2168, 2193, 2219,
    2244, 2270, 2295, 2321, 2347, 2373, 2400, 2426, 2453, 2479, 2506, 2534,
    2561, 2588, 2616, 2644, 2671, 2700, 2728, 2756, 2785, 2813, 2842, 2871,
    2900, 2930, 2959, 2989, 3019, 3049, 3079, 3109, 3140, 3170, 3201, 3232,
    3263, 3295, 3326, 3358, 3390, 3421, 3454, 3486, 3518, 3551, 3584, 3617,
    3650, 3683, 3716, 3750, 3784, 3818, 3852, 3886, 3920, 3955, 3990, 4025,
    4060, 4095
};


#define NUM_ROWS	4
#define XFERS_PER_LED   3
#define XFERS_PER_ROW	((NUM_LEDS) * (XFERS_PER_LED) / (NUM_ROWS))



const uint8_t digit_table[6] = {57, 41, 25, 50, 34, 18};
const uint8_t icon_table[8] = {49, 33, 17, 1, 48, 32, 16, 0};
const uint8_t seg_table[128] =
	{
		/* 0x00 - 0x0F */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		/* 0x10 - 0x1F */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* 0x20 - 0x2F */ 0x00, 0x6B, 0x22, 0x56, 0x64, 0x2D, 0x64, 0x02, 0x31, 0x07, 0x40, 0x40, 0x08, 0x40, 0x08, 0x52,
		/* 0x30 - 0x3F */ 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x08, 0x08, 0x58, 0x48, 0x4C, 0x0B,
		/* 0x40 - 0x4F */ 0x63, 0x77, 0x7C, 0x58, 0x5E, 0x79, 0x71, 0x6F, 0x74, 0x10, 0x0E, 0x70, 0x38, 0x55, 0x54, 0x5C,
		/* 0x50 - 0x5F */ 0x73, 0x67, 0x50, 0x6D, 0x78, 0x3E, 0x1C, 0x1D, 0x76, 0x6E, 0x5B, 0x59, 0x0F, 0x64, 0x23, 0x08,
		/* 0x60 - 0x6F */ 0x20, 0x77, 0x7C, 0x58, 0x5E, 0x79, 0x71, 0x6F, 0x74, 0x10, 0x0E, 0x70, 0x38, 0x55, 0x54, 0x5C,
		/* 0x70 - 0x7F */ 0x73, 0x67, 0x50, 0x6D, 0x78, 0x3E, 0x1C, 0x1D, 0x76, 0x6E, 0x5B, 0x46, 0x30, 0x70, 0x40, 0x00
	};


static uint8_t row = 0;
static uint16_t led_buffer[192]={0,};



void SPI0_Handler(void){

    //disable the interrupt
    gpioSetVal(XLAT, 1);
    SPI0->SPI_IDR = SPI_IER_TXEMPTY;
    gpioSetVal(XLAT, 0);
    gpioSetVal(blanks[row], 0);
}

void ledsInit(void){

    uint8_t i;

    //init SPI
    SPI_Configure(SPI0, ID_SPI0, SPI_MR_MSTR | SPI_MR_MODFDIS | SPI_MR_PCS(0));
    SPI_ConfigureNPCS(SPI0, 0 , SPI_CSR_NCPHA | SPI_CSR_BITS_12_BIT | SPI_CSR_SCBR(3) | SPI_CSR_DLYBS(2) | SPI_CSR_DLYBCT(0)); //30MHz spi speed
    SPI_Enable(SPI0);
    SPI0->SPI_IDR = 0xFFFFFFFF;
    NVIC_EnableIRQ(SPI0_IRQn);

    //init pins: SPI
    gpioSetFun(MISO, GPIO_FUNC_A);
    gpioSetFun(MOSI, GPIO_FUNC_A);
    gpioSetFun(SCLK, GPIO_FUNC_A);

    //init pins: debug LED
    gpioSetFun(DBGLED, GPIO_FUNC_GPIO);
    gpioSetDir(DBGLED, 0);
    gpioSetVal(DBGLED, 0);

    //init pins: latch
    gpioSetFun(XLAT, GPIO_FUNC_GPIO);
    gpioSetDir(XLAT, 0);
    gpioSetVal(XLAT, 0);

    //init pins: blanking
    for(i = 0; i < sizeof(blanks); i++){
        gpioSetFun(blanks[i], GPIO_FUNC_GPIO);
        gpioSetDir(blanks[i], 0);
        gpioSetVal(blanks[i], 1);
    }

    periodicAdd(ledUpdate, 0, 4);
}

void ledWrite(uint8_t led_id, uint8_t r, uint8_t g, uint8_t b){

    if(led_id >= NUM_LEDS) return;

    led_id += led_id << 1;

    led_buffer[led_id++] = led_gamma[b];
    led_buffer[led_id++] = led_gamma[g];
    led_buffer[led_id++] = led_gamma[r];
}

void ledDrawIcon(uint8_t offset, uint8_t r, uint8_t g, uint8_t b){

    ledWrite(icon_table[offset], r, g, b);
}

void ledDrawLetter(char letter, uint8_t val, uint8_t r, uint8_t g, uint8_t b){

    uint8_t i;
    uint8_t seg = seg_table[val];
    uint8_t offset = digit_table[letter];

    for (i = 0; i < 7; i++, seg <<= 1) ledWrite(i + offset, (seg & 64) ? r : 0, (seg & 64) ? g : 0, (seg & 64) ? b : 0);
}

void ledUpdate(void){

    volatile uint16_t* row_data;
    unsigned i;

    gpioSetVal(blanks[row], 1);
 
    if(++row == NUM_ROWS) row = 0;
    row_data = led_buffer + XFERS_PER_ROW * row;

    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_SADDR = (uint32_t)row_data;
    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_DADDR = (uint32_t)&SPI0->SPI_TDR;
    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_DSCR = 0;
    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_CTRLA = DMAC_CTRLA_BTSIZE(XFERS_PER_ROW) | DMAC_CTRLA_SCSIZE_CHK_16 | DMAC_CTRLA_DCSIZE_CHK_1 | DMAC_CTRLA_SRC_WIDTH_HALF_WORD | DMAC_CTRLA_DST_WIDTH_HALF_WORD;
    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_CTRLB = DMAC_CTRLB_FC_MEM2PER_DMA_FC | DMAC_CTRLB_SRC_INCR_INCREMENTING | DMAC_CTRLB_DST_INCR_FIXED;
    DMAC->DMAC_CH_NUM[DMA_CHANNEL_LEDS].DMAC_CFG = DMAC_CFG_DST_PER(1) | DMAC_CFG_SRC_PER(1) | DMAC_CFG_DST_H2SEL_HW | DMAC_CFG_SOD | DMAC_CFG_FIFOCFG_ALAP_CFG;
    DMAC->DMAC_CHER = (1 << DMA_CHANNEL_LEDS);

    //wait for transfer to start
    while(SPI0->SPI_SR & SPI_SR_TXEMPTY);
    SPI0->SPI_IER = SPI_IER_TXEMPTY;
}

void ledDbgState(char on){

    gpioSetVal(DBGLED, on);
}


